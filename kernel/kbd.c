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
volatile int kbd_xpend;

static void push(int k) {
    struct task *t = &tabs[active_tab];
    int n = (t->kh + 1) % 64;
    if (n != t->kt) { t->kb[t->kh] = k; t->kh = n; }
}

static void xset(int on) { kbd_xpend = on; draw_tabbar(); }

/* flush prefix */
static void xflush(void) {
    if (!kbd_xpend) return;
    xset(0);
    push(CTRL('x'));
}

/* C-x window keys */
static int xkey(int c) {
    switch (c) {
    case 'o': switch_tab((active_tab + 1) % NTABS); return 1;
    }
    return 0;
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
    if (sc >= 0x3B && sc < 0x3B + NTABS) { if (kbd_xpend) xset(0); switch_tab(sc - 0x3B); return; }        /* F1..F4 */
    if (alt && sc >= 0x02 && sc < 0x02 + NTABS) { if (kbd_xpend) xset(0); switch_tab(sc - 0x02); return; }  /* alt+1..4 */
    switch (sc) {
    case 0x48: xflush(); push(K_UP); return;
    case 0x50: xflush(); push(K_DOWN); return;
    case 0x4B: xflush(); push(K_LEFT); return;
    case 0x4D: xflush(); push(K_RIGHT); return;
    case 0x47: xflush(); push(K_HOME); return;
    case 0x4F: xflush(); push(K_END); return;
    case 0x49: xflush(); push(K_PGUP); return;
    case 0x51: xflush(); push(K_PGDN); return;
    case 0x53: xflush(); push(K_DEL); return;
    case 0x0F: if (shift) { xflush(); push(K_BTAB); return; } break;
    }
    if (e && sc == 0x1C) { xflush(); push('\n'); return; }
    if (sc >= sizeof normal - 1) return;
    char c = shift ? shifted[sc] : normal[sc];
    if (!c) return;
    if (caps && c >= 'a' && c <= 'z') c -= 32;
    else if (caps && c >= 'A' && c <= 'Z') c += 32;
    if (ctrl && (c == 'c' || c == 'C' || c == 'g' || c == 'G')) tabs[active_tab].brkflag = 1;
    int k = (u8)c;
    if (ctrl) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) k = CTRL(c);
        else if (c == ' ' || c == '@') k = 0;
        else if (c == '_' || c == '/') k = 31;
        else if (c == '[' || c == ']' || c == '\\') k = CTRL(c);
        if (c == 'h' || c == 'H') k = K_CH;
        else if (c == 'm' || c == 'M') k = '\n';
    }
    if (alt) { xflush(); push(ctrl ? META(k) : META(c)); return; }
    if (kbd_xpend) {
        xset(0);
        if (k == CTRL('g')) return;
        if (k != CTRL('x') && xkey(k)) return;
        push(CTRL('x'));
        push(k);
        return;
    }
    if (k == CTRL('x')) { xset(1); return; }
    push(k);
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
