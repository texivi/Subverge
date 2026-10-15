/* big integers */
#include "mem.h"
#include "omega.h"
#include "task.h"

#define MAXL 2048
#define BASE 4294967296ull

typedef struct { obj h; u32 n; u8 neg; u32 d[]; } big;

static big *bnew(u32 n) {
    if (n > MAXL) { fail("that number is too big, the limit is 2^65536%s", ""); return NULL; }
    big *b = (big *)gnew(sizeof(big) + (u64)(n ? n : 1) * 4, O_BIG);
    if (!b) return NULL;
    b->n = n;
    b->neg = 0;
    memset(b->d, 0, (u64)(n ? n : 1) * 4);
    return b;
}

/* small or big */
static V norm(big *b) {
    if (!b) return I(0);
    while (b->n && !b->d[b->n - 1]) b->n--;
    if (b->n <= 2) {
        u64 m = b->n ? b->d[0] | (b->n > 1 ? (u64)b->d[1] << 32 : 0) : 0;
        if (m < (1ull << 63)) return I(b->neg ? -(i64)m : (i64)m);
        if (b->neg && m == (1ull << 63)) return I((i64)m);
    }
    return (V){ T_BIG, 0, &b->h };
}

static big *of(V v, big *tmp) {
    if (v.t == T_BIG) return (big *)v.p;
    u64 m = v.n < 0 ? 0 - (u64)v.n : (u64)v.n;
    tmp->neg = v.n < 0;
    tmp->d[0] = (u32)m;
    tmp->d[1] = (u32)(m >> 32);
    tmp->n = tmp->d[1] ? 2 : tmp->d[0] ? 1 : 0;
    return tmp;
}
#define SMALL(name) struct { big b; u32 d[2]; } name

static int mcmp(const big *a, const big *b) {
    if (a->n != b->n) return a->n < b->n ? -1 : 1;
    for (u32 i = a->n; i--;) if (a->d[i] != b->d[i]) return a->d[i] < b->d[i] ? -1 : 1;
    return 0;
}

static big *madd(const big *a, const big *b) {
    if (a->n < b->n) { const big *t = a; a = b; b = t; }
    big *r = bnew(a->n + 1);
    if (!r) return NULL;
    u64 c = 0;
    for (u32 i = 0; i < a->n; i++) {
        c += (u64)a->d[i] + (i < b->n ? b->d[i] : 0);
        r->d[i] = (u32)c;
        c >>= 32;
    }
    r->d[a->n] = (u32)c;
    return r;
}

/* a >= b */
static big *msub(const big *a, const big *b) {
    big *r = bnew(a->n);
    if (!r) return NULL;
    i64 c = 0;
    for (u32 i = 0; i < a->n; i++) {
        c += (i64)a->d[i] - (i < b->n ? b->d[i] : 0);
        r->d[i] = (u32)c;
        c = c < 0 ? -1 : 0;
    }
    return r;
}

static big *mmul(const big *a, const big *b) {
    big *r = bnew(a->n + b->n);
    if (!r) return NULL;
    for (u32 i = 0; i < a->n; i++) {
        u64 c = 0;
        for (u32 j = 0; j < b->n; j++) {
            c += (u64)a->d[i] * b->d[j] + r->d[i + j];
            r->d[i + j] = (u32)c;
            c >>= 32;
        }
        r->d[i + b->n] = (u32)c;
    }
    return r;
}

/* knuth d */
static int mdivmod(const big *a, const big *b, big **q, big **r) {
    u32 n = b->n, m = a->n;
    *q = bnew(m >= n ? m - n + 1 : 1);
    *r = bnew(n);
    if (!*q || !*r) return 0;
    if (m < n) { memcpy((*r)->d, a->d, (u64)m * 4); (*r)->n = m; (*q)->n = 0; return 1; }
    if (n == 1) {
        u64 rem = 0;
        for (u32 i = m; i--;) {
            u64 cur = rem << 32 | a->d[i];
            (*q)->d[i] = (u32)(cur / b->d[0]);
            rem = cur % b->d[0];
        }
        (*r)->d[0] = (u32)rem;
        return 1;
    }
    int s = 0;
    while (!(b->d[n - 1] << s & 0x80000000u)) s++;
    big *u = bnew(m + 1), *v = bnew(n);
    if (!u || !v) return 0;
    for (u32 i = n; i--;) v->d[i] = b->d[i] << s | (s && i ? b->d[i - 1] >> (32 - s) : 0);
    u->d[m] = s ? a->d[m - 1] >> (32 - s) : 0;
    for (u32 i = m; i--;) u->d[i] = a->d[i] << s | (s && i ? a->d[i - 1] >> (32 - s) : 0);
    for (u32 j = m - n + 1; j--;) {
        u64 num = (u64)u->d[j + n] << 32 | u->d[j + n - 1];
        u64 qh = num / v->d[n - 1], rh = num % v->d[n - 1];
        while (qh >= BASE || qh * v->d[n - 2] > (rh << 32 | u->d[j + n - 2])) {
            qh--;
            rh += v->d[n - 1];
            if (rh >= BASE) break;
        }
        i64 borrow = 0;
        u64 carry = 0;
        for (u32 i = 0; i < n; i++) {
            u64 p = qh * v->d[i] + carry;
            carry = p >> 32;
            i64 t = (i64)u->d[i + j] - (i64)(u32)p + borrow;
            u->d[i + j] = (u32)t;
            borrow = t >> 32;
        }
        i64 t = (i64)u->d[j + n] - (i64)carry + borrow;
        u->d[j + n] = (u32)t;
        if (t < 0) {
            qh--;
            u64 c = 0;
            for (u32 i = 0; i < n; i++) {
                c += (u64)u->d[i + j] + v->d[i];
                u->d[i + j] = (u32)c;
                c >>= 32;
            }
            u->d[j + n] += (u32)c;
        }
        (*q)->d[j] = (u32)qh;
    }
    for (u32 i = 0; i < n; i++) (*r)->d[i] = u->d[i] >> s | (s && i + 1 < n + 1 ? (u32)((u64)u->d[i + 1] << (32 - s)) : 0);
    return 1;
}

static V signed_add(const big *a, int an, const big *b, int bn) {
    big *r;
    if (an == bn) { r = madd(a, b); if (r) r->neg = (u8)an; return norm(r); }
    int c = mcmp(a, b);
    if (!c) return I(0);
    r = c > 0 ? msub(a, b) : msub(b, a);
    if (r) r->neg = (u8)(c > 0 ? an : bn);
    return norm(r);
}

static u32 bits(const big *a) {
    if (!a->n) return 0;
    u32 t = a->d[a->n - 1], k = 0;
    while (t) { k++; t >>= 1; }
    return (a->n - 1) * 32 + k;
}

static V bpow(V base, V e) {
    if (e.t == T_BIG) {
        if (base.t == T_INT && (base.n == 0 || base.n == 1)) return base;
        if (base.t == T_INT && base.n == -1) return I(((big *)e.p)->d[0] & 1 ? -1 : 1);
        if (((big *)e.p)->neg) fail("negative powers aren't supported%s", "");
        else fail("that number is too big, the limit is 2^65536%s", "");
        return I(0);
    }
    if (e.n < 0) { fail("negative powers aren't supported%s", ""); return I(0); }
    SMALL(t);
    big *b = of(base, &t.b);
    if (b->n == 0) return I(e.n == 0);
    if ((u64)(bits(b) - 1) * (u64)e.n > MAXL * 32ull) { fail("that number is too big, the limit is 2^65536%s", ""); return I(0); }
    V r = I(1), p = base;
    for (i64 k = e.n; k && !W->err; k >>= 1) {
        if (k & 1) r = big_arith('*', r, p);
        if (k > 1 && !W->err) p = big_arith('*', p, p);
    }
    return r;
}

V big_arith(char op, V a, V b) {
    SMALL(ta); SMALL(tb);
    if (op == '^') return bpow(a, b);
    big *x = of(a, &ta.b), *y = of(b, &tb.b);
    switch (op) {
    case '+': return signed_add(x, x->neg, y, y->neg);
    case '-': return signed_add(x, x->neg, y, !y->neg);
    case '*': {
        big *r = mmul(x, y);
        if (r) r->neg = (u8)(x->neg != y->neg);
        return norm(r);
    }
    case '/': case '%': {
        if (!y->n) { fail("division by zero%s", ""); return I(0); }
        big *q, *r;
        if (!mdivmod(x, y, &q, &r)) return I(0);
        if (op == '/') { q->neg = (u8)(x->neg != y->neg); return norm(q); }
        r->neg = x->neg;
        return norm(r);
    }
    }
    return I(0);
}

int big_cmp(V a, V b) {
    SMALL(ta); SMALL(tb);
    big *x = of(a, &ta.b), *y = of(b, &tb.b);
    if (x->neg != y->neg) return x->neg ? -1 : 1;
    int c = mcmp(x, y);
    return x->neg ? -c : c;
}

V big_neg(V a) {
    if (a.t == T_INT) {
        if (a.n != (i64)(1ull << 63)) return I(-a.n);
        return big_arith('-', I(0), a);
    }
    big *s = (big *)a.p, *r = bnew(s->n);
    if (!r) return I(0);
    memcpy(r->d, s->d, (u64)s->n * 4);
    r->neg = !s->neg;
    return norm(r);
}

/* digits in */
int big_parse(const char *t, u32 l, V *out) {
    u32 i = 0, base = 10;
    int neg = 0;
    if (l && t[0] == '-') { neg = 1; i = 1; }
    if (i >= l) return 0;
    if (l - i > 2 && t[i] == '0' && (t[i + 1] == 'x' || t[i + 1] == 'b')) { base = t[i + 1] == 'x' ? 16 : 2; i += 2; }
    for (u32 k = i; k < l; k++) {
        char c = (char)(t[k] | 32);
        u32 d = c >= '0' && c <= '9' ? (u32)(c - '0') : c >= 'a' && c <= 'f' ? (u32)(c - 'a' + 10) : 99;
        if (d >= base) return 0;
    }
    big *r = bnew((l - i) * 4 / 32 + 2);
    if (!r) return 1;
    for (; i < l; i++) {
        char c = (char)(t[i] | 32);
        u64 cy = c >= 'a' ? (u64)(c - 'a' + 10) : (u64)(c - '0');
        for (u32 k = 0; k < r->n; k++) {
            cy += (u64)r->d[k] * base;
            r->d[k] = (u32)cy;
            cy >>= 32;
        }
    }
    r->neg = (u8)neg;
    *out = norm(r);
    return 1;
}

/* digits out */
void big_repr(sbuf *o, V v) {
    big *b = (big *)v.p;
    u32 *d = kmalloc_try((u64)b->n * 4 + 4);
    char *s = kmalloc_try((u64)b->n * 10 + 4);
    if (!d || !s) { kfree(d); kfree(s); sputs(o, "?"); return; }
    memcpy(d, b->d, (u64)b->n * 4);
    u32 n = b->n, k = 0;
    while (n) {
        u64 rem = 0;
        for (u32 i = n; i--;) {
            u64 cur = rem << 32 | d[i];
            d[i] = (u32)(cur / 1000000000u);
            rem = cur % 1000000000u;
        }
        while (n && !d[n - 1]) n--;
        for (int j = 0; j < 9 && (n || rem); j++) { s[k++] = (char)('0' + rem % 10); rem /= 10; }
    }
    if (b->neg) s[k++] = '-';
    for (u32 i = 0; i < k / 2; i++) { char c = s[i]; s[i] = s[k - 1 - i]; s[k - 1 - i] = c; }
    sput(o, s, k);
    kfree(d);
    kfree(s);
}

u32 big_digits(V v) { return ((big *)v.p)->n * 10 + 2; }
