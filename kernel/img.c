#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

/* JPEG decoder */

extern const char _binary_build_pics_bin_start[], _binary_build_pics_bin_end[];

#define NBMP 16
#define MAXDIM 2048

static struct bmp { u32 w, h; u32 *px; } bmps[NBMP];

static const u8 zz[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

struct huff { u8 val[256]; i32 maxcode[18], mincode[17], valptr[17]; };
struct comp { int id, h, v, tq, td, ta, stride, pred; u8 *plane; };
struct jpeg {
    const u8 *p, *end;
    u16 q[4][64];
    struct huff dc[4], ac[4];
    struct comp c[3];
    int nc, w, h, hmax, vmax, restart, nb, bad, marker;
    u32 acc;
};

static int getbit(struct jpeg *j) {
    if (!j->nb) {
        u8 b = 0;
        if (j->marker || j->p >= j->end) j->marker = 1;
        else {
            b = *j->p++;
            if (b == 0xFF) {
                u8 b2 = j->p < j->end ? *j->p : 0xD9;
                if (b2 == 0) j->p++;
                else { j->p--; j->marker = 1; b = 0; }
            }
        }
        j->acc = b;
        j->nb = 8;
    }
    j->nb--;
    return (int)((j->acc >> j->nb) & 1);
}

static int receive(struct jpeg *j, int n) {
    int v = 0;
    while (n--) v = v << 1 | getbit(j);
    return v;
}

static int extend(int v, int n) { return v < (1 << (n - 1)) ? v - (1 << n) + 1 : v; }

static int hdecode(struct jpeg *j, const struct huff *h) {
    int code = 0;
    for (int l = 1; l <= 16; l++) {
        code = code << 1 | getbit(j);
        if (code <= h->maxcode[l]) return h->val[h->valptr[l] + code - h->mincode[l]];
    }
    j->bad = 1;
    return 0;
}

#define F(x) (x)
#define IDCT_1D(s0, s1, s2, s3, s4, s5, s6, s7) \
    int t0, t1, t2, t3, p1, p2, p3, p4, p5, x0, x1, x2, x3; \
    p2 = s2; p3 = s6; \
    p1 = (p2 + p3) * F(2217); \
    t2 = p1 + p3 * F(-7567); \
    t3 = p1 + p2 * F(3135); \
    p2 = s0; p3 = s4; \
    t0 = (p2 + p3) * 4096; \
    t1 = (p2 - p3) * 4096; \
    x0 = t0 + t3; x3 = t0 - t3; x1 = t1 + t2; x2 = t1 - t2; \
    t0 = s7; t1 = s5; t2 = s3; t3 = s1; \
    p3 = t0 + t2; p4 = t1 + t3; p1 = t0 + t3; p2 = t1 + t2; \
    p5 = (p3 + p4) * F(4816); \
    t0 = t0 * F(1223); t1 = t1 * F(8410); t2 = t2 * F(12586); t3 = t3 * F(6149); \
    p1 = p5 + p1 * F(-3685); p2 = p5 + p2 * F(-10497); \
    p3 = p3 * F(-8034); p4 = p4 * F(-1597); \
    t3 += p1 + p4; t2 += p2 + p3; t1 += p2 + p4; t0 += p1 + p3;

static u8 clamp8(int v) { return v < 0 ? 0 : v > 255 ? 255 : (u8)v; }

static void idct(int *d, u8 *out, int stride) {
    for (int i = 0; i < 8; i++) {
        int *v = d + i;
        if (!v[8] && !v[16] && !v[24] && !v[32] && !v[40] && !v[48] && !v[56]) {
            int dc = v[0] * 4;
            for (int k = 0; k < 8; k++) v[k * 8] = dc;
        } else {
            IDCT_1D(v[0], v[8], v[16], v[24], v[32], v[40], v[48], v[56])
            x0 += 512; x1 += 512; x2 += 512; x3 += 512;
            v[0] = (x0 + t3) >> 10; v[56] = (x0 - t3) >> 10;
            v[8] = (x1 + t2) >> 10; v[48] = (x1 - t2) >> 10;
            v[16] = (x2 + t1) >> 10; v[40] = (x2 - t1) >> 10;
            v[24] = (x3 + t0) >> 10; v[32] = (x3 - t0) >> 10;
        }
    }
    for (int i = 0; i < 8; i++) {
        int *v = d + i * 8;
        IDCT_1D(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7])
        x0 += 65536 + (128 << 17); x1 += 65536 + (128 << 17); x2 += 65536 + (128 << 17); x3 += 65536 + (128 << 17);
        u8 *o = out + i * stride;
        o[0] = clamp8((x0 + t3) >> 17); o[7] = clamp8((x0 - t3) >> 17);
        o[1] = clamp8((x1 + t2) >> 17); o[6] = clamp8((x1 - t2) >> 17);
        o[2] = clamp8((x2 + t1) >> 17); o[5] = clamp8((x2 - t1) >> 17);
        o[3] = clamp8((x3 + t0) >> 17); o[4] = clamp8((x3 - t0) >> 17);
    }
}

static void block(struct jpeg *j, struct comp *c, int bx, int by) {
    int d[64];
    for (int i = 0; i < 64; i++) d[i] = 0;
    const u16 *q = j->q[c->tq];
    int t = hdecode(j, &j->dc[c->td]);
    c->pred += t ? extend(receive(j, t), t) : 0;
    d[0] = c->pred * q[0];
    for (int k = 1; k < 64;) {
        int rs = hdecode(j, &j->ac[c->ta]), r = rs >> 4, s = rs & 15;
        if (!s) { if (r != 15) break; k += 16; continue; }
        k += r;
        if (k > 63) { j->bad = 1; break; }
        d[zz[k]] = extend(receive(j, s), s) * q[k];
        k++;
    }
    idct(d, c->plane + by * 8 * c->stride + bx * 8, c->stride);
}

static void build(struct huff *h, const u8 *bits, int n) {
    int code = 0, k = 0;
    for (int l = 1; l <= 16; l++) {
        h->valptr[l] = k;
        h->mincode[l] = code;
        code += bits[l - 1];
        k += bits[l - 1];
        h->maxcode[l] = bits[l - 1] ? code - 1 : -1;
        code <<= 1;
    }
    (void)n;
}

static int u16be(const u8 *p) { return p[0] << 8 | p[1]; }

static int sample(const struct jpeg *j, const struct comp *c, int x, int y) {
    if (c->h == j->hmax && c->v == j->vmax) return c->plane[y * c->stride + x];
    int cw = (j->w * c->h + j->hmax - 1) / j->hmax, ch = (j->h * c->v + j->vmax - 1) / j->vmax;
    int u = ((2 * x + 1) * c->h * 128) / j->hmax - 128, v = ((2 * y + 1) * c->v * 128) / j->vmax - 128;
    int ix = u >> 8, fx = u & 255, iy = v >> 8, fy = v & 255;
    int x0 = ix < 0 ? 0 : ix, x1 = ix + 1 < cw ? ix + 1 : cw - 1, y0 = iy < 0 ? 0 : iy, y1 = iy + 1 < ch ? iy + 1 : ch - 1;
    if (x0 >= cw) x0 = cw - 1;
    if (y0 >= ch) y0 = ch - 1;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    const u8 *p = c->plane;
    int a = p[y0 * c->stride + x0] * (256 - fx) + p[y0 * c->stride + x1] * fx;
    int b = p[y1 * c->stride + x0] * (256 - fx) + p[y1 * c->stride + x1] * fx;
    return (a * (256 - fy) + b * fy + 32768) >> 16;
}

static void restart(struct jpeg *j) {
    j->nb = 0;
    j->marker = 0;
    while (j->p + 1 < j->end && !(j->p[0] == 0xFF && j->p[1] >= 0xD0 && j->p[1] <= 0xD7)) j->p++;
    j->p += 2;
    for (int i = 0; i < j->nc; i++) j->c[i].pred = 0;
}

/* decode bitmap */
static int bmp_new(u32 w, u32 h);

static int decode(const u8 *d, u32 len, const char **why) {
    struct jpeg *j = kmalloc_try(sizeof *j);
    int id = -1, i;
    *why = "out of memory";
    if (!j) return -1;
    *why = "damaged";
    memset(j, 0, sizeof *j);
    j->p = d; j->end = d + len;
    if (len < 4 || d[0] != 0xFF || d[1] != 0xD8) return -1;
    j->p += 2;
    int have_sof = 0;
    for (;;) {
        while (j->p < j->end && *j->p != 0xFF) j->p++;
        while (j->p < j->end && *j->p == 0xFF) j->p++;
        if (j->p >= j->end) goto done;
        u8 m = *j->p++;
        if (m == 0xD9) goto done;
        if (m == 0x01 || (m >= 0xD0 && m <= 0xD8)) continue;
        if (j->p + 2 > j->end) goto done;
        int n = u16be(j->p);
        const u8 *s = j->p + 2, *e = j->p + n;
        if (n < 2 || e > j->end) goto done;
        if (m == 0xDB) {
            while (s < e) {
                int pq = *s >> 4, tq = *s & 15;
                s++;
                if (tq > 3) goto done;
                for (i = 0; i < 64; i++) { j->q[tq][i] = pq ? (u16)u16be(s) : *s; s += pq ? 2 : 1; }
            }
        } else if (m == 0xC4) {
            while (s + 17 <= e) {
                int tc = *s >> 4, th = *s & 15, cnt = 0;
                if (th > 3) goto done;
                for (i = 0; i < 16; i++) cnt += s[1 + i];
                if (cnt > 256 || s + 17 + cnt > e) goto done;
                struct huff *h = tc ? &j->ac[th] : &j->dc[th];
                build(h, s + 1, cnt);
                for (i = 0; i < cnt; i++) h->val[i] = s[17 + i];
                s += 17 + cnt;
            }
        } else if (m == 0xC0 || m == 0xC1) {
            if (s[0] != 8 || (s[5] != 1 && s[5] != 3)) { *why = "unsupported"; goto done; }
            j->h = u16be(s + 1); j->w = u16be(s + 3); j->nc = s[5];
            for (i = 0; i < j->nc; i++) {
                j->c[i].id = s[6 + i * 3]; j->c[i].h = s[7 + i * 3] >> 4; j->c[i].v = s[7 + i * 3] & 15; j->c[i].tq = s[8 + i * 3] & 3;
                if (j->c[i].h < 1 || j->c[i].h > 2 || j->c[i].v < 1 || j->c[i].v > 2) { *why = "unsupported"; goto done; }
                if (j->c[i].h > j->hmax) j->hmax = j->c[i].h;
                if (j->c[i].v > j->vmax) j->vmax = j->c[i].v;
            }
            have_sof = 1;
        } else if (m >= 0xC2 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) {
            *why = "not baseline";
            goto done;
        } else if (m == 0xDD) {
            j->restart = u16be(s);
        } else if (m == 0xDA) {
            if (!have_sof || j->w < 1 || j->h < 1 || j->w > MAXDIM || j->h > MAXDIM) goto done;
            int ns = s[0];
            if (ns != j->nc) { *why = "unsupported"; goto done; }
            for (i = 0; i < ns; i++) { j->c[i].td = s[2 + i * 2] >> 4 & 3; j->c[i].ta = s[2 + i * 2] & 3; }
            j->p = e;
            int mw = (j->w + 8 * j->hmax - 1) / (8 * j->hmax), mh = (j->h + 8 * j->vmax - 1) / (8 * j->vmax);
            for (i = 0; i < j->nc; i++) {
                j->c[i].stride = mw * j->c[i].h * 8;
                j->c[i].plane = kmalloc_try((u64)j->c[i].stride * (u64)(mh * j->c[i].v * 8));
                if (!j->c[i].plane) { *why = "out of memory"; goto done; }
            }
            int cnt = 0;
            for (int my = 0; my < mh && !j->bad; my++)
                for (int mx = 0; mx < mw && !j->bad; mx++) {
                    for (i = 0; i < j->nc; i++)
                        for (int v = 0; v < j->c[i].v; v++)
                            for (int h = 0; h < j->c[i].h; h++) block(j, &j->c[i], mx * j->c[i].h + h, my * j->c[i].v + v);
                    if (j->restart && ++cnt % j->restart == 0 && (my < mh - 1 || mx < mw - 1)) restart(j);
                }
            if (j->bad) goto done;
            id = bmp_new((u32)j->w, (u32)j->h);
            if (id < 0) { *why = "out of memory"; goto done; }
            u32 *o = bmps[id].px;
            for (int y = 0; y < j->h; y++)
                for (int x = 0; x < j->w; x++) {
                    int Y = sample(j, &j->c[0], x, y), r, g, b;
                    if (j->nc == 3) {
                        int cb = sample(j, &j->c[1], x, y) - 128, cr = sample(j, &j->c[2], x, y) - 128;
                        r = Y + ((91881 * cr + 32768) >> 16);
                        g = Y - ((22554 * cb + 46802 * cr + 32768) >> 16);
                        b = Y + ((116130 * cb + 32768) >> 16);
                    } else r = g = b = Y;
                    *o++ = (u32)clamp8(r) << 16 | (u32)clamp8(g) << 8 | clamp8(b);
                }
            *why = "";
            goto done;
        }
        j->p = e;
    }
done:
    for (i = 0; i < 3; i++) if (j->c[i].plane) kfree(j->c[i].plane);
    kfree(j);
    return id;
}

static int bmp_new(u32 w, u32 h) {
    for (int i = 0; i < NBMP; i++)
        if (!bmps[i].px) {
            u32 *p = kmalloc_try((u64)w * h * 4);
            if (!p) return -1;
            for (u32 k = 0; k < w * h; k++) p[k] = 0;
            bmps[i].w = w; bmps[i].h = h; bmps[i].px = p;
            return i;
        }
    return -1;
}

static struct bmp *get(const char *w, i64 id) {
    if (id < 0 || id >= NBMP || !bmps[id].px) { fail("%s needs the number of a bitmap", w); return NULL; }
    return &bmps[id];
}

static void b_bitmap(void) {
    i64 w, h;
    if (!pop_int("bitmap", &h) || !pop_int("bitmap", &w)) return;
    if (w < 1 || h < 1 || w > MAXDIM || h > MAXDIM) { fail("a bitmap is 1 to 2048 wide and high%s", ""); return; }
    int id = bmp_new((u32)w, (u32)h);
    if (id < 0) { fail("no room for another bitmap, free one with bmp-free%s", ""); return; }
    push(I(id));
}

static void b_bset(void) {
    i64 id, x, y, c;
    if (!pop_int("bmp-set", &c) || !pop_int("bmp-set", &y) || !pop_int("bmp-set", &x) || !pop_int("bmp-set", &id)) return;
    struct bmp *b = get("bmp-set", id);
    if (b && x >= 0 && y >= 0 && x < b->w && y < b->h) b->px[y * b->w + x] = (u32)c & 0xFFFFFF;
}

static void b_bget(void) {
    i64 id, x, y;
    if (!pop_int("bmp-get", &y) || !pop_int("bmp-get", &x) || !pop_int("bmp-get", &id)) return;
    struct bmp *b = get("bmp-get", id);
    if (!b) return;
    push(I(x >= 0 && y >= 0 && x < b->w && y < b->h ? (i64)b->px[y * b->w + x] : 0));
}

static void b_bfill(void) {
    i64 id, c;
    if (!pop_int("bmp-fill", &c) || !pop_int("bmp-fill", &id)) return;
    struct bmp *b = get("bmp-fill", id);
    if (b) for (u32 k = 0; k < b->w * b->h; k++) b->px[k] = (u32)c & 0xFFFFFF;
}

static void b_bbox(void) {
    i64 id, x, y, w, h, c;
    if (!pop_int("bmp-box", &c) || !pop_int("bmp-box", &h) || !pop_int("bmp-box", &w) || !pop_int("bmp-box", &y) || !pop_int("bmp-box", &x) || !pop_int("bmp-box", &id)) return;
    struct bmp *b = get("bmp-box", id);
    if (!b) return;
    for (i64 j = y; j < y + h; j++)
        for (i64 i = x; i < x + w; i++)
            if (i >= 0 && j >= 0 && i < b->w && j < b->h) b->px[j * b->w + i] = (u32)c & 0xFFFFFF;
}

static void b_bsize(void) {
    i64 id;
    if (!pop_int("bmp-size", &id)) return;
    struct bmp *b = get("bmp-size", id);
    if (b) { push(I(b->w)); push(I(b->h)); }
}

static void b_bfree(void) {
    i64 id;
    if (!pop_int("bmp-free", &id)) return;
    struct bmp *b = get("bmp-free", id);
    if (b) { kfree(b->px); b->px = NULL; }
}

static void b_bshow(void) {
    i64 id, x, y;
    if (!pop_int("bmp-show", &y) || !pop_int("bmp-show", &x) || !pop_int("bmp-show", &id)) return;
    struct bmp *b = get("bmp-show", id);
    if (!b) return;
    if (!gfx_on) { fail("graphics mode is off, run gfx first%s", ""); return; }
    for (i64 j = 0; j < b->h; j++) {
        i64 yy = y + j;
        if (yy < 0 || yy >= GH) continue;
        for (i64 i = 0; i < b->w; i++) {
            i64 xx = x + i;
            if (xx >= 0 && xx < GW) gfx_lfb[yy * GW + xx] = b->px[j * b->w + i];
        }
    }
}

static void b_pack(void) {
    i64 r, g, b;
    if (!pop_int("pack-rgb", &b) || !pop_int("pack-rgb", &g) || !pop_int("pack-rgb", &r)) return;
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) { fail("pack-rgb takes red, green and blue from 0 to 255%s", ""); return; }
    push(I(r << 16 | g << 8 | b));
}

/* blob format */
static const char *next_pic(const char *p, const char *end, const char **name, u32 *nlen, const u8 **data, u32 *dlen) {
    if (p + 4 > end || p[0] != '#') return NULL;
    const char *q = p + 4;
    *name = q;
    while (q < end && *q != ' ') q++;
    *nlen = (u32)(q - *name);
    u32 n = 0;
    for (q++; q < end && *q != '\n'; q++) n = n * 10 + (u32)(*q - '0');
    *data = (const u8 *)q + 1;
    *dlen = n;
    return q + 1 + n;
}

static void b_picture(void) {
    str *s;
    if (!pop_str("picture", &s)) return;
    const char *p = _binary_build_pics_bin_start, *end = _binary_build_pics_bin_end, *name;
    const u8 *data;
    u32 nl, dl;
    while ((p = next_pic(p, end, &name, &nl, &data, &dl))) {
        if (nl != s->len || memcmp(name, s->s, nl)) continue;
        const char *why;
        int id = decode(data, dl, &why);
        if (id < 0) fail("that picture could not be read: %s", why);
        else push(I(id));
        return;
    }
    fail("no picture named %s, pictures lists them", s->s);
}

static void b_pictures(void) {
    V names[32];
    u32 n = 0;
    const char *p = _binary_build_pics_bin_start, *end = _binary_build_pics_bin_end, *name;
    const u8 *data;
    u32 nl, dl;
    while (n < 32 && (p = next_pic(p, end, &name, &nl, &data, &dl))) names[n++] = S(str_new(name, nl));
    push(Q(quote_new(names, n)));
}

const struct builtin img_words[] = {
    { "bitmap", b_bitmap }, { "bmp-set", b_bset }, { "bmp-get", b_bget }, { "bmp-fill", b_bfill }, { "bmp-box", b_bbox },
    { "bmp-size", b_bsize }, { "bmp-free", b_bfree }, { "bmp-show", b_bshow }, { "pack-rgb", b_pack },
    { "picture", b_picture }, { "pictures", b_pictures }, { NULL, NULL }
};
