#include "cpu.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

/* Bochs VBE */

volatile u32 *gfx_lfb;
static u32 pal[16];
static u8 saved_font[FONT_BYTES];

static void outw(u16 p, u16 v) { __asm__ volatile("outw %0, %1" : : "a"(v), "Nd"(p)); }
static u16 inw(u16 p) { u16 v; __asm__ volatile("inw %1, %0" : "=a"(v) : "Nd"(p)); return v; }
static void vbe(u16 i, u16 v) { outw(0x1CE, i); outw(0x1CF, v); }

static u64 find_lfb(void) {
    for (u32 dev = 0; dev < 32; dev++)
        if (pci_read(0, dev, 0, 0) == 0x11111234u) return pci_read(0, dev, 0, 0x10) & ~0xFu;
    return 0;
}

void gfx_leave(void) {
    if (!gfx_on) return;
    vbe(4, 0);
    font_restore(saved_font);
    vga_restore();
    vga_theme(vga_current_theme());
    gfx_on = 0;
}

static void b_gfx(void) {
    if (gfx_on) return;
    u64 fb = find_lfb();
    outw(0x1CE, 0);
    if (!fb || fb >= 1ull << 32 || (inw(0x1CF) & 0xFFF0) != 0xB0C0) {
        fail("no Bochs display here, start QEMU with -vga std%s", "");
        return;
    }
    gfx_lfb = (volatile u32 *)fb;
    vga_save();
    font_save(saved_font);
    gfx_on = 1;
    vbe(4, 0);
    vbe(1, GW); vbe(2, GH); vbe(3, 32);
    vbe(4, 0x41);
    const u32 *rgb = themes[vga_current_theme()].rgb;
    for (int i = 0; i < 16; i++) pal[i] = rgb[i] & 0xFFFFFF;
    for (int i = 0; i < GW * GH; i++) gfx_lfb[i] = pal[0];
}

static void b_gfxoff(void) {
    if (!gfx_on) return;
    gfx_leave();
    switch_tab(active_tab);
}

static int need_gfx(const char *w) {
    if (!gfx_on) { fail("graphics mode is off, run gfx first (%s)", w); return 0; }
    return 1;
}

static void b_dot(void) {
    i64 x, y, c;
    if (!pop_int("dot", &c) || !pop_int("dot", &y) || !pop_int("dot", &x)) return;
    if (!gfx_on) { fail("graphics mode is off, run gfx first%s", ""); return; }
    if (c < 0 || c > 15) { fail("dot takes a colour from 0 to 15%s", ""); return; }
    if (x >= 0 && x < GW && y >= 0 && y < GH) gfx_lfb[y * GW + x] = pal[c];
}

static void rect(i64 x, i64 y, i64 w, i64 h, u32 c) {
    for (i64 j = y; j < y + h; j++)
        for (i64 i = x; i < x + w; i++)
            if (i >= 0 && i < GW && j >= 0 && j < GH) gfx_lfb[j * GW + i] = c;
}

static void b_box(void) {
    i64 x, y, w, h, c;
    if (!pop_int("box", &c) || !pop_int("box", &h) || !pop_int("box", &w) || !pop_int("box", &y) || !pop_int("box", &x)) return;
    if (!gfx_on) { fail("graphics mode is off, run gfx first%s", ""); return; }
    if (c < 0 || c > 15) { fail("box takes a colour from 0 to 15%s", ""); return; }
    rect(x, y, w, h, pal[c]);
}

static void glyph(int x, int y, u8 ch, u32 c) {
    const u8 *g = saved_font + ch * 32;
    for (int r = 0; r < 16; r++)
        for (int b = 0; b < 8; b++)
            if (g[r] & (0x80 >> b) && x + b >= 0 && x + b < GW && y + r >= 0 && y + r < GH) gfx_lfb[(y + r) * GW + x + b] = c;
}

void gfx_text(i64 x, i64 y, i64 c, const char *s, u32 len) {
    for (u32 k = 0; k < len; k++) glyph((int)x + 8 * (int)k, (int)y, (u8)s[k], pal[c & 15]);
}

static void b_window(void) {
    i64 x, y, w, h;
    str *t;
    if (!pop_str("window", &t) || !pop_int("window", &h) || !pop_int("window", &w) || !pop_int("window", &y) || !pop_int("window", &x)) return;
    if (!need_gfx("window")) return;
    if (w < 16 || h < 24) { fail("window needs at least 16 by 24%s", ""); return; }
    rect(x, y, w, h, pal[7]);
    rect(x + 1, y + 1, w - 2, 18, pal[1]);
    rect(x + 2, y + 20, w - 4, h - 22, pal[0]);
    for (u32 i = 0; i < t->len && 8 * (i64)i + 12 < w; i++) glyph((int)x + 6 + 8 * (int)i, (int)y + 2, (u8)t->s[i], pal[15]);
}

const struct builtin gfx_words[] = {
    { "gfx", b_gfx }, { "gfx-off", b_gfxoff }, { "dot", b_dot }, { "box", b_box }, { "window", b_window }, { NULL, NULL }
};
