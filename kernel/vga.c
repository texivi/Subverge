#include "cpu.h"
#include "kbd.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

#ifdef HOST
u16 host_screen[80 * 25];
static volatile u16 *const HW = host_screen;
#define outb(p, v) ((void)(p), (void)(v))
#define inb(p) ((void)(p), 0)
#else
static volatile u16 *const HW = (volatile u16 *)0xB8000;
#endif

/* colour names */
const struct theme themes[] = {
    { "dusk", {
        0x1e1e2e, 0x313244, 0xa6e3a1, 0x94e2d5, 0xf38ba8, 0xcba6f7, 0xfab387, 0xcdd6f4,
        0x6c7086, 0x89b4fa, 0xa6e3a1, 0x89dceb, 0xf38ba8, 0xf5c2e7, 0xf6dfad, 0xcad3f1 } },
    { "raw", {
        0x000000, 0x0000aa, 0x00aa00, 0x00aaaa, 0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
        0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff } },
    { "paper", {
        0xf1ebdd, 0xddd3bc, 0x2f7a32, 0x00756a, 0xb3141e, 0x6b2a92, 0xb85600, 0x2d2b26,
        0x8c8473, 0x1f4f9e, 0x2f7a32, 0x0a6a86, 0xc0262e, 0xa3155e, 0xb0101a, 0x121110 } },
    { "amber", {
        0x0c0700, 0x2a1a00, 0xffb000, 0xd99200, 0xff7a00, 0xcc8a00, 0xffc040, 0xffb000,
        0x8a5c00, 0xe0a000, 0xffcc4d, 0xffd27a, 0xff8c1a, 0xffbf66, 0xffd000, 0xffe0a0 } },
    { "phosphor", {
        0x000a02, 0x022a0a, 0x33ff66, 0x22cc77, 0x7dff3d, 0x2ee88a, 0x9cff57, 0x33ff66,
        0x188a3a, 0x29d17a, 0x66ff8f, 0x8affb0, 0xa6ff4d, 0x5cff9d, 0xb3ff99, 0xd6ffe0 } },
    { "haelide", {
        0x010104, 0x08045a, 0x3a2cff, 0x2a48ff, 0x7a2aff, 0x5a30f0, 0x4a3cff, 0x3d34ff,
        0x1c12b4, 0x1102e9, 0x5a52ff, 0x5272ff, 0x9a5aff, 0x7a62ff, 0x2a18ff, 0x9890ff } },
    { NULL, { 0 } }
};
static int cur_theme;
int gfx_on;

static u8 dac_index(int i) { return i < 6 ? (u8)i : i == 6 ? 0x14 : i == 7 ? 7 : (u8)(0x38 + i - 8); }

void vga_rgb(int i, u32 rgb) {
    outb(0x3C8, dac_index(i & 15));
    outb(0x3C9, (u8)((rgb >> 18) & 0x3F));
    outb(0x3C9, (u8)((rgb >> 10) & 0x3F));
    outb(0x3C9, (u8)((rgb >> 2) & 0x3F));
}

int vga_theme(int n) {
    int k = 0;
    while (themes[k].name) k++;
    if (n < 0 || n >= k) return -1;
    cur_theme = n;
    for (int i = 0; i < 16; i++) vga_rgb(i, themes[n].rgb[i]);
    return 0;
}

int vga_current_theme(void) { return cur_theme; }

void hw_put(int x, int y, char c, u8 a) {
    if (!gfx_on && x >= 0 && x < VW && y >= 0 && y <= VH) HW[y * VW + x] = (u16)(u8)c | (u16)a << 8;
}

void hw_cursor(int x, int y) {
    u16 p = (u16)(y * VW + x);
    outb(0x3D4, 0x0F); outb(0x3D5, (u8)(p & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (u8)(p >> 8));
}

/* text registers */

static struct { u8 misc, sr[5], cr[25], gr[9], ar[21]; } rs;
static u8 rd(u16 idx, u8 i) { outb(idx, i); return inb((u16)(idx + 1)); }
static void wr(u16 idx, u8 i, u8 v) { outb(idx, i); outb((u16)(idx + 1), v); }

void vga_save(void) {
    rs.misc = inb(0x3CC);
    for (int i = 0; i < 5; i++) rs.sr[i] = rd(0x3C4, (u8)i);
    for (int i = 0; i < 25; i++) rs.cr[i] = rd(0x3D4, (u8)i);
    for (int i = 0; i < 9; i++) rs.gr[i] = rd(0x3CE, (u8)i);
    for (int i = 0; i < 21; i++) { inb(0x3DA); outb(0x3C0, (u8)(i | 0x20)); rs.ar[i] = inb(0x3C1); }
    inb(0x3DA);
    outb(0x3C0, 0x20);
}

void vga_restore(void) {
    outb(0x3C2, rs.misc);
    wr(0x3C4, 0, 1);
    for (int i = 1; i < 5; i++) wr(0x3C4, (u8)i, rs.sr[i]);
    wr(0x3C4, 0, 3);
    wr(0x3D4, 0x11, rs.cr[0x11] & 0x7F);
    for (int i = 0; i < 25; i++) if (i != 0x11) wr(0x3D4, (u8)i, rs.cr[i]);
    wr(0x3D4, 0x11, rs.cr[0x11]);
    for (int i = 0; i < 9; i++) wr(0x3CE, (u8)i, rs.gr[i]);
    for (int i = 0; i < 21; i++) { inb(0x3DA); outb(0x3C0, (u8)i); outb(0x3C0, rs.ar[i]); }
    inb(0x3DA);
    outb(0x3C0, 0x20);
}

void vga_init(void) {
    vga_theme(0);
    /* no blink */
    inb(0x3DA);
    outb(0x3C0, 0x30);
    u8 mode = inb(0x3C1);
    inb(0x3DA);
    outb(0x3C0, 0x30);
    outb(0x3C0, (u8)(mode & ~0x08));
    outb(0x3D4, 0x0A); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0B); outb(0x3D5, 0x0F);
    for (int i = 0; i < VW * (VH + 1); i++) HW[i] = ' ' | (u16)ATTR(C_TEXT, C_BG) << 8;
}

#define T (&current->term)

/* window tree */
struct wn { u8 kind, alive; int a, b, tab, x, y, w, h, w1; };
static struct wn wn[8] = { { 0, 1, 0, 0, 0, 0, 0, VW, VH, 0 } };
static int wroot, last_tab, nleaves = 1;
static struct { int vis, x, y, w, h; } tw[NTABS] = { { 1, 0, 0, VW, VH } };

int term_h(void) { return current->term.h; }
int term_w(void) { return current->term.w; }

/* hw cell of a tab cell */
static int hwcell(int id, int x, int y) {
    if (id < 0 || !tw[id].vis || x < 0 || y < 0 || x >= tabs[id].term.w || y >= tabs[id].term.h) return -1;
    return (1 + tw[id].y + y) * VW + tw[id].x + x;
}

static void paint_tab(int id) {
    if (gfx_on || id < 0) return;
    for (int y = 0; y < tabs[id].term.h; y++)
        for (int x = 0; x < tabs[id].term.w; x++) {
            int c = hwcell(id, x, y);
            if (c >= 0) HW[c] = tabs[id].term.buf[y * VW + x];
        }
}

/* mode lines, dividers */
static void decorations(void) {
    if (nleaves < 2 || gfx_on) return;
    for (int i = 0; i < 8; i++) {
        struct wn *k = &wn[i];
        if (!k->alive) continue;
        if (k->kind == 0) {
            u8 at = k->tab == active_tab ? ATTR(C_BG, C_MOON) : ATTR(C_DIM, C_BAR);
            char t[40];
            snfmt(t, sizeof t, " %d %s", k->tab + 1, tabs[k->tab].title);
            for (int x = 0; x < k->w; x++) hw_put(k->x + x, k->y + k->h, x < (int)strlen(t) ? t[x] : ' ', at);
        } else if (k->kind == 1) {
            for (int y = 0; y < k->h; y++) hw_put(k->x + k->w1, 1 + k->y + y, 0xB3, ATTR(C_DIM, C_BG));
        }
    }
}

void vga_put(int x, int y, char c, u8 a) {
    if (x < 0 || x >= T->w || y < 0 || y >= T->h) return;
    u64 f = irq_save();
    u16 v = (u16)(u8)c | (u16)a << 8;
    T->buf[y * VW + x] = v;
    int hc = hwcell(current->id, x, y);
    if (hc >= 0) {
        if (!gfx_on) { HW[hc] = v; if (mouse_shown && hc % VW == mouse_cx && hc / VW == mouse_cy) mouse_redraw(); }
    } else if (current->id < 0 || !tw[current->id].vis) T->activity = 1;
    irq_restore(f);
}

void vga_puts_at(int x, int y, const char *s, u8 a) {
    for (; *s && x < T->w; s++, x++) vga_put(x, y, *s, a);
}

void vga_fill(int x, int y, int n, char c, u8 a) {
    while (n-- > 0) vga_put(x++, y, c, a);
}

void vga_cursor(int x, int y) {
    u64 f = irq_save();
    T->curx = x;
    T->cury = y;
    if (current->id == active_tab) {
        int hc = hwcell(current->id, x, y);
        if (hc >= 0) hw_cursor(hc % VW, hc / VW); else hw_cursor(0, VH + 1);
    }
    irq_restore(f);
}

void vga_clear(void) {
    for (int y = 0; y < T->h; y++) vga_fill(0, y, T->w, ' ', ATTR(C_TEXT, C_BG));
    T->cx = T->cy = 0;
    vga_cursor(0, 0);
}

void vga_color(u8 fg) { T->attr = ATTR(fg, C_BG); }
void vga_attr(u8 a) { T->attr = a; }
u8 vga_getattr(void) { return T->attr; }
void vga_getpos(int *x, int *y) { *x = T->cx; *y = T->cy; }
void vga_setpos(int x, int y) { T->cx = x; T->cy = y; vga_cursor(x, y); }

static void scroll(void) {
    int h = T->h;
    u64 f = irq_save();
    memmove(T->buf, T->buf + VW, VW * (size_t)(h - 1) * 2);
    for (int x = 0; x < VW; x++) T->buf[(h - 1) * VW + x] = ' ' | (u16)ATTR(C_TEXT, C_BG) << 8;
    paint_tab(current->id);
    mouse_redraw();
    irq_restore(f);
    T->cy = h - 1;
}

void kputc(char c) {
    if (c == '\n') { T->cx = 0; T->cy++; }
    else if (c == '\r') T->cx = 0;
    else if (c == '\t') { do kputc(' '); while (T->cx % 4); return; }
    else if (c == '\b') { if (T->cx) T->cx--; }
    else {
        vga_put(T->cx, T->cy, c, T->attr);
        if (++T->cx >= T->w) { T->cx = 0; T->cy++; }
    }
    if (T->cy >= T->h) scroll();
    vga_cursor(T->cx, T->cy);
}

void kputs(const char *s) { while (*s) kputc(*s++); }

void kprintf(const char *f, ...) {
    char b[256];
    va_list ap;
    va_start(ap, f);
    vsnfmt(b, sizeof b, f, ap);
    va_end(ap);
    kputs(b);
}

/* tabs */

static u8 cmos(u8 r) { outb(0x70, r); return inb(0x71); }

static void tab_text(int i, char *t, int cap) {
    struct task *k = &tabs[i];
    snfmt(t, (size_t)cap, " %d %s%s ", i + 1, k->title, i != active_tab && k->term.activity ? "*" : "");
}

/* tab at x */
int tab_at(int x) {
    int c = 1;
    for (int i = 0; i < NTABS; i++) {
        char t[32];
        tab_text(i, t, sizeof t);
        int n = (int)strlen(t);
        if (x >= c && x < c + n) return i;
        c += n + 1;
    }
    return -1;
}

void draw_tabbar(void) {
    u64 f = irq_save();
    u8 bar = ATTR(C_DIM, C_BAR);
    for (int x = 0; x < VW; x++) hw_put(x, 0, ' ', bar);
    int x = 1;
    for (int i = 0; i < NTABS; i++) {
        char t[32];
        int on = i == active_tab, also = nleaves > 1 && tw[i].vis;
        tab_text(i, t, sizeof t);
        for (const char *s = t; *s; s++) hw_put(x++, 0, *s, on ? ATTR(C_BG, C_MOON) : also ? ATTR(C_MOON, C_BAR) : bar);
        x++;
    }
    u8 h = cmos(4), m = cmos(2), b = cmos(0x0B);
    if (!(b & 0x04)) { h = (u8)((h & 0x0F) + ((h >> 4) & 7) * 10); m = (u8)((m & 0x0F) + (m >> 4) * 10); }
    if (kbd_xpend) for (int i = 0; "C-x -"[i]; i++) hw_put(VW - 19 + i, 0, "C-x -"[i], ATTR(C_MOON, C_BAR));
    char t[8];
    snfmt(t, sizeof t, "%02u:%02u", h, m);
    for (int i = 0; t[i]; i++) hw_put(VW - 6 + i, 0, t[i], ATTR(C_TEXT, C_BAR));
    decorations();
    if (mouse_cy == 0) mouse_redraw();
    irq_restore(f);
}

/* fit a tab to a size */
static void set_size(int id, int w, int h) {
    struct term *t = &tabs[id].term;
    if (t->w == w && t->h == h) return;
    int d = t->cy >= h ? t->cy - h + 1 : 0;
    if (d) {
        memmove(t->buf, t->buf + d * VW, VW * (size_t)(VH - d) * 2);
        for (int i = (VH - d) * VW; i < VW * VH; i++) t->buf[i] = ' ' | (u16)ATTR(C_TEXT, C_BG) << 8;
        t->cy -= d;
        t->cury = t->cury >= d ? t->cury - d : 0;
    }
    t->w = w;
    t->h = h;
    if (t->cx >= w) t->cx = w > 0 ? w - 1 : 0;
    if (tabs[id].wait_key) kbd_post(id, K_REDRAW);
}

static int count_leaves(int n) { return wn[n].kind == 0 ? 1 : count_leaves(wn[n].a) + count_leaves(wn[n].b); }

static void lay(int n, int x, int y, int w, int h) {
    struct wn *k = &wn[n];
    k->x = x; k->y = y; k->w = w; k->h = h;
    if (k->kind == 0) {
        tw[k->tab].vis = 1;
        tw[k->tab].x = x;
        tw[k->tab].y = y;
        tw[k->tab].w = w;
        tw[k->tab].h = nleaves > 1 ? h - 1 : h;
    } else if (k->kind == 1) {
        k->w1 = w / 2;
        lay(k->a, x, y, k->w1, h);
        lay(k->b, x + k->w1 + 1, y, w - k->w1 - 1, h);
    } else {
        int h1 = h / 2;
        lay(k->a, x, y, w, h1);
        lay(k->b, x, y + h1, w, h - h1);
    }
}

/* lay out and repaint */
static void apply(void) {
    memset(tw, 0, sizeof tw);
    nleaves = count_leaves(wroot);
    lay(wroot, 0, 0, VW, VH);
    for (int i = 0; i < NTABS; i++) {
        if (tw[i].vis) set_size(i, tw[i].w, tw[i].h);
        else set_size(i, VW, VH);
    }
    if (gfx_on) return;
    for (int i = 0; i < VW * VH; i++) HW[VW + i] = ' ' | (u16)ATTR(C_TEXT, C_BG) << 8;
    for (int i = 0; i < NTABS; i++) paint_tab(i);
    struct term *t = &tabs[active_tab].term;
    int hc = hwcell(active_tab, t->cury >= 0 ? t->curx : 0, t->cury);
    if (hc >= 0) hw_cursor(hc % VW, hc / VW); else hw_cursor(0, VH + 1);
    draw_tabbar();
    if (mouse_cy != 0) mouse_redraw();
}

static int walloc(void) {
    for (int i = 0; i < 8; i++)
        if (!wn[i].alive) { memset(&wn[i], 0, sizeof wn[i]); wn[i].alive = 1; return i; }
    return -1;
}

static int leaf_of(int tab) {
    for (int i = 0; i < 8; i++) if (wn[i].alive && wn[i].kind == 0 && wn[i].tab == tab) return i;
    return -1;
}

static int parent_of(int n) {
    for (int i = 0; i < 8; i++) if (wn[i].alive && wn[i].kind && (wn[i].a == n || wn[i].b == n)) return i;
    return -1;
}

static int first_leaf(int n) {
    while (wn[n].kind) n = wn[n].a;
    return wn[n].tab;
}

static void leaves(int n, int *o, int *k) {
    if (wn[n].kind == 0) { o[(*k)++] = wn[n].tab; return; }
    leaves(wn[n].a, o, k);
    leaves(wn[n].b, o, k);
}

void switch_tab(int n) {
    if (n < 0 || n >= NTABS) return;
    gfx_leave();
    u64 f = irq_save();
    if (n != active_tab) last_tab = active_tab;
    if (!tw[n].vis) {
        int lf = leaf_of(active_tab);
        if (lf >= 0) wn[lf].tab = n;
    }
    active_tab = n;
    tabs[n].term.activity = 0;
    apply();
    irq_restore(f);
}

/* dir 0 picks by shape, 1 side by side, 2 stacked */
void win_new(int dir) {
    gfx_leave();
    u64 f = irq_save();
    int hid = -1, lf = leaf_of(active_tab);
    if (!tw[last_tab].vis) hid = last_tab;
    for (int i = 0; i < NTABS && hid < 0; i++) if (!tw[i].vis) hid = i;
    if (hid >= 0 && lf >= 0) {
        int w = tw[active_tab].w, h = tw[active_tab].h + (nleaves > 1 ? 1 : 0);
        if (!dir) dir = w * 8 >= h * 16 ? 1 : 2;
        if ((dir == 1 && w >= 21) || (dir == 2 && h >= 8)) {
            int a = walloc(), b = walloc();
            if (a >= 0 && b >= 0) {
                wn[a].tab = active_tab;
                wn[b].tab = hid;
                wn[lf].kind = (u8)dir;
                wn[lf].a = a;
                wn[lf].b = b;
                last_tab = active_tab;
                active_tab = hid;
                tabs[hid].term.activity = 0;
                apply();
            } else {
                if (a >= 0) wn[a].alive = 0;
                if (b >= 0) wn[b].alive = 0;
            }
        }
    }
    irq_restore(f);
}

void win_close(void) {
    u64 f = irq_save();
    int lf = leaf_of(active_tab), p = lf >= 0 ? parent_of(lf) : -1;
    if (p >= 0) {
        int sib = wn[p].a == lf ? wn[p].b : wn[p].a;
        struct wn s = wn[sib];
        wn[lf].alive = wn[sib].alive = 0;
        wn[p].kind = s.kind;
        wn[p].a = s.a;
        wn[p].b = s.b;
        wn[p].tab = s.tab;
        last_tab = active_tab;
        active_tab = first_leaf(p);
        apply();
    }
    irq_restore(f);
}

void win_one(void) {
    u64 f = irq_save();
    if (nleaves > 1) {
        for (int i = 0; i < 8; i++) wn[i].alive = 0;
        memset(&wn[0], 0, sizeof wn[0]);
        wn[0].alive = 1;
        wn[0].tab = active_tab;
        wroot = 0;
        apply();
    }
    irq_restore(f);
}

void win_other(void) {
    if (nleaves < 2) { switch_tab((active_tab + 1) % NTABS); return; }
    u64 f = irq_save();
    int o[NTABS], k = 0, at = 0;
    leaves(wroot, o, &k);
    for (int i = 0; i < k; i++) if (o[i] == active_tab) at = i;
    last_tab = active_tab;
    active_tab = o[(at + 1) % k];
    tabs[active_tab].term.activity = 0;
    apply();
    irq_restore(f);
}

/* focus the window in a direction */
void win_move(int dx, int dy) {
    u64 f = irq_save();
    int best = -1, bd = 1 << 30;
    int cx = tw[active_tab].x + tw[active_tab].w / 2, cy = tw[active_tab].y + tw[active_tab].h / 2;
    for (int i = 0; i < NTABS; i++) {
        if (i == active_tab || !tw[i].vis) continue;
        int x = tw[i].x + tw[i].w / 2, y = tw[i].y + tw[i].h / 2;
        int fx = (x - cx) * dx, fy = (y - cy) * dy;
        if (fx + fy <= 0) continue;
        int d = (x - cx) * (x - cx) * (dx ? 1 : 4) + (y - cy) * (y - cy) * (dy ? 1 : 4);
        if (d < bd) { bd = d; best = i; }
    }
    if (best >= 0) {
        last_tab = active_tab;
        active_tab = best;
        tabs[best].term.activity = 0;
        apply();
    }
    irq_restore(f);
}

/* tab at a screen cell */
int win_at(int x, int y) {
    for (int i = 0; i < NTABS; i++)
        if (tw[i].vis && x >= tw[i].x && x < tw[i].x + tw[i].w && y - 1 >= tw[i].y && y - 1 < tw[i].y + tw[i].h) return i;
    return -1;
}

void win_origin(int id, int *x, int *y) {
    *x = id >= 0 && tw[id].vis ? tw[id].x : 0;
    *y = id >= 0 && tw[id].vis ? tw[id].y : 0;
}

void set_title(const char *s) {
    scopy(current->title, s, sizeof current->title);
    draw_tabbar();
}

/* pointer cell */

static void restore_cell(int x, int y) {
    for (int i = 0; i < NTABS; i++) {
        int lx = x - tw[i].x, ly = y - 1 - tw[i].y;
        if (tw[i].vis && lx >= 0 && ly >= 0 && lx < tabs[i].term.w && ly < tabs[i].term.h) {
            HW[y * VW + x] = tabs[i].term.buf[ly * VW + lx];
            return;
        }
    }
    decorations();
}

static void swap_cell(int x, int y) {
    u16 v = HW[y * VW + x], a = v >> 8;
    HW[y * VW + x] = (u16)((v & 0xFF) | (u16)(((a << 4) | (a >> 4)) & 0xFF) << 8);
}

void mouse_redraw(void) {
    if (mouse_shown && !gfx_on) swap_cell(mouse_cx, mouse_cy);
}

void mouse_move(int x, int y) {
    u64 f = irq_save();
    if (mouse_shown && !gfx_on) {
        int ox = mouse_cx, oy = mouse_cy;
        mouse_shown = 0;
        if (oy > 0) restore_cell(ox, oy);
        else draw_tabbar();
        mouse_shown = 1;
    }
    mouse_cx = x;
    mouse_cy = y;
    mouse_shown = 1;
    mouse_redraw();
    irq_restore(f);
}
