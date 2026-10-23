#pragma once
#include "lib.h"

#define VW 80
#define VH 24

enum {
    C_BG = 0, C_BAR = 1, C_GREEN = 2, C_TEAL = 3, C_RED = 4, C_MAUVE = 5, C_PEACH = 6, C_TEXT = 7,
    C_DIM = 8, C_BLUE = 9, C_LGREEN = 10, C_SKY = 11, C_LRED = 12, C_PINK = 13, C_MOON = 14, C_CAT = 15
};
#define ATTR(fg, bg) ((u8)((fg) | ((bg) << 4)))

void vga_init(void);
void vga_clear(void);
void vga_color(u8 fg);
void vga_attr(u8 a);
u8 vga_getattr(void);
void vga_put(int x, int y, char c, u8 a);
void vga_puts_at(int x, int y, const char *s, u8 a);
void vga_fill(int x, int y, int n, char c, u8 a);
void vga_cursor(int x, int y);
void vga_getpos(int *x, int *y);
void vga_setpos(int x, int y);
void kputc(char c);
void kputs(const char *s);
void kprintf(const char *f, ...);

void hw_put(int x, int y, char c, u8 a);
extern int gfx_on;
void gfx_leave(void);
void gfx_text(i64 x, i64 y, i64 c, const char *s, u32 len);
u32 pci_read(u32 bus, u32 dev, u32 fn, u32 off);
#define GW 640
#define GH 480
extern volatile u32 *gfx_lfb;
void vga_save(void);
void vga_restore(void);

struct theme { const char *name; u32 rgb[16]; };
extern const struct theme themes[];
int vga_theme(int n);
int vga_current_theme(void);
void vga_rgb(int i, u32 rgb);
void hw_cursor(int x, int y);
extern char logo_name[24];
