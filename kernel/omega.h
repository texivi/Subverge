#pragma once
#include "lib.h"
#include "line.h"

enum { T_NONE, T_INT, T_STR, T_QUOTE, T_SYM, T_WORD, T_DEF, T_LOCALS, T_SET, T_ORD, T_CARD, T_BIG };
enum { O_STR, O_QUOTE, O_BIND, O_ORD, O_BIG };

typedef struct obj { struct obj *all; u8 kind, mark, pinned; } obj;
typedef struct { u8 t; i64 n; obj *p; } V;

typedef struct str { obj h; u32 len; void (*fn)(void); char s[]; } str;
typedef struct {
    obj h;
    u32 n;
    u32 hits;
    void *jit;
    u32 jitlen, jitver;
    u8 nojit;            /* tried, couldn't compile */
    V v[];
} quote;
typedef struct bind { obj h; str *name; V v; struct bind *next; } bind;
typedef struct { quote *stack; bind *dict; } world;

#define STK 1024
#define HIST 64
#define DEPTH 400
#define NLOC 256

struct omega {
    obj *objs;
    u64 nobjs, ngc;
    V stk[STK];
    int sp, depth, nback, err, pin, tracing, srcmode, restoring, shadowy;
    char errmsg[96];
    bind *dict;
    struct { str *name; V v; } loc[NLOC];
    int nloc;
    world cur, hist[HIST], fut[HIST];
    int nhist, nfut, nredo;
    V keep[256];
    int nkeep;
    quote *linequote;
    u64 gc_threshold;
    const char *src;
    struct history lines;
};
#define W (current->om)

struct builtin { const char *name; void (*fn)(void); };

extern bind *root_dict;

void fail(const char *fmt, const char *a);
obj *gnew(u64 size, u8 kind);
str *str_new(const char *s, u32 len);
str *intern(const char *s, u32 len);
quote *quote_new(const V *v, u32 n);
V I(i64 n);
V S(str *s);
V Q(quote *q);
const char *name_of(V v);

void push(V v);
V pop(void);
int need(int n, const char *w);
int want(V v, u8 t, const char *w);
int brk(void);
int pop_int(const char *w, i64 *out);
int pop_str(const char *w, str **out);

bind *lookup(str *name);
void define(str *name, V v);
void call(V v);
void run(quote *q);
quote *parse_text(const char *text);

typedef struct { char *b; int cap, len; } sbuf;
void sput(sbuf *o, const char *s, u32 n);
void sputs(sbuf *o, const char *s);
void repr(sbuf *o, V v, int raw);
V big_arith(char op, V a, V b);
int big_cmp(V a, V b);
V big_neg(V a);
int big_parse(const char *t, u32 l, V *out);
void big_repr(sbuf *o, V v);
u32 big_digits(V v);
void body(sbuf *o, quote *q);

void register_words(const struct builtin *w);

/* modules */
extern const struct builtin core_words[], hw_words[], ord_words[], root_words[], persist_words[];
void ord_init(void);
int ord_arith(char op, V a, V b, V *out);
int ord_compare(V a, V b, int *out);
void ord_repr(sbuf *o, V v);
void ord_source(sbuf *o, V v);
void ord_mark(obj *o);
void root_boot(void);
const char *root_boot_error(void);
void mark(obj *o);
void keep(V v);
void unkeep(int n);
void gc_point(void);

/* the jit */
extern u32 jit_version, jit_enabled;
int jit_run(quote *q);
void jit_forget(quote *q);
void jit_release_one(bind *b);
void jit_sweep(void);
extern const struct builtin jit_words[], dis_words[], help_words[];
void exec_word(str *name);
void exec_item(V *item);
