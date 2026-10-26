#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
int memcmp(const void *, const void *, unsigned long);
#include "cpu.h"
#include "kbd.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

struct bootinfo boot = { 256 * 1024, 200 * 1024, "host" };
extern u16 host_screen[80 * 25];
static u64 fake_ticks;
jmp_buf out_of_keys;

u64 ticks(void) { return fake_ticks; }
u64 idle_count(void) { return fake_ticks / 2; }
u64 irq_count(int irq) { return irq == 0 ? fake_ticks : 7; }
void irq_set(int irq, void (*fn)(void)) { (void)irq; (void)fn; }
void timer_init(u32 hz) { (void)hz; }
void panic(const char *msg) { printf("PANIC: %s\n", msg); exit(1); }
void cpu_brand(char *out, int cap) { scopy(out, "Host CPU", (size_t)cap); }
void host_yield(void) { fake_ticks++; }
void font_init(void) {}
void font_save(u8 *buf) { (void)buf; }
void font_restore(const u8 *buf) { (void)buf; }
char *persist_load(int slot, u32 *len) { (void)slot; (void)len; return NULL; }
void persist_save_tab(int tab) { (void)tab; }
void persist_save_root(void) {}
void persist_save_phi(void) {}
void persist_save_theme(void) {}
int persist_home(void) { return -1; }
static void b_home(void) { push(I(-1)); }
static void b_nothing(void) {}
const struct builtin persist_words[] = { { "home", b_home }, { "drives", b_nothing }, { "ask", b_nothing }, { "install-to", b_nothing }, { "save", b_nothing }, { NULL, NULL } };
u32 omega_source(struct omega *om, char *buf, u32 cap);
int dis1(const u8 *p, u64 addr, char *out, int cap);
int help_check(void);
int audio_render(const char *name, i16 *out, int cap);
void *jit_alloc(u64 n) {
    u64 *p = mmap(NULL, n + 16, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) return NULL;
    p[0] = n + 16;
    return p + 2;
}
void jit_free(void *v) { u64 *p = (u64 *)v - 2; munmap(p, p[0]); }

static int empty_polls;
int kbd_poll(void) {
    struct task *t = current;
    if (t->kh == t->kt) { if (++empty_polls > 3000) { empty_polls = 0; longjmp(out_of_keys, 1); } return -1; }
    empty_polls = 0;
    int k = t->kb[t->kt];
    t->kt = (t->kt + 1) % 64;
    return k;
}
int kbd_get(void) {
    int k = kbd_poll();
    if (k < 0) longjmp(out_of_keys, 1);
    return k;
}
volatile int kbd_xpend;
void kbd_init(void) {}

static void key(int k) { struct task *t = current; t->kb[t->kh] = k; t->kh = (t->kh + 1) % 64; }

static void dump(void) {
    printf("+--------------------------------------------------------------------------------+\n");
    for (int y = 0; y < 25; y++) {
        putchar('|');
        for (int x = 0; x < 80; x++) {
            unsigned c = host_screen[y * 80 + x] & 0xFF;
            if (c == 0x80) printf("w");
            else if (c == 0x81) printf("A");
            else if (c >= 0x82 && c <= 0x8B) printf("%c", '0' + (c - 0x82));
            else if (c == 0xEE) printf("e");
            else if (c == 0xFA) printf(".");
            else if (c == 0xFD) printf("2");
            else if (c == 0xAF) printf(">");
            else if (c == 0xDB) printf("#");
            else if (c == 7) printf("*");
            else putchar(c >= 32 && c < 127 ? (int)c : '?');
        }
        printf("|\n");
    }
    printf("+--------------------------------------------------------------------------------+\n");
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    static u8 heap[64 << 20];
    heap_init((u64)heap, sizeof heap);
    vga_init();
    sched_init();
    omega_init_world();
    register_words(core_words);
    register_words(hw_words);
    register_words(gfx_words);
    register_words(img_words);
    register_words(ord_words);
    register_words(root_words);
    register_words(jit_words);
    register_words(dis_words);
    register_words(help_words);
    register_words(doc_words);
    register_words(mouse_words);
    register_words(key_words);
    register_words(audio_words);
    register_words(phi_words);
    phi_init();
    register_words(persist_words);
    ord_init();
    root_boot();
    if (root_boot_error()) printf("ROOT ERROR: %s\n", root_boot_error());

    current = &tabs[0];
    tabs[0].entry = omega_main;
    active_tab = 0;
    omega_init_world();

    char line[512];
    while (fgets(line, sizeof line, stdin)) {
        line[strlen(line) - 1] = 0;
        if (!strncmp(line, "@screen", 7)) { dump(); continue; }
        if (!strncmp(line, "@readline", 9)) {
            char out[LINE_MAX + 1];
            W->lines.complete = 1;
            if (!setjmp(out_of_keys)) { int n = readline(out, &W->lines); printf("readline: %d \"%s\"\n", n, out); }
            else printf("readline: ran out of keys\n");
            continue;
        }
        if (!strncmp(line, "@render ", 8)) {
            static i16 pcm[44100 * 400];
            int n = audio_render(line + 8, pcm, 44100 * 400);
            char fn[80];
            snprintf(fn, sizeof fn, "build/%s.raw", line + 8);
            FILE *f = fopen(fn, "wb");
            if (f && n > 0) fwrite(pcm, 2, (size_t)n, f);
            if (f) fclose(f);
            printf("rendered %d samples\n", n);
            continue;
        }
        if (!strncmp(line, "@docs", 5)) { printf("%d problems\n", help_check()); continue; }
        if (!strncmp(line, "@dis ", 5)) {
            char *nm = line + 5;
            nm[strlen(nm) - (nm[strlen(nm) - 1] == '\n')] = 0;
            bind *b = lookup(intern(nm, (u32)strlen(nm)));
            quote *q = b && b->v.t == T_QUOTE ? (quote *)b->v.p : NULL;
            if (!q || !q->jit) { printf("@dis: no code\n"); continue; }
            FILE *f = fopen("build/dis.bin", "wb");
            fwrite(q->jit, 1, q->jitlen, f);
            fclose(f);
            FILE *g = fopen("build/dis.txt", "w");
            char t[96];
            for (u32 o = 0; o < q->jitlen;) { int n = dis1((u8 *)q->jit + o, o, t, sizeof t); fprintf(g, "%x: %s\n", o, t); o += (u32)n; }
            fclose(g);
            continue;
        }
        if (!strncmp(line, "@roundtrip", 10)) {
            static char a[1 << 20], b[1 << 20];
            u32 la = omega_source(W, a, sizeof a);
            printf("---- saved world (%u bytes) ----\n%s----\n", la, a);
            struct omega *old = W;
            W = NULL;
            omega_init_world();
            W->restoring = 1;
            eval_line(a);
            W->restoring = 0;
            u32 lb = omega_source(W, b, sizeof b);
            printf("roundtrip %s\n", la == lb && !memcmp(a, b, la) ? "MATCHES" : "DIFFERS");
            if (la != lb || memcmp(a, b, la)) printf("---- second ----\n%s----\n", b);
            (void)old;
            continue;
        }
        if (!strncmp(line, "@keys ", 6)) {
            char *p = line + 6, *tok;
            while (*p) {
                while (*p == ' ') p++;
                if (!*p) break;
                tok = p;
                while (*p && *p != ' ') p++;
                if (*p) *p++ = 0;
                if (!strcmp(tok, "up")) key(K_UP); else if (!strcmp(tok, "down")) key(K_DOWN);
                else if (!strcmp(tok, "left")) key(K_LEFT); else if (!strcmp(tok, "right")) key(K_RIGHT);
                else if (!strcmp(tok, "enter")) key('\n'); else if (!strcmp(tok, "esc")) key(27);
                else if (!strcmp(tok, "space")) key(' ');
                else if (!strcmp(tok, "tab")) key('\t');
                else if (!strcmp(tok, "ctrl-s")) key(CTRL('s')); else if (!strcmp(tok, "ctrl-q")) key(CTRL('q'));
                else for (char *c = tok; *c; c++) key(*c);
            }
            continue;
        }
        vga_color(C_SKY); kputs("> "); vga_color(C_TEXT); kputs(line); kputc('\n');
        if (!setjmp(out_of_keys)) eval_line(line);
        else printf("(ran out of keys during: %s)\n", line);
        char st[400]; sbuf o = { st, sizeof st, 0 }; st[0] = 0;
        for (u32 i = 0; i < W->cur.stack->n; i++) { repr(&o, W->cur.stack->v[i], 0); sputs(&o, " "); }
        printf("[%s] %s\n", line, st);
    }
    dump();
    return 0;
}
