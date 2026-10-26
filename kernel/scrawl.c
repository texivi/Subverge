#include "kbd.h"
#include "line.h"
#include "mem.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

#define TOP 1
#define ROWS (term_h() - 2)
#define GUT 5
#define TW (term_w() - GUT)
#define A_TEXT ATTR(C_TEXT, C_BG)
#define A_DIM ATTR(C_DIM, C_BG)
#define A_BAR ATTR(C_TEXT, C_BAR)

typedef struct { char *s; int len, cap; } line;

struct scrawl_state {
    line *L;
    int nl, capl, cx, cy, rowoff, coloff, dirty, quitarm, cliplen, my, mx, wk;
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
    vga_puts_at(term_w() - 1 - (int)strlen(t), 0, t, ATTR(C_DIM, C_BAR));

    for (int r = 0; r < ROWS; r++) {
        int y = r + E->rowoff, sy = r + TOP;
        vga_fill(0, sy, VW, ' ', A_TEXT);
        if (y >= E->nl) { vga_put(GUT - 2, sy, '~', A_DIM); continue; }
        snfmt(t, sizeof t, "%4d", y + 1);
        vga_puts_at(0, sy, t, y == E->cy ? ATTR(C_MOON, C_BG) : A_DIM);
        for (int x = 0; x < TW && x + E->coloff < E->L[y].len; x++) vga_put(GUT + x, sy, E->L[y].s[x + E->coloff], A_TEXT);
    }

    vga_fill(0, term_h() - 1, VW, ' ', A_BAR);
    if (E->msg[0]) vga_puts_at(1, term_h() - 1, E->msg, ATTR(C_MOON, C_BAR));
    else {
        static const char *const k[][2] = { { "C-x C-s", "save" }, { "C-x C-c", "quit" }, { "C-s", "search" }, { "C-k", "kill" }, { "C-SPC", "mark" }, { "M-g", "goto" } };
        int x = 1;
        for (int i = 0; i < 6; i++) {
            vga_puts_at(x, term_h() - 1, k[i][0], ATTR(C_SKY, C_BAR));
            x += (int)strlen(k[i][0]) + 1;
            vga_puts_at(x, term_h() - 1, k[i][1], A_BAR);
            x += (int)strlen(k[i][1]) + 2;
        }
    }
    vga_cursor(GUT + E->cx - E->coloff, TOP + E->cy - E->rowoff);
}

static int ask(const char *label, char *out, int cap) {
    int n = 0, lx = (int)strlen(label) + 2;
    out[0] = 0;
    for (;;) {
        vga_fill(0, term_h() - 1, VW, ' ', A_BAR);
        vga_puts_at(1, term_h() - 1, label, ATTR(C_SKY, C_BAR));
        vga_puts_at(lx, term_h() - 1, out, A_BAR);
        vga_cursor(lx + n, term_h() - 1);
        int k = kbd_get();
        if (k == K_ESC || k == CTRL('g')) return -1;
        if (k == '\n') return n;
        if (k == '\b' && n) out[--n] = 0;
        else if (k >= ' ' && k <= '~' && n < cap - 1 && lx + n < term_w() - 2) { out[n++] = (char)k; out[n] = 0; }
    }
}

static int word_ch(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static void find(int dir) {
    static char q[48];
    char in[48];
    if (ask(dir > 0 ? "search:" : "search back:", in, sizeof in) < 0) return;
    if (in[0]) scopy(q, in, sizeof q);
    if (!q[0]) return;
    int ql = (int)strlen(q);
    for (int i = 0; i <= E->nl; i++) {
        int y = ((E->cy + dir * i) % E->nl + E->nl) % E->nl;
        line *l = &E->L[y];
        if (dir > 0) {
            int from = i == 0 ? E->cx + 1 : 0;
            if (from > l->len) continue;
            const char *p = strstr(l->s + from, q);
            if (p) { E->cy = y; E->cx = (int)(p - l->s); return; }
        } else {
            int last = i == 0 ? E->cx - 1 : l->len;
            for (int x = MIN(last, l->len - ql); x >= 0; x--)
                if (!strncmp(l->s + x, q, (size_t)ql)) { E->cy = y; E->cx = x; return; }
        }
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

static void split_line(void) {
    ins_line(E->cy + 1, E->L[E->cy].s + E->cx, E->L[E->cy].len - E->cx);
    E->L[E->cy].len = E->cx;
    E->L[E->cy].s[E->cx] = 0;
    E->cy++;
    E->cx = 0;
    E->dirty = 1;
}

static void newline(void) {
    int ind = 0;
    while (ind < E->L[E->cy].len && ind < E->cx && E->L[E->cy].s[ind] == ' ') ind++;
    split_line();
    for (int i = 0; i < ind; i++) insert_char(' ');
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

static void delete_forward(void) {
    if (E->cx < E->L[E->cy].len) { E->cx++; backspace(); }
    else if (E->cy < E->nl - 1) { E->cy++; E->cx = 0; backspace(); }
}

/* range to text */
static char *range_text(int y0, int x0, int y1, int x1, int *n) {
    int total = 0;
    for (int y = y0; y <= y1; y++) total += E->L[y].len + 1;
    char *b = kmalloc((u32)total + 1), *p = b;
    for (int y = y0; y <= y1; y++) {
        int a = y == y0 ? x0 : 0, e = y == y1 ? x1 : E->L[y].len;
        memcpy(p, E->L[y].s + a, (size_t)(e - a));
        p += e - a;
        if (y < y1) *p++ = '\n';
    }
    *p = 0;
    *n = (int)(p - b);
    return b;
}

static void delete_range(int y0, int x0, int y1, int x1) {
    line *a = &E->L[y0], *b = &E->L[y1];
    int tail = b->len - x1;
    line_reserve(a, x0 + tail);
    memmove(a->s + x0, b->s + x1, (size_t)tail + 1);
    a->len = x0 + tail;
    while (E->nl > 1 && y1 > y0) { del_line(y0 + 1); y1--; }
    E->cy = y0;
    E->cx = x0;
    E->dirty = 1;
}

static void clip_put(const char *s, int n, int append, int front) {
    if (append && E->clip) {
        char *c = kmalloc((u32)(E->cliplen + n) + 1);
        if (front) { memcpy(c, s, (size_t)n); memcpy(c + n, E->clip, (size_t)E->cliplen); }
        else { memcpy(c, E->clip, (size_t)E->cliplen); memcpy(c + E->cliplen, s, (size_t)n); }
        kfree(E->clip);
        E->clip = c;
        E->cliplen += n;
    } else {
        kfree(E->clip);
        E->clip = kmalloc((u32)n + 1);
        memcpy(E->clip, s, (size_t)n);
        E->cliplen = n;
    }
    E->clip[E->cliplen] = 0;
}

/* kill a range */
static void kill_range(int y0, int x0, int y1, int x1, int front) {
    if (y0 == y1 && x0 >= x1) return;
    int n;
    char *t = range_text(y0, x0, y1, x1, &n);
    clip_put(t, n, E->wk, front);
    kfree(t);
    delete_range(y0, x0, y1, x1);
    E->wk = 2;
}

static void yank(void) {
    if (E->cliplen <= 0) { snfmt(E->msg, sizeof E->msg, "nothing killed yet"); return; }
    for (int i = 0; i < E->cliplen; i++) {
        if (E->clip[i] == '\n') split_line();
        else insert_char(E->clip[i]);
    }
}

/* mark to point */
static int region(int *y0, int *x0, int *y1, int *x1) {
    if (E->my < 0 || E->my >= E->nl) { snfmt(E->msg, sizeof E->msg, "no mark set"); return 0; }
    int my = E->my, mx = MIN(E->mx, E->L[my].len);
    if (my < E->cy || (my == E->cy && mx <= E->cx)) { *y0 = my; *x0 = mx; *y1 = E->cy; *x1 = E->cx; }
    else { *y0 = E->cy; *x0 = E->cx; *y1 = my; *x1 = mx; }
    return 1;
}

static void word_fwd(void) {
    for (;;) {
        line *l = &E->L[E->cy];
        while (E->cx < l->len && !word_ch(l->s[E->cx])) E->cx++;
        if (E->cx < l->len) break;
        if (E->cy == E->nl - 1) return;
        E->cy++;
        E->cx = 0;
    }
    line *l = &E->L[E->cy];
    while (E->cx < l->len && word_ch(l->s[E->cx])) E->cx++;
}

static void word_back(void) {
    for (;;) {
        line *l = &E->L[E->cy];
        while (E->cx > 0 && !word_ch(l->s[E->cx - 1])) E->cx--;
        if (E->cx > 0) break;
        if (E->cy == 0) return;
        E->cy--;
        E->cx = E->L[E->cy].len;
    }
    line *l = &E->L[E->cy];
    while (E->cx > 0 && word_ch(l->s[E->cx - 1])) E->cx--;
}

static int quit_ok(void) {
    if (E->dirty && !E->quitarm) { E->quitarm = 1; snfmt(E->msg, sizeof E->msg, "unsaved. quit again to discard"); return 0; }
    return 1;
}

static char *leave(u32 *outlen) {
    for (int i = 0; i < E->nl; i++) kfree(E->L[i].s);
    E->nl = 0;
    vga_clear();
    set_title(SHELL_NAME);
    *outlen = E->savedlen;
    return E->saved;
}

char *scrawl(const char *name, const char *text, u32 len, u32 *outlen) {
    char title[24];
    if (!E) { E = kzalloc(sizeof *E); E->cliplen = -1; }
    snfmt(title, sizeof title, "edit %s", name);
    set_title(title);
    scopy(E->fname, name, sizeof E->fname);
    E->cx = E->cy = E->rowoff = E->coloff = E->dirty = E->quitarm = E->wk = 0;
    E->my = -1;
    E->saved = NULL;
    E->msg[0] = 0;
    load(text, len);
    for (;;) {
        draw();
        int k = kbd_get();
        int quitkey = k == CTRL('q') || k == CTRL('x');
        if (!quitkey) E->quitarm = 0;
        E->msg[0] = 0;
        E->wk = E->wk == 2 ? 1 : 0;
        switch (k) {
        case CTRL('q'):
            if (quit_ok()) return leave(outlen);
            break;
        case CTRL('x'): {
            int k2 = kbd_get();
            if (k2 == CTRL('s')) save();
            else if (k2 == CTRL('c')) { if (quit_ok()) return leave(outlen); }
            else if (k2 == CTRL('x') && E->my >= 0) {
                int y = E->cy, x = E->cx;
                E->cy = MIN(E->my, E->nl - 1);
                E->cx = MIN(E->mx, E->L[E->cy].len);
                E->my = y;
                E->mx = x;
            } else E->quitarm = 0;
            break;
        }
        case CTRL('s'): find(1); break;
        case CTRL('r'): find(-1); break;
        case CTRL('g'):
            if (E->my >= 0) { E->my = -1; snfmt(E->msg, sizeof E->msg, "mark cleared"); }
            else snfmt(E->msg, sizeof E->msg, "quit");
            break;
        case 0: E->my = E->cy; E->mx = E->cx; snfmt(E->msg, sizeof E->msg, "mark set"); break;
        case CTRL('k'):
            if (E->cx < E->L[E->cy].len) kill_range(E->cy, E->cx, E->cy, E->L[E->cy].len, 0);
            else if (E->cy < E->nl - 1) kill_range(E->cy, E->cx, E->cy + 1, 0, 0);
            break;
        case META('d'): {
            int y = E->cy, x = E->cx;
            word_fwd();
            int y2 = E->cy, x2 = E->cx;
            E->cy = y;
            E->cx = x;
            kill_range(y, x, y2, x2, 0);
            break;
        }
        case META('\b'): {
            int y = E->cy, x = E->cx;
            word_back();
            int y2 = E->cy, x2 = E->cx;
            kill_range(y2, x2, y, x, 1);
            break;
        }
        case CTRL('w'): case META('w'): {
            int y0, x0, y1, x1;
            if (!region(&y0, &x0, &y1, &x1)) break;
            if (k == CTRL('w')) kill_range(y0, x0, y1, x1, 0);
            else {
                int n;
                char *t = range_text(y0, x0, y1, x1, &n);
                clip_put(t, n, 0, 0);
                kfree(t);
                snfmt(E->msg, sizeof E->msg, "copied %d bytes", n);
            }
            E->my = -1;
            break;
        }
        case CTRL('y'): yank(); break;
        case CTRL('o'): { int y = E->cy, x = E->cx; split_line(); E->cy = y; E->cx = x; break; }
        case CTRL('t'):
            if (E->cx > 0 && E->L[E->cy].len > 1) {
                line *l = &E->L[E->cy];
                if (E->cx == l->len) E->cx--;
                char t = l->s[E->cx - 1];
                l->s[E->cx - 1] = l->s[E->cx];
                l->s[E->cx++] = t;
                E->dirty = 1;
            }
            break;
        case META('g'): {
            char in[8];
            if (ask("go to line:", in, sizeof in) <= 0) break;
            int n = 0;
            for (char *p = in; *p >= '0' && *p <= '9'; p++) n = n * 10 + (*p - '0');
            E->cy = MAX(0, MIN(E->nl - 1, n - 1));
            E->cx = 0;
            break;
        }
        case META('<'): E->cy = E->cx = 0; break;
        case META('>'): E->cy = E->nl - 1; E->cx = E->L[E->cy].len; break;
        case CTRL('l'): E->rowoff = MAX(0, E->cy - ROWS / 2); break;
        case META('f'): word_fwd(); break;
        case META('b'): word_back(); break;
        case CTRL('p'): case K_UP: if (E->cy > 0) E->cy--; break;
        case CTRL('n'): case K_DOWN: if (E->cy < E->nl - 1) E->cy++; break;
        case CTRL('b'): case K_LEFT: if (E->cx > 0) E->cx--; else if (E->cy > 0) { E->cy--; E->cx = E->L[E->cy].len; } break;
        case CTRL('f'): case K_RIGHT: if (E->cx < E->L[E->cy].len) E->cx++; else if (E->cy < E->nl - 1) { E->cy++; E->cx = 0; } break;
        case CTRL('a'): case K_HOME: E->cx = 0; break;
        case CTRL('e'): case K_END: E->cx = E->L[E->cy].len; break;
        case META('v'): case K_PGUP: E->cy = MAX(0, E->cy - ROWS); break;
        case CTRL('v'): case K_PGDN: E->cy = MIN(E->nl - 1, E->cy + ROWS); break;
        case CTRL('d'): case K_DEL: delete_forward(); break;
        case '\n': newline(); break;
        case '\b': backspace(); break;
        case '\t': do insert_char(' '); while (E->cx % 4); break;
        default:
            if (k >= ' ' && k <= '~') insert_char((char)k);
        }
        if (E->cx > E->L[E->cy].len) E->cx = E->L[E->cy].len;
    }
}
