#include "kbd.h"
#include "mem.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

#define TOP 1
#define ROWS (VH - 2)
#define GUT 5
#define TW (VW - GUT)
#define A_TEXT ATTR(C_TEXT, C_BG)
#define A_DIM ATTR(C_DIM, C_BG)
#define A_BAR ATTR(C_TEXT, C_BAR)

typedef struct { char *s; int len, cap; } line;

struct scrawl_state {
    line *L;
    int nl, capl, cx, cy, rowoff, coloff, dirty, quitarm, cliplen;
    char fname[32], msg[VW], *clip, *saved;
    u32 savedlen;
};
#define E (current->ed)

static void line_reserve(line *l, int n) {
    if (n + 1 <= l->cap) return;
    l->cap = MAX(n + 1, l->cap * 2);
    if (l->cap < 16) l->cap = 16;
    l->s = krealloc(l->s, (u32)l->cap);
}

static void ins_line(int at, const char *s, int len) {
    if (E->nl == E->capl) {
        E->capl = E->capl ? E->capl * 2 : 64;
        E->L = krealloc(E->L, sizeof(line) * (u32)E->capl);
    }
    memmove(&E->L[at + 1], &E->L[at], sizeof(line) * (u32)(E->nl - at));
    E->L[at] = (line){ NULL, 0, 0 };
    line_reserve(&E->L[at], len);
    memcpy(E->L[at].s, s, (size_t)len);
    E->L[at].len = len;
    E->L[at].s[len] = 0;
    E->nl++;
}

static void del_line(int at) {
    kfree(E->L[at].s);
    memmove(&E->L[at], &E->L[at + 1], sizeof(line) * (u32)(E->nl - at - 1));
    E->nl--;
}

static void load(const char *text, u32 len) {
    const char *p = text, *end = text + len;
    E->nl = 0;
    for (;;) {
        const char *q = p;
        while (q < end && *q != '\n') q++;
        ins_line(E->nl, p, (int)(q - p));
        if (q >= end) break;
        p = q + 1;
    }
}

static void save(void) {
    u32 n = 0;
    for (int i = 0; i < E->nl; i++) n += (u32)E->L[i].len + (i < E->nl - 1);
    char *b = kmalloc(n + 1), *p = b;
    for (int i = 0; i < E->nl; i++) {
        memcpy(p, E->L[i].s, (size_t)E->L[i].len);
        p += E->L[i].len;
        if (i < E->nl - 1) *p++ = '\n';
    }
    *p = 0;
    kfree(E->saved);
    E->saved = b;
    E->savedlen = n;
    E->dirty = 0;
    snfmt(E->msg, sizeof E->msg, "saved, %u bytes", n);
}

static void draw(void) {
    char t[VW + 1];
    if (E->cy < E->rowoff) E->rowoff = E->cy;
    if (E->cy >= E->rowoff + ROWS) E->rowoff = E->cy - ROWS + 1;
    if (E->cx < E->coloff) E->coloff = E->cx;
    if (E->cx >= E->coloff + TW) E->coloff = E->cx - TW + 1;

    vga_fill(0, 0, VW, ' ', A_BAR);
    vga_puts_at(1, 0, "scrawl", ATTR(C_MOON, C_BAR));
    vga_puts_at(8, 0, E->fname, A_BAR);
    if (E->dirty) vga_puts_at(9 + (int)strlen(E->fname), 0, "[+]", ATTR(C_PEACH, C_BAR));
    snfmt(t, sizeof t, "ln %d/%d  col %d", E->cy + 1, E->nl, E->cx + 1);
    vga_puts_at(VW - 1 - (int)strlen(t), 0, t, ATTR(C_DIM, C_BAR));

    for (int r = 0; r < ROWS; r++) {
        int y = r + E->rowoff, sy = r + TOP;
        vga_fill(0, sy, VW, ' ', A_TEXT);
        if (y >= E->nl) { vga_put(GUT - 2, sy, '~', A_DIM); continue; }
        snfmt(t, sizeof t, "%4d", y + 1);
        vga_puts_at(0, sy, t, y == E->cy ? ATTR(C_MOON, C_BG) : A_DIM);
        for (int x = 0; x < TW && x + E->coloff < E->L[y].len; x++) vga_put(GUT + x, sy, E->L[y].s[x + E->coloff], A_TEXT);
    }

    vga_fill(0, VH - 1, VW, ' ', A_BAR);
    if (E->msg[0]) vga_puts_at(1, VH - 1, E->msg, ATTR(C_MOON, C_BAR));
    else {
        static const char *const k[][2] = { { "^S", "save" }, { "^Q", "quit" }, { "^F", "find" }, { "^K", "cut" }, { "^U", "paste" }, { "^T/^B", "top/bot" } };
        int x = 1;
        for (int i = 0; i < 6; i++) {
            vga_puts_at(x, VH - 1, k[i][0], ATTR(C_SKY, C_BAR));
            x += (int)strlen(k[i][0]) + 1;
            vga_puts_at(x, VH - 1, k[i][1], A_BAR);
            x += (int)strlen(k[i][1]) + 2;
        }
    }
    vga_cursor(GUT + E->cx - E->coloff, TOP + E->cy - E->rowoff);
}

static int ask(const char *label, char *out, int cap) {
    int n = 0, lx = (int)strlen(label) + 2;
    out[0] = 0;
    for (;;) {
        vga_fill(0, VH - 1, VW, ' ', A_BAR);
        vga_puts_at(1, VH - 1, label, ATTR(C_SKY, C_BAR));
        vga_puts_at(lx, VH - 1, out, A_BAR);
        vga_cursor(lx + n, VH - 1);
        int k = kbd_get();
        if (k == K_ESC) return -1;
        if (k == '\n') return n;
        if (k == '\b' && n) out[--n] = 0;
        else if (k >= ' ' && k <= '~' && n < cap - 1 && lx + n < VW - 2) { out[n++] = (char)k; out[n] = 0; }
    }
}

static void find(void) {
    static char q[48];
    char in[48];
    if (ask("find:", in, sizeof in) < 0) return;
    if (in[0]) scopy(q, in, sizeof q);
    if (!q[0]) return;
    for (int i = 0; i <= E->nl; i++) {
        int y = (E->cy + i) % E->nl, from = i == 0 ? E->cx + 1 : 0;
        if (from > E->L[y].len) continue;
        const char *p = strstr(E->L[y].s + from, q);
        if (p) { E->cy = y; E->cx = (int)(p - E->L[y].s); return; }
    }
    snfmt(E->msg, sizeof E->msg, "not found: %s", q);
}

static void insert_char(char c) {
    line *l = &E->L[E->cy];
    line_reserve(l, l->len + 1);
    memmove(l->s + E->cx + 1, l->s + E->cx, (size_t)(l->len - E->cx + 1));
    l->s[E->cx++] = c;
    l->len++;
    E->dirty = 1;
}

static void newline(void) {
    int ind = 0;
    while (ind < E->L[E->cy].len && ind < E->cx && E->L[E->cy].s[ind] == ' ') ind++;
    ins_line(E->cy + 1, E->L[E->cy].s + E->cx, E->L[E->cy].len - E->cx);
    E->L[E->cy].len = E->cx;
    E->L[E->cy].s[E->cx] = 0;
    E->cy++;
    E->cx = 0;
    for (int i = 0; i < ind; i++) insert_char(' ');
    E->dirty = 1;
}

static void backspace(void) {
    if (E->cx > 0) {
        line *l = &E->L[E->cy];
        memmove(l->s + E->cx - 1, l->s + E->cx, (size_t)(l->len - E->cx + 1));
        l->len--;
        E->cx--;
    } else if (E->cy > 0) {
        line *a = &E->L[E->cy - 1], *b = &E->L[E->cy];
        E->cx = a->len;
        line_reserve(a, a->len + b->len);
        memcpy(a->s + a->len, b->s, (size_t)b->len + 1);
        a->len += b->len;
        del_line(E->cy--);
    } else return;
    E->dirty = 1;
}

static void cut_line(void) {
    kfree(E->clip);
    E->cliplen = E->L[E->cy].len;
    E->clip = kmalloc((u32)E->cliplen + 1);
    memcpy(E->clip, E->L[E->cy].s, (size_t)E->cliplen + 1);
    if (E->nl == 1) { E->L[0].len = 0; E->L[0].s[0] = 0; }
    else { del_line(E->cy); if (E->cy >= E->nl) E->cy = E->nl - 1; }
    E->cx = 0;
    E->dirty = 1;
}

char *scrawl(const char *name, const char *text, u32 len, u32 *outlen) {
    char title[24];
    if (!E) { E = kzalloc(sizeof *E); E->cliplen = -1; }
    snfmt(title, sizeof title, "edit %s", name);
    set_title(title);
    scopy(E->fname, name, sizeof E->fname);
    E->cx = E->cy = E->rowoff = E->coloff = E->dirty = E->quitarm = 0;
    E->saved = NULL;
    E->msg[0] = 0;
    load(text, len);
    for (;;) {
        draw();
        int k = kbd_get();
        if (k != CTRL('q')) E->quitarm = 0;
        E->msg[0] = 0;
        switch (k) {
        case CTRL('q'):
            if (E->dirty && !E->quitarm) { E->quitarm = 1; snfmt(E->msg, sizeof E->msg, "unsaved. ^Q again to discard"); break; }
            for (int i = 0; i < E->nl; i++) kfree(E->L[i].s);
            E->nl = 0;
            vga_clear();
            set_title(SHELL_NAME);
            *outlen = E->savedlen;
            return E->saved;
        case CTRL('s'): save(); break;
        case CTRL('f'): find(); break;
        case CTRL('k'): cut_line(); break;
        case CTRL('u'):
            if (E->cliplen < 0) { snfmt(E->msg, sizeof E->msg, "clipboard empty"); break; }
            ins_line(E->cy, E->clip, E->cliplen);
            E->cx = 0;
            E->dirty = 1;
            break;
        case CTRL('t'): E->cy = E->cx = 0; break;
        case CTRL('b'): E->cy = E->nl - 1; E->cx = E->L[E->cy].len; break;
        case K_UP: if (E->cy > 0) E->cy--; break;
        case K_DOWN: if (E->cy < E->nl - 1) E->cy++; break;
        case K_LEFT: if (E->cx > 0) E->cx--; else if (E->cy > 0) { E->cy--; E->cx = E->L[E->cy].len; } break;
        case K_RIGHT: if (E->cx < E->L[E->cy].len) E->cx++; else if (E->cy < E->nl - 1) { E->cy++; E->cx = 0; } break;
        case K_HOME: E->cx = 0; break;
        case K_END: E->cx = E->L[E->cy].len; break;
        case K_PGUP: E->cy = MAX(0, E->cy - ROWS); break;
        case K_PGDN: E->cy = MIN(E->nl - 1, E->cy + ROWS); break;
        case K_DEL:
            if (E->cx < E->L[E->cy].len) { E->cx++; backspace(); }
            else if (E->cy < E->nl - 1) { E->cy++; E->cx = 0; backspace(); }
            break;
        case '\n': newline(); break;
        case '\b': backspace(); break;
        case '\t': do insert_char(' '); while (E->cx % 4); break;
        default:
            if (k >= ' ' && k <= '~') insert_char((char)k);
        }
        if (E->cx > E->L[E->cy].len) E->cx = E->L[E->cy].len;
    }
}
