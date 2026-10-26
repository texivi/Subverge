#pragma once
#include "lib.h"
#include "task.h"

enum { K_UP = 0x100, K_DOWN, K_LEFT, K_RIGHT, K_HOME, K_END, K_PGUP, K_PGDN, K_DEL, K_MOUSE, K_BTAB, K_CH, K_REDRAW };
#define K_META 0x200
#define META(c) (K_META | (u8)(c))
#define K_ESC 27

#define kbreak (current->brkflag)

extern volatile int kbd_xpend;
void kbd_init(void);
void kbd_post(int tab, int k);
int kbd_get(void);
int kbd_poll(void);
