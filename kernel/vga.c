#include "cpu.h"
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

static const u32 theme[16] = {
    0x1e1e2e, 0x313244, 0xa6e3a1, 0x94e2d5, 0xf38ba8, 0xcba6f7, 0xfab387, 0xcdd6f4,
    0x6c7086, 0x89b4fa, 0xa6e3a1, 0x89dceb, 0xf38ba8, 0xf5c2e7, 0xf6dfad, 0xcad3f1
};

static u8 dac_index(int i) { return i < 6 ? (u8)i : i == 6 ? 0x14 : i == 7 ? 7 : (u8)(0x38 + i - 8); }

void hw_put(int x, int y, char c, u8 a) {
    if (x >= 0 && x < VW && y >= 0 && y <= VH) HW[y * VW + x] = (u16)(u8)c | (u16)a << 8;
}

void hw_cursor(int x, int y) {
    u16 p = (u16)(y * VW + x);
    outb(0x3D4, 0x0F); outb(0x3D5, (u8)(p & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (u8)(p >> 8));
}

void vga_init(void) {
    for (int i = 0; i < 16; i++) {
        outb(0x3C8, dac_index(i));
        outb(0x3C9, (u8)((theme[i] >> 18) & 0x3F));
        outb(0x3C9, (u8)((theme[i] >> 10) & 0x3F));
        outb(0x3C9, (u8)((theme[i] >> 2) & 0x3F));
    }
    outb(0x3D4, 0x0A); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0B); outb(0x3D5, 0x0F);
    for (int i = 0; i < VW * (VH + 1); i++) HW[i] = ' ' | (u16)ATTR(C_TEXT, C_BG) << 8;
}

#define T (&current->term)
static int is_active(void) { return current == &tabs[active_tab]; }

void vga_put(int x, int y, char c, u8 a) {
    if (x < 0 || x >= VW || y < 0 || y >= VH) return;
    u64 f = irq_save();
    u16 v = (u16)(u8)c | (u16)a << 8;
    T->buf[y * VW + x] = v;
    if (is_active()) HW[(y + 1) * VW + x] = v;
    else T->activity = 1;
    irq_restore(f);
}

void vga_puts_at(int x, int y, const char *s, u8 a) {
    for (; *s && x < VW; s++, x++) vga_put(x, y, *s, a);
}

void vga_fill(int x, int y, int n, char c, u8 a) {
    while (n-- > 0) vga_put(x++, y, c, a);
}

void vga_cursor(int x, int y) {
    u64 f = irq_save();
    T->curx = x;
    T->cury = y;
    if (is_active()) hw_cursor(x, y + 1);
    irq_restore(f);
}

void vga_clear(void) {
    for (int y = 0; y < VH; y++) vga_fill(0, y, VW, ' ', ATTR(C_TEXT, C_BG));
    T->cx = T->cy = 0;
    vga_cursor(0, 0);
}

void vga_color(u8 fg) { T->attr = ATTR(fg, C_BG); }
void vga_attr(u8 a) { T->attr = a; }
u8 vga_getattr(void) { return T->attr; }
void vga_getpos(int *x, int *y) { *x = T->cx; *y = T->cy; }
void vga_setpos(int x, int y) { T->cx = x; T->cy = y; vga_cursor(x, y); }

static void scroll(void) {
    u64 f = irq_save();
    memmove(T->buf, T->buf + VW, VW * (VH - 1) * 2);
    for (int x = 0; x < VW; x++) T->buf[(VH - 1) * VW + x] = ' ' | (u16)ATTR(C_TEXT, C_BG) << 8;
    if (is_active()) for (int i = 0; i < VW * VH; i++) HW[VW + i] = T->buf[i];
    irq_restore(f);
    T->cy = VH - 1;
}

void kputc(char c) {
    if (c == '\n') { T->cx = 0; T->cy++; }
    else if (c == '\r') T->cx = 0;
    else if (c == '\t') { do kputc(' '); while (T->cx % 4); return; }
    else if (c == '\b') { if (T->cx) T->cx--; }
    else {
        vga_put(T->cx, T->cy, c, T->attr);
        if (++T->cx == VW) { T->cx = 0; T->cy++; }
    }
    if (T->cy == VH) scroll();
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

void draw_tabbar(void) {
    u64 f = irq_save();
    u8 bar = ATTR(C_DIM, C_BAR);
    for (int x = 0; x < VW; x++) hw_put(x, 0, ' ', bar);
    int x = 1;
    for (int i = 0; i < NTABS; i++) {
        char t[32];
        struct task *k = &tabs[i];
        int on = i == active_tab;
        snfmt(t, sizeof t, " %d %s%s ", i + 1, k->title, !on && k->term.activity ? "*" : "");
        for (const char *s = t; *s; s++) hw_put(x++, 0, *s, on ? ATTR(C_BG, C_MOON) : bar);
        x++;
    }
    u8 h = cmos(4), m = cmos(2), b = cmos(0x0B);
    if (!(b & 0x04)) { h = (u8)((h & 0x0F) + ((h >> 4) & 7) * 10); m = (u8)((m & 0x0F) + (m >> 4) * 10); }
    char t[8];
    snfmt(t, sizeof t, "%02u:%02u", h, m);
    for (int i = 0; t[i]; i++) hw_put(VW - 6 + i, 0, t[i], ATTR(C_TEXT, C_BAR));
    irq_restore(f);
}

void switch_tab(int n) {
    if (n < 0 || n >= NTABS) return;
    u64 f = irq_save();
    active_tab = n;
    struct term *t = &tabs[n].term;
    t->activity = 0;
    for (int i = 0; i < VW * VH; i++) HW[VW + i] = t->buf[i];
    hw_cursor(t->curx, t->cury + 1);
    draw_tabbar();
    irq_restore(f);
}

void set_title(const char *s) {
    scopy(current->title, s, sizeof current->title);
    draw_tabbar();
}
