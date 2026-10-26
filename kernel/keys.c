#include "kbd.h"
#include "line.h"
#include "omega.h"
#include "task.h"
#include "vga.h"

static const struct { int k; const char *n; } special[] = {
    { K_UP, "<up>" }, { K_DOWN, "<down>" }, { K_LEFT, "<left>" }, { K_RIGHT, "<right>" },
    { K_HOME, "<home>" }, { K_END, "<end>" }, { K_PGUP, "<prior>" }, { K_PGDN, "<next>" },
    { K_DEL, "<delete>" }, { K_BTAB, "<backtab>" }, { K_CH, "C-h" },
    { 9, "TAB" }, { 10, "RET" }, { 27, "ESC" }, { 8, "DEL" }, { ' ', "SPC" }, { 0, "C-SPC" }, { 31, "C-_" },
};

static int base_name(int k, char *o) {
    for (u32 i = 0; i < sizeof special / sizeof *special; i++)
        if (special[i].k == k) { scopy(o, special[i].n, 16); return (int)strlen(o); }
    if (k > 0 && k < 27) { o[0] = 'C'; o[1] = '-'; o[2] = (char)(k + 96); o[3] = 0; return 3; }
    if (k >= 28 && k < 32) { o[0] = 'C'; o[1] = '-'; o[2] = (char)(k + 64); o[3] = 0; return 3; }
    o[0] = (char)k;
    o[1] = 0;
    return 1;
}

void key_name(int k, char *o, int cap) {
    char t[24], b[16];
    int n = 0, meta = (k & K_META) != 0;
    if (meta) k &= 0xFF;
    base_name(k, b);
    if (meta && b[0] == 'C' && b[1] == '-' && b[2] && !b[3]) { t[n++] = 'C'; t[n++] = '-'; t[n++] = 'M'; t[n++] = '-'; t[n++] = b[2]; t[n] = 0; }
    else if (meta) snfmt(t, sizeof t, "M-%s", b);
    else scopy(t, b, sizeof t);
    scopy(o, t, (size_t)cap);
}

/* name to key code */
static int key_parse(const char *t, int len) {
    int ctrl = 0, meta = 0;
    while (len > 2 && t[1] == '-' && (t[0] == 'C' || t[0] == 'M')) {
        if (t[0] == 'C') ctrl = 1; else meta = 1;
        t += 2;
        len -= 2;
    }
    int k = -1;
    for (u32 i = 0; i < sizeof special / sizeof *special; i++)
        if ((int)strlen(special[i].n) == len && !memcmp(special[i].n, t, (size_t)len)) { k = special[i].k; break; }
    if (k < 0 && len == 1) {
        k = (u8)t[0];
        if (ctrl) {
            if (k >= 'a' && k <= 'z') k = CTRL(k);
            else if (k >= 'A' && k <= 'Z') k = CTRL(k);
            else if (k == '@') k = 0;
            else if (k == '_' || k == '/') k = 31;
            else if (k == '[' || k == ']' || k == '\\' || k == '^') k = CTRL(k);
            else return -1;
            ctrl = 0;
        }
    }
    if (k < 0 || ctrl) return -1;
    if (k == K_CH + 0 && meta) return -1;
    if (k == CTRL('m')) k = '\n';
    if (k == CTRL('h')) k = K_CH;
    return meta ? META(k) : k;
}

/* sequence to word name */
static str *word_for(const int *ks, int n) {
    char t[64], c[24];
    int l = snfmt(t, sizeof t, "key:");
    for (int i = 0; i < n; i++) {
        if (i) t[l++] = '_';
        key_name(ks[i], c, sizeof c);
        l += snfmt(t + l, sizeof t - (size_t)l, "%s", c);
    }
    return intern(t, (u32)l);
}

static bind *find(str *n) {
    for (bind *b = W->cur.dict; b; b = b->next) if (b->name == n) return b->v.t == T_NONE ? NULL : b;
    for (bind *b = root_dict; b; b = b->next) if (b->name == n) return b->v.t == T_NONE ? NULL : b;
    return NULL;
}

str *key_bound(const int *ks, int n) {
    str *w = word_for(ks, n);
    return find(w) ? w : NULL;
}

/* parse "C-x C-s" */
static int parse_seq(const char *s, int *ks, int cap, const char *w) {
    int n = 0;
    while (*s) {
        while (*s == ' ') s++;
        if (!*s) break;
        const char *e = s;
        while (*e && *e != ' ') e++;
        int k = key_parse(s, (int)(e - s));
        if (k < 0 || n == cap) { fail("%s: that isn't a key name", w); return 0; }
        ks[n++] = k;
        s = e;
    }
    if (!n) { fail("%s needs a key name", w); return 0; }
    if (n > 2 || (n == 2 && ks[0] != CTRL('x'))) { fail("%s: only C-x starts a sequence", w); return 0; }
    if (ks[0] >= ' ' && ks[0] < 127) { fail("%s: plain keys type text, use C- or M-", w); return 0; }
    if (ks[0] == K_CH || ks[0] == '\n' || ks[0] == K_MOUSE) { fail("%s: that key is reserved", w); return 0; }
    return n;
}

static void b_bind(void) {
    if (!need(2, "bind")) return;
    V q = pop(), k = pop();
    if (k.t != T_STR) { fail("bind wants a key name and a quote%s", ""); return; }
    int ks[2], n = parse_seq(k.p ? ((str *)k.p)->s : "", ks, 2, "bind");
    if (!n) return;
    if (q.t != T_QUOTE) { fail("bind wants a key name and a quote%s", ""); return; }
    define(word_for(ks, n), q);
}

static void b_unbind(void) {
    str *s;
    if (!pop_str("unbind", &s)) return;
    int ks[2], n = parse_seq(s->s, ks, 2, "unbind");
    if (!n) return;
    str *w = word_for(ks, n);
    if (!find(w)) { fail("that key isn't bound%s", ""); return; }
    define(w, (V){ T_NONE, 0, NULL });
}

/* bindings from other screens */
static const struct { const char *ctx, *key, *doc; } screens[] = {
    { "global", "C-x o", "Switch to the next tab." },
    { "global", "M-1 .. M-4", "Switch to tab 1 to 4." },
    { "global", "C-g", "Cancel, and stop a running word." },
    { "pager", "C-n  C-p", "Scroll a line down or up." },
    { "pager", "C-v  M-v", "Scroll a page down or up." },
    { "pager", "M-<  M->", "Go to the top or the bottom." },
    { "pager", "C-s  C-r", "Search for text, forward or back." },
    { "pager", "l", "Go back to the previous page." },
    { "pager", "t", "Go to the index." },
    { "pager", "c", "Close the pager and show the credits." },
    { "pager", "G", "Close the pager and show the gallery." },
    { "pager", "g", "Go to a page by name." },
    { "pager", "M-x", "Close the pager and run a word." },
    { "editor", "C-f  C-b", "Forward or back one character." },
    { "editor", "C-n  C-p", "Down or up one line." },
    { "editor", "C-a  C-e", "Start or end of the line." },
    { "editor", "M-f  M-b", "Forward or back one word." },
    { "editor", "M-<  M->", "Start or end of the text." },
    { "editor", "C-v  M-v", "Page down or up." },
    { "editor", "C-d  DEL", "Delete forward or back." },
    { "editor", "C-k", "Kill to the end of the line." },
    { "editor", "C-SPC  C-w  M-w", "Set the mark, kill the region, copy it." },
    { "editor", "C-y", "Yank what was killed." },
    { "editor", "C-s  C-r", "Search forward or back." },
    { "editor", "M-g", "Go to a line." },
    { "editor", "C-x C-s", "Save." },
    { "editor", "C-x C-c", "Quit." },
};

/* rows for one screen */
void keys_each(const char *ctx, void (*f)(const char *key, const char *doc, void *u), void *u) {
    char n[24], t[64];
    if (!strcmp(ctx, "prompt")) {
        for (int i = 0; i < n_line_keys; i++) {
            int first = 1, l = 0;
            for (int j = 0; j < i; j++) if (!strcmp(line_keys[j].name, line_keys[i].name)) first = 0;
            if (!first) continue;
            for (int j = i; j < n_line_keys; j++) {
                if (strcmp(line_keys[j].name, line_keys[i].name)) continue;
                key_name(line_keys[j].key, n, sizeof n);
                l += snfmt(t + l, sizeof t - (size_t)l, "%s%s", l ? "  " : "", n);
            }
            f(t, line_keys[i].doc, u);
        }
        return;
    }
    if (!strcmp(ctx, "yours")) {
        bind *seen[64];
        int ns = 0;
        for (int pass = 0; pass < 2; pass++)
            for (bind *b = pass ? root_dict : W->dict; b; b = b->next) {
                if (b->name->len < 5 || memcmp(b->name->s, "key:", 4)) continue;
                int dup = 0;
                for (int i = 0; i < ns; i++) if (seen[i]->name == b->name) dup = 1;
                if (dup || ns == 64) continue;
                seen[ns++] = b;
                if (b->v.t == T_NONE) continue;
                int l = 0;
                for (const char *c = b->name->s + 4; *c && l < 38; c++) t[l++] = *c == '_' ? ' ' : *c;
                t[l] = 0;
                f(t, "A binding you made.", u);
            }
        return;
    }
    for (u32 i = 0; i < sizeof screens / sizeof *screens; i++) if (!strcmp(screens[i].ctx, ctx)) f(screens[i].key, screens[i].doc, u);
}

static void b_bindings(void) { doc_show("bindings"); }

/* alternatives in a list */
static int has_key(const char *list, const char *n) {
    int l = (int)strlen(n);
    for (const char *s = list; *s;) {
        const char *e = s;
        while (*e && !(e[0] == ' ' && e[1] == ' ')) e++;
        if ((int)(e - s) == l && !memcmp(s, n, (size_t)l)) return 1;
        while (*e == ' ') e++;
        s = e;
    }
    return 0;
}

/* describe a key */
void keys_describe(int k1, int k2) {
    char n[48], m[24];
    int ks[2] = { k1, k2 }, nk = k2 >= 0 ? 2 : 1, hit = 0;
    key_name(k1, n, sizeof n);
    if (nk == 2) { key_name(k2, m, sizeof m); snfmt(n + strlen(n), sizeof n - strlen(n), " %s", m); }
    vga_color(C_SKY);
    kputs(n);
    vga_color(C_TEXT);
    str *w = key_bound(ks, nk);
    if (w) {
        kputs(" runs your binding ");
        repr(NULL, find(w)->v, 0);
        kputc('\n');
        return;
    }
    if (nk == 1)
        for (int i = 0; i < n_line_keys; i++)
            if (line_keys[i].key == k1) { kprintf(" at the prompt: %s, %s\n", line_keys[i].name, line_keys[i].doc); hit = 1; }
    for (u32 i = 0; i < sizeof screens / sizeof *screens; i++)
        if (has_key(screens[i].key, n)) { kprintf("%s in the %s: %s\n", hit ? "  " : "", screens[i].ctx, screens[i].doc); hit = 1; }
    if (!hit) kputs(k1 >= ' ' && k1 < 127 && nk == 1 ? " types itself.\n" : " isn't bound.\n");
}

static void b_describe_key(void) {
    kputs("Press a key: ");
    int k = kbd_get(), k2 = -1;
    if (k == CTRL('x')) k2 = kbd_get();
    kputc('\n');
    keys_describe(k, k2);
}

/* line editing words */
static struct { char *b; int *len, *pos, room; } ed;

void keys_edit(char *b, int *len, int *pos, int room) {
    ed.b = b;
    ed.len = len;
    ed.pos = pos;
    ed.room = room;
}

static int editing(const char *w) {
    if (!ed.b) { fail("%s only works while a line is being edited", w); return 0; }
    return 1;
}

static void b_line_get(void) {
    if (!editing("line@")) return;
    push(S(str_new(ed.b, (u32)*ed.len)));
}

static void b_line_set(void) {
    str *s;
    if (!editing("line!") || !pop_str("line!", &s)) return;
    int n = MIN((int)s->len, ed.room);
    memcpy(ed.b, s->s, (size_t)n);
    ed.b[n] = 0;
    *ed.len = n;
    *ed.pos = MIN(*ed.pos, n);
}

static void b_point_get(void) {
    if (!editing("point@")) return;
    push(I(*ed.pos));
}

static void b_point_set(void) {
    i64 n;
    if (!editing("point!") || !pop_int("point!", &n)) return;
    *ed.pos = (int)MAX(0, MIN(n, *ed.len));
}

static void b_insert(void) {
    str *s;
    if (!editing("insert") || !pop_str("insert", &s)) return;
    int n = MIN((int)s->len, ed.room - *ed.len);
    if (n <= 0) return;
    memmove(ed.b + *ed.pos + n, ed.b + *ed.pos, (size_t)(*ed.len - *ed.pos + 1));
    memcpy(ed.b + *ed.pos, s->s, (size_t)n);
    *ed.len += n;
    *ed.pos += n;
}

const struct builtin key_words[] = {
    { "bind", b_bind }, { "unbind", b_unbind }, { "bindings", b_bindings }, { "describe-key", b_describe_key },
    { "line@", b_line_get }, { "line!", b_line_set }, { "point@", b_point_get },
    { "point!", b_point_set }, { "insert", b_insert }, { NULL, NULL }
};
