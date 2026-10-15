/* x86-64 disassembler */
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

static const char *r64[16] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
static const char *r32[16] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
static const char *r16[16] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };
static const char *r8n[16] = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };
static const char *r8o[8] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
static const char *alu[8] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
static const char *shf[8] = { "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar" };
static const char *cc[16] = { "o", "no", "b", "ae", "e", "ne", "be", "a", "s", "ns", "p", "np", "l", "ge", "le", "g" };

typedef struct {
    const u8 *p, *start;
    u64 base;
    int rex, opsz, ok;
    char out[96];
    int len;
} dctx;

static void put(dctx *d, const char *s) { while (*s && d->len < 95) d->out[d->len++] = *s++; d->out[d->len] = 0; }
static void putf(dctx *d, const char *f, i64 v) { char t[40]; snfmt(t, sizeof t, f, v); put(d, t); }
static u8 u8_(dctx *d) { return *d->p++; }
static i32 s8_(dctx *d) { return (i8)*d->p++; }
static i32 s32_(dctx *d) { i32 v = (i32)(d->p[0] | d->p[1] << 8 | d->p[2] << 16 | (u32)d->p[3] << 24); d->p += 4; return v; }
static u16 u16_(dctx *d) { u16 v = (u16)(d->p[0] | d->p[1] << 8); d->p += 2; return v; }
static u64 u64_(dctx *d) { u64 v = 0; for (int i = 0; i < 8; i++) v |= (u64)d->p[i] << (i * 8); d->p += 8; return v; }

static const char *reg(dctx *d, int r, int size) {
    if (size == 8) return d->rex ? r8n[r] : r < 8 ? r8o[r] : r8n[r];
    if (size == 16) return r16[r];
    if (size == 32) return r32[r];
    return r64[r];
}

static void hex(dctx *d, i64 v) {
    if (v < 0) { put(d, "-"); v = -v; }
    putf(d, "0x%lx", v);
}

/* modrm operand */
static int modrm(dctx *d, int size, int *regf) {
    u8 m = u8_(d);
    int mod = m >> 6, r = (m >> 3 & 7) | (d->rex & 4 ? 8 : 0), rm = m & 7;
    *regf = r;
    if (mod == 3) { put(d, reg(d, rm | (d->rex & 1 ? 8 : 0), size)); return 0; }
    put(d, size == 8 ? "byte [" : size == 16 ? "word [" : size == 32 ? "dword [" : "qword [");
    i32 disp = 0;
    if (rm == 4) {
        u8 sib = u8_(d);
        int sc = sib >> 6, idx = (sib >> 3 & 7) | (d->rex & 2 ? 8 : 0), b = (sib & 7) | (d->rex & 1 ? 8 : 0);
        int first = 1;
        if (!((sib & 7) == 5 && mod == 0)) { put(d, r64[b]); first = 0; }
        if (idx != 4) { if (!first) put(d, "+"); put(d, r64[idx]); if (sc) putf(d, "*%ld", 1 << sc); first = 0; }
        if ((sib & 7) == 5 && mod == 0) { disp = s32_(d); if (!first) put(d, disp < 0 ? "" : "+"); hex(d, disp); }
    } else if (rm == 5 && mod == 0) {
        disp = s32_(d);
        put(d, "rip");
        put(d, disp < 0 ? "" : "+");
        hex(d, disp);
    } else put(d, r64[rm | (d->rex & 1 ? 8 : 0)]);
    if (mod == 1) { disp = s8_(d); if (disp) { put(d, disp < 0 ? "" : "+"); hex(d, disp); } }
    if (mod == 2) { disp = s32_(d); if (disp) { put(d, disp < 0 ? "" : "+"); hex(d, disp); } }
    put(d, "]");
    return 1;
}

static int osz(dctx *d) { return d->rex & 8 ? 64 : d->opsz ? 16 : 32; }

static void rm_reg(dctx *d, const char *op, int size, int reg_first) {
    int r;
    char save[96];
    put(d, op); put(d, " ");
    if (reg_first) {
        int at = d->len;
        modrm(d, size, &r);
        memcpy(save, d->out + at, (u64)(d->len - at + 1));
        d->len = at;
        put(d, reg(d, r, size)); put(d, ", "); put(d, save);
    } else {
        modrm(d, size, &r);
        put(d, ", "); put(d, reg(d, r, size));
    }
}

static void imm(dctx *d, int size) {
    put(d, ", ");
    if (size == 8) hex(d, s8_(d));
    else if (size == 16) hex(d, u16_(d));
    else hex(d, s32_(d));
}

static void rel(dctx *d, i64 off) { hex(d, (i64)(d->base + (u64)(d->p - d->start) + (u64)off)); }

static void two_byte(dctx *d) {
    u8 op = u8_(d);
    int r, s = osz(d);
    if (op >= 0x80 && op <= 0x8F) { put(d, "j"); put(d, cc[op & 15]); put(d, " "); i32 o = s32_(d); rel(d, o); return; }
    if (op >= 0x90 && op <= 0x9F) { put(d, "set"); put(d, cc[op & 15]); put(d, " "); modrm(d, 8, &r); return; }
    if (op >= 0x40 && op <= 0x4F) { put(d, "cmov"); put(d, cc[op & 15]); rm_reg(d, "", s, 1); return; }
    switch (op) {
    case 0x05: put(d, "syscall"); return;
    case 0x0B: put(d, "ud2"); return;
    case 0x1F: put(d, "nop "); modrm(d, s, &r); return;
    case 0x30: put(d, "wrmsr"); return;
    case 0x31: put(d, "rdtsc"); return;
    case 0x32: put(d, "rdmsr"); return;
    case 0xA3: rm_reg(d, "bt", s, 0); return;
    case 0xAB: rm_reg(d, "bts", s, 0); return;
    case 0xB3: rm_reg(d, "btr", s, 0); return;
    case 0xBB: rm_reg(d, "btc", s, 0); return;
    case 0xBC: rm_reg(d, "bsf", s, 1); return;
    case 0xBD: rm_reg(d, "bsr", s, 1); return;
    case 0xBA: {
        static const char *g8[8] = { "?", "?", "?", "?", "bt", "bts", "btr", "btc" };
        put(d, g8[*d->p >> 3 & 7]); put(d, " ");
        modrm(d, s, &r);
        imm(d, 8);
        return;
    }
    case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF:
        put(d, "bswap "); put(d, reg(d, (op & 7) | (d->rex & 1 ? 8 : 0), s)); return;
    case 0xA2: put(d, "cpuid"); return;
    case 0xAF: rm_reg(d, "imul", s, 1); return;
    case 0xB6: case 0xB7: case 0xBE: case 0xBF: {
        put(d, op < 0xB8 ? "movzx " : "movsx ");
        int at = d->len;
        char save[96];
        modrm(d, op & 1 ? 16 : 8, &r);
        memcpy(save, d->out + at, (u64)(d->len - at + 1));
        d->len = at;
        put(d, reg(d, r, s)); put(d, ", "); put(d, save);
        return;
    }
    case 0x20: case 0x22: {
        u8 m = u8_(d);
        if (op == 0x20) { put(d, "mov "); put(d, r64[m & 7]); putf(d, ", cr%ld", m >> 3 & 7); }
        else { putf(d, "mov cr%ld, ", m >> 3 & 7); put(d, r64[m & 7]); }
        return;
    }
    case 0x01: {
        u8 m = *d->p;
        static const char *g7[8] = { "sgdt", "sidt", "lgdt", "lidt", "smsw", "?", "lmsw", "invlpg" };
        put(d, g7[m >> 3 & 7]); put(d, " ");
        modrm(d, 64, &r);
        return;
    }
    }
    d->ok = 0;
}

static void one(dctx *d) {
    int r, s;
    for (;;) {
        u8 b = *d->p;
        if (b == 0x66) { d->opsz = 1; d->p++; continue; }
        if (b == 0x2E || b == 0x3E || b == 0x26 || b == 0x36 || b == 0x64 || b == 0x65 || b == 0xF0) {
            if (b == 0x64 || b == 0x65) put(d, b == 0x64 ? "fs " : "gs ");
            if (b == 0xF0) put(d, "lock ");
            d->p++;
            continue;
        }
        if (b == 0xF3 && d->p[1] == 0x0F && d->p[2] == 0x1E && d->p[3] == 0xFA) { d->p += 4; put(d, "endbr64"); return; }
        if (b == 0xF3 || b == 0xF2) { put(d, b == 0xF3 ? "rep " : "repne "); d->p++; continue; }
        break;
    }
    if ((*d->p & 0xF0) == 0x40) d->rex = *d->p++;
    s = osz(d);
    u8 op = u8_(d);
    if (op < 0x40 && (op & 7) < 6) {
        const char *n = alu[op >> 3];
        switch (op & 7) {
        case 0: rm_reg(d, n, 8, 0); return;
        case 1: rm_reg(d, n, s, 0); return;
        case 2: rm_reg(d, n, 8, 1); return;
        case 3: rm_reg(d, n, s, 1); return;
        case 4: put(d, n); put(d, " al"); imm(d, 8); return;
        case 5: put(d, n); put(d, " "); put(d, reg(d, 0, s)); imm(d, s == 16 ? 16 : 32); return;
        }
    }
    if (op >= 0x50 && op <= 0x57) { put(d, "push "); put(d, r64[(op & 7) | (d->rex & 1 ? 8 : 0)]); return; }
    if (op >= 0x58 && op <= 0x5F) { put(d, "pop "); put(d, r64[(op & 7) | (d->rex & 1 ? 8 : 0)]); return; }
    if (op >= 0x70 && op <= 0x7F) { put(d, "j"); put(d, cc[op & 15]); put(d, " "); i32 o = s8_(d); rel(d, o); return; }
    if (op >= 0x91 && op <= 0x97) { put(d, "xchg "); put(d, reg(d, 0, s)); put(d, ", "); put(d, reg(d, (op & 7) | (d->rex & 1 ? 8 : 0), s)); return; }
    if (op >= 0xB0 && op <= 0xB7) { put(d, "mov "); put(d, reg(d, (op & 7) | (d->rex & 1 ? 8 : 0), 8)); imm(d, 8); return; }
    if (op >= 0xB8 && op <= 0xBF) {
        put(d, "mov "); put(d, reg(d, (op & 7) | (d->rex & 1 ? 8 : 0), s)); put(d, ", ");
        if (s == 64) hex(d, (i64)u64_(d)); else if (s == 16) hex(d, u16_(d)); else hex(d, (u32)s32_(d));
        return;
    }
    switch (op) {
    case 0x0F: two_byte(d); return;
    case 0x63: rm_reg(d, "movsxd", 64, 1); return;
    case 0x68: put(d, "push"); imm(d, 32); d->out[4] = ' '; return;
    case 0x6A: put(d, "push"); imm(d, 8); d->out[4] = ' '; return;
    case 0x69: case 0x6B: rm_reg(d, "imul", s, 1); imm(d, op == 0x6B ? 8 : 32); return;
    case 0x80: case 0x81: case 0x83: {
        u8 m = *d->p;
        put(d, alu[m >> 3 & 7]); put(d, " ");
        modrm(d, op == 0x80 ? 8 : s, &r);
        imm(d, op == 0x81 ? (s == 16 ? 16 : 32) : 8);
        return;
    }
    case 0x84: rm_reg(d, "test", 8, 0); return;
    case 0x85: rm_reg(d, "test", s, 0); return;
    case 0x86: rm_reg(d, "xchg", 8, 0); return;
    case 0x87: rm_reg(d, "xchg", s, 0); return;
    case 0x88: rm_reg(d, "mov", 8, 0); return;
    case 0x89: rm_reg(d, "mov", s, 0); return;
    case 0x8A: rm_reg(d, "mov", 8, 1); return;
    case 0x8B: rm_reg(d, "mov", s, 1); return;
    case 0x8D: rm_reg(d, "lea", s, 1); return;
    case 0x90: put(d, d->opsz ? "xchg ax, ax" : "nop"); return;
    case 0x98: put(d, s == 64 ? "cdqe" : "cwde"); return;
    case 0x99: put(d, s == 64 ? "cqo" : "cdq"); return;
    case 0x9C: put(d, "pushf"); return;
    case 0x9D: put(d, "popf"); return;
    case 0xA8: put(d, "test al"); imm(d, 8); return;
    case 0xA9: put(d, "test "); put(d, reg(d, 0, s)); imm(d, 32); return;
    case 0xAA: put(d, "stosb"); return;
    case 0xAB: put(d, s == 64 ? "stosq" : "stosd"); return;
    case 0xA4: put(d, "movsb"); return;
    case 0xA5: put(d, s == 64 ? "movsq" : "movsd"); return;
    case 0xC0: case 0xC1: case 0xD0: case 0xD1: case 0xD2: case 0xD3: {
        u8 m = *d->p;
        put(d, shf[m >> 3 & 7]); put(d, " ");
        modrm(d, op & 1 ? s : 8, &r);
        if (op <= 0xC1) imm(d, 8); else if (op <= 0xD1) put(d, ", 1"); else put(d, ", cl");
        return;
    }
    case 0xC3: put(d, "ret"); return;
    case 0xC6: case 0xC7: put(d, "mov "); modrm(d, op == 0xC6 ? 8 : s, &r); imm(d, op == 0xC6 ? 8 : s == 16 ? 16 : 32); return;
    case 0xC9: put(d, "leave"); return;
    case 0xCC: put(d, "int3"); return;
    case 0xCD: putf(d, "int 0x%lx", u8_(d)); return;
    case 0xCF: put(d, s == 64 ? "iretq" : "iret"); return;
    case 0xE4: putf(d, "in al, 0x%lx", u8_(d)); return;
    case 0xE6: putf(d, "out 0x%lx, al", u8_(d)); return;
    case 0xE8: { put(d, "call "); i32 o = s32_(d); rel(d, o); return; }
    case 0xE9: { put(d, "jmp "); i32 o = s32_(d); rel(d, o); return; }
    case 0xEB: { put(d, "jmp "); i32 o = s8_(d); rel(d, o); return; }
    case 0xEC: put(d, "in al, dx"); return;
    case 0xED: put(d, s == 16 ? "in ax, dx" : "in eax, dx"); return;
    case 0xEE: put(d, "out dx, al"); return;
    case 0xEF: put(d, s == 16 ? "out dx, ax" : "out dx, eax"); return;
    case 0xF4: put(d, "hlt"); return;
    case 0xFA: put(d, "cli"); return;
    case 0xFB: put(d, "sti"); return;
    case 0xFC: put(d, "cld"); return;
    case 0xF6: case 0xF7: {
        static const char *g3[8] = { "test", "test", "not", "neg", "mul", "imul", "div", "idiv" };
        u8 m = *d->p;
        put(d, g3[m >> 3 & 7]); put(d, " ");
        modrm(d, op == 0xF6 ? 8 : s, &r);
        if ((m >> 3 & 7) < 2) imm(d, op == 0xF6 ? 8 : 32);
        return;
    }
    case 0xFE: case 0xFF: {
        static const char *g5[8] = { "inc", "dec", "call", "callf", "jmp", "jmpf", "push", "?" };
        u8 m = *d->p;
        int k = m >> 3 & 7;
        put(d, g5[k]); put(d, " ");
        modrm(d, op == 0xFE ? 8 : (k == 2 || k == 4 || k == 6) ? 64 : s, &r);
        return;
    }
    }
    d->ok = 0;
}

/* one instruction */
int dis1(const u8 *p, u64 addr, char *out, int cap) {
    dctx d = { 0 };
    d.p = d.start = p;
    d.base = addr;
    d.ok = 1;
    one(&d);
    int n = (int)(d.p - p);
    if (!d.ok) { n = 1; snfmt(d.out, sizeof d.out, "db 0x%lx", (u64)p[0]); }
    scopy(out, d.out, (u64)cap);
    return n;
}

static void show(u64 a, int count, u64 limit) {
    char t[96];
    for (int i = 0; i < count && (!limit || a < limit); i++) {
        const u8 *p = (const u8 *)a;
        int n = dis1(p, a, t, sizeof t);
        vga_color(C_DIM);
        kprintf("  %08lx  ", a);
        char hx[32];
        int k = 0;
        for (int j = 0; j < n && j < 8; j++) {
            if (j == 7 && n > 8) { scopy(hx + k, "+", sizeof hx - (u64)k); break; }
            snfmt(hx + k, sizeof hx - (u64)k, "%02x", p[j]);
            k += 2;
        }
        kprintf("%-17s", hx);
        vga_color(C_TEXT);
        kprintf("%s\n", t);
        a += (u64)n;
    }
}

static void b_dis(void) {   /* addr n dis */
    i64 a, n;
    if (pop_int("dis", &n) && pop_int("dis", &a)) show((u64)a, (int)n, 0);
}

static void b_disr(void) {  /* addr bytes disr */
    i64 a, n;
    if (pop_int("disr", &n) && pop_int("disr", &a)) show((u64)a, 100000, (u64)(a + n));
}

const struct builtin dis_words[] = {
    { "dis", b_dis }, { "disr", b_disr },
    { NULL, NULL }
};
