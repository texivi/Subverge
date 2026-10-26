#include "kbd.h"
#include "line.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "vga.h"

void (*line_redraw)(void);

struct snap { char b[LINE_MAX + 1]; int pos; };

struct ed {
    char *b;
    struct history *h;
    int len, pos, room, px, py, at, cyc, cstart, done, ret, wk, wy, typing, ystart, ylen, pend, havepend, nu;
    char typed[64];
    struct snap u[16];
};

typedef void (*cmd)(struct ed *e);

static char kring[8][LINE_MAX + 1];
static int ktop = -1, kcount;
static struct history mbh[NTABS];
static int mbdepth[NTABS];

static void remember(struct history *h, const char *b) {
    if (!*b) return;
    if (h->n && !strcmp(h->h[(h->n - 1) % LINE_HIST], b)) return;
    scopy(h->h[h->n++ % LINE_HIST], b, LINE_MAX + 1);
}

static int stop(char c) { return c == ' ' || c == '[' || c == ']' || c == '{' || c == '}' || c == '"'; }

static void redisplay(struct ed *e) {
    vga_fill(e->px, e->py, VW - e->px, ' ', ATTR(C_TEXT, C_BG));
    vga_puts_at(e->px, e->py, e->b, ATTR(C_TEXT, C_BG));
    vga_cursor(e->px + e->pos, e->py);
}

/* undo snapshot */
static void snap(struct ed *e) {
    if (e->nu == 16) { memmove(e->u, e->u + 1, 15 * sizeof e->u[0]); e->nu--; }
    scopy(e->u[e->nu].b, e->b, LINE_MAX + 1);
    e->u[e->nu++].pos = e->pos;
}

static int ins(struct ed *e, const char *s, int n) {
    n = MIN(n, e->room - e->len);
    if (n <= 0) return 0;
    memmove(e->b + e->pos + n, e->b + e->pos, (size_t)(e->len - e->pos + 1));
    memcpy(e->b + e->pos, s, (size_t)n);
    e->len += n;
    e->pos += n;
    return n;
}

static void del(struct ed *e, int a, int b) {
    if (a >= b) return;
    memmove(e->b + a, e->b + b, (size_t)(e->len - b + 1));
    e->len -= b - a;
    e->pos = e->pos > b ? e->pos - (b - a) : MIN(e->pos, a);
}

static void kill(struct ed *e, int a, int b, int back) {
    if (a >= b) return;
    char *r = kring[ktop < 0 ? 0 : ktop];
    int n = b - a;
    if (e->wk && ktop >= 0) {
        int l = (int)strlen(r);
        n = MIN(n, LINE_MAX - l);
        if (back) { memmove(r + n, r, (size_t)l + 1); memcpy(r, e->b + b - n, (size_t)n); }
        else { memcpy(r + l, e->b + a, (size_t)n); r[l + n] = 0; }
    } else {
        ktop = (ktop + 1) % 8;
        if (kcount < 8) kcount++;
        n = MIN(n, LINE_MAX);
        memcpy(kring[ktop], e->b + a, (size_t)n);
        kring[ktop][n] = 0;
    }
    snap(e);
    del(e, a, b);
    e->wk = 2;
}

static int fwd_word(struct ed *e, int p) {
    while (p < e->len && stop(e->b[p])) p++;
    while (p < e->len && !stop(e->b[p])) p++;
    return p;
}

static int back_word(struct ed *e, int p) {
    while (p > 0 && stop(e->b[p - 1])) p--;
    while (p > 0 && !stop(e->b[p - 1])) p--;
    return p;
}

/* prompt on a fresh line */
static int ask(const char *label, char *out) {
    int id = current->id < 0 ? 0 : current->id;
    struct history *h = &mbh[id];
    h->complete = 1;
    kputc('\n');
    vga_color(C_SKY);
    kputs(label);
    vga_color(C_TEXT);
    mbdepth[id]++;
    int n = readline(out, h);
    mbdepth[id]--;
    return n;
}

/* last-row prompt */
int minibuf(const char *label, char *out, int cap, int complete) {
    char tmp[LINE_MAX + 1];
    int id = current->id < 0 ? 0 : current->id;
    struct history *h = &mbh[id];
    h->complete = complete;
    vga_fill(0, term_h() - 1, VW, ' ', ATTR(C_TEXT, C_BG));
    vga_setpos(0, term_h() - 1);
    vga_color(C_SKY);
    kputs(label);
    vga_color(C_TEXT);
    mbdepth[id]++;
    int n = readline(tmp, h);
    mbdepth[id]--;
    scopy(out, n < 0 ? "" : tmp, (size_t)cap);
    return n < 0 ? n : MIN(n, cap - 1);
}

/* back to the prompt */
static void resume(struct ed *e) {
    int x, y;
    vga_getpos(&x, &y);
    if (x) kputc('\n');
    if (line_redraw) line_redraw();
    vga_getpos(&e->px, &e->py);
    e->room = MIN(LINE_MAX, VW - e->px - 1);
    if (e->len > e->room) { e->len = e->room; e->b[e->len] = 0; }
    e->pos = MIN(e->pos, e->len);
}

static void c_bol(struct ed *e) { e->pos = 0; }
static void c_eol(struct ed *e) { e->pos = e->len; }
static void c_back(struct ed *e) { if (e->pos) e->pos--; }
static void c_fwd(struct ed *e) { if (e->pos < e->len) e->pos++; }
static void c_wback(struct ed *e) { e->pos = back_word(e, e->pos); }
static void c_wfwd(struct ed *e) { e->pos = fwd_word(e, e->pos); }

static void hist_set(struct ed *e, int at) {
    e->at = at;
    scopy(e->b, at == e->h->n ? "" : e->h->h[at % LINE_HIST], (size_t)e->room + 1);
    e->len = e->pos = (int)strlen(e->b);
}

static void c_hprev(struct ed *e) { if (e->at > 0 && e->at > e->h->n - LINE_HIST) { snap(e); hist_set(e, e->at - 1); } }
static void c_hnext(struct ed *e) { if (e->at < e->h->n) { snap(e); hist_set(e, e->at + 1); } }
static void c_hfirst(struct ed *e) { if (e->h->n) { snap(e); hist_set(e, MAX(0, e->h->n - LINE_HIST)); } }
static void c_hlast(struct ed *e) { snap(e); hist_set(e, e->h->n); }

static void c_delete(struct ed *e) { if (e->pos < e->len) { snap(e); del(e, e->pos, e->pos + 1); } }
static void c_backspace(struct ed *e) { if (e->pos) { snap(e); del(e, e->pos - 1, e->pos); } }
static void c_killeol(struct ed *e) { kill(e, e->pos, e->len, 0); }
static void c_killbol(struct ed *e) { kill(e, 0, e->pos, 1); }
static void c_killwback(struct ed *e) { kill(e, back_word(e, e->pos), e->pos, 1); }
static void c_killwfwd(struct ed *e) { kill(e, e->pos, fwd_word(e, e->pos), 0); }

static void c_yank(struct ed *e) {
    if (ktop < 0) return;
    snap(e);
    e->ystart = e->pos;
    e->ylen = ins(e, kring[ktop], (int)strlen(kring[ktop]));
    e->wy = 2;
}

static void c_yankpop(struct ed *e) {
    if (!e->wy || kcount < 2) return;
    snap(e);
    del(e, e->ystart, e->ystart + e->ylen);
    e->pos = e->ystart;
    ktop = (ktop + 7) % 8;
    e->ylen = ins(e, kring[ktop], (int)strlen(kring[ktop]));
    e->wy = 2;
}

static void c_transpose(struct ed *e) {
    if (e->len < 2) return;
    if (e->pos == e->len) e->pos--;
    if (!e->pos) return;
    snap(e);
    char t = e->b[e->pos - 1];
    e->b[e->pos - 1] = e->b[e->pos];
    e->b[e->pos++] = t;
}

static void word_case(struct ed *e, int mode) {
    int s = e->pos;
    while (s < e->len && stop(e->b[s])) s++;
    int f = fwd_word(e, e->pos);
    if (s == f) return;
    snap(e);
    for (int i = s; i < f; i++) {
        char c = e->b[i];
        int up = mode == 0 || (mode == 2 && i == s);
        if (up && c >= 'a' && c <= 'z') c -= 32;
        else if (!up && c >= 'A' && c <= 'Z') c += 32;
        e->b[i] = c;
    }
    e->pos = f;
}

static void c_upcase(struct ed *e) { word_case(e, 0); }
static void c_downcase(struct ed *e) { word_case(e, 1); }
static void c_capitalize(struct ed *e) { word_case(e, 2); }

static void c_undo(struct ed *e) {
    if (!e->nu) return;
    struct snap *s = &e->u[--e->nu];
    scopy(e->b, s->b, LINE_MAX + 1);
    e->len = (int)strlen(e->b);
    e->pos = MIN(s->pos, e->len);
}

static void c_clear(struct ed *e) {
    if (!e->h->shell) return;
    vga_clear();
    resume(e);
}

static void c_enter(struct ed *e) {
    vga_setpos(e->px + e->len, e->py);
    kputc('\n');
    remember(e->h, e->b);
    e->done = 1;
    e->ret = e->len;
}

static void quit_with(struct ed *e, const char *s) {
    vga_setpos(e->px + e->len, e->py);
    vga_color(C_DIM);
    kputs(s);
    vga_color(C_TEXT);
    e->done = 1;
    e->ret = -1;
}

static void c_break(struct ed *e) { quit_with(e, "^C\n"); }
static void c_quit(struct ed *e) { quit_with(e, "^G\n"); }

static void c_complete(struct ed *e) {
    if (!e->h->complete) return;
    char m[64];
    int lcp, cnt;
    if (e->cyc < 0) {
        int s = e->pos;
        while (s > 0 && !stop(e->b[s - 1])) s--;
        if (e->b[s] == '\'' || e->b[s] == ':') s++;
        else if (e->b[s] == '-' && e->b[s + 1] == '>') s += 2;
        if (s >= e->pos || e->pos - s > 62) return;
        memcpy(e->typed, e->b + s, (size_t)(e->pos - s));
        e->typed[e->pos - s] = 0;
        cnt = complete_name(e->typed, e->pos - s, 0, m, sizeof m, &lcp);
        if (!cnt) return;
        e->cstart = s;
        if (cnt == 1) { int l = (int)strlen(m); m[l] = ' '; m[l + 1] = 0; }
        else if (lcp > e->pos - s) m[lcp] = 0;
        else e->cyc = 1 % cnt;
    } else {
        cnt = complete_name(e->typed, (int)strlen(e->typed), e->cyc, m, sizeof m, &lcp);
        if (!cnt) { e->cyc = -1; return; }
        e->cyc = (e->cyc + 1) % cnt;
    }
    int ml = (int)strlen(m), tail = e->len - e->pos;
    if (e->cstart + ml + tail > e->room) { e->cyc = -1; return; }
    snap(e);
    memmove(e->b + e->cstart + ml, e->b + e->pos, (size_t)tail + 1);
    memcpy(e->b + e->cstart, m, (size_t)ml);
    e->len = e->cstart + ml + tail;
    e->pos = e->cstart + ml;
}

/* incremental search */
static int hmatch(struct history *h, int from, int dir, const char *q) {
    int lo = MAX(0, h->n - LINE_HIST);
    for (int i = from; i >= lo && i < h->n; i += dir)
        if (strstr(h->h[i % LINE_HIST], q)) return i;
    return -1;
}

static void isearch(struct ed *e, int dir) {
    char q[48] = "", save[LINE_MAX + 1];
    int ql = 0, found = -1, spos = e->pos, from = dir < 0 ? e->h->n - 1 : MAX(0, e->h->n - LINE_HIST);
    scopy(save, e->b, sizeof save);
    for (;;) {
        char t[LINE_MAX + 64];
        const char *m = found >= 0 ? e->h->h[found % LINE_HIST] : "";
        snfmt(t, sizeof t, "[%s] %s: %s", dir < 0 ? "back" : "forward", q, found < 0 && ql ? "not found" : m);
        vga_fill(e->px, e->py, VW - e->px, ' ', ATTR(C_TEXT, C_BG));
        vga_puts_at(e->px, e->py, t, ATTR(C_SKY, C_BG));
        vga_cursor(e->px + 10 + ql, e->py);
        int k = kbd_get();
        if (k == CTRL('g') || k == CTRL('c')) { scopy(e->b, save, sizeof save); e->len = (int)strlen(e->b); e->pos = MIN(spos, e->len); return; }
        if (k >= ' ' && k <= '~' && ql < 46) {
            q[ql++] = (char)k;
            q[ql] = 0;
            int f = hmatch(e->h, found >= 0 ? found : from, dir, q);
            if (f >= 0) found = f;
            else found = -1;
        } else if (k == '\b') {
            if (ql) q[--ql] = 0;
            found = ql ? hmatch(e->h, from, dir, q) : -1;
        } else if (k == CTRL('r') || k == CTRL('s')) {
            dir = k == CTRL('r') ? -1 : 1;
            int f = hmatch(e->h, (found >= 0 ? found : from) + dir, dir, q);
            if (f >= 0) found = f;
        } else {
            if (found >= 0) {
                scopy(e->b, e->h->h[found % LINE_HIST], (size_t)e->room + 1);
                e->len = (int)strlen(e->b);
                const char *p = strstr(e->b, q);
                e->pos = p ? (int)(p - e->b) + ql : e->len;
                e->at = e->h->n;
            }
            if (k != '\n') { e->pend = k; e->havepend = 1; }
            return;
        }
    }
}

static void c_isearch_back(struct ed *e) { isearch(e, -1); }
static void c_isearch_fwd(struct ed *e) { isearch(e, 1); }

static int eval_ask(struct ed *e, const char *label, const char *fmt) {
    char m[LINE_MAX + 1], c[LINE_MAX + 40];
    if (ask(label, m) <= 0) return 0;
    for (char *p = m; *p; p++) if (*p == '"') return 0;
    snfmt(c, sizeof c, fmt, m);
    eval_line(c);
    (void)e;
    return 1;
}

static void c_mx(struct ed *e) {
    if (!e->h->shell || mbdepth[current->id < 0 ? 0 : current->id]) return;
    char m[LINE_MAX + 1];
    if (ask("M-x ", m) > 0) eval_line(m);
    resume(e);
}

static void c_help(struct ed *e) {
    if (!e->h->shell || mbdepth[current->id < 0 ? 0 : current->id]) return;
    int k = kbd_get();
    if (k == CTRL('g')) return;
    kputc('\n');
    switch (k) {
    case 'k': eval_line("describe-key"); break;
    case 'b': eval_line("bindings"); break;
    case 'i': eval_line("docs"); break;
    case 'f': eval_ask(e, "Describe word: ", "'%s help"); break;
    case 'a': eval_ask(e, "Apropos: ", "\"%s\" apropos"); break;
    default:
        vga_color(C_DIM);
        kputs("C-h k key  f word  a search  b bindings  i docs\n");
        vga_color(C_TEXT);
    }
    resume(e);
}

static void c_prefix(struct ed *e) { (void)e; }

const struct keydoc line_keys[] = {
    { CTRL('a'), "beginning-of-line", "Go to the start of the line." },
    { K_HOME, "beginning-of-line", "Go to the start of the line." },
    { CTRL('e'), "end-of-line", "Go to the end of the line." },
    { K_END, "end-of-line", "Go to the end of the line." },
    { CTRL('b'), "backward-char", "Move back one character." },
    { K_LEFT, "backward-char", "Move back one character." },
    { CTRL('f'), "forward-char", "Move forward one character." },
    { K_RIGHT, "forward-char", "Move forward one character." },
    { META('b'), "backward-word", "Move back one word." },
    { META('f'), "forward-word", "Move forward one word." },
    { CTRL('p'), "previous-history", "Recall the previous line." },
    { K_UP, "previous-history", "Recall the previous line." },
    { CTRL('n'), "next-history", "Recall the next line." },
    { K_DOWN, "next-history", "Recall the next line." },
    { META('<'), "first-history", "Recall the oldest line." },
    { META('>'), "last-history", "Back to the empty line." },
    { CTRL('d'), "delete-char", "Delete the character at the cursor." },
    { K_DEL, "delete-char", "Delete the character at the cursor." },
    { '\b', "delete-backward-char", "Delete the character before the cursor." },
    { CTRL('k'), "kill-line", "Kill to the end of the line." },
    { CTRL('u'), "kill-to-start", "Kill to the start of the line." },
    { CTRL('w'), "backward-kill-word", "Kill the word before the cursor." },
    { META('\b'), "backward-kill-word", "Kill the word before the cursor." },
    { META('d'), "kill-word", "Kill the word after the cursor." },
    { CTRL('y'), "yank", "Insert the last kill." },
    { META('y'), "yank-pop", "Replace the yank with an older kill." },
    { CTRL('t'), "transpose-chars", "Swap the two characters at the cursor." },
    { META('u'), "upcase-word", "Make the next word upper case." },
    { META('l'), "downcase-word", "Make the next word lower case." },
    { META('c'), "capitalize-word", "Capitalize the next word." },
    { CTRL('_'), "undo", "Undo the last change to the line." },
    { CTRL('l'), "clear-screen", "Clear the screen and redraw the prompt." },
    { '\t', "complete", "Complete a word, press again for the next." },
    { '\n', "accept-line", "Run the line." },
    { CTRL('c'), "interrupt", "Throw the line away." },
    { CTRL('g'), "keyboard-quit", "Throw the line away." },
    { CTRL('r'), "isearch-backward", "Search the history, newest first." },
    { CTRL('s'), "isearch-forward", "Search the history, oldest first." },
    { META('x'), "execute-extended-command", "Run a word by name, with completion." },
    { K_CH, "help-command", "Help: k key, f word, a search, b bindings, i docs." },
    { CTRL('x'), "prefix", "Starts a key sequence such as C-x o." },
};
const int n_line_keys = sizeof line_keys / sizeof *line_keys;

static cmd command_for(int i) {
    const char *n = line_keys[i].name;
    static const struct { const char *n; cmd f; } t[] = {
        { "beginning-of-line", c_bol }, { "end-of-line", c_eol }, { "backward-char", c_back }, { "forward-char", c_fwd },
        { "backward-word", c_wback }, { "forward-word", c_wfwd }, { "previous-history", c_hprev }, { "next-history", c_hnext },
        { "first-history", c_hfirst }, { "last-history", c_hlast }, { "delete-char", c_delete }, { "delete-backward-char", c_backspace },
        { "kill-line", c_killeol }, { "kill-to-start", c_killbol }, { "backward-kill-word", c_killwback }, { "kill-word", c_killwfwd },
        { "yank", c_yank }, { "yank-pop", c_yankpop }, { "transpose-chars", c_transpose }, { "upcase-word", c_upcase },
        { "downcase-word", c_downcase }, { "capitalize-word", c_capitalize }, { "undo", c_undo }, { "clear-screen", c_clear },
        { "complete", c_complete }, { "accept-line", c_enter }, { "interrupt", c_break }, { "keyboard-quit", c_quit },
        { "isearch-backward", c_isearch_back }, { "isearch-forward", c_isearch_fwd }, { "execute-extended-command", c_mx },
        { "help-command", c_help }, { "prefix", c_prefix },
    };
    for (u32 j = 0; j < sizeof t / sizeof *t; j++) if (!strcmp(t[j].n, n)) return t[j].f;
    return NULL;
}

/* your bindings first */
static int user_key(struct ed *e, int k) {
    if (!e->h->shell || !W) return 0;
    int ks[2] = { k, 0 }, n = 1;
    if (k == CTRL('x')) {
        ks[1] = kbd_get();
        n = 2;
        if (ks[1] == CTRL('g')) return 1;
    }
    str *w = key_bound(ks, n);
    if (!w) return n == 2;
    keys_edit(e->b, &e->len, &e->pos, e->room);
    int x0 = e->px, y0 = e->py, x, y;
    snap(e);
    key_run(w);
    keys_edit(NULL, NULL, NULL, 0);
    e->len = (int)strlen(e->b);
    e->pos = MIN(e->pos, e->len);
    vga_getpos(&x, &y);
    if (x != x0 || y != y0) resume(e);
    return 1;
}

int readline(char *b, struct history *h) {
    struct ed *e = kmalloc(sizeof *e);
    if (!e) return -1;
    memset(e, 0, sizeof *e);
    e->b = b;
    e->h = h;
    e->at = h->n;
    e->cyc = -1;
    b[0] = 0;
    vga_getpos(&e->px, &e->py);
    e->room = MIN(LINE_MAX, VW - e->px - 1);
    while (!e->done) {
        vga_getpos(&e->px, &e->py);
        redisplay(e);
        int k = e->havepend ? (e->havepend = 0, e->pend) : kbd_get();
        int pk = e->wk == 2, py = e->wy == 2, pt = e->typing;
        e->wk = pk;
        e->wy = py;
        e->typing = 0;
        if (user_key(e, k)) { e->wk = e->wy = 0; continue; }
        cmd f = NULL;
        for (int i = 0; i < n_line_keys; i++) if (line_keys[i].key == k) { f = command_for(i); break; }
        if (f != c_complete) e->cyc = -1;
        if (f) f(e);
        else if (k >= ' ' && k <= '~') {
            char c = (char)k;
            if (!pt) snap(e);
            ins(e, &c, 1);
            e->typing = 1;
        }
        if (e->wk == 1) e->wk = 0;
        if (e->wy == 1) e->wy = 0;
    }
    int r = e->ret;
    kfree(e);
    return r;
}
