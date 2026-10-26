/* sound blaster */

#include "audio.h"
#include "cpu.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

#define SB 0x220
#define RATE 44100
#define BLK 4096
#define NV 8
#define TPQ 48


struct nev { u32 t, dur; u8 pitch, timb, vol; };

struct voice {
    u8 on, rel, timb;
    u16 n, pos;
    i32 c, x1, y1, px, env, vel;
    u32 ph, inc, ph2, inc2, off, age;
    i16 dl[1024];
};

static const i16 sine[257] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179, 6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
    0, -804, -1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179, -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804,
    0,
};

static const u32 base[12] = { 16352, 17324, 18354, 19445, 20602, 21827, 23125, 24500, 25957, 27500, 29135, 30868 };

static int have, running, half, song_on, finished;
static i16 dmabuf[2 * BLK] __attribute__((aligned(65536)));
static struct voice vc[NV];
static struct nev *ev;
static int nev_n, nev_i;
static u32 tick, acc, spt, tail, rng = 12345;
static i16 cb[4][1400], ab[2][560];
static int cpos[4], apos[2];
static i32 cst[4];
static const u16 cd[4] = { 1116, 1188, 1277, 1356 }, ad[2] = { 556, 441 };

/* dsp port helpers */
static void pause(void) { for (int i = 0; i < 8; i++) inb(0x80); }
static int dsp_w(u8 v) {
    for (int i = 0; i < 20000; i++) if (!(inb(SB + 0xC) & 0x80)) { outb(SB + 0xC, v); return 1; }
    return 0;
}
static int dsp_r(void) {
    for (int i = 0; i < 20000; i++) if (inb(SB + 0xE) & 0x80) return inb(SB + 0xA);
    return -1;
}
static int dsp_reset(void) {
    outb(SB + 6, 1);
    pause();
    outb(SB + 6, 0);
    return dsp_r() == 0xAA;
}

static i32 clip(i32 v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : v; }

/* soft knee */
static i32 soft(i32 v) {
    i32 a = v < 0 ? -v : v;
    if (a > 24000) a = 24000 + ((a - 24000) >> 2);
    if (a > 32767) a = 32767;
    return v < 0 ? -a : a;
}

static u32 rnd(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static i32 sin_at(u32 ph) {
    u32 i = ph >> 24, f = (ph >> 16) & 255;
    return sine[i] + (((sine[i + 1] - sine[i]) * (i32)f) >> 8);
}

static void note_on(int pitch, int timb, int vol, u32 off) {
    struct voice *v = NULL;
    for (int i = 0; i < NV; i++) if (!vc[i].on) { v = &vc[i]; break; }
    if (!v) {
        v = &vc[0];
        for (int i = 1; i < NV; i++) if (vc[i].age < v->age) v = &vc[i];
    }
    u64 f = (u64)base[pitch % 12] << (pitch / 12);
    v->on = 1;
    v->rel = 0;
    v->timb = (u8)timb;
    v->off = off;
    v->age = tick;
    v->vel = vol * 2184;
    if (timb == 0) {
        u64 p = (u64)RATE * 1000 * 65536 / f;
        i64 n = ((i64)p - 65536) >> 16;
        if (n < 2) n = 2;
        if (n > 1023) n = 1023;
        i64 d = (i64)p - 32768 - (n << 16);
        v->n = (u16)n;
        v->pos = 0;
        v->c = (i32)(((65536 - d) << 15) / (65536 + d));
        v->x1 = v->y1 = v->px = 0;
        v->env = 32767;
        for (int i = 0; i < n; i++) v->dl[i] = (i16)((i32)(rnd() & 0xFFFF) - 32768);
        for (int pass = 0; pass < 2; pass++)
            for (int i = 1; i < n; i++) v->dl[i] = (i16)((v->dl[i] + v->dl[i - 1]) >> 1);
    } else {
        v->inc = (u32)(f * 4294967296ull / ((u64)RATE * 1000));
        v->inc2 = v->inc + (v->inc >> 8);
        v->ph = v->ph2 = 0;
        v->env = 0;
    }
}

static i32 voice_sample(struct voice *v) {
    i32 s;
    if (v->timb == 0) {
        i32 x = v->dl[v->pos];
        i32 lp = (x + v->px) >> 1;
        v->px = x;
        i32 y = ((v->c * (lp - v->y1)) >> 15) + v->x1;
        v->x1 = lp;
        v->y1 = y;
        v->dl[v->pos] = (i16)clip((y * 253) >> 8);
        if (++v->pos == v->n) v->pos = 0;
        s = x;
        if (v->rel) v->env -= (v->env >> 10) + 1;
        s = (s * v->env) >> 15;
    } else {
        i32 a = sin_at(v->ph), b = sin_at(v->ph << 1), c = sin_at(v->ph * 3), d = sin_at(v->ph2);
        v->ph += v->inc;
        v->ph2 += v->inc2;
        s = (4 * a + 2 * b + c + 3 * d) / 10;
        if (v->rel) v->env -= (v->env >> 11) + 1;
        else if (v->env < 32767 - 24) v->env += 24;
        s = (s * v->env) >> 17;
    }
    if (v->rel && v->env < 40) v->on = 0;
    return (s * v->vel) >> 15;
}

static i32 reverb(i32 in) {
    i32 acc2 = 0;
    for (int i = 0; i < 4; i++) {
        i32 y = cb[i][cpos[i]];
        cst[i] = ((y * 26214) >> 15) + ((cst[i] * 6554) >> 15);
        cb[i][cpos[i]] = (i16)clip(in + ((cst[i] * 27500) >> 15));
        if (++cpos[i] == cd[i]) cpos[i] = 0;
        acc2 += y;
    }
    for (int i = 0; i < 2; i++) {
        i32 b = ab[i][apos[i]];
        ab[i][apos[i]] = (i16)clip(acc2 + (b >> 1));
        if (++apos[i] == ad[i]) apos[i] = 0;
        acc2 = b - acc2;
    }
    return acc2;
}

static void step_tick(void) {
    int busy = 0;
    for (int i = 0; i < NV; i++) {
        if (vc[i].on && !vc[i].rel && tick >= vc[i].off) vc[i].rel = 1;
        if (vc[i].on) busy = 1;
    }
    while (nev_i < nev_n && ev[nev_i].t <= tick) {
        struct nev *e = &ev[nev_i++];
        note_on(e->pitch, e->timb, e->vol, e->t + e->dur);
        busy = 1;
    }
    tick++;
    if (nev_i >= nev_n && !busy) finished = 1;
}

static void mix_block(i16 *out) {
    for (int i = 0; i < BLK; i++) {
        i32 m = 0;
        if (song_on) {
            acc += 65536;
            if (acc >= spt) { acc -= spt; step_tick(); }
            for (int k = 0; k < NV; k++) if (vc[k].on) m += voice_sample(&vc[k]);
            if (finished) tail++;
        }
        i32 w = reverb(m >> 5);
        out[i] = (i16)soft(m + w);
    }
}

/* dma channel 5, auto-init */
static void sb_start(void) {
    u64 f = irq_save();
    dsp_reset();
    half = 0;
    mix_block(dmabuf);
    mix_block(dmabuf + BLK);
    u32 a = (u32)(u64)dmabuf;
    outb(0xD4, 5);
    outb(0xD8, 0);
    outb(0xD6, 0x59);
    outb(0xC4, (u8)((a >> 1) & 0xFF));
    outb(0xC4, (u8)((a >> 9) & 0xFF));
    outb(0x8B, (u8)((a >> 16) & 0xFE));
    outb(0xD8, 0);
    outb(0xC6, (u8)((2 * BLK - 1) & 0xFF));
    outb(0xC6, (u8)((2 * BLK - 1) >> 8));
    outb(0xD4, 1);
    dsp_w(0xD1);
    dsp_w(0x41);
    dsp_w(RATE >> 8);
    dsp_w(RATE & 0xFF);
    dsp_w(0xB6);
    dsp_w(0x10);
    dsp_w((BLK - 1) & 0xFF);
    dsp_w((BLK - 1) >> 8);
    outb(0x21, inb(0x21) & ~0x20);
    running = 1;
    irq_restore(f);
}

static void sb_stop(void) {
    u64 f = irq_save();
    if (running) {
        dsp_w(0xD5);
        dsp_w(0xD3);
        outb(0xD4, 5);
        outb(0x21, inb(0x21) | 0x20);
        running = 0;
    }
    song_on = 0;
    irq_restore(f);
}

static void sb_irq(void) {
    inb(SB + 0xF);
    if (!running) return;
    mix_block(dmabuf + half * BLK);
    half ^= 1;
    if (finished && tail > RATE * 2) sb_stop();
}

int audio_playing(void) { return running; }

void audio_init(void) {
    if (!dsp_reset()) return;
    if (!dsp_w(0xE1) || dsp_r() < 4) return;
    dsp_r();
    have = 1;
    irq_set(5, sb_irq);
}

/* mml */
struct mml { int oct, len, vol, timb; u32 t; };
struct evbuf { struct nev *e; int n, cap; };

static int add_ev(struct evbuf *b, struct mml *m, int pitch, u32 dur) {
    if (b->n == b->cap) {
        int nc = b->cap ? b->cap * 2 : 256;
        struct nev *p = krealloc(b->e, (u32)nc * sizeof *p);
        if (!p) return 0;
        b->e = p;
        b->cap = nc;
    }
    if (pitch < 0) pitch = 0;
    if (pitch > 107) pitch = 107;
    b->e[b->n++] = (struct nev){ m->t, dur, (u8)pitch, (u8)m->timb, (u8)m->vol };
    return 1;
}

static int num(const char **p, int def) {
    if (**p < '0' || **p > '9') return def;
    int n = 0;
    while (**p >= '0' && **p <= '9') n = n * 10 + (*(*p)++ - '0');
    return n;
}

static u32 length(const char **p, int def) {
    int n = num(p, def);
    if (n < 1 || n > 192) n = 4;
    u32 t = (u32)(TPQ * 4 / n), add = t / 2;
    while (**p == '.') { t += add; add /= 2; (*p)++; }
    return t;
}

static int pitch_of(char c, const char **p, struct mml *m) {
    static const int semi[7] = { 9, 11, 0, 2, 4, 5, 7 };
    int n = m->oct * 12 + semi[c - 'a'];
    for (;; (*p)++) {
        if (**p == '+' || **p == '#') n++;
        else if (**p == '-') n--;
        else break;
    }
    return n;
}

static int parse(const char *s, const char *end, struct mml *m, struct evbuf *b) {
    while (s < end) {
        char c = *s++;
        if (c == ' ' || c == '\n' || c == '|') continue;
        if (c == 'o') m->oct = num(&s, 4);
        else if (c == '<') m->oct--;
        else if (c == '>') m->oct++;
        else if (c == 'l') m->len = num(&s, 4);
        else if (c == 'v') m->vol = num(&s, 10);
        else if (c == '@') m->timb = num(&s, 0) ? 1 : 0;
        else if (c == 't') num(&s, 0);
        else if (c == 'r') m->t += length(&s, m->len);
        else if (c >= 'a' && c <= 'g') {
            int p = pitch_of(c, &s, m);
            u32 d = length(&s, m->len);
            if (!add_ev(b, m, p, d)) return 0;
            m->t += d;
        } else if (c == '{') {
            int ps[8], np = 0;
            while (s < end && *s != '}') {
                c = *s++;
                if (c == 'o') m->oct = num(&s, 4);
                else if (c == '<') m->oct--;
                else if (c == '>') m->oct++;
                else if (c >= 'a' && c <= 'g' && np < 8) ps[np++] = pitch_of(c, &s, m);
            }
            if (s < end) s++;
            u32 d = length(&s, m->len);
            for (int i = 0; i < np; i++) if (!add_ev(b, m, ps[i], d)) return 0;
            m->t += d;
        } else if (c == '[') {
            const char *e = s;
            int depth = 1;
            while (e < end && depth) { if (*e == '[') depth++; else if (*e == ']') depth--; e++; }
            if (depth) return 0;
            const char *inner_end = e - 1, *after = e;
            int n = num(&after, 2);
            for (int i = 0; i < n; i++) if (!parse(s, inner_end, m, b)) return 0;
            s = after;
        }
    }
    return 1;
}

static int cmp_ev(const struct nev *a, const struct nev *b) { return a->t < b->t ? -1 : a->t > b->t; }

static int load_song(const struct song *sg, struct nev **out, int *n) {
    struct evbuf b = { NULL, 0, 0 };
    for (int i = 0; i < 6 && sg->tracks[i]; i++) {
        struct mml m = { 4, 4, 10, 0, 0 };
        const char *t = sg->tracks[i];
        if (!parse(t, t + strlen(t), &m, &b)) { kfree(b.e); return 0; }
    }
    for (int i = 1; i < b.n; i++) {
        struct nev k = b.e[i];
        int j = i;
        while (j > 0 && cmp_ev(&b.e[j - 1], &k) > 0) { b.e[j] = b.e[j - 1]; j--; }
        b.e[j] = k;
    }
    *out = b.e;
    *n = b.n;
    return 1;
}

/* words */
static void b_play(void) {
    str *s;
    if (!pop_str("play", &s)) return;
    if (!have) { fail("no sound card: start QEMU with -device sb16%s", ""); return; }
    const struct song *sg = NULL;
    int hits = 0;
    for (int i = 0; i < nsongs; i++) {
        if (!strcmp(songs[i].name, s->s)) { sg = &songs[i]; hits = 1; break; }
        if (!strncmp(songs[i].name, s->s, s->len) && s->len) { sg = &songs[i]; hits++; }
    }
    if (hits != 1) { fail(hits ? "that matches several songs%s" : "there is no song with that name, see songs%s", ""); return; }
    struct nev *e;
    int n;
    if (!load_song(sg, &e, &n)) { fail("out of memory%s", ""); return; }
    sb_stop();
    u64 f = irq_save();
    kfree(ev);
    ev = e;
    nev_n = n;
    nev_i = 0;
    tick = acc = tail = 0;
    finished = 0;
    spt = (u32)((u64)RATE * 60 * 65536 / ((u64)sg->bpm * TPQ));
    for (int i = 0; i < NV; i++) vc[i].on = 0;
    for (int i = 0; i < 4; i++) { memset(cb[i], 0, sizeof cb[i]); cst[i] = 0; }
    for (int i = 0; i < 2; i++) memset(ab[i], 0, sizeof ab[i]);
    song_on = 1;
    irq_restore(f);
    sb_start();
}

static void b_stop(void) { sb_stop(); }

static void b_playing(void) { push(I(running)); }

static void b_sound(void) { push(I(have)); }

static void b_songs(void) {
    for (int i = 0; i < nsongs; i++) {
        vga_color(C_BLUE);
        kprintf("  %-13s", songs[i].name);
        vga_color(C_TEXT);
        kprintf("%-30s", songs[i].title);
        vga_color(C_DIM);
        kprintf("%s\n", songs[i].by);
        vga_color(C_TEXT);
    }
}

const struct builtin audio_words[] = {
    { "play", b_play }, { "stop", b_stop }, { "playing?", b_playing }, { "sound?", b_sound }, { "songs", b_songs }, { NULL, NULL }
};

#ifdef HOST
/* offline render */
int audio_render(const char *name, i16 *out, int cap) {
    const struct song *sg = NULL;
    for (int i = 0; i < nsongs; i++) if (!strcmp(songs[i].name, name)) sg = &songs[i];
    if (!sg) return -1;
    struct nev *e;
    int n;
    if (!load_song(sg, &e, &n)) return -2;
    ev = e;
    nev_n = n;
    nev_i = 0;
    tick = acc = tail = 0;
    finished = 0;
    spt = (u32)((u64)RATE * 60 * 65536 / ((u64)sg->bpm * TPQ));
    song_on = 1;
    int total = 0;
    while (total + BLK <= cap && !(finished && tail > RATE * 2)) {
        mix_block(out + total);
        total += BLK;
    }
    return total;
}
#endif
