/* x86-64 jit */
#include <stddef.h>
#include "cpu.h"
#include "kbd.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

u32 jit_version = 1, jit_enabled = 1;
static u64 jit_count, jit_bytes;

#ifdef HOST
void *jit_alloc(u64 n);
void jit_free(void *p);
#else
static void *jit_alloc(u64 n) { return kmalloc_try(n); }
static void jit_free(void *p) { kfree(p); }
#endif

enum { RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15 };

#define VSZ ((i32)sizeof(V))
#define OFF_T ((i32)offsetof(V, t))
#define OFF_N ((i32)offsetof(V, n))
#define OFF_P ((i32)offsetof(V, p))

typedef struct {
    u8 *p, *start, *end;
    int bad, depth;
    u32 fix[2048];
    int nfix;
} em;

/* bytes */

static void b1(em *e, u8 v) { if (e->p < e->end) *e->p++ = v; else e->bad = 1; }
static void b4(em *e, u32 v) { for (int i = 0; i < 4; i++) b1(e, (u8)(v >> (i * 8))); }
static void b8(em *e, u64 v) { for (int i = 0; i < 8; i++) b1(e, (u8)(v >> (i * 8))); }
static u32 here(em *e) { return (u32)(e->p - e->start); }

static void rex(em *e, int w, int reg, int base) {
    u8 r = (u8)(0x40 | (w << 3) | ((reg >> 3) & 1) << 2 | ((base >> 3) & 1));
    if (r != 0x40) b1(e, r);
}

/* op reg, [base+disp] */
static void mem(em *e, int w, u8 op1, int op2, int reg, int base, i32 disp) {
    rex(e, w, reg, base);
    b1(e, op1);
    if (op2 >= 0) b1(e, (u8)op2);
    b1(e, (u8)(0x80 | (reg & 7) << 3 | (base & 7)));
    if ((base & 7) == 4) b1(e, 0x24);
    b4(e, (u32)disp);
}

/* op reg, rm */
static void rr(em *e, int w, u8 op1, int op2, int reg, int rm) {
    rex(e, w, reg, rm);
    b1(e, op1);
    if (op2 >= 0) b1(e, (u8)op2);
    b1(e, (u8)(0xC0 | (reg & 7) << 3 | (rm & 7)));
}

static void push_r(em *e, int r) { if (r >= 8) b1(e, 0x41); b1(e, (u8)(0x50 + (r & 7))); }
static void pop_r(em *e, int r) { if (r >= 8) b1(e, 0x41); b1(e, (u8)(0x58 + (r & 7))); }
static void mov_ri(em *e, int r, u64 v) { rex(e, 1, 0, r); b1(e, (u8)(0xB8 + (r & 7))); b8(e, v); }
static void ld64(em *e, int r, int base, i32 d) { mem(e, 1, 0x8B, -1, r, base, d); }
static void st64(em *e, int base, i32 d, int r) { mem(e, 1, 0x89, -1, r, base, d); }
static void ld32(em *e, int r, int base, i32 d) { mem(e, 0, 0x8B, -1, r, base, d); }
static void st32(em *e, int base, i32 d, int r) { mem(e, 0, 0x89, -1, r, base, d); }
static void st64i(em *e, int base, i32 d, i32 imm) { mem(e, 1, 0xC7, -1, 0, base, d); b4(e, (u32)imm); }
static void cmp8mi(em *e, int base, i32 d, u8 imm) { mem(e, 0, 0x80, -1, 7, base, d); b1(e, imm); }
static void cmp32mi(em *e, int base, i32 d, i8 imm) { mem(e, 0, 0x83, -1, 7, base, d); b1(e, (u8)imm); }
static void cmp64mi(em *e, int base, i32 d, i8 imm) { mem(e, 1, 0x83, -1, 7, base, d); b1(e, (u8)imm); }
static void incm32(em *e, int base, i32 d, int dec) { mem(e, 0, 0xFF, -1, dec, base, d); }
static void incm64(em *e, int base, i32 d, int dec) { mem(e, 1, 0xFF, -1, dec, base, d); }
static void alu_ri8(em *e, int r, int ext, i8 imm) { rex(e, 1, 0, r); b1(e, 0x83); b1(e, (u8)(0xC0 | ext << 3 | (r & 7))); b1(e, (u8)imm); }
static void incr32(em *e, int r, int dec) { rex(e, 0, 0, r); b1(e, 0xFF); b1(e, (u8)(0xC0 | dec << 3 | (r & 7))); }
static void cmpr32i(em *e, int r, i32 imm) { rex(e, 0, 0, r); b1(e, 0x81); b1(e, (u8)(0xF8 | (r & 7))); b4(e, (u32)imm); }
static void call_abs(em *e, void *fn) { mov_ri(e, RAX, (u64)fn); b1(e, 0xFF); b1(e, 0xD0); }

static u32 jmp32(em *e) { b1(e, 0xE9); u32 at = here(e); b4(e, 0); return at; }
static u32 jcc32(em *e, u8 cc) { b1(e, 0x0F); b1(e, (u8)(0x80 | cc)); u32 at = here(e); b4(e, 0); return at; }
static void patch(em *e, u32 at, u32 to) { if (!e->bad) { u32 rel = to - (at + 4); for (int i = 0; i < 4; i++) e->start[at + i] = (u8)(rel >> (i * 8)); } }
static void to_exit(em *e, u32 at) { if (e->nfix < 2048) e->fix[e->nfix++] = at; else e->bad = 1; }

enum { CC_O = 0, CC_E = 4, CC_NE = 5, CC_LE = 0xE, CC_G = 0xF, CC_L = 0xC, CC_GE = 0xD };

/* helpers */

static void h_word(str *s) { exec_word(s); }
static void h_item(V *v) { exec_item(v); }
static void h_push(V *v) { push(*v); }
static void h_deep(void) { fail("this nests too deep%s", ""); }
static void h_tick(void) { if (!brk()) gc_point(); }
static void h_whilebad(void) { if (need(1, "while")) want(W->stk[W->sp - 1], T_INT, "while"); }
static void h_overflow(void) { fail("the stack is full%s", ""); }

/* register state */

static void sync_out(em *e) { st32(e, RBX, (i32)offsetof(struct omega, sp), R14); }
static void sync_in(em *e) {
    ld32(e, R14, RBX, (i32)offsetof(struct omega, sp));
    b1(e, 0x4D); b1(e, 0x6B); b1(e, 0xE6); b1(e, (u8)VSZ);    /* imul r12,r14,24 */
    b1(e, 0x4D); b1(e, 0x01); b1(e, 0xEC);                    /* add r12,r13 */
}
static void err_exit(em *e) { cmp32mi(e, RBX, (i32)offsetof(struct omega, err), 0); to_exit(e, jcc32(e, CC_NE)); }
static void helper(em *e, void *fn, u64 arg, int has_arg) {
    sync_out(e);
    if (has_arg) mov_ri(e, RDI, arg);
    call_abs(e, fn);
    sync_in(e);
    err_exit(e);
}

/* stack ops */

static void need_n(em *e, int n, u32 *slow) { cmpr32i(e, R14, n); *slow = jcc32(e, CC_L); }
static void room(em *e, u32 *slow) { cmpr32i(e, R14, STK - 1); *slow = jcc32(e, 0xF); }
static u32 is_int(em *e, int slot) { cmp8mi(e, R12, -VSZ * slot + OFF_T, T_INT); return jcc32(e, CC_NE); }
static void drop_n(em *e, int n) { alu_ri8(e, R12, 5, (i8)(VSZ * n)); for (int i = 0; i < n; i++) incr32(e, R14, 1); }
static void grow1(em *e) { alu_ri8(e, R12, 0, (i8)VSZ); incr32(e, R14, 0); }
static void copy_v(em *e, int src, i32 sd, int dst, i32 dd) {
    for (int k = 0; k < 3; k++) { ld64(e, RCX, src, sd + k * 8); st64(e, dst, dd + k * 8, RCX); }
}
static void push_int_rax(em *e) {
    st64i(e, R12, OFF_T, T_INT);
    st64(e, R12, OFF_N, RAX);
    st64i(e, R12, OFF_P, 0);
    grow1(e);
}

static void compile_seq(em *e, quote *q);

static int named(V v, const char *s) {
    if (v.t != T_WORD) return 0;
    str *n = (str *)v.p;
    return n->fn && !strcmp(n->s, s) && !lookup(n);
}

static int inlinable(V v) {
    if (v.t != T_QUOTE) return 0;
    quote *q = (quote *)v.p;
    if (q->n > 200) return 0;
    for (u32 i = 0; i < q->n; i++)
        if (q->v[i].t == T_LOCALS || q->v[i].t == T_SET || q->v[i].t == T_DEF) return 0;
    return 1;
}

static void lit(em *e, V *v) {
    u32 slow, done;
    room(e, &slow);
    if (v->t == T_INT) {
        mov_ri(e, RAX, (u64)v->n);
        push_int_rax(e);
    } else {
        mov_ri(e, RAX, (u64)v);
        copy_v(e, RAX, 0, R12, 0);
        grow1(e);
    }
    done = jmp32(e);
    patch(e, slow, here(e));
    helper(e, (void *)h_push, (u64)v, 1);
    patch(e, done, here(e));
}

/* inline builtins */
static int inline_word(em *e, V *v) {
    str *n = (str *)v->p;
    if (!n->fn || lookup(n)) return 0;
    const char *s = n->s;
    u32 slow, s2, s3, s4 = 0, done;
    if (!strcmp(s, "+") || !strcmp(s, "-") || !strcmp(s, "*")) {
        need_n(e, 2, &slow); s2 = is_int(e, 1); s3 = is_int(e, 2);
        ld64(e, RAX, R12, -VSZ + OFF_N);
        ld64(e, RCX, R12, -2 * VSZ + OFF_N);
        if (s[0] == '*') rr(e, 1, 0x0F, 0xAF, RCX, RAX);
        else rr(e, 1, s[0] == '+' ? 0x01 : 0x29, -1, RAX, RCX);
        s4 = jcc32(e, CC_O);                                          /* overflow: bignum */
        st64(e, R12, -2 * VSZ + OFF_N, RCX);
        drop_n(e, 1);
    } else if (!strcmp(s, "&") || !strcmp(s, "|") || !strcmp(s, "xor")) {
        need_n(e, 2, &slow); s2 = is_int(e, 1); s3 = is_int(e, 2);
        ld64(e, RAX, R12, -VSZ + OFF_N);
        mem(e, 1, s[0] == '&' ? 0x21 : s[0] == '|' ? 0x09 : 0x31, -1, RAX, R12, -2 * VSZ + OFF_N);
        drop_n(e, 1);
    } else if (!strcmp(s, "<") || !strcmp(s, ">") || !strcmp(s, "<=") || !strcmp(s, ">=") || !strcmp(s, "=")) {
        u8 cc = !strcmp(s, "<") ? CC_L : !strcmp(s, ">") ? CC_G : !strcmp(s, "<=") ? CC_LE : !strcmp(s, ">=") ? CC_GE : CC_E;
        need_n(e, 2, &slow); s2 = is_int(e, 1); s3 = is_int(e, 2);
        ld64(e, RAX, R12, -2 * VSZ + OFF_N);
        mem(e, 1, 0x3B, -1, RAX, R12, -VSZ + OFF_N);
        b1(e, 0x0F); b1(e, (u8)(0x90 | cc)); b1(e, 0xC0);            /* setcc al */
        b1(e, 0x0F); b1(e, 0xB6); b1(e, 0xC0);                       /* movzx eax,al */
        st64(e, R12, -2 * VSZ + OFF_N, RAX);
        drop_n(e, 1);
    } else if (!strcmp(s, "dup") || !strcmp(s, "over")) {
        int k = s[0] == 'd' ? 1 : 2;
        need_n(e, k, &slow); room(e, &s2); s3 = 0;
        copy_v(e, R12, -VSZ * k, R12, 0);
        grow1(e);
    } else if (!strcmp(s, "drop")) {
        need_n(e, 1, &slow); s2 = s3 = 0;
        drop_n(e, 1);
    } else if (!strcmp(s, "swap")) {
        need_n(e, 2, &slow); s2 = s3 = 0;
        for (int k = 0; k < 3; k++) {
            ld64(e, RAX, R12, -VSZ + k * 8); ld64(e, RCX, R12, -2 * VSZ + k * 8);
            st64(e, R12, -VSZ + k * 8, RCX); st64(e, R12, -2 * VSZ + k * 8, RAX);
        }
    } else if (!strcmp(s, "nip")) {
        need_n(e, 2, &slow); s2 = s3 = 0;
        copy_v(e, R12, -VSZ, R12, -2 * VSZ);
        drop_n(e, 1);
    } else if (!strcmp(s, "not") || !strcmp(s, "neg")) {
        need_n(e, 1, &slow); s2 = is_int(e, 1); s3 = 0;
        if (s[1] == 'o') {
            cmp64mi(e, R12, -VSZ + OFF_N, 0);
            b1(e, 0x0F); b1(e, 0x94); b1(e, 0xC0);
            b1(e, 0x0F); b1(e, 0xB6); b1(e, 0xC0);
            st64(e, R12, -VSZ + OFF_N, RAX);
        } else {
            mem(e, 1, 0xF7, -1, 3, R12, -VSZ + OFF_N);               /* neg */
            s4 = jcc32(e, CC_O);
        }
    } else return 0;
    done = jmp32(e);
    patch(e, slow, here(e));
    if (s2) patch(e, s2, here(e));
    if (s3) patch(e, s3, here(e));
    if (s4) patch(e, s4, here(e));
    helper(e, (void *)h_word, (u64)n, 1);
    patch(e, done, here(e));
    return 1;
}

static void jpop_int(em *e, u32 *slow, u32 *s2) {
    need_n(e, 1, slow); *s2 = is_int(e, 1);
    ld64(e, RAX, R12, -VSZ + OFF_N);
    drop_n(e, 1);
}

static void slow_path(em *e, V **pushes, int np, str *word) {
    for (int i = 0; i < np; i++) helper(e, (void *)h_push, (u64)pushes[i], 1);
    helper(e, (void *)h_word, (u64)word, 1);
}

static void tick(em *e) { helper(e, (void *)h_tick, 0, 0); }

static void c_when(em *e, V *a, V *w, int unless) {
    u32 slow, s2, skip, done;
    jpop_int(e, &slow, &s2);
    rr(e, 1, 0x85, -1, RAX, RAX);
    skip = jcc32(e, unless ? CC_NE : CC_E);
    compile_seq(e, (quote *)a->p);
    done = jmp32(e);
    patch(e, slow, here(e)); patch(e, s2, here(e));
    slow_path(e, &a, 1, (str *)w->p);
    patch(e, skip, here(e)); patch(e, done, here(e));
}

/* [A] [B] if */
static void c_if(em *e, V *a, V *b, V *w) {
    u32 slow, s2, els, done, done2;
    jpop_int(e, &slow, &s2);
    rr(e, 1, 0x85, -1, RAX, RAX);
    els = jcc32(e, CC_E);
    compile_seq(e, (quote *)a->p);
    done = jmp32(e);
    patch(e, els, here(e));
    compile_seq(e, (quote *)b->p);
    done2 = jmp32(e);
    patch(e, slow, here(e)); patch(e, s2, here(e));
    V *ps[2] = { a, b };
    slow_path(e, ps, 2, (str *)w->p);
    patch(e, done, here(e)); patch(e, done2, here(e));
}

/* n [A] times */
static void c_times(em *e, V *a, V *w) {
    u32 slow, s2, top, end, done;
    jpop_int(e, &slow, &s2);
    alu_ri8(e, RSP, 5, 16);
    st64(e, RSP, 0, RAX);
    top = here(e);
    cmp64mi(e, RSP, 0, 0);
    end = jcc32(e, CC_LE);
    incm64(e, RSP, 0, 1);
    compile_seq(e, (quote *)a->p);
    tick(e);
    patch(e, jmp32(e), top);
    patch(e, end, here(e));
    alu_ri8(e, RSP, 0, 16);
    done = jmp32(e);
    patch(e, slow, here(e)); patch(e, s2, here(e));
    slow_path(e, &a, 1, (str *)w->p);
    patch(e, done, here(e));
}

static void c_for(em *e, V *a, V *w) {
    u32 slow, s2, s3, top, end, ovf, done;
    need_n(e, 2, &slow); s2 = is_int(e, 1); s3 = is_int(e, 2);
    ld64(e, RAX, R12, -2 * VSZ + OFF_N);
    ld64(e, RCX, R12, -VSZ + OFF_N);
    drop_n(e, 2);
    alu_ri8(e, RSP, 5, 16);
    st64(e, RSP, 0, RAX);
    st64(e, RSP, 8, RCX);
    top = here(e);
    ld64(e, RAX, RSP, 0);
    mem(e, 1, 0x3B, -1, RAX, RSP, 8);
    end = jcc32(e, CC_GE);
    room(e, &ovf);
    push_int_rax(e);
    incm64(e, RSP, 0, 0);
    compile_seq(e, (quote *)a->p);
    tick(e);
    patch(e, jmp32(e), top);
    patch(e, ovf, here(e));
    helper(e, (void *)h_overflow, 0, 0);
    patch(e, end, here(e));
    alu_ri8(e, RSP, 0, 16);
    done = jmp32(e);
    patch(e, slow, here(e)); patch(e, s2, here(e)); patch(e, s3, here(e));
    slow_path(e, &a, 1, (str *)w->p);
    patch(e, done, here(e));
}

/* [C] [B] while */
static void c_while(em *e, V *c, V *b) {
    u32 top, bad, bad2, end;
    top = here(e);
    compile_seq(e, (quote *)c->p);
    jpop_int(e, &bad, &bad2);
    rr(e, 1, 0x85, -1, RAX, RAX);
    end = jcc32(e, CC_E);
    compile_seq(e, (quote *)b->p);
    tick(e);
    patch(e, jmp32(e), top);
    patch(e, bad, here(e)); patch(e, bad2, here(e));
    helper(e, (void *)h_whilebad, 0, 0);
    patch(e, end, here(e));
}

static void compile_seq(em *e, quote *q) {
    if (++e->depth > 16) { e->bad = 1; return; }
    for (u32 i = 0; i < q->n && !e->bad; i++) {
        V *v = &q->v[i], *n1 = i + 1 < q->n ? &q->v[i + 1] : NULL, *n2 = i + 2 < q->n ? &q->v[i + 2] : NULL;
        if (v->t == T_QUOTE && n1) {
            if ((named(*n1, "when") || named(*n1, "unless")) && inlinable(*v)) { c_when(e, v, n1, ((str *)n1->p)->s[0] == 'u'); i++; continue; }
            if (named(*n1, "times") && inlinable(*v)) { c_times(e, v, n1); i++; continue; }
            if (named(*n1, "for") && inlinable(*v)) { c_for(e, v, n1); i++; continue; }
            if (n2 && n1->t == T_QUOTE && named(*n2, "if") && inlinable(*v) && inlinable(*n1)) { c_if(e, v, n1, n2); i += 2; continue; }
            if (n2 && n1->t == T_QUOTE && named(*n2, "while") && inlinable(*v) && inlinable(*n1)) { c_while(e, v, n1); i += 2; continue; }
        }
        switch (v->t) {
        case T_INT: case T_STR: case T_QUOTE: case T_SYM: case T_ORD: case T_CARD: case T_BIG: lit(e, v); break;
        case T_WORD: if (!inline_word(e, v)) helper(e, (void *)h_word, (u64)v->p, 1); break;
        default: helper(e, (void *)h_item, (u64)v, 1);
        }
    }
    e->depth--;
}

static void compile(quote *q) {
    static u8 buf[65536];
    static int busy;
    u64 f = irq_save();
    if (busy) { irq_restore(f); return; }
    busy = 1;
    irq_restore(f);
    em *e = kzalloc(sizeof *e);
    e->p = e->start = buf;
    e->end = buf + sizeof buf;

    /* prologue */
    push_r(e, RBP); push_r(e, RBX); push_r(e, R12); push_r(e, R13); push_r(e, R14); push_r(e, R15);
    alu_ri8(e, RSP, 5, 8);
    b1(e, 0x48); b1(e, 0x89); b1(e, 0xE5);                           /* mov rbp,rsp */
    mov_ri(e, RAX, (u64)&current);
    ld64(e, RAX, RAX, 0);
    ld64(e, RBX, RAX, (i32)offsetof(struct task, om));
    mem(e, 1, 0x8D, -1, R13, RBX, (i32)offsetof(struct omega, stk));
    sync_in(e);
    ld32(e, R15, RBX, (i32)offsetof(struct omega, nloc));
    incm32(e, RBX, (i32)offsetof(struct omega, depth), 0);
    mem(e, 0, 0x81, -1, 7, RBX, (i32)offsetof(struct omega, depth)); b4(e, DEPTH);
    u32 ok = jcc32(e, CC_LE);
    helper(e, (void *)h_deep, 0, 0);
    patch(e, ok, here(e));

    compile_seq(e, q);

    /* epilogue */
    u32 exit = here(e);
    for (int i = 0; i < e->nfix; i++) patch(e, e->fix[i], exit);
    sync_out(e);
    st32(e, RBX, (i32)offsetof(struct omega, nloc), R15);
    incm32(e, RBX, (i32)offsetof(struct omega, depth), 1);
    b1(e, 0x48); b1(e, 0x89); b1(e, 0xEC);                           /* mov rsp,rbp */
    alu_ri8(e, RSP, 0, 8);
    pop_r(e, R15); pop_r(e, R14); pop_r(e, R13); pop_r(e, R12); pop_r(e, RBX); pop_r(e, RBP);
    b1(e, 0xC3);

    u32 len = here(e);
    u8 *code = e->bad ? NULL : jit_alloc(len);
    if (code) memcpy(code, buf, len);
    kfree(e);
    busy = 0;
    if (!code) { q->nojit = 1; return; }
    q->jit = code;
    q->jitlen = len;
    q->jitver = jit_version;
    jit_count++;
    jit_bytes += len;
}

static volatile int jit_running;
static void *dead[512];
static int ndead;

static void retire(quote *q) {
    if (ndead < 512) dead[ndead++] = q->jit;
    q->jit = NULL;
    jit_count--;
    jit_bytes -= q->jitlen;
}

static void sweep(void) {
    if (jit_running) return;
    while (ndead) jit_free(dead[--ndead]);
}

static void release(quote *q, int depth) {
    if (q->jit) retire(q);
    if (depth > 40) return;
    for (u32 i = 0; i < q->n; i++)
        if (q->v[i].t == T_QUOTE) release((quote *)q->v[i].p, depth + 1);
}

void jit_release_one(bind *b) {
    if (b->v.t == T_QUOTE) release((quote *)b->v.p, 0);
}

void jit_sweep(void) { sweep(); }

void jit_forget(quote *q) {
    if (!q->jit) return;
    jit_free(q->jit);
    jit_count--;
    jit_bytes -= q->jitlen;
    q->jit = NULL;
}

int jit_run(quote *q) {
    if (!jit_enabled || W->tracing || W->shadowy || W->pin) return 0;
    if (q->jit && q->jitver != jit_version) { retire(q); sweep(); }
    if (!q->jit) {
        if (q->nojit || ++q->hits < 2) return 0;
        compile(q);
        if (!q->jit) return 0;
    }
    __atomic_fetch_add(&jit_running, 1, __ATOMIC_RELAXED);
    ((void (*)(void))q->jit)();
    __atomic_fetch_sub(&jit_running, 1, __ATOMIC_RELAXED);
    return 1;
}

/* words */

static void b_jit(void) {
    vga_color(C_DIM);
    kprintf("  The jit is %s. %lu quotes are compiled, using %lu bytes.\n", jit_enabled ? "on" : "off", jit_count, jit_bytes);
    vga_color(C_TEXT);
}
static void b_jiton(void) { jit_enabled = 1; }
static void b_jitoff(void) { jit_enabled = 0; }

static int sym_quote(const char *w, quote **out) {
    if (!need(1, w)) return 0;
    V s = pop();
    if (!want(s, T_SYM, w)) return 0;
    bind *b = lookup((str *)s.p);
    if (!b || b->v.t != T_QUOTE) { fail("%s isn't a quote", name_of(s)); return 0; }
    *out = (quote *)b->v.p;
    return 1;
}

static void b_compile(void) {
    quote *q;
    if (!sym_quote("compile", &q)) return;
    if (q->jit && q->jitver != jit_version) { retire(q); sweep(); }
    if (!q->jit) { q->nojit = 0; compile(q); }
    if (!q->jit) { fail("%s can't be compiled", "that"); return; }
    push(I((i64)(u64)q->jit));
    push(I(q->jitlen));
}

static void b_addr(void) {   /* 'name addr */
    if (!need(1, "addr")) return;
    V s = pop();
    if (!want(s, T_SYM, "addr")) return;
    str *n = (str *)s.p;
    bind *b = lookup(n);
    if (b && b->v.t == T_QUOTE && ((quote *)b->v.p)->jit) push(I((i64)(u64)((quote *)b->v.p)->jit));
    else if (!b && n->fn) push(I((i64)(u64)n->fn));
    else fail("%s has no machine code", n->s);
}

const struct builtin jit_words[] = {
    { "jit", b_jit }, { "jit-on", b_jiton }, { "jit-off", b_jitoff }, { "compile", b_compile }, { "addr", b_addr },
    { NULL, NULL }
};
