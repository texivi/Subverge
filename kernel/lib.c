#include "lib.h"

void *memset(void *d, int c, size_t n) {
    u8 *p = d;
    while (n--) *p++ = (u8)c;
    return d;
}

void *memcpy(void *d, const void *s, size_t n) {
    u8 *a = d;
    const u8 *b = s;
    while (n--) *a++ = *b++;
    return d;
}

void *memmove(void *d, const void *s, size_t n) {
    u8 *a = d;
    const u8 *b = s;
    if (a < b) while (n--) *a++ = *b++;
    else while (n--) a[n] = b[n];
    return d;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

int memcmp(const void *a, const void *b, size_t n) {
    const u8 *x = a, *y = b;
    for (; n; n--, x++, y++)
        if (*x != *y) return *x - *y;
    return 0;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) a++, b++;
    return (u8)*a - (u8)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    for (; n; n--, a++, b++)
        if (*a != *b || !*a) return (u8)*a - (u8)*b;
    return 0;
}

const char *strstr(const char *h, const char *n) {
    size_t l = strlen(n);
    if (!l) return h;
    for (; *h; h++)
        if (!strncmp(h, n, l)) return h;
    return NULL;
}

void scopy(char *d, const char *s, size_t cap) {
    size_t i = 0;
    if (!cap) return;
    for (; s[i] && i + 1 < cap; i++) d[i] = s[i];
    d[i] = 0;
}

int vsnfmt(char *b, size_t n, const char *f, va_list ap) {
    size_t o = 0;
#define PUT(c) do { if (o + 1 < n) b[o] = (c); o++; } while (0)
    for (; *f; f++) {
        if (*f != '%') { PUT(*f); continue; }
        f++;
        int left = 0, zero = 0, w = 0, len = 0;
        char tmp[32];
        const char *s = tmp;
        if (*f == '-') { left = 1; f++; }
        if (*f == '0') { zero = 1; f++; }
        while (*f >= '0' && *f <= '9') w = w * 10 + (*f++ - '0');
        int lng = 0;
        while (*f == 'l') { lng = 1; f++; }
        if (!*f) break;
        switch (*f) {
        case 's':
            s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            len = (int)strlen(s);
            break;
        case 'c':
            tmp[0] = (char)va_arg(ap, int);
            len = 1;
            break;
        case 'd': case 'u': case 'x': {
            u64 v, base = *f == 'x' ? 16 : 10;
            char r[24];
            int k = 0;
            if (*f == 'd') {
                i64 x = lng ? va_arg(ap, i64) : va_arg(ap, i32);
                if (x < 0) { tmp[len++] = '-'; v = (u64)(-(x + 1)) + 1; }
                else v = (u64)x;
            } else v = lng ? va_arg(ap, u64) : va_arg(ap, u32);
            do { r[k++] = "0123456789abcdef"[v % base]; v /= base; } while (v);
            while (k) tmp[len++] = r[--k];
            break;
        }
        default:
            tmp[0] = *f;
            len = 1;
        }
        int pad = w > len ? w - len : 0;
        if (!left) while (pad-- > 0) PUT(zero ? '0' : ' ');
        for (int i = 0; i < len; i++) PUT(s[i]);
        if (left) while (pad-- > 0) PUT(' ');
    }
    if (n) b[o < n ? o : n - 1] = 0;
#undef PUT
    return (int)o;
}

int snfmt(char *b, size_t n, const char *f, ...) {
    va_list ap;
    va_start(ap, f);
    int r = vsnfmt(b, n, f, ap);
    va_end(ap);
    return r;
}
