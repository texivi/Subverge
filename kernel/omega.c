#include "cpu.h"
#include "disk.h"
#include "kbd.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

bind *root_dict;

/* errors, allocation, values */

void fail(const char *fmt, const char *a) {
    if (W->err) return;
    W->err = 1;
    snfmt(W->errmsg, sizeof W->errmsg, fmt, a);
}

obj *gnew(u64 size, u8 kind) {
    obj *o = kmalloc_try(size);
    if (!o) { fail("ran out of memory%s", ""); return NULL; }
    o->kind = kind;
    o->mark = 0;
    o->pinned = (u8)W->pin;
    if (!W->pin) {
        o->all = W->objs;
        W->objs = o;
        W->nobjs++;
    }
    return o;
}

str *str_new(const char *s, u32 len) {
    str *x = (str *)gnew(sizeof(str) + len + 1, O_STR);
    if (!x) return NULL;
    x->len = len;
    x->fn = NULL;
    memcpy(x->s, s, len);
    x->s[len] = 0;
    return x;
}

#define ITAB 8192
static str *itab[ITAB];

str *intern(const char *s, u32 len) {
    u32 h = 2166136261u;
    for (u32 i = 0; i < len; i++) h = (h ^ (u8)s[i]) * 16777619u;
    u64 f = irq_save();
    for (u32 k = 0; k < ITAB; k++) {
        str **slot = &itab[(h + k) % ITAB];
        if (!*slot) {
            str *x = kmalloc(sizeof(str) + len + 1);
            x->h = (obj){ NULL, O_STR, 0, 1 };
            x->len = len;
            x->fn = NULL;
            memcpy(x->s, s, len);
            x->s[len] = 0;
            *slot = x;
            irq_restore(f);
            return x;
        }
        if ((*slot)->len == len && !memcmp((*slot)->s, s, len)) { irq_restore(f); return *slot; }
    }
    irq_restore(f);
    panic("symbol table full");
}

quote *quote_new(const V *v, u32 n) {
    quote *q = (quote *)gnew(sizeof(quote) + (u64)n * sizeof(V), O_QUOTE);
    if (!q) return NULL;
    q->n = n;
    q->hits = 0;
    q->jit = NULL;
    q->jitlen = q->jitver = 0;
    q->nojit = 0;
    if (n && v) memcpy(q->v, v, (u64)n * sizeof(V));
    else if (n) memset(q->v, 0, (u64)n * sizeof(V));
    return q;
}

V I(i64 n) { return (V){ T_INT, n, NULL }; }
V S(str *s) { return (V){ s ? T_STR : T_NONE, 0, s ? &s->h : NULL }; }
V Q(quote *q) { return (V){ q ? T_QUOTE : T_NONE, 0, q ? &q->h : NULL }; }
const char *name_of(V v) { return ((str *)v.p)->s; }

void push(V v) {
    if (W->err) return;
    if (v.t == T_NONE) { fail("that value can't go on the stack%s", ""); return; }
    if (W->sp == STK) { fail("the stack is full%s", ""); return; }
    W->stk[W->sp++] = v;
}

int need(int n, const char *w) {
    if (W->sp >= n) return 1;
    fail("%s needs more values on the stack", w);
    return 0;
}

V pop(void) { return W->stk[--W->sp]; }

int want(V v, u8 t, const char *w) {
    static const char *const tn[] = { "nothing", "a number", "a string", "a quote", "a 'name" };
    if (v.t == t) return 1;
    char m[64];
    snfmt(m, sizeof m, "%s needs %s", w, t < 5 ? tn[t] : "something else");
    fail("%s", m);
    return 0;
}

int pop_int(const char *w, i64 *out) {
    if (!need(1, w)) return 0;
    V v = pop();
    if (v.t == T_BIG) { char m[64]; snfmt(m, sizeof m, "%s needs a number that fits in 64 bits", w); fail("%s", m); return 0; }
    if (!want(v, T_INT, w)) return 0;
    *out = v.n;
    return 1;
}

int pop_str(const char *w, str **out) {
    if (!need(1, w)) return 0;
    V v = pop();
    if (!want(v, T_STR, w)) return 0;
    *out = (str *)v.p;
    return 1;
}

int brk(void) {
    if (!kbreak) return 0;
    kbreak = 0;
    fail("interrupted%s", "");
    return 1;
}

/* the world */

bind *lookup(str *name) {
    for (bind *b = W->dict; b; b = b->next)
        if (b->name == name) { if (b->v.t != T_NONE) return b; break; }
    for (bind *b = root_dict; b; b = b->next)
        if (b->name == name) return b->v.t == T_NONE ? NULL : b;
    return NULL;
}

void define(str *name, V v) {
    if (name->fn) jit_version++;
    bind *b = (bind *)gnew(sizeof(bind), O_BIND);
    if (!b) return;
    b->name = name;
    b->v = v;
    b->next = W->dict;
    W->dict = b;
}

static str *builtins[512];
static int nbuiltins;

void register_words(const struct builtin *w) {
    for (; w->name; w++) {
        str *s = intern(w->name, (u32)strlen(w->name));
        s->fn = w->fn;
        if (nbuiltins < 512) builtins[nbuiltins++] = s;
    }
}

/* prompt completion */
static int add_match(str **m, int k, str *s, const char *pre, int n) {
    if (k == 256 || (int)s->len < n || memcmp(s->s, pre, (size_t)n)) return k;
    for (int i = 0; i < k; i++) if (m[i] == s) return k;
    m[k] = s;
    return k + 1;
}

int complete_name(const char *pre, int n, int nth, char *out, int cap, int *lcp) {
    str *m[256];
    int k = 0;
    for (bind *b = W->cur.dict; b; b = b->next) if (b->v.t != T_NONE && lookup(b->name)) k = add_match(m, k, b->name, pre, n);
    for (bind *b = root_dict; b; b = b->next) if (b->v.t != T_NONE) k = add_match(m, k, b->name, pre, n);
    for (int i = 0; i < nbuiltins; i++) k = add_match(m, k, builtins[i], pre, n);
    if (!k) return 0;
    for (int i = 1; i < k; i++)
        for (int j = i; j > 0 && strcmp(m[j - 1]->s, m[j]->s) > 0; j--) { str *t = m[j]; m[j] = m[j - 1]; m[j - 1] = t; }
    int l = (int)m[0]->len;
    for (int i = 1; i < k; i++) {
        int c = 0;
        while (c < l && c < (int)m[i]->len && m[i]->s[c] == m[0]->s[c]) c++;
        l = c;
    }
    *lcp = MIN(l, cap - 2);
    scopy(out, m[nth % k]->s, (size_t)cap - 1);
    return k;
}

/* printing */

void sput(sbuf *o, const char *s, u32 n) {
    if (!o) { while (n--) kputc(*s++); return; }
    while (n-- && o->len < o->cap - 1) o->b[o->len++] = *s++;
    o->b[o->len] = 0;
}
void sputs(sbuf *o, const char *s) { sput(o, s, (u32)strlen(s)); }

void body(sbuf *o, quote *q) {
    for (u32 i = 0; i < q->n; i++) {
        if (i) sputs(o, " ");
        repr(o, q->v[i], 0);
    }
}

void repr(sbuf *o, V v, int raw) {
    char t[24];
    switch (v.t) {
    case T_INT: snfmt(t, sizeof t, "%ld", v.n); sputs(o, t); break;
    case T_STR: {
        str *s = (str *)v.p;
        if (raw) { sput(o, s->s, s->len); break; }
        sputs(o, "\"");
        for (u32 i = 0; i < s->len; i++) {
            char c = s->s[i];
            if (c == '\n') sputs(o, "\\n");
            else if (c == '\t') sputs(o, "\\t");
            else if (c == 27) sputs(o, "\\e");
            else if (c == '"') sputs(o, "\\\"");
            else if (c == '\\') sputs(o, "\\\\");
            else if ((u8)c == 0x80 && W && W->srcmode) sputs(o, "\\w");
            else if ((u8)c == 0x81 && W && W->srcmode) sputs(o, "\\A");
            else sput(o, &c, 1);
        }
        sputs(o, "\"");
        break;
    }
    case T_QUOTE: sputs(o, "[ "); body(o, (quote *)v.p); sputs(o, ((quote *)v.p)->n ? " ]" : "]"); break;
    case T_SYM: sputs(o, "'"); sputs(o, name_of(v)); break;
    case T_WORD: sputs(o, name_of(v)); break;
    case T_DEF: sputs(o, ":"); sputs(o, name_of(v)); break;
    case T_SET: sputs(o, "->"); sputs(o, name_of(v)); break;
    case T_LOCALS: {
        quote *q = (quote *)v.p;
        sputs(o, "{");
        for (u32 i = 0; i < q->n; i++) { sputs(o, " "); sputs(o, name_of(q->v[i])); }
        sputs(o, " }");
        break;
    }
    case T_BIG: big_repr(o, v); break;
    case T_ORD: case T_CARD: if (W && W->srcmode) ord_source(o, v); else ord_repr(o, v); break;
    default: sputs(o, "?");
    }
}

/* parsing */

static int delim(char c) {
    return !c || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '[' || c == ']' || c == '"' || c == '{' || c == '}';
}

static int to_int(const char *t, u32 l, i64 *out) {
    u64 i = 0, v = 0, base = 10;
    int neg = 0;
    if (l && t[0] == '-') { neg = 1; i = 1; }
    if (i >= l) return 0;
    if (l - i > 2 && t[i] == '0' && (t[i + 1] == 'x' || t[i + 1] == 'b')) { base = t[i + 1] == 'x' ? 16 : 2; i += 2; }
    for (; i < l; i++) {
        char c = (char)(t[i] | 32);
        u32 d = c >= '0' && c <= '9' ? (u32)(c - '0') : c >= 'a' && c <= 'f' ? (u32)(c - 'a' + 10) : 99;
        if (d >= base) return 0;
        if (v > (~0ull - d) / base) return -1;
        v = v * base + d;
    }
    if (v > (1ull << 63) - !neg) return -1;
    *out = neg ? (i64)(0 - v) : (i64)v;
    return 1;
}

static void skip_space(void) {
    for (;;) {
        while (*W->src == ' ' || *W->src == '\t' || *W->src == '\n' || *W->src == '\r') W->src++;
        if (*W->src != '#') return;
        while (*W->src && *W->src != '\n') W->src++;
    }
}

static quote *parse(int nested) {
    V items[256];
    u32 n = 0;
    if (nested > 32) { fail("the brackets are nested too deep%s", ""); return NULL; }
    for (;;) {
        skip_space();
        char c = *W->src;
        if (!c) { if (nested) { fail("a [ is never closed%s", ""); return NULL; } break; }
        if (c == ']') { if (!nested) { fail("there is a ] with no [ before it%s", ""); return NULL; } W->src++; break; }
        if (c == '}') { fail("there is a } with no { before it%s", ""); return NULL; }
        if (n == 256) { fail("that quote is too long%s", ""); return NULL; }
        if (c == '[') {
            W->src++;
            quote *q = parse(nested + 1);
            if (!q) return NULL;
            items[n++] = Q(q);
            continue;
        }
        if (c == '{') {
            V names[32];
            u32 k = 0;
            W->src++;
            for (;;) {
                skip_space();
                if (*W->src == '}') { W->src++; break; }
                if (!*W->src || delim(*W->src)) { fail("{ } needs names inside it%s", ""); return NULL; }
                const char *t = W->src;
                while (!delim(*W->src)) W->src++;
                if (k == 32) { fail("too many local names%s", ""); return NULL; }
                names[k++] = (V){ T_SYM, 0, &intern(t, (u32)(W->src - t))->h };
            }
            if (!k) { fail("{ } needs names inside it%s", ""); return NULL; }
            quote *q = quote_new(names, k);
            if (!q) return NULL;
            items[n++] = (V){ T_LOCALS, 0, &q->h };
            continue;
        }
        if (c == '"') {
            const char *e = W->src + 1;
            u32 l = 0;
            while (*e && *e != '"') { if (*e == '\\' && e[1]) e++; e++; l++; }
            if (*e != '"') { fail("a string is never closed%s", ""); return NULL; }
            str *full = (str *)gnew(sizeof(str) + l + 1, O_STR);
            if (!full) return NULL;
            full->len = l;
            full->fn = NULL;
            u32 k = 0;
            for (W->src++; *W->src != '"'; ) {
                char ch = *W->src++;
                if (ch == '\\') {
                    ch = *W->src++;
                    ch = ch == 'n' ? '\n' : ch == 't' ? '\t' : ch == 'e' ? 27 : ch == 'w' ? (char)0x80 : ch == 'A' ? (char)0x81 : ch;
                }
                full->s[k++] = ch;
            }
            full->s[k] = 0;
            W->src++;
            items[n++] = S(full);
            continue;
        }
        const char *t = W->src;
        while (!delim(*W->src)) W->src++;
        u32 l = (u32)(W->src - t);
        i64 num;
        int isnum = to_int(t, l, &num);
        if (isnum == 1) { items[n++] = I(num); continue; }
        V b;
        if (isnum < 0 && big_parse(t, l, &b)) {
            if (W->err) return NULL;
            items[n++] = b;
            continue;
        }
        u8 type = T_WORD;
        if (l > 1 && t[0] == '\'') { type = T_SYM; t++; l--; }
        else if (l > 1 && t[0] == ':') { type = T_DEF; t++; l--; }
        else if (l > 2 && t[0] == '-' && t[1] == '>') { type = T_SET; t += 2; l -= 2; }
        items[n++] = (V){ type, 0, &intern(t, l)->h };
    }
    return quote_new(items, n);
}

quote *parse_text(const char *text) {
    const char *save = W->src;
    W->src = text;
    quote *q = parse(0);
    W->src = save;
    return q;
}

/* running */

void keep(V v) {
    if (W->nkeep < 256) W->keep[W->nkeep] = v;
    W->nkeep++;
}
void unkeep(int n) { W->nkeep -= n; }

void call(V v) {
    if (v.t != T_QUOTE) { push(v); return; }
    keep(v);
    run((quote *)v.p);
    unkeep(1);
}

void exec_word(str *name) {
    for (int i = W->nloc - 1; i >= 0; i--)
        if (W->loc[i].name == name) { push(W->loc[i].v); return; }
    bind *b = lookup(name);
    if (b) { call(b->v); return; }
    if (name->fn) { name->fn(); return; }
    fail("%s isn't defined", name->s);
}

static void set_local(str *name, V v) {
    for (int i = W->nloc - 1; i >= 0; i--)
        if (W->loc[i].name == name) { W->loc[i].v = v; return; }
    if (W->nloc == NLOC) { fail("too many local names%s", ""); return; }
    if (name->fn) W->shadowy = 1;
    W->loc[W->nloc].name = name;
    W->loc[W->nloc++].v = v;
}

static void trace_step(V item) {
    char t[160];
    sbuf o = { t, sizeof t, 0 };
    t[0] = 0;
    for (int i = MAX(0, W->sp - 6); i < W->sp; i++) { repr(&o, W->stk[i], 0); sputs(&o, " "); }
    vga_color(C_DIM);
    kprintf("  %s", t);
    vga_color(C_PEACH);
    kputs("\xaf ");
    o = (sbuf){ t, sizeof t, 0 };
    t[0] = 0;
    repr(&o, item, 0);
    kprintf("%s", t);
    vga_color(C_DIM);
    kputs("   (space step, c continue, q stop)\n");
    vga_color(C_TEXT);
    int k = kbd_get();
    if (k == 'c') W->tracing = 0;
    if (k == 'q' || k == K_ESC || k == CTRL('c')) { W->tracing = 0; fail("stopped%s", ""); }
}

void exec_item(V *item) {
    V v = *item;
    switch (v.t) {
    case T_WORD: exec_word((str *)v.p); break;
    case T_DEF: if (need(1, ":")) define((str *)v.p, pop()); break;
    case T_SET: if (need(1, "->")) set_local((str *)v.p, pop()); break;
    case T_LOCALS: {
        quote *names = (quote *)v.p;
        if (!need((int)names->n, "{ }")) break;
        V vals[32];
        for (int k = (int)names->n - 1; k >= 0; k--) vals[k] = pop();
        for (u32 k = 0; k < names->n; k++) {
            if (W->nloc == NLOC) { fail("too many local names%s", ""); break; }
            if (((str *)names->v[k].p)->fn) W->shadowy = 1;
            W->loc[W->nloc].name = (str *)names->v[k].p;
            W->loc[W->nloc++].v = vals[k];
        }
        break;
    }
    default: push(v);
    }
}

void run(quote *q) {
    if (jit_run(q)) return;
    if (++W->depth > DEPTH) { fail("this nests too deep%s", ""); W->depth--; return; }
    int base = W->nloc;
    for (u32 i = 0; i < q->n && !W->err; i++) {
        if (brk()) break;
        gc_point();
        if (W->tracing) { trace_step(q->v[i]); if (W->err) break; }
        exec_item(&q->v[i]);
    }
    W->nloc = base;
    W->depth--;
}

/* garbage collection */

static void markv(V v) { if (v.p) mark(v.p); }

void mark(obj *o) {
    while (o && !o->pinned && !o->mark) {
        o->mark = 1;
        switch (o->kind) {
        case O_QUOTE: {
            quote *q = (quote *)o;
            for (u32 i = 0; i < q->n; i++) markv(q->v[i]);
            return;
        }
        case O_ORD: ord_mark(o); return;
        case O_BIND: {
            bind *b = (bind *)o;
            markv(b->v);
            o = b->next ? &b->next->h : NULL;
            break;
        }
        default: return;
        }
    }
}

static void markw(world w) {
    if (w.stack) mark(&w.stack->h);
    if (w.dict) mark(&w.dict->h);
}

static void gc(void) {
    markw(W->cur);
    for (int i = 0; i < W->nhist; i++) markw(W->hist[i]);
    for (int i = 0; i < W->nfut; i++) markw(W->fut[i]);
    for (int i = 0; i < W->sp; i++) markv(W->stk[i]);
    for (int i = 0; i < W->nloc; i++) markv(W->loc[i].v);
    for (int i = 0; i < MIN(W->nkeep, 256); i++) markv(W->keep[i]);
    if (W->dict) mark(&W->dict->h);
    if (W->linequote) mark(&W->linequote->h);
    for (obj **pp = &W->objs; *pp; ) {
        obj *o = *pp;
        if (o->mark) { o->mark = 0; pp = &o->all; }
        else {
            *pp = o->all;
            if (o->kind == O_QUOTE) jit_forget((quote *)o);
            kfree(o);
            W->nobjs--;
        }
    }
    W->ngc++;
    W->gc_threshold = MAX(8192, W->nobjs * 2);
}

void gc_point(void) {
#ifdef GC_STRESS
    if (!W->pin && W->nkeep <= 256) gc();
#else
    if (!W->pin && W->nobjs > W->gc_threshold && W->nkeep <= 256) gc();
#endif
}

void omega_stats(struct omega_stats *s) {
    u64 defs = 0;
    for (bind *b = W->cur.dict; b; b = b->next) defs++;
    s->objects = W->nobjs;
    s->gcs = W->ngc;
    s->bindings = defs;
    s->stack = W->cur.stack ? W->cur.stack->n : 0;
    s->history = (u64)W->nhist;
}

/* the loop */

static void prompt(void) {
    char t[512];
    sbuf o = { t, sizeof t, 0 };
    t[0] = 0;
    for (u32 i = 0; i < W->cur.stack->n; i++) {
        if (i) sputs(&o, " ");
        repr(&o, W->cur.stack->v[i], 0);
    }
    vga_color(C_DIM);
    if (o.len > 40) kprintf("..%s ", t + o.len - 38);
    else if (o.len) kprintf("%s ", t);
    vga_color(C_SKY);
    kputs("\xaf ");
    vga_color(C_TEXT);
}

static void report(u8 color, const char *fmt, const char *a) {
    vga_color(color);
    kputs("  ");
    kprintf(fmt, a);
    kputc('\n');
    vga_color(C_TEXT);
}

/* run a key word */
int key_run(str *w) {
    bind *b;
    W->sp = (int)W->cur.stack->n;
    memcpy(W->stk, W->cur.stack->v, (u64)W->sp * sizeof(V));
    W->dict = W->cur.dict;
    W->err = W->nback = W->nredo = W->depth = W->nloc = W->tracing = W->nkeep = W->shadowy = 0;
    kbreak = 0;
    if (!(b = lookup(w))) return 0;
    call(b->v);
    W->dict = W->cur.dict;
    if (W->err) {
        kputc('\n');
        report(C_LRED, "! %s", W->errmsg);
        return 0;
    }
    return 1;
}

extern char doc_run[200];

void eval_line(const char *line) {
    W->sp = (int)W->cur.stack->n;
    memcpy(W->stk, W->cur.stack->v, (u64)W->sp * sizeof(V));
    W->dict = W->cur.dict;
    W->err = W->nback = W->nredo = W->depth = W->nloc = W->tracing = W->nkeep = W->shadowy = 0;
    kbreak = 0;

    quote *q = parse_text(line);
    W->linequote = q;
    if (q) run(q);
    W->linequote = NULL;

    if (W->err) {
        while (kbd_poll() >= 0) ;
        report(C_LRED, "! %s", W->errmsg);
        return;
    }
    if (W->nback) {
        int k = MIN(W->nback, W->nhist);
        if (!k) { report(C_DIM, "there is nothing to go back to%s", ""); return; }
        for (int i = 0; i < k; i++) {
            if (W->nfut == HIST) { memmove(W->fut, W->fut + 1, (HIST - 1) * sizeof(world)); W->nfut--; }
            W->fut[W->nfut++] = W->cur;
            W->cur = W->hist[--W->nhist];
        }
        char t[16];
        snfmt(t, sizeof t, "%d", k);
        report(C_DIM, "rewound %s", t);
        if (!W->restoring) persist_save_tab(current->id);
        return;
    }
    if (W->nredo) {
        int k = MIN(W->nredo, W->nfut);
        if (!k) { report(C_DIM, "nothing to redo%s", ""); return; }
        for (int i = 0; i < k; i++) {
            if (W->nhist == HIST) { memmove(W->hist, W->hist + 1, (HIST - 1) * sizeof(world)); W->nhist--; }
            W->hist[W->nhist++] = W->cur;
            W->cur = W->fut[--W->nfut];
        }
        char t[16];
        snfmt(t, sizeof t, "%d", k);
        report(C_DIM, "redone %s", t);
        if (!W->restoring) persist_save_tab(current->id);
        return;
    }
    if (W->dict == W->cur.dict && (u32)W->sp == W->cur.stack->n && !memcmp(W->stk, W->cur.stack->v, (u64)W->sp * sizeof(V))) return;
    quote *ns = quote_new(W->stk, (u32)W->sp);
    if (!ns) { report(C_LRED, "! %s", W->errmsg); return; }
    if (W->nhist == HIST) { memmove(W->hist, W->hist + 1, (HIST - 1) * sizeof(world)); W->nhist--; }
    W->hist[W->nhist++] = W->cur;
    W->cur = (world){ ns, W->dict };
    W->nfut = 0;
    if (!W->restoring) persist_save_tab(current->id);
}

u32 omega_source(struct omega *om, char *buf, u32 cap) {
    world w = om->cur;
    bind *seen[2048], *emit[2048];
    int nseen = 0, nemit = 0;
    for (bind *b = w.dict; b && nseen < 2048; b = b->next) {
        int dup = 0;
        for (int i = 0; i < nseen; i++) if (seen[i]->name == b->name) { dup = 1; break; }
        if (dup) continue;
        seen[nseen++] = b;
        if (b->v.t != T_NONE) emit[nemit++] = b;
    }
    struct omega *me = W;
    int was = me ? me->srcmode : 0;
    if (me) me->srcmode = 1;
    sbuf o = { buf, (int)cap, 0 };
    buf[0] = 0;
    sputs(&o, "# saved world\n");
    for (int i = nemit - 1; i >= 0; i--) {
        repr(&o, emit[i]->v, 0);
        sputs(&o, " :");
        sputs(&o, emit[i]->name->s);
        sputs(&o, "\n");
    }
    for (u32 i = 0; w.stack && i < w.stack->n; i++) { repr(&o, w.stack->v[i], 0); sputs(&o, " "); }
    sputs(&o, "\n");
    if (me) me->srcmode = was;
    return o.len >= o.cap - 1 ? 0 : (u32)o.len;
}

void omega_init_world(void) {
    if (W) return;
    W = kzalloc(sizeof *W);
    W->cur = (world){ quote_new(NULL, 0), NULL };
}

void omega_main(void) {
    char line[LINE_MAX + 1];
    line_redraw = prompt;
    set_title(SHELL_NAME);
    if (W) W->lines.complete = W->lines.shell = 1;
    if (!W) {
        omega_init_world();
        u32 len;
        char *saved = persist_load(1 + current->id, &len);
        if (saved) {
            W->restoring = 1;
            eval_line(saved);
            W->restoring = 0;
            W->nhist = 0;
            kfree(saved);
        }
        W->lines.complete = W->lines.shell = 1;
        if (current->id == 0) {
            if (root_boot_error()) {
                sysinfo();
                report(C_LRED, "! root: %s", root_boot_error());
            } else eval_line("'startup defined? [ startup ] [ sys ] if");
        }
    }
    if (current->fault[0]) {
        report(C_LRED, "! %s", current->fault);
        report(C_DIM, "your last line was dropped%s", "");
        current->fault[0] = 0;
    }
    for (;;) {
        gc();
        prompt();
        if (readline(line, &W->lines) <= 0) continue;
        eval_line(line);
        while (doc_run[0]) {
            char run[sizeof doc_run];
            scopy(run, doc_run, sizeof run);
            doc_run[0] = 0;
            kprintf("%s\n", run);
            eval_line(run);
        }
    }
}
