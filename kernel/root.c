#include "disk.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

extern const char _binary_build_rootfs_bin_start[], _binary_build_rootfs_bin_end[];

#define NUNITS 32
#define UWORDS 96

struct unit {
    char name[24];
    char *src;
    u32 len;
    str *words[UWORDS];
    int nwords;
    char *draft;
};

static struct unit units[NUNITS];
static int nunits;
static char boot_err[120];
static char shadow_note[160];
static bind *after[NUNITS];
static int nafter;

const char *root_boot_error(void) { return boot_err[0] ? boot_err : NULL; }

static void split_units(const char *p, const char *end) {
    for (int i = 0; i < nunits; i++) { kfree(units[i].src); kfree(units[i].draft); }
    memset(units, 0, sizeof units);
    nunits = 0;
    struct unit *u = NULL;
    const char *body = NULL;
    while (p < end) {
        const char *eol = p;
        while (eol < end && *eol != '\n') eol++;
        if (eol - p > 4 && p[0] == '#' && p[1] == '>' && p[2] == '>' && p[3] == ' ') {
            if (u) { u->len = (u32)(p - body); u->src = kmalloc(u->len + 1); memcpy(u->src, body, u->len); u->src[u->len] = 0; }
            if (nunits == NUNITS) break;
            u = &units[nunits++];
            u32 l = (u32)MIN(eol - p - 4, (long)sizeof u->name - 1);
            memcpy(u->name, p + 4, l);
            u->name[l] = 0;
            body = eol < end ? eol + 1 : eol;
        }
        p = eol + 1;
    }
    if (u) { u->len = (u32)(end - body); u->src = kmalloc(u->len + 1); memcpy(u->src, body, u->len); u->src[u->len] = 0; }
}

static const char *rebuild(int which, const char *src) {
    static char msg[120];
    int saved_sp = W->sp, saved_nloc = W->nloc, saved_depth = W->depth, saved_tracing = W->tracing;
    V *saved = kmalloc((u64)(saved_sp + 1) * sizeof(V));
    memcpy(saved, W->stk, (u64)saved_sp * sizeof(V));
    bind *saved_dict = W->dict;
    str *words[NUNITS][UWORDS];
    int nwords[NUNITS];
    bind *marks[NUNITS];
    int start = (which > 0 && which <= nafter) ? which : 0;
    bind *shared = start ? after[start - 1] : NULL;

    W->pin = 1;
    W->dict = shared;
    W->err = W->sp = W->nloc = W->depth = W->tracing = 0;
    const char *bad = NULL;
    for (int i = start; i < nunits; i++) {
        bind *before = W->dict;
        quote *q = parse_text(i == which ? src : units[i].src);
        if (q) run(q);
        if (W->err) {
            snfmt(msg, sizeof msg, "%s: %s", units[i].name, W->errmsg);
            bad = msg;
            break;
        }
        marks[i] = W->dict;
        nwords[i] = 0;
        for (bind *b = W->dict; b && b != before && nwords[i] < UWORDS; b = b->next) {
            int dup = 0;
            for (int k = 0; k < nwords[i]; k++) if (words[i][k] == b->name) dup = 1;
            if (!dup) words[i][nwords[i]++] = b->name;
        }
    }
    bind *built = W->dict;

    W->pin = 0;
    W->err = 0;
    W->dict = saved_dict;
    W->sp = saved_sp;
    memcpy(W->stk, saved, (u64)saved_sp * sizeof(V));
    kfree(saved);
    W->nloc = saved_nloc;
    W->depth = saved_depth;
    W->tracing = saved_tracing;
    if (bad) return bad;

    bind *old_root = root_dict;
    root_dict = built;
    for (bind *b = old_root; b && b != shared; b = b->next) jit_release_one(b);
    jit_sweep();
    jit_version++;
    shadow_note[0] = 0;
    for (bind *b = built; b; b = b->next)
        if (b->name->fn && strlen(shadow_note) < sizeof shadow_note - 30) {
            if (!shadow_note[0]) scopy(shadow_note, "shadows:", sizeof shadow_note);
            int l = (int)strlen(shadow_note);
            snfmt(shadow_note + l, sizeof shadow_note - (u64)l, " %s", b->name->s);
        }
    for (int i = start; i < nunits; i++) {
        units[i].nwords = nwords[i];
        memcpy(units[i].words, words[i], sizeof(str *) * (u64)nwords[i]);
        after[i] = marks[i];
    }
    nafter = nunits;
    return NULL;
}

void root_boot(void) {
    u32 len;
    char *saved = persist_load(0, &len);
    if (saved) {
        split_units(saved, saved + len);
        kfree(saved);
        if (!rebuild(-1, NULL)) return;
        scopy(boot_err, "the saved root didn't build, using the built-in one", sizeof boot_err);
    }
    split_units(_binary_build_rootfs_bin_start, _binary_build_rootfs_bin_end);
    const char *e = rebuild(-1, NULL);
    if (e) scopy(boot_err, e, sizeof boot_err);
}

u32 root_source(char *buf, u32 cap) {
    sbuf o = { buf, (int)cap, 0 };
    buf[0] = 0;
    for (int i = 0; i < nunits; i++) {
        sputs(&o, "#>> ");
        sputs(&o, units[i].name);
        sputs(&o, "\n");
        sputs(&o, units[i].src);
        if (units[i].len && units[i].src[units[i].len - 1] != '\n') sputs(&o, "\n");
    }
    return o.len >= o.cap - 1 ? 0 : (u32)o.len;
}

static int find_unit(str *name) {
    for (int i = 0; i < nunits; i++) if (!strcmp(units[i].name, name->s)) return i;
    for (int i = 0; i < nunits; i++)
        for (int k = 0; k < units[i].nwords; k++) if (units[i].words[k] == name) return i;
    return -1;
}

static void b_root(void) {
    kputc('\n');
    for (int i = 0; i < nunits; i++) {
        struct unit *u = &units[i];
        vga_color(C_MOON);
        kprintf("  %-10s", u->name);
        vga_color(C_DIM);
        kprintf("%5u bytes%s  ", u->len, u->draft ? " (draft)" : "");
        vga_color(C_TEXT);
        int x, y, col = 0;
        for (int k = u->nwords - 1; k >= 0; k--) {
            vga_getpos(&x, &y);
            if (x + (int)u->words[k]->len + 1 >= VW) { if (++col > 1) { kputs(".."); break; } kputs("\n                          "); }
            kprintf("%s ", u->words[k]->s);
        }
        kputc('\n');
    }
    vga_color(C_DIM);
    kputs("\n  Edit a unit with 'word open, where word is any name in it.\n\n");
    vga_color(C_TEXT);
}

static int unit_arg(const char *w) {
    if (!need(1, w)) return -1;
    V v = pop();
    if (!want(v, T_SYM, w)) return -1;
    int i = find_unit((str *)v.p);
    if (i < 0) fail("%s isn't in any root unit", name_of(v));
    return i;
}

static void edit_unit(int i) {
    struct unit *u = &units[i];
    const char *text = u->draft ? u->draft : u->src;
    u32 outlen;
    char name[32];
    snfmt(name, sizeof name, "root/%s", u->name);
    char *out = scrawl(name, text, (u32)strlen(text), &outlen);
    if (!out) return;
    const char *e = rebuild(i, out);
    if (e) {
        kfree(u->draft);
        u->draft = out;
        char m[96];
        snfmt(m, sizeof m, "root unchanged, %s (your edit is kept: open it again)", e);
        fail("%s", m);
        return;
    }
    kfree(u->src);
    kfree(u->draft);
    u->src = out;
    u->len = outlen;
    u->draft = NULL;
    persist_save_root();
    vga_color(C_LGREEN);
    kprintf("  edited root\n", u->name);
    if (shadow_note[0]) { vga_color(C_PEACH); kprintf("  %s\n", shadow_note); }
    vga_color(C_TEXT);
}

static void b_open(void) {
    int i = unit_arg("open");
    if (i >= 0) edit_unit(i);
}

static void b_unit(void) {
    str *s;
    if (!pop_str("unit", &s)) return;
    int n = (int)strlen(s->s);
    if (n < 1 || n > 20) { fail("a unit name is 1 to 20 characters%s", ""); return; }
    for (int k = 0; k < n; k++) {
        char c = s->s[k];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) { fail("a unit name uses lowercase letters, digits and -%s", ""); return; }
    }
    for (int k = 0; k < nunits; k++) if (!strcmp(units[k].name, s->s)) { fail("%s is already a unit", s->s); return; }
    if (nunits == NUNITS) { fail("there is no room for another unit%s", ""); return; }
    struct unit *u = &units[nunits];
    memset(u, 0, sizeof *u);
    scopy(u->name, s->s, sizeof u->name);
    u->src = kmalloc(64);
    scopy(u->src, "\n", 64);
    u->len = (u32)strlen(u->src);
    nunits++;
    edit_unit(nunits - 1);
}

static void b_src(void) {
    int i = unit_arg("src");
    if (i >= 0) push(S(str_new(units[i].src, units[i].len)));
}

const struct builtin root_words[] = {
    { "root", b_root }, { "open", b_open }, { "unit", b_unit }, { "src", b_src },
    { NULL, NULL }
};
