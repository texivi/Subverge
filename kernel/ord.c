#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"

typedef struct ord ord;
struct oterm { ord *e; u64 c; };
struct ord { obj h; u8 atom; u32 n; struct oterm t[]; };

enum { A_NONE, A_E0, A_W1 };
static ord *atom_e0, *atom_w1;

#define G_OMEGA "\x80"
#define G_ALEPH "\x81"
#define G_SUB0 0x82
#define G_EPS "\xee"

static ord *ord_new(u32 n) {
    ord *o = (ord *)gnew(sizeof(ord) + (u64)n * sizeof(struct oterm), O_ORD);
    if (!o) return NULL;
    o->atom = A_NONE;
    o->n = n;
    return o;
}

static V OV(ord *o) { return (V){ o ? T_ORD : T_NONE, 0, o ? &o->h : NULL }; }

void ord_mark(obj *o) {
    ord *x = (ord *)o;
    for (u32 i = 0; i < x->n; i++) if (x->t[i].e) mark(&x->t[i].e->h);
}

static ord *fin(u64 k) {
    ord *o = ord_new(k ? 1 : 0);
    if (o && k) o->t[0] = (struct oterm){ NULL, k };
    return o;
}

static int is_zero(ord *a) { return !a || (!a->atom && a->n == 0); }

static const struct oterm *terms(ord *a, u32 *n, struct oterm *scratch) {
    if (!a) { *n = 0; return NULL; }
    if (a->atom) { scratch->e = a; scratch->c = 1; *n = 1; return scratch; }
    *n = a->n;
    return a->t;
}

static int cmp(ord *a, ord *b) {
    if (a == b) return 0;
    if (a && b && a->atom && b->atom) return a->atom < b->atom ? -1 : 1;
    struct oterm sa, sb;
    u32 na, nb;
    const struct oterm *ta = terms(a, &na, &sa), *tb = terms(b, &nb, &sb);
    for (u32 i = 0; i < na && i < nb; i++) {
        int c = cmp(ta[i].e, tb[i].e);
        if (c) return c;
        if (ta[i].c != tb[i].c) return ta[i].c < tb[i].c ? -1 : 1;
    }
    return na < nb ? -1 : na > nb;
}

static ord *norm_exp(ord *e) {
    if (e && !e->atom && e->n == 1 && e->t[0].c == 1 && e->t[0].e && e->t[0].e->atom) return e->t[0].e;
    return e;
}

static ord *build(const struct oterm *t, u32 n) {
    ord *o = ord_new(n);
    if (!o) return NULL;
    for (u32 i = 0; i < n; i++) o->t[i] = (struct oterm){ norm_exp(t[i].e), t[i].c };
    if (n == 1 && o->t[0].c == 1 && o->t[0].e && o->t[0].e->atom) return o->t[0].e;
    return o;
}

static ord *add(ord *a, ord *b) {
    if (is_zero(b)) return a ? a : fin(0);
    if (is_zero(a)) return b;
    struct oterm sa, sb, out[64];
    u32 na, nb, k = 0;
    const struct oterm *ta = terms(a, &na, &sa), *tb = terms(b, &nb, &sb);
    ord *lead = tb[0].e;
    u64 carry = 0;
    for (u32 i = 0; i < na; i++) {
        int c = cmp(ta[i].e, lead);
        if (c > 0) { if (k < 64) out[k++] = ta[i]; }
        else if (c == 0) carry = ta[i].c;
    }
    for (u32 i = 0; i < nb && k < 64; i++) {
        out[k] = tb[i];
        if (i == 0 && __builtin_add_overflow(out[k].c, carry, &out[k].c)) { fail("that coefficient doesn't fit in 64 bits%s", ""); return NULL; }
        k++;
    }
    return build(out, k);
}

static ord *mul(ord *a, ord *b) {
    if (is_zero(a) || is_zero(b)) return fin(0);
    struct oterm sa, sb;
    u32 na, nb;
    const struct oterm *ta = terms(a, &na, &sa), *tb = terms(b, &nb, &sb);
    ord *r = fin(0);
    for (u32 i = 0; i < nb && !W->err; i++) {
        ord *piece;
        if (!tb[i].e) {
            struct oterm out[64];
            u32 k = MIN(na, 64u);
            for (u32 j = 0; j < k; j++) out[j] = ta[j];
            if (__builtin_mul_overflow(out[0].c, tb[i].c, &out[0].c)) { fail("that coefficient doesn't fit in 64 bits%s", ""); return NULL; }
            piece = build(out, k);
        } else {
            struct oterm one = { add(ta[0].e ? ta[0].e : fin(0), tb[i].e), tb[i].c };
            if (is_zero(one.e)) one.e = NULL;
            piece = build(&one, 1);
        }
        r = add(r, piece);
    }
    return r;
}

static int finite(ord *a, u64 *k) {
    if (is_zero(a)) { *k = 0; return 1; }
    if (a->atom || a->n != 1 || a->t[0].e) return 0;
    *k = a->t[0].c;
    return 1;
}

static void split(ord *b, ord **lim, u64 *n) {
    struct oterm sb;
    u32 nb;
    const struct oterm *tb = terms(b, &nb, &sb);
    *n = 0;
    if (nb && !tb[nb - 1].e) { *n = tb[nb - 1].c; nb--; }
    *lim = build(tb, nb);
}

static ord *power(ord *a, ord *b) {
    u64 x, y;
    if (is_zero(b)) return fin(1);
    if (is_zero(a)) return fin(0);
    if (finite(a, &x) && x == 1) return fin(1);
    if (finite(a, &x) && finite(b, &y)) { u64 r = 1; while (y--) r *= x; return fin(r); }
    ord *lim;
    u64 n;
    split(b, &lim, &n);
    if (finite(a, &x)) {
        struct oterm sl, out[64];
        u32 nl;
        const struct oterm *tl = terms(lim, &nl, &sl);
        u32 k = MIN(nl, 64u);
        for (u32 i = 0; i < k; i++) {
            out[i] = tl[i];
            u64 f;
            if (finite(tl[i].e, &f)) out[i].e = f > 1 ? fin(f - 1) : NULL;
        }
        ord *bp = build(out, k);
        u64 kn = 1;
        while (n--) if (__builtin_mul_overflow(kn, x, &kn)) { fail("that coefficient doesn't fit in 64 bits%s", ""); return NULL; }
        struct oterm t = { bp, 1 };
        return mul(build(&t, 1), fin(kn));
    }
    struct oterm sa;
    u32 na;
    const struct oterm *ta = terms(a, &na, &sa);
    ord *r = fin(1);
    if (!is_zero(lim)) {
        struct oterm t = { mul(ta[0].e ? ta[0].e : fin(0), lim), 1 };
        r = build(&t, 1);
    }
    if (n > 64) { fail("that exponent is too big to write out%s", ""); return NULL; }
    while (n-- && !W->err) r = mul(r, a);
    return r;
}

/* printing */

static void put_ord(sbuf *o, ord *a);

static int simple(ord *e) {
    u64 k;
    if (!e || e->atom || finite(e, &k)) return 1;
    return e->n == 1 && e->t[0].c == 1 && (!e->t[0].e || simple(e->t[0].e));
}

static void put_ord(sbuf *o, ord *a) {
    char t[24];
    if (is_zero(a)) { sputs(o, "0"); return; }
    if (a->atom == A_E0) { char s[3] = { G_EPS[0], (char)G_SUB0, 0 }; sputs(o, s); return; }
    if (a->atom == A_W1) { char s[3] = { G_OMEGA[0], (char)(G_SUB0 + 1), 0 }; sputs(o, s); return; }
    for (u32 i = 0; i < a->n; i++) {
        if (i) sputs(o, "+");
        ord *e = a->t[i].e;
        u64 k;
        if (!e) { snfmt(t, sizeof t, "%lu", a->t[i].c); sputs(o, t); continue; }
        if (e->atom) put_ord(o, e);
        else {
            sputs(o, G_OMEGA);
            if (!(finite(e, &k) && k == 1)) {
                if (finite(e, &k) && k == 2) sputs(o, "\xfd");
                else {
                    sputs(o, "^");
                    if (!simple(e)) sputs(o, "(");
                    put_ord(o, e);
                    if (!simple(e)) sputs(o, ")");
                }
            }
        }
        if (a->t[i].c > 1) { snfmt(t, sizeof t, "\xfa%lu", a->t[i].c); sputs(o, t); }
    }
}

static void put_aleph(sbuf *o, i64 k) {
    char t[24];
    sputs(o, G_ALEPH);
    if (k < 10) { char s[2] = { (char)(G_SUB0 + k), 0 }; sputs(o, s); }
    else { snfmt(t, sizeof t, "_%ld", k); sputs(o, t); }
}

void ord_repr(sbuf *o, V v) {
    if (v.t == T_ORD) { put_ord(o, (ord *)v.p); return; }
    if (v.n >= 0) put_aleph(o, v.n);
    else { sputs(o, "2^"); put_aleph(o, -v.n - 1); }
}

static void src_ord(sbuf *o, ord *a) {
    char t[24];
    if (is_zero(a)) { sputs(o, "0"); return; }
    if (a->atom == A_E0) { sputs(o, "epsilon0"); return; }
    if (a->atom == A_W1) { sputs(o, "omega1"); return; }
    for (u32 i = 0; i < a->n; i++) {
        if (i) sputs(o, " ");
        ord *e = a->t[i].e;
        u64 k;
        if (!e) { snfmt(t, sizeof t, "%lu", a->t[i].c); sputs(o, t); }
        else if (e->atom) src_ord(o, e);
        else {
            sputs(o, "omega");
            if (!(finite(e, &k) && k == 1)) { sputs(o, " "); src_ord(o, e); sputs(o, " ^"); }
        }
        if (e && a->t[i].c > 1) { snfmt(t, sizeof t, " %lu *", a->t[i].c); sputs(o, t); }
        if (i) sputs(o, " +");
    }
}

void ord_source(sbuf *o, V v) {
    char t[32];
    if (v.t == T_ORD) { src_ord(o, (ord *)v.p); return; }
    if (v.n >= 0) snfmt(t, sizeof t, "%ld aleph", v.n);
    else snfmt(t, sizeof t, "2 %ld aleph ^", -v.n - 1);
    sputs(o, t);
}

/* arithmetic glue */

static ord *to_ord(V v) {
    if (v.t == T_ORD) return (ord *)v.p;
    if (v.t == T_INT && v.n >= 0) return fin((u64)v.n);
    return NULL;
}

static V ord_value(ord *r) {
    u64 k;
    if (r && finite(r, &k) && k < (1ull << 62)) return I((i64)k);
    return OV(r);
}

static V CARD(i64 n) { return (V){ T_CARD, n, NULL }; }

const char *undecidable =
    "ZFC can't decide this one (continuum hypothesis, Godel 1940 and Cohen 1963)";

static int finite_v(V v) { return v.t == T_INT || v.t == T_BIG; }

static int card_cmp(V a, V b) {
    if (finite_v(a) && finite_v(b)) return big_cmp(a, b);
    if (finite_v(a)) return -1;
    if (finite_v(b)) return 1;
    if (a.n >= 0 && b.n >= 0) return a.n < b.n ? -1 : a.n > b.n;
    if (a.n < 0 && b.n < 0) {
        if (a.n == b.n) return 0;
        return a.n > b.n ? 2 : -2;
    }
    if (a.n < 0) { int c = card_cmp(b, a); return c == 3 ? 3 : -c; }
    i64 j = a.n, k = -b.n - 1;
    if (j <= k) return -1;
    if (j == k + 1) return 2;
    return 3;
}

static int is_card(V v) { return v.t == T_CARD || (finite_v(v) && big_cmp(v, I(0)) >= 0); }

int ord_arith(char op, V a, V b, V *out) {
    if (a.t == T_CARD || b.t == T_CARD) {
        if (!is_card(a) || !is_card(b)) { fail("can't mix cardinals with that%s", ""); return 1; }
        if (op == '+' || op == '*') {
            if (op == '*' && ((a.t == T_INT && a.n == 0) || (b.t == T_INT && b.n == 0))) { *out = I(0); return 1; }
            int c = card_cmp(a, b);
            if (c == 3) { fail("%s", undecidable); return 1; }
            *out = c == 0 || c == 1 || c == -2 ? a : b;
            return 1;
        }
        if (op == '^') {
            if (finite_v(a) && big_cmp(a, I(2)) >= 0 && b.t == T_CARD && b.n >= 0) { *out = CARD(-b.n - 1); return 1; }   /* power set */
            if (a.t == T_CARD && b.t == T_INT && b.n >= 1) { *out = a; return 1; }
            fail("that cardinal power isn't supported%s", "");
            return 1;
        }
        fail("cardinals only support + * and ^%s", "");
        return 1;
    }
    if (a.t != T_ORD && b.t != T_ORD) return 0;
    ord *x = to_ord(a), *y = to_ord(b);
    if ((a.t == T_BIG || b.t == T_BIG) && (!x || !y)) {
        V f = a.t == T_BIG ? a : b;
        if (big_cmp(f, I(0)) < 0) { fail("ordinals are never negative%s", ""); return 1; }
        ord *o = a.t == T_BIG ? y : x;
        if (op == '+' && a.t == T_BIG) { *out = b; return 1; }
        if (op == '*' && a.t == T_BIG) { *out = b; return 1; }
        (void)o;
        fail("that number is too big to mix with an ordinal that way%s", "");
        return 1;
    }
    if (!x || !y) { fail("ordinals are never negative%s", ""); return 1; }
    ord *r;
    switch (op) {
    case '+': r = add(x, y); break;
    case '*': r = mul(x, y); break;
    case '^': r = power(x, y); break;
    default: fail("ordinals only support + * and ^%s", ""); return 1;
    }
    if (!W->err) *out = ord_value(r);
    return 1;
}

int ord_compare(V a, V b, int *out) {
    if (a.t == T_CARD || b.t == T_CARD) {
        if (!is_card(a) || !is_card(b)) { fail("can't compare a cardinal with that%s", ""); return 1; }
        *out = card_cmp(a, b);
        return 1;
    }
    if (a.t != T_ORD && b.t != T_ORD) return 0;
    ord *x = to_ord(a), *y = to_ord(b);
    if (!x || !y) { *out = !x ? -1 : 1; return 1; }
    *out = cmp(x, y);
    return 1;
}

/* words */

static void b_omega(void) { struct oterm t = { fin(1), 1 }; push(OV(build(&t, 1))); }
static void b_e0(void) { push(OV(atom_e0)); }
static void b_w1(void) { push(OV(atom_w1)); }
static void b_aleph0(void) { push(CARD(0)); }
static void b_aleph1(void) { push(CARD(1)); }
static void b_aleph(void) { i64 k; if (pop_int("aleph", &k)) { if (k < 0) fail("aleph needs 0 or more%s", ""); else push(CARD(k)); } }
static void b_continuum(void) { push(CARD(-1)); }

static void b_card(void) {
    if (!need(1, "card")) return;
    V v = pop();
    if (v.t == T_INT) { push(v); return; }
    if (v.t == T_CARD) { push(v); return; }
    if (v.t != T_ORD) { fail("card needs an ordinal%s", ""); return; }
    ord *o = (ord *)v.p;
    push(CARD(cmp(o, atom_w1) >= 0 ? 1 : 0));
}

static void b_isord(void) { if (need(1, "ordinal?")) push(I(pop().t == T_ORD)); }

void ord_init(void) {
    int was = W->pin;
    W->pin = 1;
    atom_e0 = ord_new(0);
    atom_e0->atom = A_E0;
    atom_w1 = ord_new(0);
    atom_w1->atom = A_W1;
    W->pin = was;
}

const struct builtin ord_words[] = {
    { "omega", b_omega }, { "epsilon0", b_e0 }, { "omega1", b_w1 },
    { "aleph0", b_aleph0 }, { "aleph1", b_aleph1 }, { "aleph", b_aleph }, { "continuum", b_continuum },
    { "card", b_card }, { "ordinal?", b_isord },
    { NULL, NULL }
};
