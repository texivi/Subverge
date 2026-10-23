#include "cpu.h"
#include "kbd.h"
#include "task.h"

static const char normal[] =
    "\0" "\x1b" "1234567890-=" "\b" "\t"
    "qwertyuiop[]" "\n" "\0"
    "asdfghjkl;'`" "\0" "\\"
    "zxcvbnm,./" "\0" "*" "\0" " ";
static const char shifted[] =
    "\0" "\x1b" "!@#$%^&*()_+" "\b" "\t"
    "QWERTYUIOP{}" "\n" "\0"
    "ASDFGHJKL:\"~" "\0" "|"
    "ZXCVBNM<>?" "\0" "*" "\0" " ";

static int shift, ctrl, alt, caps, ext;

static void push(int k) {
    struct task *t = &tabs[active_tab];
    int n = (t->kh + 1) % 64;
    if (n != t->kt) { t->kb[t->kh] = k; t->kh = n; }
}

static void on_key(void) {
    u8 sc = inb(0x60);
    if (sc == 0xE0) { ext = 1; return; }
    int up = sc & 0x80, e = ext;
    sc &= 0x7F;
    ext = 0;
    if (sc == 0x2A || sc == 0x36) { shift = !up; return; }
    if (sc == 0x1D) { ctrl = !up; return; }
    if (sc == 0x38) { alt = !up; return; }
    if (up) return;
    if (sc == 0x3A) { caps = !caps; return; }
    if (sc >= 0x3B && sc < 0x3B + NTABS) { switch_tab(sc - 0x3B); return; }        /* F1..F4 */
    if (alt && sc >= 0x02 && sc < 0x02 + NTABS) { switch_tab(sc - 0x02); return; }  /* alt+1..4 */
    switch (sc) {
    case 0x48: push(K_UP); return;
    case 0x50: push(K_DOWN); return;
    case 0x4B: push(K_LEFT); return;
    case 0x4D: push(K_RIGHT); return;
    case 0x47: push(K_HOME); return;
    case 0x4F: push(K_END); return;
    case 0x49: push(K_PGUP); return;
    case 0x51: push(K_PGDN); return;
    case 0x53: push(K_DEL); return;
    case 0x0F: if (shift) { push(K_BTAB); return; } break;
    }
    if (e && sc == 0x1C) { push('\n'); return; }
    if (sc >= sizeof normal - 1) return;
    char c = shift ? shifted[sc] : normal[sc];
    if (!c) return;
    if (caps && c >= 'a' && c <= 'z') c -= 32;
    else if (caps && c >= 'A' && c <= 'Z') c += 32;
    if (ctrl && (c == 'c' || c == 'C')) tabs[active_tab].brkflag = 1;
    if (ctrl && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) c = CTRL(c);
    push((u8)c);
}

void kbd_init(void) {
    while (inb(0x64) & 1) inb(0x60);
    irq_set(1, on_key);
}

int kbd_poll(void) {
    struct task *t = current;
    if (t->kh == t->kt) return -1;
    int k = t->kb[t->kt];
    t->kt = (t->kt + 1) % 64;
    return k;
}

int kbd_get(void) {
    int k;
    while ((k = kbd_poll()) < 0) {
        current->wait_key = 1;
        yield();
    }
    current->wait_key = 0;
    return k;
}
