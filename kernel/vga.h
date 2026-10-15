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
void hw_cursor(int x, int y);
