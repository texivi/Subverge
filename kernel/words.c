#include "cpu.h"
#include "kbd.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

/* stack */

static void b_dup(void) { if (need(1, "dup")) push(W->stk[W->sp - 1]); }
static void b_drop(void) { if (need(1, "drop")) W->sp--; }
static void b_swap(void) { if (!need(2, "swap")) return; V t = W->stk[W->sp - 1]; W->stk[W->sp - 1] = W->stk[W->sp - 2]; W->stk[W->sp - 2] = t; }
static void b_over(void) { if (need(2, "over")) push(W->stk[W->sp - 2]); }
static void b_rot(void) { if (!need(3, "rot")) return; V a = W->stk[W->sp - 3]; W->stk[W->sp - 3] = W->stk[W->sp - 2]; W->stk[W->sp - 2] = W->stk[W->sp - 1]; W->stk[W->sp - 1] = a; }
static void b_nip(void) { if (!need(2, "nip")) return; W->stk[W->sp - 2] = W->stk[W->sp - 1]; W->sp--; }
static void b_empty(void) { W->sp = 0; }
static void b_pick(void) {
    i64 n;
    if (!pop_int("pick", &n)) return;
    if (n < 0) { fail("pick needs 0 or more%s", ""); return; }
    if (need((int)n + 1, "pick")) push(W->stk[W->sp - 1 - n]);
}
static void b_tuck(void) { if (!need(2, "tuck")) return; V b = pop(), a = pop(); push(b); push(a); push(b); }
static void b_2dup(void) { if (need(2, "2dup")) { V a = W->stk[W->sp - 2], b = W->stk[W->sp - 1]; push(a); push(b); } }
static void b_depth(void) { push(I(W->sp)); }

static int num(V v) { return v.t == T_INT || v.t == T_BIG; }

/* 0 on overflow */
static int ipow(i64 a, i64 b, i64 *r) {
    *r = 1;
    while (b-- > 0) if (__builtin_mul_overflow(*r, a, r)) return 0;
    return 1;
}

static void arith(char op, const char *w) {
    if (!need(2, w)) return;
    V b = pop(), a = pop();
    if (a.t == T_INT && b.t == T_INT) {
        i64 x = a.n, y = b.n, r = 0;
        int ok = 1;
        switch (op) {
        case '+': ok = !__builtin_add_overflow(x, y, &r); break;
        case '-': ok = !__builtin_sub_overflow(x, y, &r); break;
        case '*': ok = !__builtin_mul_overflow(x, y, &r); break;
        case '/': case '%':
            if (!y) { fail("division by zero%s", ""); return; }
            if (y == -1) { ok = op == '%' || x != (i64)(1ull << 63); r = op == '/' ? -x : 0; break; }
            r = op == '/' ? x / y : x % y;
            break;
        case '^':
            if (y < 0) { fail("negative powers aren't supported%s", ""); return; }
            ok = y < 64 && ipow(x, y, &r);
            break;
        }
        if (ok) { push(I(r)); return; }
    }
    if (num(a) && num(b)) {
        V r = big_arith(op, a, b);
        if (!W->err) push(r);
        return;
    }
    V out;
    if (ord_arith(op, a, b, &out)) { if (!W->err) push(out); return; }
    char m[48];
    snfmt(m, sizeof m, "%s needs numbers", w);
    fail("%s", m);
}

static void b_add(void) { arith('+', "+"); }
static void b_sub(void) { arith('-', "-"); }
static void b_mul(void) { arith('*', "*"); }
static void b_div(void) { arith('/', "/"); }
static void b_mod(void) { arith('%', "mod"); }
static void b_pow(void) { arith('^', "^"); }

static int compare(const char *w, int *r) {
    if (!need(2, w)) return 0;
    V b = pop(), a = pop();
    if (a.t == T_INT && b.t == T_INT) { *r = a.n < b.n ? -1 : a.n > b.n; return 1; }
    if (num(a) && num(b)) { *r = big_cmp(a, b); return 1; }
    if (ord_compare(a, b, r)) return !W->err;
    char m[48];
    snfmt(m, sizeof m, "%s needs numbers", w);
    fail("%s", m);
    return 0;
}

extern const char *undecidable;
static void answer(int r, int yes_lt, int yes_eq, int yes_gt) {
    if (r == 3) { fail("%s", undecidable); return; }
    if (r == 2) { if (yes_lt == yes_eq) push(I(yes_lt)); else if (!yes_lt && !yes_eq) push(I(0)); else fail("%s", undecidable); return; }
    if (r == -2) { if (yes_gt == yes_eq) push(I(yes_gt)); else if (!yes_gt && !yes_eq) push(I(0)); else fail("%s", undecidable); return; }
    push(I(r < 0 ? yes_lt : r == 0 ? yes_eq : yes_gt));
}
static void b_lt(void) { int r; if (compare("<", &r)) answer(r, 1, 0, 0); }
static void b_gt(void) { int r; if (compare(">", &r)) answer(r, 0, 0, 1); }
static void b_le(void) { int r; if (compare("<=", &r)) answer(r, 1, 1, 0); }
static void b_ge(void) { int r; if (compare(">=", &r)) answer(r, 0, 1, 1); }

static void b_eq(void) {
    if (!need(2, "=")) return;
    V b = W->stk[W->sp - 1], a = W->stk[W->sp - 2];
    if ((a.t == T_ORD || a.t == T_CARD || b.t == T_ORD || b.t == T_CARD) && a.t != T_STR && b.t != T_STR) {
        int r;
        if (compare("=", &r)) answer(r, 0, 1, 0);
        return;
    }
    W->sp -= 2;
    int r = 0;
    if (a.t == b.t) {
        if (a.t == T_INT) r = a.n == b.n;
        else if (a.t == T_BIG) r = !big_cmp(a, b);
        else if (a.t == T_STR) r = ((str *)a.p)->len == ((str *)b.p)->len && !strcmp(((str *)a.p)->s, ((str *)b.p)->s);
        else r = a.p == b.p;
    }
    push(I(r));
}

static int ints(const char *w, i64 *a, i64 *b) { return pop_int(w, b) && pop_int(w, a); }
#define IMATH(fname, w, expr) static void fname(void) { i64 a, b; if (ints(w, &a, &b)) push(I(expr)); }
IMATH(b_band, "&", a & b)
IMATH(b_bor, "|", a | b)
IMATH(b_bxor, "xor", a ^ b)
IMATH(b_shl, "<<", (i64)((u64)a << (b & 63)))
IMATH(b_shr, ">>", (i64)((u64)a >> (b & 63)))

/* bignums are never 0 */
static int flag(V c, const char *w, int *out) {
    if (c.t == T_BIG) { *out = 1; return 1; }
    if (!want(c, T_INT, w)) return 0;
    *out = c.n != 0;
    return 1;
}
static void b_not(void) { int a; if (need(1, "not") && flag(pop(), "not", &a)) push(I(!a)); }
static void logic(const char *w, int is_and) {
    int a, b;
    if (!need(2, w)) return;
    V y = pop(), x = pop();
    if (flag(x, w, &a) && flag(y, w, &b)) push(I(is_and ? a && b : a || b));
}
static void b_and(void) { logic("and", 1); }
static void b_or(void) { logic("or", 0); }
static int pop_num(const char *w, V *v) {
    if (!need(1, w)) return 0;
    *v = pop();
    if (num(*v)) return 1;
    want(*v, T_INT, w);
    return 0;
}
static void b_neg(void) { V a; if (pop_num("neg", &a)) push(big_neg(a)); }
static void b_abs(void) { V a; if (pop_num("abs", &a)) push(big_cmp(a, I(0)) < 0 ? big_neg(a) : a); }
static void minmax(const char *w, int want_max) {
    V b, a;
    if (!pop_num(w, &b) || !pop_num(w, &a)) return;
    push((big_cmp(a, b) > 0) == want_max ? a : b);
}
static void b_min(void) { minmax("min", 0); }
static void b_max(void) { minmax("max", 1); }
static void b_bnot(void) { i64 a; if (pop_int("~", &a)) push(I(~a)); }

/* flow */

static void b_do(void) { if (need(1, "do")) call(pop()); }

static void b_if(void) {
    if (!need(3, "if")) return;
    V f = pop(), t = pop(), c = pop();
    int k;
    if (flag(c, "if", &k)) call(k ? t : f);
}

static void b_when(void) {
    if (!need(2, "when")) return;
    V t = pop(), c = pop();
    int k;
    if (flag(c, "when", &k) && k) call(t);
}

static void b_unless(void) {
    if (!need(2, "unless")) return;
    V t = pop(), c = pop();
    int k;
    if (flag(c, "unless", &k) && !k) call(t);
}

static void b_times(void) {
    if (!need(2, "times")) return;
    V q = pop(), n = pop();
    if (!want(n, T_INT, "times")) return;
    keep(q);
    for (i64 i = 0; i < n.n && !W->err && !brk(); i++) call(q);
    unkeep(1);
}

static void b_for(void) {
    if (!need(3, "for")) return;
    V q = pop();
    i64 a, b;
    if (!ints("for", &a, &b)) return;
    keep(q);
    for (i64 i = a; i < b && !W->err && !brk(); i++) { push(I(i)); call(q); }
    unkeep(1);
}

static void b_while(void) {
    if (!need(2, "while")) return;
    V body_ = pop(), cond = pop();
    keep(cond); keep(body_);
    while (!W->err && !brk()) {
        call(cond);
        if (W->err || !need(1, "while")) break;
        V c = pop();
        int k;
        if (!flag(c, "while", &k) || !k) break;
        call(body_);
    }
    unkeep(2);
}

static V as_item(V it) {
    if (it.t == T_WORD || it.t == T_DEF || it.t == T_SET) it.t = T_SYM;
    return it;
}

static void b_each(void) {
    if (!need(2, "each")) return;
    V q = pop(), xs = pop();
    keep(q); keep(xs);
    if (xs.t == T_QUOTE) {
        quote *l = (quote *)xs.p;
        for (u32 i = 0; i < l->n && !W->err && !brk(); i++) { push(as_item(l->v[i])); call(q); }
    } else if (xs.t == T_STR) {
        str *s = (str *)xs.p;
        for (u32 i = 0; i < s->len && !W->err && !brk(); i++) { push(I((u8)s->s[i])); call(q); }
    } else fail("each needs a list or a string%s", "");
    unkeep(2);
}

static void b_map(void) {
    if (!need(2, "map")) return;
    V q = pop(), xs = pop();
    if (!want(xs, T_QUOTE, "map")) return;
    quote *l = (quote *)xs.p;
    keep(q); keep(xs);
    int base = W->sp;
    for (u32 i = 0; i < l->n && !W->err && !brk(); i++) {
        int before = W->sp;
        push(as_item(l->v[i]));
        call(q);
        if (!W->err && W->sp != before + 1) fail("map's quote has to leave exactly one value%s", "");
    }
    unkeep(2);
    if (W->err) return;
    quote *r = quote_new(&W->stk[base], (u32)(W->sp - base));
    W->sp = base;
    push(Q(r));
}

static void b_filter(void) {
    if (!need(2, "filter")) return;
    V q = pop(), xs = pop();
    if (!want(xs, T_QUOTE, "filter")) return;
    quote *l = (quote *)xs.p;
    keep(q); keep(xs);
    int base = W->sp;
    for (u32 i = 0; i < l->n && !W->err && !brk(); i++) {
        V it = as_item(l->v[i]);
        push(it);
        call(q);
        i64 yes;
        if (W->err || !pop_int("filter", &yes)) break;
        if (yes) push(it);
    }
    unkeep(2);
    if (W->err) return;
    quote *r = quote_new(&W->stk[base], (u32)(W->sp - base));
    W->sp = base;
    push(Q(r));
}

static void b_append(void) {
    if (!need(2, "append")) return;
    V it = pop(), xs = pop();
    if (!want(xs, T_QUOTE, "append")) return;
    quote *l = (quote *)xs.p, *q = quote_new(l->v, l->n + 1);
    if (!q) return;
    q->v[l->n] = it;
    push(Q(q));
}

static void b_range(void) {
    i64 a, b;
    if (!ints("range", &a, &b)) return;
    if (b - a > 100000 || b < a) { fail("that range is too big%s", ""); return; }
    quote *q = quote_new(NULL, (u32)(b - a));
    if (!q) return;
    for (i64 i = a; i < b; i++) q->v[i - a] = I(i);
    push(Q(q));
}

static void b_list(void) {
    i64 n;
    if (!pop_int("list", &n)) return;
    if (n < 0 || !need((int)n, "list")) return;
    W->sp -= (int)n;
    push(Q(quote_new(&W->stk[W->sp], (u32)n)));
}

static void b_spread(void) {
    if (!need(1, "spread")) return;
    V xs = pop();
    if (!want(xs, T_QUOTE, "spread")) return;
    quote *l = (quote *)xs.p;
    for (u32 i = 0; i < l->n && !W->err; i++) push(as_item(l->v[i]));
}

static void b_first(void) {
    if (!need(1, "first")) return;
    V xs = pop();
    if (!want(xs, T_QUOTE, "first")) return;
    if (!((quote *)xs.p)->n) { fail("that list is empty%s", ""); return; }
    push(as_item(((quote *)xs.p)->v[0]));
}

static void b_last(void) {
    if (!need(1, "last")) return;
    V xs = pop();
    if (!want(xs, T_QUOTE, "last")) return;
    quote *l = (quote *)xs.p;
    if (!l->n) { fail("that list is empty%s", ""); return; }
    push(as_item(l->v[l->n - 1]));
}

static void b_rest(void) {
    if (!need(1, "rest")) return;
    V xs = pop();
    if (!want(xs, T_QUOTE, "rest")) return;
    quote *l = (quote *)xs.p;
    push(Q(quote_new(l->n ? l->v + 1 : l->v, l->n ? l->n - 1 : 0)));
}

static int same(V a, V b) {
    if (a.t != b.t) return 0;
    if (a.t == T_INT) return a.n == b.n;
    if (a.t == T_STR) return !strcmp(((str *)a.p)->s, ((str *)b.p)->s);
    return a.p == b.p;
}

static void b_index(void) {
    if (!need(2, "index")) return;
    V it = pop(), xs = pop();
    if (!want(xs, T_QUOTE, "index")) return;
    quote *l = (quote *)xs.p;
    for (u32 i = 0; i < l->n; i++) if (same(as_item(l->v[i]), it)) { push(I(i)); return; }
    push(I(-1));
}

static void b_has(void) {   /* list item has */
    if (!need(2, "has")) return;
    V it = pop(), xs = pop();
    if (!want(xs, T_QUOTE, "has")) return;
    quote *l = (quote *)xs.p;
    for (u32 i = 0; i < l->n; i++) if (same(as_item(l->v[i]), it)) { push(I(1)); return; }
    push(I(0));
}

static void b_alloc(void) {
    i64 n;
    if (!pop_int("alloc", &n)) return;
    if (n <= 0 || n > 16 * 1024 * 1024) { fail("alloc takes 1 byte up to 16M%s", ""); return; }
    void *p = kmalloc_try((u64)n);
    if (!p) { fail("ran out of memory%s", ""); return; }
    memset(p, 0, (u64)n);
    push(I((i64)(u64)p));
}

static void b_free(void) { i64 a; if (pop_int("free", &a)) kfree((void *)(u64)a); }

/* text */

static void b_len(void) {
    if (!need(1, "len")) return;
    V v = pop();
    if (v.t == T_STR) push(I(((str *)v.p)->len));
    else if (v.t == T_QUOTE) push(I(((quote *)v.p)->n));
    else fail("len needs a string or a list%s", "");
}

static void b_type(void) {
    if (!need(1, "type")) return;
    static const char *const names[] = { "nothing", "number", "string", "list", "symbol", "word", "word", "word",
                                         "set", "ordinal", "cardinal", "number" };
    V v = pop();
    const char *n = v.t < sizeof names / sizeof *names ? names[v.t] : "word";
    push(S(str_new(n, (u32)strlen(n))));
}

static void b_cat(void) {
    if (!need(2, "cat")) return;
    V b = pop(), a = pop();
    if (a.t == T_STR && b.t == T_STR) {
        str *x = (str *)a.p, *y = (str *)b.p, *r = (str *)gnew(sizeof(str) + x->len + y->len + 1, O_STR);
        if (!r) return;
        r->len = x->len + y->len;
        r->fn = NULL;
        memcpy(r->s, x->s, x->len);
        memcpy(r->s + x->len, y->s, y->len + 1);
        push(S(r));
    } else if (a.t == T_QUOTE && b.t == T_QUOTE) {
        quote *x = (quote *)a.p, *y = (quote *)b.p, *q = quote_new(x->v, x->n + y->n);
        if (!q) return;
        memcpy(q->v + x->n, y->v, (u64)y->n * sizeof(V));
        push(Q(q));
    } else fail("cat needs two strings or two lists%s", "");
}

static void b_str(void) {
    if (!need(1, "str")) return;
    if (W->stk[W->sp - 1].t == T_BIG) {
        V v = pop();
        u32 cap = big_digits(v);
        char *t = kmalloc_try(cap);
        if (!t) { fail("ran out of memory%s", ""); return; }
        sbuf o = { t, (int)cap, 0 };
        t[0] = 0;
        repr(&o, v, 1);
        push(S(str_new(t, (u32)o.len)));
        kfree(t);
        return;
    }
    char t[512];
    sbuf o = { t, sizeof t, 0 };
    t[0] = 0;
    repr(&o, pop(), 1);
    push(S(str_new(t, (u32)o.len)));
}

static void b_hex(void) {
    i64 v;
    if (!pop_int("hex", &v)) return;
    char t[24];
    snfmt(t, sizeof t, "%lx", (u64)v);
    push(S(str_new(t, (u32)strlen(t))));
}

static void b_chr(void) {
    i64 v;
    if (!pop_int("chr", &v)) return;
    char c = (char)v;
    push(S(str_new(&c, 1)));
}

static void b_at(void) {
    i64 i;
    if (!pop_int("at", &i) || !need(1, "at")) return;
    V xs = pop();
    if (xs.t == T_QUOTE && i >= 0 && (u64)i < ((quote *)xs.p)->n) push(as_item(((quote *)xs.p)->v[i]));
    else if (xs.t == T_STR && i >= 0 && (u64)i < ((str *)xs.p)->len) push(I((u8)((str *)xs.p)->s[i]));
    else fail("that position is outside the list%s", "");
}

static void b_slice(void) {
    i64 a, n;
    str *s;
    if (!pop_int("slice", &n) || !pop_int("slice", &a) || !pop_str("slice", &s)) return;
    if (a < 0) a = 0;
    if (a > s->len) a = s->len;
    if (n < 0 || a + n > s->len) n = s->len - a;
    push(S(str_new(s->s + a, (u32)n)));
}

static void b_pad(void) {
    i64 n;
    str *s;
    if (!pop_int("pad", &n) || !pop_str("pad", &s)) return;
    int left = n < 0;
    u64 w = (u64)(left ? -n : n);
    if (w <= s->len || w > 4096) { push(S(s)); return; }
    char *t = kmalloc_try(w);
    if (!t) { fail("ran out of memory%s", ""); return; }
    memset(t, ' ', w);
    memcpy(left ? t + w - s->len : t, s->s, s->len);
    push(S(str_new(t, (u32)w)));
    kfree(t);
}

static void b_find(void) {
    str *h, *n;
    if (!pop_str("find", &n) || !pop_str("find", &h)) return;
    const char *p = strstr(h->s, n->s);
    push(I(p ? p - h->s : -1));
}

static void b_split(void) {
    str *s, *sep;
    if (!pop_str("split", &sep) || !pop_str("split", &s)) return;
    V parts[256];
    u32 k = 0;
    const char *p = s->s;
    for (;;) {
        const char *q = sep->len ? strstr(p, sep->s) : NULL;
        u32 l = q ? (u32)(q - p) : (u32)(s->s + s->len - p);
        if (k == 256) { fail("that splits into too many parts%s", ""); return; }
        parts[k++] = S(str_new(p, l));
        if (!q) break;
        p = q + sep->len;
    }
    push(Q(quote_new(parts, k)));
}

static void b_tonum(void) {
    str *s;
    if (!pop_str(">num", &s)) return;
    quote *q = parse_text(s->s);
    if (q && q->n == 1 && num(q->v[0])) push(q->v[0]);
    else fail("not a number: %s", s->s);
}

static void b_upper(void) {
    str *s;
    if (!pop_str("upper", &s)) return;
    str *r = str_new(s->s, s->len);
    if (!r) return;
    for (u32 i = 0; i < r->len; i++) if (r->s[i] >= 'a' && r->s[i] <= 'z') r->s[i] -= 32;
    push(S(r));
}

static void b_lower(void) {
    str *s;
    if (!pop_str("lower", &s)) return;
    str *r = str_new(s->s, s->len);
    if (!r) return;
    for (u32 i = 0; i < r->len; i++) if (r->s[i] >= 'A' && r->s[i] <= 'Z') r->s[i] += 32;
    push(S(r));
}

static int blank(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

static void b_trim(void) {
    str *s;
    if (!pop_str("trim", &s)) return;
    u32 a = 0, b = s->len;
    while (a < b && blank(s->s[a])) a++;
    while (b > a && blank(s->s[b - 1])) b--;
    push(S(str_new(s->s + a, b - a)));
}

static void b_starts(void) {
    str *s, *p;
    if (!pop_str("starts?", &p) || !pop_str("starts?", &s)) return;
    push(I(p->len <= s->len && !memcmp(s->s, p->s, p->len)));
}

static void b_ends(void) {
    str *s, *p;
    if (!pop_str("ends?", &p) || !pop_str("ends?", &s)) return;
    push(I(p->len <= s->len && !memcmp(s->s + s->len - p->len, p->s, p->len)));
}

static void b_replace(void) {
    str *s, *from, *to;
    if (!pop_str("replace", &to) || !pop_str("replace", &from) || !pop_str("replace", &s)) return;
    if (!from->len) { fail("replace needs something to look for%s", ""); return; }
    u64 n = 0;
    for (const char *p = s->s; (p = strstr(p, from->s)) && p < s->s + s->len; p += from->len) n++;
    u64 len = s->len + n * to->len - n * from->len;
    if (len > 1 << 20) { fail("that string would be too long%s", ""); return; }
    str *r = (str *)gnew(sizeof(str) + len + 1, O_STR);
    if (!r) return;
    r->len = (u32)len;
    r->fn = NULL;
    char *o = r->s;
    const char *p = s->s, *q;
    while ((q = strstr(p, from->s)) && q < s->s + s->len) {
        memcpy(o, p, (u64)(q - p)); o += q - p;
        memcpy(o, to->s, to->len); o += to->len;
        p = q + from->len;
    }
    memcpy(o, p, (u64)(s->s + s->len - p));
    r->s[len] = 0;
    push(S(r));
}

static void b_join(void) {
    str *sep;
    if (!pop_str("join", &sep) || !need(1, "join")) return;
    V xs = pop();
    if (!want(xs, T_QUOTE, "join")) return;
    quote *l = (quote *)xs.p;
    char *t = kmalloc_try(65536);
    if (!t) { fail("ran out of memory%s", ""); return; }
    sbuf o = { t, 65536, 0 };
    t[0] = 0;
    for (u32 i = 0; i < l->n; i++) {
        if (i) sput(&o, sep->s, sep->len);
        repr(&o, as_item(l->v[i]), 1);
    }
    if (o.len >= o.cap - 1) fail("that string would be too long%s", "");
    else push(S(str_new(t, (u32)o.len)));
    kfree(t);
}

static void b_reverse(void) {
    if (!need(1, "reverse")) return;
    V v = pop();
    if (v.t == T_STR) {
        str *s = (str *)v.p, *r = str_new(s->s, s->len);
        if (!r) return;
        for (u32 i = 0; i < r->len / 2; i++) { char c = r->s[i]; r->s[i] = r->s[r->len - 1 - i]; r->s[r->len - 1 - i] = c; }
        push(S(r));
    } else if (v.t == T_QUOTE) {
        quote *l = (quote *)v.p, *q = quote_new(NULL, l->n);
        if (!q) return;
        for (u32 i = 0; i < l->n; i++) q->v[i] = l->v[l->n - 1 - i];
        push(Q(q));
    } else fail("reverse needs a list or a string%s", "");
}

static int order(V a, V b) {
    if (num(a) && num(b)) return big_cmp(a, b);
    if (a.t == T_STR && b.t == T_STR) return strcmp(((str *)a.p)->s, ((str *)b.p)->s);
    return 2;
}

static void b_sort(void) {
    if (!need(1, "sort")) return;
    V xs = pop();
    if (!want(xs, T_QUOTE, "sort")) return;
    quote *l = (quote *)xs.p;
    int strs = l->n && l->v[0].t == T_STR;
    for (u32 i = 0; i < l->n; i++)
        if (strs ? l->v[i].t != T_STR : !num(l->v[i])) { fail("sort needs a list of numbers or a list of strings%s", ""); return; }
    quote *q = quote_new(l->v, l->n);
    V *tmp = kmalloc_try((u64)l->n * sizeof(V) + 1);
    if (!q || !tmp) { kfree(tmp); if (q) fail("ran out of memory%s", ""); return; }
    /* bottom-up merge */
    for (u32 w = 1; w < q->n; w *= 2) {
        for (u32 lo = 0; lo < q->n; lo += 2 * w) {
            u32 mid = MIN(lo + w, q->n), hi = MIN(lo + 2 * w, q->n), i = lo, j = mid, k = lo;
            while (i < mid && j < hi) tmp[k++] = order(q->v[j], q->v[i]) < 0 ? q->v[j++] : q->v[i++];
            while (i < mid) tmp[k++] = q->v[i++];
            while (j < hi) tmp[k++] = q->v[j++];
        }
        memcpy(q->v, tmp, (u64)q->n * sizeof(V));
    }
    kfree(tmp);
    push(Q(q));
}

static void b_sqrt(void) {
    V a;
    if (!pop_num("sqrt", &a)) return;
    if (big_cmp(a, I(0)) < 0) { fail("sqrt needs a number that isn't negative%s", ""); return; }
    if (a.t == T_INT) {
        u64 n = (u64)a.n, x = n, y = (x + 1) / 2;
        if (n < 2) { push(a); return; }
        while (y < x) { x = y; y = (x + n / x) / 2; }
        push(I((i64)x));
        return;
    }
    V x = a, y = big_arith('/', big_arith('+', x, I(1)), I(2));
    while (!W->err && big_cmp(y, x) < 0) { x = y; y = big_arith('/', big_arith('+', x, big_arith('/', a, x)), I(2)); }
    if (!W->err) push(x);
}

static void b_gcd(void) {
    V b, a;
    if (!pop_num("gcd", &b) || !pop_num("gcd", &a)) return;
    if (big_cmp(a, I(0)) < 0) a = big_neg(a);
    if (big_cmp(b, I(0)) < 0) b = big_neg(b);
    while (!W->err && big_cmp(b, I(0)) != 0) { V t = big_arith('%', a, b); a = b; b = t; }
    if (!W->err) push(a);
}

static void b_print(void) { if (need(1, ".")) { repr(NULL, pop(), 0); kputc('\n'); } }
static void b_say(void) { if (need(1, "say")) { repr(NULL, pop(), 1); kputc('\n'); } }
static void b_write(void) { if (need(1, "write")) repr(NULL, pop(), 1); }
static void b_emit(void) { i64 c; if (pop_int("emit", &c)) kputc((char)c); }
static void b_nl(void) { kputc('\n'); }
static void b_color(void) { i64 c; if (pop_int("color", &c)) vga_color((u8)(c & 15)); }

/* the world */

static int sym(const char *w, str **out) {
    if (!need(1, w)) return 0;
    V v = pop();
    if (!want(v, T_SYM, w)) return 0;
    *out = (str *)v.p;
    return 1;
}

static void b_get(void) {
    str *s;
    if (!sym("get", &s)) return;
    bind *b = lookup(s);
    if (!b) fail("%s isn't defined", s->s);
    else push(b->v);
}

static void b_forget(void) {
    str *s;
    if (!sym("forget", &s)) return;
    bind *b = W->dict;
    while (b && b->name != s) b = b->next;
    if (!b || b->v.t == T_NONE) { fail("%s isn't defined in this tab", s->s); return; }
    define(s, (V){ T_NONE, 0, NULL });
}

static void b_tosym(void) {
    str *s;
    if (pop_str(">sym", &s)) push((V){ T_SYM, 0, &intern(s->s, s->len)->h });
}

static void b_defined(void) {
    str *s;
    if (sym("defined?", &s)) push(I(lookup(s) || s->fn));
}

static void b_see(void) {
    str *s;
    if (!sym("see", &s)) return;
    bind *b = lookup(s);
    if (b) {
        int in_root = 1;
        for (bind *c = W->dict; c; c = c->next) if (c == b) { in_root = 0; break; }
        vga_color(C_CAT);
        kprintf("  %s%s ", s->s, in_root ? " (root)" : "");
        vga_color(C_TEXT);
        repr(NULL, b->v, 0);
        kputc('\n');
    } else if (s->fn) kprintf("  %s: builtin\n", s->s);
    else fail("%s isn't defined", s->s);
}

static void list_dict(bind *d, bind *stop, u8 color) {
    for (bind *b = d; b && b != stop; b = b->next) {
        int shadowed = 0;
        for (bind *c = d; c != b; c = c->next)
            if (c->name == b->name) { shadowed = 1; break; }
        if (shadowed || b->v.t == T_NONE) continue;
        char t[56];
        sbuf o = { t, sizeof t, 0 };
        t[0] = 0;
        repr(&o, b->v, 0);
        vga_color(color);
        kprintf("  %-16s", b->name->s);
        vga_color(C_DIM);
        kprintf("%s\n", t);
    }
}

static void b_words(void) {
    if (!W->dict) {
        vga_color(C_DIM);
        kputs("  nothing is defined in this tab\n");
        vga_color(C_TEXT);
        return;
    }
    list_dict(W->dict, NULL, C_CAT);
    vga_color(C_TEXT);
}

static void b_back(void) { W->nback++; }
static void b_redo(void) { W->nredo++; }

static void b_eval(void) {
    str *s;
    if (!pop_str("eval", &s)) return;
    quote *q = parse_text(s->s);
    if (q) call(Q(q));
}

static void b_fail(void) {
    str *s;
    if (pop_str("fail", &s)) fail("%s", s->s);
}

static void b_try(void) {
    if (!need(1, "try")) return;
    V q = pop();
    call(q);
    if (W->err) {
        vga_color(C_LRED);
        kprintf("! %s\n", W->errmsg);
        vga_color(C_TEXT);
        W->err = 0;
        push(I(0));
    } else push(I(1));
}

static void b_trace(void) {
    if (!need(1, "trace")) return;
    V q = pop();
    int was = W->tracing;
    W->tracing = 1;
    call(q);
    W->tracing = was;
}

static void b_edit(void) {
    str *s;
    if (!sym("edit", &s)) return;
    bind *b = lookup(s);
    char *text = kmalloc_try(16384);
    if (!text) { fail("ran out of memory%s", ""); return; }
    sbuf o = { text, 16384, 0 };
    text[0] = 0;
    if (b && b->v.t == T_QUOTE) body(&o, (quote *)b->v.p);
    else if (b) repr(&o, b->v, 1);
    u32 outlen;
    char *out = scrawl(s->s, text, (u32)o.len, &outlen);
    kfree(text);
    if (!out) return;
    if (!b || b->v.t == T_QUOTE) {
        quote *q = parse_text(out);
        if (q) { define(s, Q(q)); kfree(out); return; }
        W->err = 0;
        vga_color(C_PEACH);
        kprintf("  that didn't parse (%s), so it was kept as a string\n", W->errmsg);
        vga_color(C_TEXT);
    }
    define(s, S(str_new(out, outlen)));
    kfree(out);
}

static void b_calc(void) { calc(); }


const struct builtin core_words[] = {
    { "dup", b_dup }, { "drop", b_drop }, { "swap", b_swap }, { "over", b_over }, { "rot", b_rot },
    { "nip", b_nip }, { "empty", b_empty }, { "pick", b_pick }, { "tuck", b_tuck }, { "2dup", b_2dup }, { "depth", b_depth },
    { "+", b_add }, { "-", b_sub }, { "*", b_mul }, { "/", b_div }, { "mod", b_mod }, { "^", b_pow },
    { "<", b_lt }, { ">", b_gt }, { "<=", b_le }, { ">=", b_ge }, { "=", b_eq },
    { "&", b_band }, { "|", b_bor }, { "xor", b_bxor }, { "<<", b_shl }, { ">>", b_shr },
    { "min", b_min }, { "max", b_max }, { "and", b_and }, { "or", b_or }, { "not", b_not },
    { "neg", b_neg }, { "abs", b_abs }, { "~", b_bnot },
    { "do", b_do }, { "if", b_if }, { "when", b_when }, { "unless", b_unless }, { "times", b_times },
    { "for", b_for }, { "while", b_while }, { "each", b_each }, { "map", b_map }, { "filter", b_filter },
    { "append", b_append }, { "range", b_range }, { "list", b_list }, { "spread", b_spread },
    { "first", b_first }, { "last", b_last }, { "rest", b_rest }, { "has", b_has }, { "index", b_index },
    { "alloc", b_alloc }, { "free", b_free },
    { "len", b_len }, { "type", b_type }, { "cat", b_cat }, { "str", b_str }, { "hex", b_hex }, { "chr", b_chr }, { "at", b_at },
    { "slice", b_slice }, { "pad", b_pad }, { "find", b_find }, { "split", b_split }, { ">num", b_tonum },
    { "upper", b_upper }, { "lower", b_lower }, { "trim", b_trim }, { "starts?", b_starts }, { "ends?", b_ends },
    { "replace", b_replace }, { "join", b_join }, { "reverse", b_reverse }, { "sort", b_sort },
    { "sqrt", b_sqrt }, { "gcd", b_gcd },
    { ".", b_print }, { "say", b_say }, { "write", b_write }, { "emit", b_emit }, { "nl", b_nl }, { "color", b_color },
    { "get", b_get }, { "forget", b_forget }, { ">sym", b_tosym }, { "defined?", b_defined }, { "see", b_see },
    { "words", b_words }, { "back", b_back }, { "redo", b_redo }, { "eval", b_eval }, { "try", b_try }, { "fail", b_fail }, { "trace", b_trace }, { "edit", b_edit },
    { "calc", b_calc },
    { NULL, NULL }
};
