/* phi sets */

#include "disk.h"
#include "kbd.h"
#include "line.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

int doc_pages(void);
void doc_page_at(int i, const char **name, const char **src, int *len);
int pic_at(int i, const char **name, u32 *nlen, const u8 **data, u32 *dlen);

#define NAMEMAX 31
#define DEPTH_MAX 16
#define STORE_MAX (480 * 1024)

struct node { struct node *next, *kid; char name[NAMEMAX + 1]; u8 set, ro, owned; u32 len; const char *data; };

static struct node root = { .set = 1, .ro = 0 };
static u32 stored;
static int loading;

static void save(void) { if (!loading) persist_save_phi(); }

/* path to names */
static int split(const char *path, char c[][NAMEMAX + 1]) {
    int n = 0;
    while (*path) {
        while (*path == '/') path++;
        if (!*path) break;
        const char *e = path;
        while (*e && *e != '/') e++;
        int l = (int)(e - path);
        if (l == 1 && path[0] == '.') { path = e; continue; }
        if (l == 2 && path[0] == '.' && path[1] == '.') { if (n) n--; path = e; continue; }
        if (l > NAMEMAX || n == DEPTH_MAX) return -1;
        for (int i = 0; i < l; i++) if ((u8)path[i] < ' ' || (u8)path[i] > '~') return -1;
        memcpy(c[n], path, (size_t)l);
        c[n++][l] = 0;
        path = e;
    }
    return n;
}

static struct node *child(struct node *n, const char *name) {
    for (struct node *k = n->kid; k; k = k->next) if (!strcmp(k->name, name)) return k;
    return NULL;
}

static void link_in(struct node *p, struct node *n) {
    struct node **at = &p->kid;
    while (*at && strcmp((*at)->name, n->name) < 0) at = &(*at)->next;
    n->next = *at;
    *at = n;
}

static struct node *make(struct node *p, const char *name, int set) {
    struct node *n = kzalloc(sizeof *n);
    scopy(n->name, name, sizeof n->name);
    n->set = (u8)set;
    link_in(p, n);
    return n;
}

static struct node *find_path(const char *path, struct node **parent) {
    char c[DEPTH_MAX][NAMEMAX + 1];
    int n = split(path, c);
    struct node *p = NULL, *k = &root;
    if (n < 0) return NULL;
    for (int i = 0; i < n; i++) {
        if (!k->set) return NULL;
        p = k;
        k = child(k, c[i]);
        if (!k) return NULL;
    }
    if (parent) *parent = p;
    return k;
}

/* creates parents */
static struct node *walk_create(const char *path, int set, const char **why) {
    char c[DEPTH_MAX][NAMEMAX + 1];
    int n = split(path, c);
    if (n <= 0) { *why = n < 0 ? "that path has a bad name in it" : "that is the top set"; return NULL; }
    struct node *k = &root;
    for (int i = 0; i < n; i++) {
        struct node *x = child(k, c[i]);
        if (i == n - 1) {
            if (x) { *why = "that name is already taken"; return NULL; }
            if (k->ro) { *why = "that set is read only"; return NULL; }
            return make(k, c[i], set);
        }
        if (!x) {
            if (k->ro) { *why = "that set is read only"; return NULL; }
            x = make(k, c[i], 1);
        } else if (!x->set) { *why = "a value is in the way"; return NULL; }
        k = x;
    }
    return NULL;
}

static void drop(struct node *n) {
    for (struct node *k = n->kid; k;) { struct node *nx = k->next; drop(k); k = nx; }
    if (n->owned) { stored -= n->len; kfree((void *)n->data); }
    kfree(n);
}

int phi_write(const char *path, const char *data, u32 len, const char **why) {
    struct node *n = find_path(path, NULL);
    if (n && (n->set || n->ro)) { *why = n->set ? "that is a set, not a value" : "that value is read only"; return 0; }
    u32 have = n ? n->len : 0;
    if (stored - have + len > STORE_MAX) { *why = "phi is full"; return 0; }
    if (!n && !(n = walk_create(path, 0, why))) return 0;
    char *d = kmalloc(len + 1);
    memcpy(d, data, len);
    d[len] = 0;
    if (n->owned) { stored -= n->len; kfree((void *)n->data); }
    n->data = d;
    n->len = len;
    n->owned = 1;
    stored += len;
    save();
    return 1;
}

int phi_mkset(const char *path, const char **why) {
    struct node *n = walk_create(path, 1, why);
    if (n) save();
    return n != NULL;
}

int phi_delete(const char *path, const char **why) {
    struct node *p, *n = find_path(path, &p);
    if (!n || !p) { *why = n ? "that is the top set" : "there is nothing there"; return 0; }
    if (n->ro || p->ro) { *why = "that is read only"; return 0; }
    for (struct node **at = &p->kid; *at; at = &(*at)->next)
        if (*at == n) { *at = n->next; break; }
    drop(n);
    save();
    return 1;
}

int phi_move(const char *from, const char *to, const char **why) {
    struct node *p, *n = find_path(from, &p);
    if (!n || !p) { *why = "there is nothing there"; return 0; }
    if (n->ro || p->ro) { *why = "that is read only"; return 0; }
    char c[DEPTH_MAX][NAMEMAX + 1];
    int k = split(to, c);
    if (k <= 0) { *why = "that path has a bad name in it"; return 0; }
    struct node *np = &root;
    for (int i = 0; i < k - 1; i++) {
        np = child(np, c[i]);
        if (!np || !np->set) { *why = "the target set doesn't exist"; return 0; }
    }
    if (np->ro) { *why = "that set is read only"; return 0; }
    if (child(np, c[k - 1])) { *why = "that name is already taken"; return 0; }
    for (struct node *a = np; a; a = NULL) if (a == n) { *why = "a set can't go inside itself"; return 0; }
    for (struct node **at = &p->kid; *at; at = &(*at)->next)
        if (*at == n) { *at = n->next; break; }
    scopy(n->name, c[k - 1], sizeof n->name);
    link_in(np, n);
    save();
    return 1;
}

/* save format */
static int dump_node(struct node *n, char *path, int pl, char *buf, u32 *o, u32 cap) {
    for (struct node *k = n->kid; k; k = k->next) {
        if (k->ro) continue;
        int l = pl + snfmt(path + pl, 256 - (size_t)pl, "/%s", k->name);
        if (k->set) {
            if (*o + (u32)l + 4 > cap) return 0;
            *o += (u32)snfmt(buf + *o, cap - *o, "S %s\n", path);
            if (!dump_node(k, path, l, buf, o, cap)) return 0;
        } else {
            if (*o + (u32)l + 24 + k->len > cap) return 0;
            *o += (u32)snfmt(buf + *o, cap - *o, "A %s %u\n", path, k->len);
            memcpy(buf + *o, k->data, k->len);
            *o += k->len;
            buf[(*o)++] = '\n';
        }
        path[pl] = 0;
    }
    return 1;
}

u32 phi_dump(char *buf, u32 cap) {
    char path[256];
    u32 o = 0;
    path[0] = 0;
    if (!dump_node(&root, path, 0, buf, &o, cap)) return 0;
    buf[o] = 0;
    return o;
}

static void load_text(const char *s, u32 len) {
    const char *end = s + len, *w;
    while (s < end) {
        const char *e = s;
        while (e < end && *e != '\n') e++;
        char line[300];
        u32 l = (u32)MIN(e - s, 299);
        memcpy(line, s, l);
        line[l] = 0;
        s = e + 1;
        if (line[0] == 'S' && line[1] == ' ') phi_mkset(line + 2, &w);
        else if (line[0] == 'A' && line[1] == ' ') {
            char *sp = line + 2;
            while (*sp && *sp != ' ') sp++;
            if (!*sp) break;
            *sp++ = 0;
            u32 n = 0;
            while (*sp >= '0' && *sp <= '9') n = n * 10 + (u32)(*sp++ - '0');
            if (s + n > end) break;
            phi_write(line + 2, s, n, &w);
            s += n + 1;
        }
    }
}

void phi_init(void) {
    struct node *d = make(&root, "doc", 1), *a = make(&root, "pictures", 1);
    d->ro = a->ro = 1;
    for (int i = 0; i < doc_pages(); i++) {
        const char *name, *src;
        int len;
        doc_page_at(i, &name, &src, &len);
        struct node *n = make(d, name, 0);
        n->ro = 1;
        n->data = src;
        n->len = (u32)len;
    }
    for (int i = 0;; i++) {
        const char *name;
        const u8 *data;
        u32 nl, dl;
        if (!pic_at(i, &name, &nl, &data, &dl)) break;
        char nm[NAMEMAX + 1];
        snfmt(nm, sizeof nm, "%s.jpg", "x");
        u32 m = MIN(nl, NAMEMAX - 4);
        memcpy(nm, name, m);
        memcpy(nm + m, ".jpg", 5);
        struct node *n = make(a, nm, 0);
        n->ro = 1;
        n->data = (const char *)data;
        n->len = dl;
    }
    u32 len;
    char *saved = persist_load(5, &len);
    if (saved) { loading = 1; load_text(saved, len); loading = 0; kfree(saved); }
}

/* words */
static void why_fail(const char *w, const char *why) {
    char m[96];
    snfmt(m, sizeof m, "%s: %s", w, why);
    fail("%s", m);
}

static int pop_path(const char *w, str **s) { return pop_str(w, s); }

static void b_write(void) {
    str *p, *v;
    const char *why;
    if (!pop_path("phi-write", &p) || !pop_str("phi-write", &v)) return;
    if (!phi_write(p->s, v->s, v->len, &why)) why_fail("phi-write", why);
}

static void b_read(void) {
    str *p;
    if (!pop_path("phi-read", &p)) return;
    struct node *n = find_path(p->s, NULL);
    if (!n || n->set) { why_fail("phi-read", n ? "that is a set, try members" : "there is nothing there"); return; }
    push(S(str_new(n->data ? n->data : "", n->len)));
}

static void b_set(void) {
    str *p;
    const char *why;
    if (!pop_path("phi-set", &p)) return;
    if (!phi_mkset(p->s, &why)) why_fail("phi-set", why);
}

static void b_delete(void) {
    str *p;
    const char *why;
    if (!pop_path("phi-delete", &p)) return;
    if (!phi_delete(p->s, &why)) why_fail("phi-delete", why);
}

static void b_move(void) {
    str *a, *b;
    const char *why;
    if (!pop_str("phi-move", &b) || !pop_str("phi-move", &a)) return;
    if (!phi_move(a->s, b->s, &why)) why_fail("phi-move", why);
}

static void b_has(void) {
    str *p;
    if (!pop_path("phi?", &p)) return;
    struct node *n = find_path(p->s, NULL);
    push(I(!n ? 0 : n->set ? 1 : 2));
}

static void line_for(struct node *k) {
    vga_color(k->set ? C_BLUE : C_TEXT);
    kprintf("  %-32s", k->name);
    vga_color(C_DIM);
    if (k->set) { int c = 0; for (struct node *x = k->kid; x; x = x->next) c++; kprintf("%d %s", c, c == 1 ? "member" : "members"); }
    else kprintf("%u bytes", k->len);
    if (k->ro) kputs("  read only");
    kputc('\n');
    vga_color(C_TEXT);
}

static void b_members(void) {
    str *p;
    if (!pop_path("members", &p)) return;
    struct node *n = find_path(p->s, NULL);
    if (!n || !n->set) { why_fail("members", n ? "that is a value" : "there is nothing there"); return; }
    if (!n->kid) { vga_color(C_DIM); kputs("  empty\n"); vga_color(C_TEXT); }
    for (struct node *k = n->kid; k; k = k->next) {
        if (k->set) line_for(k);
    }
    for (struct node *k = n->kid; k; k = k->next) if (!k->set) line_for(k);
}

static void tree(struct node *n, int depth) {
    for (struct node *k = n->kid; k; k = k->next) {
        if (k->ro && depth == 0) continue;
        for (int i = 0; i < depth; i++) kputs("  ");
        line_for(k);
        if (k->set) tree(k, depth + 1);
    }
}

static void b_phi(void) {
    for (struct node *k = root.kid; k; k = k->next) if (k->ro) {
        vga_color(C_BLUE);
        kprintf("  /%s", k->name);
        vga_color(C_DIM);
        int c = 0;
        for (struct node *x = k->kid; x; x = x->next) c++;
        kprintf("  %d read only\n", c);
        vga_color(C_TEXT);
    }
    tree(&root, 0);
}

static void edit_value(const char *path) {
    struct node *n = find_path(path, NULL);
    const char *why;
    const char *text = n && !n->set ? n->data : "";
    u32 tl = n && !n->set ? n->len : 0, out;
    if (n && n->set) { why_fail("phi-edit", "that is a set"); return; }
    char *r = scrawl(path, text ? text : "", tl, &out);
    if (!r) return;
    if (n && n->ro) { kfree(r); why_fail("phi-edit", "that value is read only, nothing was kept"); return; }
    if (!phi_write(path, r, out, &why)) why_fail("phi-edit", why);
    kfree(r);
}

static void b_edit(void) {
    str *p;
    if (!pop_path("phi-edit", &p)) return;
    edit_value(p->s);
}

/* dired */
static char cwd[160] = "/";
static struct node *list[256];
static int nlist, selected, scroll;
static char note[80];

static void join(char *out, size_t cap, const char *dir, const char *name) {
    snfmt(out, cap, "%s%s%s", dir, !strcmp(dir, "/") ? "" : "/", name);
}

static void refresh(void) {
    struct node *d = find_path(cwd, NULL);
    nlist = 0;
    if (!d || !d->set) { scopy(cwd, "/", sizeof cwd); d = &root; }
    for (int pass = 0; pass < 2; pass++)
        for (struct node *k = d->kid; k && nlist < 256; k = k->next) if ((int)k->set == (pass == 0)) list[nlist++] = k;
    if (selected >= nlist) selected = nlist - 1;
    if (selected < 0) selected = 0;
}

static void up_dir(void) {
    char *s = cwd + strlen(cwd);
    while (s > cwd && s[-1] != '/') s--;
    if (s > cwd + 1) s--;
    *s = 0;
    if (!cwd[0]) scopy(cwd, "/", sizeof cwd);
    selected = scroll = 0;
}

static void dired_draw(void) {
    u8 bar = ATTR(C_TEXT, C_BAR);
    vga_fill(0, 0, VW, ' ', bar);
    vga_puts_at(1, 0, "dired", ATTR(C_MOON, C_BAR));
    vga_puts_at(7, 0, cwd, bar);
    if (selected < scroll) scroll = selected;
    if (selected >= scroll + term_h() - 2) scroll = selected - (term_h() - 2) + 1;
    for (int r = 0; r < term_h() - 2; r++) {
        int i = scroll + r;
        vga_fill(0, r + 1, VW, ' ', ATTR(C_TEXT, C_BG));
        if (i >= nlist) continue;
        struct node *k = list[i];
        u8 a = i == selected ? ATTR(C_BG, C_PEACH) : ATTR(k->set ? C_BLUE : C_TEXT, C_BG);
        vga_fill(0, r + 1, VW, ' ', a);
        char t[64];
        snfmt(t, sizeof t, "%s%s", k->name, k->set ? "/" : "");
        vga_puts_at(2, r + 1, t, a);
        if (k->set) { int c = 0; for (struct node *x = k->kid; x; x = x->next) c++; snfmt(t, sizeof t, "%d", c); }
        else snfmt(t, sizeof t, "%u", k->len);
        vga_puts_at(MAX(14, term_w() - 36), r + 1, t, a);
        if (k->ro) vga_puts_at(MAX(22, term_w() - 26), r + 1, "read only", i == selected ? a : ATTR(C_DIM, C_BG));
    }
    vga_fill(0, term_h() - 1, VW, ' ', bar);
    if (note[0]) vga_puts_at(1, term_h() - 1, note, ATTR(C_MOON, C_BAR));
    else vga_puts_at(1, term_h() - 1, "RET open  ^ up  + new set  c new value  d delete  R rename  g refresh  q quit", ATTR(C_DIM, C_BAR));
    vga_cursor(0, term_h());
}

static void dired_run(const char *start) {
    scopy(cwd, start, sizeof cwd);
    selected = scroll = 0;
    note[0] = 0;
    for (;;) {
        refresh();
        dired_draw();
        int k = kbd_get();
        const char *why;
        char in[LINE_MAX + 1], p[260];
        struct node *cur = nlist ? list[selected] : NULL;
        note[0] = 0;
        switch (k) {
        case 'q': case K_ESC: case CTRL('g'): vga_clear(); return;
        case 'n': case CTRL('n'): case K_DOWN: if (selected < nlist - 1) selected++; break;
        case 'p': case CTRL('p'): case K_UP: if (selected > 0) selected--; break;
        case K_HOME: case META('<'): selected = 0; break;
        case K_END: case META('>'): selected = nlist - 1; break;
        case '^': case '\b': case 'u': up_dir(); break;
        case 'g': break;
        case '\n': case 'f': case 'e':
            if (!cur) break;
            join(p, sizeof p, cwd, cur->name);
            if (cur->set) { scopy(cwd, p, sizeof cwd); selected = scroll = 0; }
            else edit_value(p);
            break;
        case '+':
            if (minibuf("New set: ", in, sizeof in, 0) <= 0) break;
            join(p, sizeof p, cwd, in);
            if (!phi_mkset(p, &why)) snfmt(note, sizeof note, "%s", why);
            break;
        case 'c':
            if (minibuf("New value: ", in, sizeof in, 0) <= 0) break;
            join(p, sizeof p, cwd, in);
            if (find_path(p, NULL)) { snfmt(note, sizeof note, "that name is already taken"); break; }
            if (!phi_write(p, "", 0, &why)) { snfmt(note, sizeof note, "%s", why); break; }
            edit_value(p);
            break;
        case 'd': case 'D':
            if (!cur) break;
            snfmt(p, sizeof p, "Delete %s? (y or n) ", cur->name);
            if (minibuf(p, in, sizeof in, 0) > 0 && (in[0] == 'y' || in[0] == 'Y')) {
                join(p, sizeof p, cwd, cur->name);
                if (!phi_delete(p, &why)) snfmt(note, sizeof note, "%s", why);
            }
            break;
        case 'R': case 'r':
            if (!cur) break;
            if (minibuf("Rename to: ", in, sizeof in, 0) <= 0) break;
            join(p, sizeof p, cwd, cur->name);
            char q[260];
            join(q, sizeof q, cwd, in);
            if (!phi_move(p, q, &why)) snfmt(note, sizeof note, "%s", why);
            break;
        }
    }
}

static void b_dired(void) {
    set_title("dired");
    dired_run("/");
    set_title(SHELL_NAME);
}

static void b_dired_at(void) {
    str *p;
    if (!pop_path("dired-at", &p)) return;
    set_title("dired");
    dired_run(p->s);
    set_title(SHELL_NAME);
}

const struct builtin phi_words[] = {
    { "phi", b_phi }, { "members", b_members }, { "phi-read", b_read }, { "phi-write", b_write }, { "phi-set", b_set },
    { "phi-delete", b_delete }, { "phi-move", b_move }, { "phi-edit", b_edit }, { "phi?", b_has },
    { "dired", b_dired }, { "dired-at", b_dired_at }, { NULL, NULL }
};
