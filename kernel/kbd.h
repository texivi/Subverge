#pragma once
#include "lib.h"
#include "task.h"

enum { K_UP = 0x100, K_DOWN, K_LEFT, K_RIGHT, K_HOME, K_END, K_PGUP, K_PGDN, K_DEL, K_MOUSE, K_BTAB };
#define K_ESC 27

#define kbreak (current->brkflag)

void kbd_init(void);
int kbd_get(void);
int kbd_poll(void);
