#include "kbd.h"
#include "line.h"
#include "sv.h"
#include "mem.h"
#include "task.h"
#include "vga.h"

#define SC 1000000

struct calc_state { const char *p; const char *cerr; i64 ans; struct history calc_lines; };
#define C (current->calc)

static u64 udiv(u64 n, u64 d, u64 *rem) {
    if (rem) *rem = n % d;
    return n / d;
}

static u64 mag(i64 v) { return v < 0 ? (u64)-v : (u64)v; }

static i64 fmul(i64 a, i64 b) {
    u64 x = mag(a), y = mag(b), xf, yf;
    u64 xi = udiv(x, SC, &xf), yi = udiv(y, SC, &yf);
    u64 r = xi * yi * SC + xi * yf + xf * yi + udiv(xf * yf, SC, NULL);
    return (a < 0) != (b < 0) ? -(i64)r : (i64)r;
}

static i64 fdiv(i64 a, i64 b) {
    if (!b) { C->cerr = "division by zero"; return 0; }
    u64 x = mag(a), y = mag(b), rem, frac = 0;
    u64 q = udiv(x, y, &rem);
    for (int i = 0; i < 6; i++) {
        u64 d;
        rem *= 10;
        d = udiv(rem, y, &rem);
        frac = frac * 10 + d;
    }
    u64 r = q * SC + frac;
    return (a < 0) != (b < 0) ? -(i64)r : (i64)r;
}

static i64 fmod_(i64 a, i64 b) {
    if (!b) { C->cerr = "division by zero"; return 0; }
    u64 x = mag(a), y = mag(b), r;
    udiv(x, y, &r);
    return a < 0 ? -(i64)r : (i64)r;
}

static i64 whole(i64 v) { return v < 0 ? -(i64)udiv(mag(v), SC, NULL) : (i64)udiv(mag(v), SC, NULL); }

static void ws(void) { while (*C->p == ' ') C->p++; }

static i64 expr(void);

static i64 number(void) {
    u64 v = 0;
    if (C->p[0] == '0' && (C->p[1] == 'x' || C->p[1] == 'b')) {
        u64 base = C->p[1] == 'x' ? 16 : 2;
        int any = 0;
        for (C->p += 2;; C->p++, any = 1) {
            char c = (char)(*C->p | 32);
            u64 d = c >= '0' && c <= '9' ? (u64)(c - '0') : c >= 'a' && c <= 'f' ? (u64)(c - 'a' + 10) : 99;
            if (d >= base) break;
            v = v * base + d;
        }
        if (!any) C->cerr = "bad number";
        return (i64)(v * SC);
    }
    while (*C->p >= '0' && *C->p <= '9') v = v * 10 + (u64)(*C->p++ - '0');
    v *= SC;
    if (*C->p == '.') {
        u64 scale = SC / 10;
        for (C->p++; *C->p >= '0' && *C->p <= '9'; C->p++) {
            v += (u64)(*C->p - '0') * scale;
            scale = udiv(scale, 10, NULL);
        }
    }
    return (i64)v;
}

static i64 atom(void) {
    ws();
    if (*C->p == '(') {
        C->p++;
        i64 v = expr();
        ws();
        if (*C->p != ')') C->cerr = "missing )";
        else C->p++;
        return v;
    }
    if (C->p[0] == 'a' && C->p[1] == 'n' && C->p[2] == 's') { C->p += 3; return C->ans; }
    if ((*C->p >= '0' && *C->p <= '9') || *C->p == '.') return number();
    C->cerr = *C->p ? "unexpected character" : "unexpected end";
    return 0;
}

static i64 unary(void);

static i64 power(void) {
    i64 v = atom();
    ws();
    if (C->p[0] == '*' && C->p[1] == '*') {
        C->p += 2;
        i64 e = whole(unary()), r = SC;
        if (e < 0 || e > 64) { C->cerr = "exponent must be 0..64"; return 0; }
        while (e--) r = fmul(r, v);
        return r;
    }
    return v;
}

static i64 unary(void) {
    ws();
    if (*C->p == '-') { C->p++; return -unary(); }
    if (*C->p == '~') { C->p++; return (~whole(unary())) * SC; }
    return power();
}

static i64 term(void) {
    i64 v = unary();
    for (;;) {
        ws();
        if (*C->p == '*' && C->p[1] != '*') { C->p++; v = fmul(v, unary()); }
        else if (*C->p == '/') { C->p++; v = fdiv(v, unary()); }
        else if (*C->p == '%') { C->p++; v = fmod_(v, unary()); }
        else return v;
    }
}

static i64 sum(void) {
    i64 v = term();
    for (;;) {
        ws();
        if (*C->p == '+') { C->p++; v += term(); }
        else if (*C->p == '-') { C->p++; v -= term(); }
        else return v;
    }
}

static i64 shift(void) {
    i64 v = sum();
    for (;;) {
        ws();
        if (C->p[0] == '<' && C->p[1] == '<') { C->p += 2; v = (i64)((u64)whole(v) << (whole(sum()) & 63)) * SC; }
        else if (C->p[0] == '>' && C->p[1] == '>') { C->p += 2; v = (i64)((u64)whole(v) >> (whole(sum()) & 63)) * SC; }
        else return v;
    }
}

static i64 band(void) { i64 v = shift(); for (ws(); *C->p == '&'; ws()) { C->p++; v = (whole(v) & whole(shift())) * SC; } return v; }
static i64 bxor(void) { i64 v = band(); for (ws(); *C->p == '^'; ws()) { C->p++; v = (whole(v) ^ whole(band())) * SC; } return v; }
static i64 expr(void) { i64 v = bxor(); for (ws(); *C->p == '|'; ws()) { C->p++; v = (whole(v) | whole(bxor())) * SC; } return v; }

static void show(i64 v) {
    char d[24], o[64];
    u64 x = mag(v), fp, ip = udiv(x, SC, &fp), whole_part = ip;
    int n = 0, k = 0, has_frac = fp != 0;
    do { u64 r; ip = udiv(ip, 10, &r); d[n++] = (char)('0' + r); } while (ip);
    if (v < 0) o[k++] = '-';
    while (n) o[k++] = d[--n];
    if (has_frac) {
        char f[6];
        for (int i = 5; i >= 0; i--) { u64 r; fp = udiv(fp, 10, &r); f[i] = (char)('0' + r); }
        int last = 5;
        while (f[last] == '0') last--;
        o[k++] = '.';
        for (int i = 0; i <= last; i++) o[k++] = f[i];
    }
    o[k] = 0;
    vga_color(C_CAT);
    kprintf("  = %s", o);
    if (!has_frac) {
        u64 w = (u64)(v < 0 ? -(i64)whole_part : (i64)whole_part);
        char b[72];
        int bn = 0, top = 63;
        while (top > 0 && !(w >> top & 1)) top--;
        for (int i = top; i >= 0; i--) b[bn++] = (char)('0' + (w >> i & 1));
        b[bn] = 0;
        vga_color(C_DIM);
        if (bn <= 40) kprintf("   0x%lx   0b%s", w, b);
        else kprintf("   0x%lx", w);
    }
    vga_color(C_TEXT);
    kputc('\n');
}

void calc(void) {
    char line[LINE_MAX + 1];
    if (!C) C = kzalloc(sizeof *C);
    set_title("calc");
    vga_color(C_DIM);
    kputs("  Type an expression, or an empty line to leave.\n  Operators: + - * / % ** ( ) & | ^ ~ << >>\n  0x and 0b numbers work, and ans is the last result.\n");
    for (;;) {
        vga_color(C_MOON);
        kputs("  calc \xaf ");
        vga_color(C_TEXT);
        int n = readline(line, &C->calc_lines);
        if (n <= 0) break;
        C->p = line;
        C->cerr = NULL;
        i64 v = expr();
        ws();
        if (!C->cerr && *C->p) C->cerr = "unexpected character";
        if (C->cerr) {
            vga_color(C_LRED);
            kprintf("  ! %s\n", C->cerr);
            vga_color(C_TEXT);
            continue;
        }
        C->ans = v;
        show(v);
    }
    kbreak = 0;
    set_title(SHELL_NAME);
}
