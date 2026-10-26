/* help pager */

#include "kbd.h"
#include "line.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

extern const char _binary_build_docfs_bin_start[], _binary_build_docfs_bin_end[];
int help_ngroups(void);
int help_ndocs(void);
void help_group(int i, const char **name, const char **text);
void help_doc(int i, const char **group, const char **name, const char **effect, const char **text, const char **example);

char doc_run[200];

#define MAXLINES 4000
#define MAXLINKS 600
#define MAXATOMS 4096
#define CW 76
#define LX 2
#define VIEW (VH - 2)

enum { LK_PAGE = 1, LK_WORD, LK_RUN };

struct link { u8 kind; char tgt[96]; int line; };
struct atom { u8 ch, attr, brk; u16 lk; };
struct dline { u16 c[VW], lk[VW]; };
struct pos { char name[48]; int top, sel; };

/* pages */

struct page { char name[40]; const char *src; int len; };
static struct page pages[400];
static int npages;
static u8 seen[400];

static void index_pages(void) {
    if (npages) return;
    const char *p = _binary_build_docfs_bin_start, *end = _binary_build_docfs_bin_end;
    while (p < end && npages < 400) {
        const char *eol = p;
        while (eol < end && *eol != '\n') eol++;
        if (eol - p > 4 && !strncmp(p, "#>> ", 4)) {
            struct page *g = &pages[npages++];
            u32 l = (u32)MIN(eol - p - 4, (long)sizeof g->name - 1);
            memcpy(g->name, p + 4, l);
            g->name[l] = 0;
            g->src = eol < end ? eol + 1 : eol;
        }
        p = eol + 1;
    }
    for (int i = 0; i < npages; i++) {
        const char *e = i + 1 < npages ? pages[i + 1].src : end;
        if (i + 1 < npages) { e--; while (e > pages[i].src && e[-1] != '\n') e--; e--; }
        pages[i].len = MAX(0, (int)(e - pages[i].src));
    }
}

static int page_find(const char *n) {
    index_pages();
    for (int i = 0; i < npages; i++) if (!strcmp(pages[i].name, n)) return i;
    return -1;
}

int doc_pages(void) { index_pages(); return npages; }
void doc_page_at(int i, const char **name, const char **src, int *len) { *name = pages[i].name; *src = pages[i].src; *len = pages[i].len; }

/* generated pages */

static struct { char *b; int cap, len; } gb;

static void gput(const char *s) {
    while (*s && gb.len < gb.cap - 1) gb.b[gb.len++] = *s++;
    gb.b[gb.len] = 0;
}

static void gstart(void) {
    if (!gb.b) { gb.cap = 64 * 1024; gb.b = kmalloc((u32)gb.cap); }
    gb.len = 0;
    gb.b[0] = 0;
}

static void word_link(const char *n) {
    gput("[");
    for (; *n; n++) {
        if (*n == '[' || *n == ']' || *n == '\\' || *n == '`' || *n == '*' || *n == '_' || *n == '<' || *n == '|') gput("\\");
        char c[2] = { *n, 0 };
        gput(c);
    }
    gput("](?");
}

static void gen_words(const char *only) {
    gstart();
    int ng = help_ngroups(), nd = help_ndocs();
    if (only) { gput("# "); gput(only); gput("\n\n"); }
    else gput("# Index of words\n\nEvery word the system knows, by group. A name opens its page.\n\n");
    for (int g = 0; g < ng; g++) {
        const char *gn, *gt;
        help_group(g, &gn, &gt);
        if (only && strcmp(only, gn)) continue;
        if (!only) { gput("## "); gput(gn); gput("\n\n"); gput(gt); gput(".\n\n"); }
        for (int i = 0; i < nd; i++) {
            const char *gr, *nm, *ef, *tx, *ex;
            help_doc(i, &gr, &nm, &ef, &tx, &ex);
            if (strcmp(gr, gn)) continue;
            if (only) {
                gput("- ");
                word_link(nm); gput(nm); gput(")  `"); gput(ef); gput("`  "); gput(tx); gput("\n");
            } else { gput("  "); word_link(nm); gput(nm); gput(")"); }
        }
        gput("\n\n");
    }
}

static int cmpw(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

static void gen_atoz(void) {
    gstart();
    gput("# A to Z\n\nEvery documented word in alphabetical order.\n\n");
    int nd = help_ndocs();
    const char **v = kmalloc((u32)nd * sizeof *v);
    for (int i = 0; i < nd; i++) { const char *a, *b, *c, *d; help_doc(i, &a, &v[i], &b, &c, &d); }
    for (int i = 1; i < nd; i++) {
        const char *t = v[i];
        int j = i;
        while (j > 0 && cmpw(&v[j - 1], &t) > 0) { v[j] = v[j - 1]; j--; }
        v[j] = t;
    }
    for (int i = 0; i < nd; i++) { word_link(v[i]); gput(v[i]); gput(")  "); }
    gput("\n");
    kfree(v);
}

static void bind_row(const char *key, const char *doc, void *u) {
    (void)u;
    gput("| `"); gput(key); gput("` | "); gput(doc); gput(" |\n");
}

static void gen_bindings(void) {
    static const struct { const char *ctx, *head; } c[] = {
        { "prompt", "At the prompt" }, { "global", "Everywhere" }, { "pager", "In the pager" }, { "editor", "In the editor" }, { "yours", "Yours" },
    };
    gstart();
    gput("# Key bindings\n\nC- is Control and M- is Alt. See [keys](keys.md) for how to bind your own.\n\n");
    for (int i = 0; i < 5; i++) {
        gput("## "); gput(c[i].head); gput("\n\n| Key | Effect |\n|---|---|\n");
        keys_each(c[i].ctx, bind_row, NULL);
        gput("\n");
    }
}

static int gen_word(const char *n) {
    int nd = help_ndocs();
    for (int i = 0; i < nd; i++) {
        const char *gr, *nm, *ef, *tx, *ex;
        help_doc(i, &gr, &nm, &ef, &tx, &ex);
        if (strcmp(nm, n)) continue;
        gstart();
        gput("# "); gput(nm); gput("\n\n`"); gput(nm); gput(" ( "); gput(ef); gput(" )`\n\n");
        gput(tx); gput("\n\n");
        if (ex[0]) { gput("    "); gput(ex); gput("\n\n"); }
        gput("Group: [");
        gput(gr); gput("](?group:"); gput(gr); gput(")\n");
        return 1;
    }
    return 0;
}

static int gen_group(const char *n) {
    int ng = help_ngroups();
    for (int g = 0; g < ng; g++) {
        const char *gn, *gt;
        help_group(g, &gn, &gt);
        if (!strcmp(gn, n)) { gen_words(gn); return 1; }
    }
    return 0;
}

/* current page */

static struct dline *L;
static int nl, capl;
static struct link links[MAXLINKS];
static int nlinks;
static struct { char name[40]; int line; } anchors[64];
static int nanch;
static struct atom A[MAXATOMS];
static int na;
static char title[56];
static char cur[48];
static int top, sel;
static struct pos hist[16];
static int nhist;
static char msg[48], lastq[40];

static struct dline *new_line(void) {
    if (nl == capl) {
        if (capl >= MAXLINES) return &L[nl - 1];
        capl = capl ? capl * 2 : 256;
        L = krealloc(L, (u64)capl * sizeof *L);
    }
    struct dline *d = &L[nl++];
    for (int i = 0; i < VW; i++) { d->c[i] = ' ' | (u16)ATTR(C_TEXT, C_BG) << 8; d->lk[i] = 0; }
    return d;
}

static void blank(void) { new_line(); }

static int add_link(int kind, const char *t, int n) {
    if (nlinks == MAXLINKS) return 0;
    struct link *k = &links[nlinks];
    k->kind = (u8)kind;
    n = MIN(n, (int)sizeof k->tgt - 1);
    memcpy(k->tgt, t, (size_t)n);
    k->tgt[n] = 0;
    k->line = -1;
    return ++nlinks;
}

static u8 link_attr(int id) {
    struct link *k = &links[id - 1];
    if (k->kind == LK_WORD) return ATTR(C_BLUE, C_BG);
    if (k->kind == LK_RUN) return ATTR(C_TEAL, C_BAR);
    int p = page_find(k->tgt[0] == '#' ? cur : k->tgt);
    return p >= 0 && seen[p] ? ATTR(C_MAUVE, C_BG) : ATTR(C_LRED, C_BG);
}

/* inline markup */

static void atom(char ch, u8 attr, int brk, int lk) {
    if (na < MAXATOMS) A[na++] = (struct atom){ (u8)ch, attr, (u8)brk, (u16)lk };
}

static void inline_text(const char *s, int n, u8 base) {
    int bold = 0, code = 0, lk = 0;
    for (int i = 0; i < n; i++) {
        char c = s[i];
        u8 at = lk ? link_attr(lk) : code ? ATTR(C_TEAL, C_BG) : bold ? ATTR(C_MOON, C_BG) : base;
        if (c == '\\' && i + 1 < n) { atom(s[++i], at, 0, lk); continue; }
        if (c == '`') { code = !code; continue; }
        if (!code) {
            if (c == '*' && i + 1 < n && s[i + 1] == '*') { bold = !bold; i++; continue; }
            if (c == '&') {
                if (!strncmp(s + i, "&lt;", 4)) { atom('<', at, 0, lk); i += 3; continue; }
                if (!strncmp(s + i, "&gt;", 4)) { atom('>', at, 0, lk); i += 3; continue; }
                if (!strncmp(s + i, "&amp;", 5)) { atom('&', at, 0, lk); i += 4; continue; }
                if (!strncmp(s + i, "&#124;", 6)) { atom('|', at, 0, lk); i += 5; continue; }
            }
            if (c == '[' && !lk) {
                int j = i + 1, d = 1;
                while (j < n && d) { if (s[j] == '\\') j++; else if (s[j] == '[') d++; else if (s[j] == ']') d--; j++; }
                if (!d && j < n && s[j] == '(') {
                    int e = j + 1;
                    while (e < n && s[e] != ')') e++;
                    if (e < n) {
                        const char *t = s + j + 1;
                        int tn = e - j - 1, kind = LK_PAGE;
                        char buf[96];
                        int id = 0;
                        if (tn > 0 && t[0] == '?') { kind = LK_WORD; t++; tn--; }
                        else if (tn > 0 && t[0] == '>') { kind = LK_RUN; t++; tn--; }
                        else if (tn >= 4 && !strncmp(t, "http", 4)) kind = 0;
                        if (kind) {
                            int m = MIN(tn, (int)sizeof buf - 1);
                            for (int k = 0; k < m; k++) buf[k] = t[k] == '~' && kind == LK_RUN ? ' ' : t[k];
                            if (kind == LK_PAGE && m > 3 && !strncmp(buf + m - 3, ".md", 3)) m -= 3;
                            id = add_link(kind, buf, m);
                        }
                        int save = na;
                        inline_text(s + i + 1, j - i - 2, base);
                        (void)save;
                        if (id) for (int k = save; k < na; k++) { A[k].lk = (u16)id; A[k].attr = link_attr(id); }
                        i = e;
                        continue;
                    }
                }
            }
            if ((c == '*' || c == '_') && i + 1 < n && s[i + 1] != ' ' && (i == 0 || s[i - 1] == ' ')) {
                int j = i + 1;
                while (j < n && s[j] != c) j++;
                if (j < n && s[j - 1] != ' ') { inline_text(s + i + 1, j - i - 1, base); i = j; continue; }
            }
        }
        atom(c, at, c == ' ' && !code, lk);
    }
}

/* wrap atoms */

static int wrap_end(int a, int b, int width) {
    if (b - a <= width) return b;
    int brk = -1;
    for (int i = a; i <= a + width && i < b; i++) if (A[i].brk) brk = i;
    return brk > a ? brk : a + width;
}

static void put_atoms(struct dline *d, int x, int a, int b) {
    for (int i = a; i < b && x < VW; i++, x++) {
        d->c[x] = A[i].ch | (u16)A[i].attr << 8;
        d->lk[x] = A[i].lk;
        if (A[i].lk && links[A[i].lk - 1].line < 0) links[A[i].lk - 1].line = nl - 1;
    }
}

/* lay out */
static void flow(int indent, const char *first, u8 firstattr, int width) {
    int a = 0;
    while (a < na && A[a].ch == ' ' && A[a].brk) a++;
    int firstline = 1;
    while (a < na) {
        int e = wrap_end(a, na, width - indent);
        struct dline *d = new_line();
        if (firstline && first) for (int i = 0; first[i]; i++) d->c[indent - (int)strlen(first) + i] = (u8)first[i] | (u16)firstattr << 8;
        put_atoms(d, indent, a, e);
        firstline = 0;
        a = e;
        while (a < na && A[a].ch == ' ' && A[a].brk) a++;
    }
    if (firstline) blank();
}

static void add_anchor(const char *s, int n) {
    if (nanch == 64) return;
    int k = 0;
    for (int i = 0; i < n && k < 38; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        if (c == ' ') c = '-';
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') anchors[nanch].name[k++] = c;
    }
    anchors[nanch].name[k] = 0;
    anchors[nanch++].line = nl;
}

/* code links */
static int runnable(const char *s, int n) {
    int i = 0, any = 0;
    while (i < n) {
        while (i < n && (s[i] == ' ' || s[i] == '[' || s[i] == ']')) i++;
        if (i >= n) break;
        if (s[i] == '"') { i++; while (i < n && s[i] != '"') i++; i++; any = 1; continue; }
        int j = i;
        while (j < n && s[j] != ' ' && s[j] != '[' && s[j] != ']') j++;
        char w[48];
        int m = MIN(j - i, 47);
        memcpy(w, s + i, (size_t)m);
        w[m] = 0;
        i = j;
        any = 1;
        const char *p = w;
        if ((*p >= '0' && *p <= '9') || (*p == '-' && p[1] >= '0' && p[1] <= '9')) continue;
        if (*p == '\'' || *p == ':') p++;
        else if (p[0] == '-' && p[1] == '>') p += 2;
        else if (*p == '{' || *p == '}') continue;
        if (!*p) continue;
        str *x = intern(p, (u32)strlen(p));
        if (!x->fn && !lookup(x)) return 0;
    }
    return any;
}

static void code_line(const char *s, int n, int allow) {
    int note = n;
    for (int i = 0; i + 2 < n; i++) if (s[i] == ' ' && s[i + 1] == ' ' && s[i + 2] == ' ') { note = i; break; }
    int id = 0;
    if (allow && note > 0 && runnable(s, note)) id = add_link(LK_RUN, s, note);
    struct dline *d = new_line();
    u8 at = id ? ATTR(C_TEAL, C_BAR) : ATTR(C_TEAL, C_BG);
    for (int i = 0; i < n && LX + 2 + i < VW; i++) {
        d->c[LX + 2 + i] = (u8)s[i] | (u16)(id && i < note ? at : ATTR(C_TEAL, C_BG)) << 8;
        if (id && i < note) d->lk[LX + 2 + i] = (u16)id;
    }
    if (id) links[id - 1].line = nl - 1;
}

/* tables */

#define MAXCOL 8

static int split_row(const char *s, int n, const char **cs, int *cn) {
    int k = 0, i = 0;
    while (i < n && s[i] == ' ') i++;
    if (i < n && s[i] == '|') i++;
    while (i < n && k < MAXCOL) {
        int j = i;
        while (j < n && s[j] != '|') { if (s[j] == '\\') j++; j++; }
        if (j >= n && i >= n) break;
        j = MIN(j, n);
        int a = i, b = j;
        while (a < b && s[a] == ' ') a++;
        while (b > a && s[b - 1] == ' ') b--;
        cs[k] = s + a;
        cn[k++] = b - a;
        i = j + 1;
    }
    return k;
}

static void table(const char **rows, int *rn, int nr) {
    int w[MAXCOL] = { 0 }, nc = 0;
    static struct atom cell[MAXCOL][240];
    static int clen[MAXCOL];
    static int ls[MAXCOL][16], le[MAXCOL][16], nlines[MAXCOL];
    for (int r = 0; r < nr; r++) {
        const char *cs[MAXCOL];
        int cn[MAXCOL], k = split_row(rows[r], rn[r], cs, cn);
        if (r == 1) continue;
        nc = MAX(nc, k);
        for (int c = 0; c < k; c++) {
            na = 0;
            inline_text(cs[c], cn[c], ATTR(C_TEXT, C_BG));
            w[c] = MAX(w[c], na);
        }
    }
    int total = 0;
    for (int c = 0; c < nc; c++) total += w[c] + 2;
    while (total > CW - LX) {
        int m = 0;
        for (int c = 1; c < nc; c++) if (w[c] > w[m]) m = c;
        if (w[m] <= 8) break;
        w[m]--;
        total--;
    }
    for (int r = 0; r < nr; r++) {
        if (r == 1) continue;
        const char *cs[MAXCOL];
        int cn[MAXCOL], k = split_row(rows[r], rn[r], cs, cn), h = 1;
        for (int c = 0; c < nc; c++) {
            na = 0;
            if (c < k) inline_text(cs[c], cn[c], r == 0 ? ATTR(C_MOON, C_BG) : ATTR(C_TEXT, C_BG));
            if (r == 0) for (int i = 0; i < na; i++) if (!A[i].lk) A[i].attr = ATTR(C_MOON, C_BG);
            clen[c] = na;
            memcpy(cell[c], A, (size_t)MIN(na, 240) * sizeof(struct atom));
            if (clen[c] > 240) clen[c] = 240;
            nlines[c] = 0;
            int a = 0;
            memcpy(A, cell[c], (size_t)clen[c] * sizeof(struct atom));
            while (a < clen[c] && nlines[c] < 16) {
                while (a < clen[c] && A[a].ch == ' ' && A[a].brk) a++;
                if (a >= clen[c]) break;
                int e = wrap_end(a, clen[c], w[c]);
                ls[c][nlines[c]] = a;
                le[c][nlines[c]++] = e;
                a = e;
            }
            h = MAX(h, nlines[c]);
        }
        for (int i = 0; i < h; i++) {
            struct dline *d = new_line();
            int x = LX;
            for (int c = 0; c < nc; c++) {
                if (i < nlines[c]) {
                    memcpy(A, cell[c], (size_t)clen[c] * sizeof(struct atom));
                    put_atoms(d, x, ls[c][i], le[c][i]);
                }
                x += w[c] + 2;
            }
        }
        if (r == 0) {
            struct dline *d = new_line();
            int x = LX;
            for (int c = 0; c < nc; c++) {
                for (int i = 0; i < w[c]; i++) d->c[x + i] = 0xC4 | (u16)ATTR(C_DIM, C_BG) << 8;
                x += w[c] + 2;
            }
        }
    }
    blank();
}

/* the page */

static int is_table(const char *s, int n) { int i = 0; while (i < n && s[i] == ' ') i++; return i < n && s[i] == '|'; }
static int indent_of(const char *s, int n) { int i = 0; while (i < n && s[i] == ' ') i++; return i; }
static int is_item(const char *s, int n, int *skip) {
    int i = indent_of(s, n);
    if (i + 1 < n && (s[i] == '-' || s[i] == '*' || s[i] == '+') && s[i + 1] == ' ') { *skip = i + 2; return 1; }
    int j = i;
    while (j < n && s[j] >= '0' && s[j] <= '9') j++;
    if (j > i && j + 1 < n && s[j] == '.' && s[j + 1] == ' ') { *skip = j + 2; return 1; }
    return 0;
}

static void layout(const char *src, int len) {
    nl = nlinks = nanch = 0;
    const char *p = src, *end = src + len;
    int prev_blank = 1;
    while (p < end && nl < MAXLINES - 8) {
        const char *eol = p;
        while (eol < end && *eol != '\n') eol++;
        int n = (int)(eol - p), sk;
        const char *s = p;
        if (n == 0) { if (prev_blank == 2) blank(); prev_blank = 1; p = eol + 1; continue; }
        if (n >= 3 && !strncmp(s, "```", 3)) {
            int allow = strncmp(s + 3, "text", 4) != 0;
            p = eol + 1;
            while (p < end) {
                const char *q = p;
                while (q < end && *q != '\n') q++;
                if ((int)(q - p) >= 3 && !strncmp(p, "```", 3)) { p = q + 1; break; }
                code_line(p, (int)(q - p), allow);
                p = q + 1;
            }
            blank();
            prev_blank = 1;
            continue;
        }
        if (n >= 4 && !strncmp(s, "    ", 4) && prev_blank) {
            while (p < end) {
                const char *q = p;
                while (q < end && *q != '\n') q++;
                int m = (int)(q - p);
                if (m && strncmp(p, "    ", 4)) break;
                if (m == 0) {
                    const char *r = q + 1;
                    if (r >= end || strncmp(r, "    ", 4)) break;
                    new_line();
                } else code_line(p + 4, m - 4, 1);
                p = q + 1;
            }
            blank();
            prev_blank = 1;
            continue;
        }
        if (n >= 2 && s[0] == '#') {
            int h = 0;
            while (h < n && s[h] == '#') h++;
            if (h < n && s[h] == ' ' && h <= 3) {
                const char *t = s + h + 1;
                int tn = n - h - 1;
                if (h == 1) {
                    if (!title[0]) { int m = MIN(tn, 55); memcpy(title, t, (size_t)m); title[m] = 0; }
                    na = 0;
                    inline_text(t, tn, ATTR(C_MAUVE, C_BG));
                    for (int i = 0; i < na; i++) if (!A[i].lk) A[i].attr = ATTR(C_MAUVE, C_BG);
                    struct dline *d = new_line();
                    put_atoms(d, MAX(LX, (VW - na) / 2), 0, MIN(na, CW));
                } else {
                    if (prev_blank == 0 || prev_blank == 2) blank();
                    add_anchor(t, tn);
                    na = 0;
                    inline_text(t, tn, ATTR(h == 2 ? C_MOON : C_PEACH, C_BG));
                    for (int i = 0; i < na; i++) if (!A[i].lk) A[i].attr = ATTR(h == 2 ? C_MOON : C_PEACH, C_BG);
                    flow(LX, NULL, 0, CW);
                }
                blank();
                prev_blank = 1;
                p = eol + 1;
                continue;
            }
        }
        if (is_table(s, n)) {
            const char *rows[64];
            int rn[64], nr = 0;
            while (p < end && nr < 64) {
                const char *q = p;
                while (q < end && *q != '\n') q++;
                if (!is_table(p, (int)(q - p))) break;
                rows[nr] = p;
                rn[nr++] = (int)(q - p);
                p = q + 1;
            }
            table(rows, rn, nr);
            prev_blank = 1;
            continue;
        }
        if (n >= 3 && (s[0] == '-' || s[0] == '*') && s[1] == s[0] && s[2] == s[0]) {
            struct dline *d = new_line();
            for (int i = LX; i < VW - LX; i++) d->c[i] = 0xC4 | (u16)ATTR(C_DIM, C_BG) << 8;
            blank();
            prev_blank = 1;
            p = eol + 1;
            continue;
        }
        if (n == 6 && !strncmp(s, "@pages", 6)) {
            index_pages();
            int ord[400];
            for (int i = 0; i < npages; i++) ord[i] = i;
            static char ti[400][32];
            for (int i = 0; i < npages; i++) {
                const char *q = pages[i].src, *e = q;
                while (e < pages[i].src + pages[i].len && *e != '\n') e++;
                if (e - q > 2 && q[0] == '#' && q[1] == ' ') { q += 2; }
                int m = MIN((int)(e - q), 31);
                memcpy(ti[i], q, (size_t)m);
                ti[i][m] = 0;
            }
            for (int i = 1; i < npages; i++) {
                int t = ord[i], j = i;
                while (j > 0 && strcmp(ti[ord[j - 1]], ti[t]) > 0) { ord[j] = ord[j - 1]; j--; }
                ord[j] = t;
            }
            for (int i = 0; i < npages; i++) {
                int id = ord[i];
                if (!strncmp(pages[id].name, "tour-", 5) || !strcmp(pages[id].name, "index")) continue;
                int lk = add_link(LK_PAGE, pages[id].name, (int)strlen(pages[id].name));
                na = 0;
                for (const char *c = ti[id]; *c; c++) atom(*c, link_attr(lk), c[0] == ' ', lk);
                flow(LX + 2, NULL, 0, CW);
            }
            blank();
            prev_blank = 1;
            p = eol + 1;
            continue;
        }
        if (s[0] == '>' ) {
            int q = 1;
            if (q < n && s[q] == ' ') q++;
            int warn = !strncmp(s + q, "**Warning:**", 12);
            na = 0;
            inline_text(s + q, n - q, ATTR(C_TEXT, C_BG));
            p = eol + 1;
            while (p < end && *p == '>') {
                const char *e2 = p;
                while (e2 < end && *e2 != '\n') e2++;
                int o = 1;
                if (o < e2 - p && p[o] == ' ') o++;
                atom(' ', ATTR(C_TEXT, C_BG), 1, 0);
                inline_text(p + o, (int)(e2 - p) - o, ATTR(C_TEXT, C_BG));
                p = e2 + 1;
            }
            int a0 = nl;
            flow(LX + 2, NULL, 0, CW);
            for (int i = a0; i < nl; i++) L[i].c[LX] = 0xB3 | (u16)ATTR(warn ? C_LRED : C_PEACH, C_BG) << 8;
            blank();
            prev_blank = 1;
            continue;
        }
        if (is_item(s, n, &sk)) {
            int ind = indent_of(s, n);
            int num = s[ind] >= '0' && s[ind] <= '9';
            na = 0;
            inline_text(s + sk, n - sk, ATTR(C_TEXT, C_BG));
            p = eol + 1;
            while (p < end) {
                const char *e2 = p;
                while (e2 < end && *e2 != '\n') e2++;
                int m = (int)(e2 - p), k2;
                if (!m || is_item(p, m, &k2) || indent_of(p, m) < 2 || indent_of(p, m) >= 4 + ind) break;
                atom(' ', ATTR(C_TEXT, C_BG), 1, 0);
                inline_text(p + indent_of(p, m), m - indent_of(p, m), ATTR(C_TEXT, C_BG));
                p = e2 + 1;
            }
            char mark[8] = "-";
            if (num) {
                int j = ind, m = 0;
                while (s[j] != ' ' && m < 6) mark[m++] = s[j++];
                mark[m] = 0;
            } else mark[0] = (char)0xF9;
            int x = LX + ind + (num ? (int)strlen(mark) + 1 : 2);
            flow(x, mark, ATTR(C_DIM, C_BG), CW);
            prev_blank = 2;
            continue;
        }
        /* paragraph */
        na = 0;
        inline_text(s, n, ATTR(C_TEXT, C_BG));
        p = eol + 1;
        while (p < end) {
            const char *e2 = p;
            while (e2 < end && *e2 != '\n') e2++;
            int m = (int)(e2 - p), k2;
            if (!m || *p == '#' || *p == '>' || is_table(p, m) || is_item(p, m, &k2) || !strncmp(p, "```", 3) || !strncmp(p, "@pages", 6)) break;
            atom(' ', ATTR(C_TEXT, C_BG), 1, 0);
            inline_text(p, m, ATTR(C_TEXT, C_BG));
            p = e2 + 1;
        }
        flow(LX, NULL, 0, CW);
        blank();
        prev_blank = 1;
    }
    if (!nl) blank();
}

/* UTF-8 fold */

static int cp_of(u32 c) {
    switch (c) {
    case 0x3C9: return 0x80;
    case 0x2135: return 0x81;
    case 0xBB: return 0xAF;
    case 0xB7: return 0xFA;
    case 0xB2: return 0xFD;
    case 0x3B5: return 0xEE;
    case 0x3C0: return 0xE3;
    case 0x2264: return 0xF3;
    case 0x2265: return 0xF2;
    case 0x221A: return 0xFB;
    case 0xF6: return 0x94;
    case 0x2212: return '-';
    case 0x2019: return '\'';
    case 0x2192: case 0x21A6: return '>';
    case 0x2096: case 0x1D4F: return 'k';
    case 0x2C7C: return 'j';
    case 0x2099: case 0x207F: return 'n';
    case 0x208A: return '+';
    case 0xB3: return '3';
    }
    if (c >= 0x2080 && c <= 0x2089) return 0x82 + (int)(c - 0x2080);
    return '?';
}

static char *to_font(const char *s, int len, int *out) {
    char *b = kmalloc((u32)len + 1);
    int n = 0;
    for (int i = 0; i < len; i++) {
        u8 c = (u8)s[i];
        if (c < 0x80) { b[n++] = (char)c; continue; }
        u32 cp = 0;
        int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
        cp = c & (0x3F >> more);
        while (more-- && i + 1 < len) cp = cp << 6 | ((u8)s[++i] & 0x3F);
        b[n++] = (char)cp_of(cp);
    }
    b[n] = 0;
    *out = n;
    return b;
}

/* loading */

static int load(const char *name) {
    const char *src;
    int len, pg = -1;
    char alt[48];
    if (!strncmp(name, "?group:", 7)) {
        if (!gen_group(name + 7)) return 0;
        src = gb.b; len = gb.len;
    } else if (name[0] == '?') {
        if (!gen_word(name + 1)) return 0;
        src = gb.b; len = gb.len;
    } else if (!strcmp(name, "word-index")) { gen_words(NULL); src = gb.b; len = gb.len; }
    else if (!strcmp(name, "a-to-z")) { gen_atoz(); src = gb.b; len = gb.len; }
    else if (!strcmp(name, "bindings")) { gen_bindings(); src = gb.b; len = gb.len; }
    else {
        scopy(alt, name, sizeof alt);
        pg = page_find(alt);
        if (pg < 0) return 0;
        src = pages[pg].src;
        len = pages[pg].len;
        seen[pg] = 1;
    }
    scopy(cur, name, sizeof cur);
    title[0] = 0;
    int clen;
    char *cv = to_font(src, len, &clen);
    layout(cv, clen);
    kfree(cv);
    if (!title[0]) scopy(title, name, sizeof title);
    top = 0;
    sel = -1;
    return 1;
}

/* drawing */

static void draw(void) {
    u8 bar = ATTR(C_TEXT, C_BAR);
    vga_fill(0, 0, VW, ' ', bar);
    vga_puts_at(1, 0, "doc", ATTR(C_MOON, C_BAR));
    vga_puts_at(5, 0, title, bar);
    vga_puts_at(VW - 21, 0, "q quit  bksp back", ATTR(C_DIM, C_BAR));
    for (int r = 0; r < VIEW; r++) {
        int y = top + r;
        for (int x = 0; x < VW; x++) {
            u16 v = y < nl ? L[y].c[x] : (u16)(' ' | ATTR(C_TEXT, C_BG) << 8);
            if (y < nl && sel >= 0 && L[y].lk[x] == sel + 1) v = (v & 0xFF) | (u16)(links[sel].kind == LK_RUN ? ATTR(C_BG, C_MOON) : ATTR(C_BG, C_PEACH)) << 8;
            vga_put(x, r + 1, (char)(v & 0xFF), (u8)(v >> 8));
        }
    }
    vga_fill(0, VH - 1, VW, ' ', bar);
    if (sel >= 0) {
        const char *t = links[sel].tgt;
        vga_puts_at(1, VH - 1, links[sel].kind == LK_RUN ? "run:" : links[sel].kind == LK_WORD ? "word:" : "page:", ATTR(C_SKY, C_BAR));
        vga_puts_at(links[sel].kind == LK_WORD ? 7 : 6, VH - 1, t, bar);
    } else if (msg[0]) vga_puts_at(1, VH - 1, msg, ATTR(C_MOON, C_BAR));
    else vga_puts_at(1, VH - 1, "tab link  enter follow  C-s search  M-x run  g goto", ATTR(C_DIM, C_BAR));
    char pc[8];
    int pct = nl <= VIEW ? 100 : MIN(100, (top + VIEW) * 100 / nl);
    snfmt(pc, sizeof pc, "%d%%", pct);
    vga_puts_at(VW - 1 - (int)strlen(pc), VH - 1, pc, ATTR(C_MOON, C_BAR));
    vga_cursor(0, VH);
}

static void clamp(void) {
    if (top > nl - VIEW) top = nl - VIEW;
    if (top < 0) top = 0;
}

static void show_sel(void) {
    if (sel < 0) return;
    int ln = links[sel].line;
    if (ln < top || ln >= top + VIEW) { top = ln - VIEW / 3; clamp(); }
}

static void push_hist(void) {
    if (nhist == 16) { memmove(hist, hist + 1, sizeof hist - sizeof *hist); nhist--; }
    scopy(hist[nhist].name, cur, sizeof hist[0].name);
    hist[nhist].top = top;
    hist[nhist++].sel = sel;
}

static void goto_anchor(const char *a) {
    for (int i = 0; i < nanch; i++) if (!strcmp(anchors[i].name, a)) { top = anchors[i].line; clamp(); return; }
}

/* close flag */
static int follow(int id) {
    struct link *k = &links[id];
    if (k->kind == LK_RUN) { scopy(doc_run, k->tgt, sizeof doc_run); return 1; }
    char name[96];
    scopy(name, k->tgt, sizeof name);
    char *h = NULL;
    for (char *c = name; *c; c++) if (*c == '#') { h = c; *c = 0; break; }
    if (k->kind == LK_WORD) {
        char w[100];
        snfmt(w, sizeof w, "?%s", name);
        push_hist();
        if (!load(w)) { nhist--; load(hist[nhist].name); top = hist[nhist].top; sel = hist[nhist].sel; }
        return 0;
    }
    if (!name[0]) { if (h) goto_anchor(h + 1); return 0; }
    push_hist();
    if (!load(name)) { nhist--; load(hist[nhist].name); top = hist[nhist].top; sel = hist[nhist].sel; return 0; }
    if (h) goto_anchor(h + 1);
    return 0;
}

static void back(void) {
    if (!nhist) return;
    struct pos p = hist[--nhist];
    if (load(p.name)) { top = p.top; sel = p.sel; clamp(); }
}

static void step(int dir) {
    if (!nlinks) return;
    int start = sel;
    if (sel < 0) {
        /* next link */
        sel = dir > 0 ? -1 : nlinks;
        for (int i = 0; i < nlinks; i++) if (links[i].line >= top) { sel = dir > 0 ? i - 1 : i; break; }
        if (dir < 0 && sel == nlinks) sel = 0;
        (void)start;
    }
    sel += dir;
    if (sel >= nlinks) sel = 0;
    if (sel < 0) sel = nlinks - 1;
    show_sel();
}

/* search text */
static void line_text(int y, char *o) {
    int n = 0;
    for (int x = 0; x < VW; x++) { char c = (char)(L[y].c[x] & 0xFF); o[n++] = c >= 'A' && c <= 'Z' ? c + 32 : c; }
    o[n] = 0;
}

static int search(const char *q, int dir) {
    char l[VW + 1], t[40];
    int n = 0;
    for (; q[n] && n < 39; n++) t[n] = q[n] >= 'A' && q[n] <= 'Z' ? q[n] + 32 : q[n];
    t[n] = 0;
    if (!n) return 0;
    for (int i = 1; i <= nl; i++) {
        int y = ((top + dir * i) % nl + nl) % nl;
        line_text(y, l);
        if (strstr(l, t)) { top = MAX(0, y - 2); sel = -1; return 1; }
    }
    return 0;
}

static void open_page(const char *name) {
    index_pages();
    nhist = 0;
    if (!load(name) && !load("index")) {
        fail("there is no page called %s, type docs for the index", name);
        return;
    }
    current->mouse_on = 1;
    mouse_flush();
    int done = 0;
    while (!done) {
        draw();
        int k = kbd_get();
        msg[0] = 0;
        switch (k) {
        case 'q': case K_ESC: done = 1; break;
        case K_UP: case CTRL('p'): top--; break;
        case K_DOWN: case CTRL('n'): top++; break;
        case K_PGUP: case META('v'): top -= VIEW - 1; break;
        case K_PGDN: case ' ': case CTRL('v'): top += VIEW - 1; break;
        case K_HOME: case '<': case META('<'): top = 0; sel = -1; break;
        case K_END: case '>': case META('>'): top = nl; break;
        case '\t': case 'n': step(1); break;
        case K_BTAB: case 'p': step(-1); break;
        case K_RIGHT: case K_LEFT: if (nlinks) { if (sel < 0) step(1); else step(k == K_RIGHT ? 1 : -1); } break;
        case '\n': if (sel >= 0) done = follow(sel); break;
        case '\b': case 'l': back(); break;
        case CTRL('g'): sel = -1; break;
        case 't': push_hist(); if (!load("index")) nhist--; break;
        case 'g': {
            char in[48];
            if (minibuf("Go to page: ", in, sizeof in, 0) <= 0) break;
            push_hist();
            if (!load(in)) { nhist--; snfmt(msg, sizeof msg, "no page called %s", in); }
            break;
        }
        case CTRL('s'): case CTRL('r'): {
            char in[40];
            int n = minibuf(k == CTRL('s') ? "Search: " : "Search back: ", in, sizeof in, 0);
            if (n < 0) break;
            if (n) scopy(lastq, in, sizeof lastq);
            if (!search(lastq, k == CTRL('s') ? 1 : -1)) snfmt(msg, sizeof msg, "not found: %s", lastq);
            break;
        }
        case META('x'): {
            if (minibuf("M-x ", doc_run, sizeof doc_run, 1) > 0) done = 1;
            else doc_run[0] = 0;
            break;
        }
        case CTRL('x'): if (kbd_get() == CTRL('c')) done = 1; break;
        case K_CH: if (kbd_get() == 'b') { push_hist(); if (!load("bindings")) nhist--; } break;
        case K_MOUSE: {
            struct mev e;
            while (mouse_event(&e)) {
                if (e.dz) top += e.dz * 3;
                if (e.down & 2) back();
                if ((e.down & 1) && e.y >= 1 && e.y <= VIEW && top + e.y - 1 < nl && e.x >= 0 && e.x < VW) {
                    int id = L[top + e.y - 1].lk[e.x];
                    if (id) { sel = id - 1; if (follow(sel)) done = 1; }
                }
            }
            break;
        }
        }
        clamp();
    }
    current->mouse_on = 0;
    kfree(L);
    L = NULL;
    nl = capl = 0;
    vga_clear();
    set_title(SHELL_NAME);
}

static void b_doc(void) {
    str *s;
    if (!pop_str("doc", &s)) return;
    set_title("docs");
    open_page(s->s);
    set_title(SHELL_NAME);
}

void doc_show(const char *name) {
    set_title("docs");
    open_page(name);
    set_title(SHELL_NAME);
}

static void b_docs(void) {
    set_title("docs");
    open_page("index");
    set_title(SHELL_NAME);
}

const struct builtin doc_words[] = { { "doc", b_doc }, { "docs", b_docs }, { NULL, NULL } };
