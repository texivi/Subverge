#include "cpu.h"
#include "kbd.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

/* PS/2 mouse: the pointer is one inverted cell */

#define PW (VW * 8)
#define PH ((VH + 1) * 16)

int mouse_found, mouse_shown, mouse_cx, mouse_cy;
static int px = PW / 2, py = PH / 2, btn;

#define NEV 16
static struct mev ev[NEV];
static volatile int eh, et;

#ifndef HOST
static int wheel;
static u8 pk[4];
static int np;

static int wait_in(void) { for (int i = 0; i < 100000; i++) if (!(inb(0x64) & 2)) return 1; return 0; }
static int wait_out(void) { for (int i = 0; i < 100000; i++) if (inb(0x64) & 1) return 1; return 0; }

static int write_mouse(u8 v) {
    if (!wait_in()) return -1;
    outb(0x64, 0xD4);
    if (!wait_in()) return -1;
    outb(0x60, v);
    if (!wait_out()) return -1;
    return inb(0x60);
}

static void post(int x, int y, int b, int dz) {
    struct task *t = &tabs[active_tab];
    int n = (eh + 1) % NEV;
    if (!t->mouse_on) return;
    if (n != et) { ev[eh] = (struct mev){ x, y, b, dz }; eh = n; }
    int k = (t->kh + 1) % 64;
    if (k != t->kt) { t->kb[t->kh] = K_MOUSE; t->kh = k; }
}

static void on_mouse(void) {
    u8 b = inb(0x60);
    if (np == 0 && !(b & 8)) return;
    pk[np++] = b;
    if (np < (wheel ? 4 : 3)) return;
    np = 0;
    int dx = pk[1], dy = pk[2], dz = 0;
    if (pk[0] & 0x10) dx -= 256;
    if (pk[0] & 0x20) dy -= 256;
    if (wheel) { dz = pk[3] & 15; if (dz & 8) dz -= 16; }
    int old = btn;
    btn = pk[0] & 7;
    px = MAX(0, MIN(PW - 1, px + dx));
    py = MAX(0, MIN(PH - 1, py - dy));
    mouse_move(px / 8, py / 16);
    int down = btn & ~old;
    if ((down & 1) && mouse_cy == 0 && tab_at(mouse_cx) >= 0) { switch_tab(tab_at(mouse_cx)); return; }
    if (down || dz) post(mouse_cx, mouse_cy - 1, down, dz);
}

void mouse_init(void) {
    if (!wait_in()) return;
    outb(0x64, 0xA8);
    if (!wait_in()) return;
    outb(0x64, 0x20);
    if (!wait_out()) return;
    u8 st = inb(0x60);
    st = (u8)((st | 2) & ~0x20);
    if (!wait_in()) return;
    outb(0x64, 0x60);
    if (!wait_in()) return;
    outb(0x60, st);
    if (write_mouse(0xF6) != 0xFA) return;
    /* the sample rates 200, 100, 80 ask for a wheel */
    static const u8 seq[] = { 200, 100, 80 };
    for (int i = 0; i < 3; i++) { write_mouse(0xF3); write_mouse(seq[i]); }
    write_mouse(0xF2);
    if (wait_out() && inb(0x60) == 3) wheel = 1;
    if (write_mouse(0xF4) != 0xFA) return;
    irq_set(12, on_mouse);
    mouse_found = 1;
}
#endif

int mouse_event(struct mev *e) {
    u64 f = irq_save();
    int ok = et != eh;
    if (ok) { *e = ev[et]; et = (et + 1) % NEV; }
    irq_restore(f);
    return ok;
}

void mouse_flush(void) { u64 f = irq_save(); et = eh; irq_restore(f); }

static void b_mouse(void) {
    push(I(mouse_cx));
    push(I(mouse_cy - 1));
    push(I(btn));
}

static void b_mouseq(void) { push(I(mouse_found)); }

const struct builtin mouse_words[] = { { "mouse", b_mouse }, { "mouse?", b_mouseq }, { NULL, NULL } };
