#include "cpu.h"
#include "disk.h"
#include "kbd.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

struct mmap_entry { u64 base, len; u32 type; };
struct mmap_entry boot_mmap[32];
int boot_nmmap;

/* memory and ports */

static void b_peek(void) { i64 a; if (pop_int("peek", &a)) push(I(*(volatile u8 *)(u64)a)); }
static void b_peekd(void) { i64 a; if (pop_int("peekd", &a)) push(I(*(volatile u32 *)(u64)a)); }
static void b_peekq(void) { i64 a; if (pop_int("peekq", &a)) push(I(*(volatile i64 *)(u64)a)); }
static void b_poke(void) { i64 v, a; if (pop_int("poke", &a) && pop_int("poke", &v)) *(volatile u8 *)(u64)a = (u8)v; }
static void b_poked(void) { i64 v, a; if (pop_int("poked", &a) && pop_int("poked", &v)) *(volatile u32 *)(u64)a = (u32)v; }
static void b_pokeq(void) { i64 v, a; if (pop_int("pokeq", &a) && pop_int("pokeq", &v)) *(volatile i64 *)(u64)a = v; }
static void b_in(void) { i64 p; if (pop_int("in", &p)) push(I(inb((u16)p))); }
static void b_out(void) { i64 v, p; if (pop_int("out", &p) && pop_int("out", &v)) outb((u16)p, (u8)v); }

static u32 inl(u16 p) { u32 v; __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(p)); return v; }
static void outl(u16 p, u32 v) { __asm__ volatile("outl %0, %1" : : "a"(v), "Nd"(p)); }
static void b_ind(void) { i64 p; if (pop_int("ind", &p)) push(I(inl((u16)p))); }
static void b_outd(void) { i64 v, p; if (pop_int("outd", &p) && pop_int("outd", &v)) outl((u16)p, (u32)v); }

/* the cpu */

static void b_cpuid(void) {
    i64 leaf;
    if (!pop_int("cpuid", &leaf)) return;
    u32 a, b, c, d;
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"((u32)leaf), "c"(0));
    push(I(a)); push(I(b)); push(I(c)); push(I(d));
}

static void b_cpu(void) {
    char t[49];
    cpu_brand(t, sizeof t);
    push(S(str_new(t, (u32)strlen(t))));
}

static void b_rdtsc(void) {
    u32 lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    push(I((i64)(((u64)hi << 32) | lo)));
}

static void b_cr(void) {
    i64 n;
    u64 v = 0;
    if (!pop_int("cr", &n)) return;
    switch (n) {
    case 0: __asm__ volatile("mov %%cr0, %0" : "=r"(v)); break;
    case 2: __asm__ volatile("mov %%cr2, %0" : "=r"(v)); break;
    case 3: __asm__ volatile("mov %%cr3, %0" : "=r"(v)); break;
    case 4: __asm__ volatile("mov %%cr4, %0" : "=r"(v)); break;
    default: fail("cr only takes 0, 2, 3 or 4%s", ""); return;
    }
    push(I((i64)v));
}

static u32 pci_read(u32 bus, u32 dev, u32 fn, u32 off) {
    outl(0xCF8, 0x80000000u | bus << 16 | dev << 11 | fn << 8 | (off & 0xFC));
    return inl(0xCFC);
}

static void b_pci(void) {
    V devs[64];
    u32 n = 0;
    for (u32 bus = 0; bus < 256 && n < 64; bus++)
        for (u32 dev = 0; dev < 32 && n < 64; dev++)
            for (u32 fn = 0; fn < 8 && n < 64; fn++) {
                u32 id = pci_read(bus, dev, fn, 0);
                if ((id & 0xFFFF) == 0xFFFF) { if (!fn) break; continue; }
                u32 cls = pci_read(bus, dev, fn, 8);
                V f[7] = { I(bus), I(dev), I(fn), I(id & 0xFFFF), I(id >> 16), I(cls >> 24), I((cls >> 16) & 0xFF) };
                devs[n++] = Q(quote_new(f, 7));
                if (!fn && !(pci_read(bus, dev, 0, 12) >> 16 & 0x80)) break;   /* not multifunction */
            }
    push(Q(quote_new(devs, n)));
}

static void b_pciread(void) {
    i64 b, d, f, o;
    if (pop_int("pci@", &o) && pop_int("pci@", &f) && pop_int("pci@", &d) && pop_int("pci@", &b))
        push(I(pci_read((u32)b, (u32)d, (u32)f, (u32)o)));
}

static void b_memmap(void) {
    V ents[32];
    for (int i = 0; i < boot_nmmap; i++) {
        V e[3] = { I((i64)boot_mmap[i].base), I((i64)boot_mmap[i].len), I(boot_mmap[i].type) };
        ents[i] = Q(quote_new(e, 3));
    }
    push(Q(quote_new(ents, (u32)boot_nmmap)));
}

static void b_ticks(void) { push(I((i64)ticks())); }
static void b_hz(void) { push(I(HZ)); }
static void b_irq(void) { i64 n; if (pop_int("irq", &n)) push(I(n >= 0 && n < 16 ? (i64)irq_count((int)n) : 0)); }
static void b_idle(void) { push(I((i64)idle_count())); }
static void b_heap(void) { push(I((i64)heap_used())); push(I((i64)heap_total())); }
static void b_kernel(void) { push(I((i64)boot.kernel_bytes)); push(I((i64)boot.mem_k * 1024)); }

static void b_sleep(void) {
    i64 n;
    if (!pop_int("sleep", &n)) return;
    u64 until = ticks() + (u64)n;
    current->wake_at = until;
    while (ticks() < until && !kbreak) yield();
    current->wake_at = 0;
    brk();
}

static void b_tab(void) {
    i64 n;
    if (!pop_int("tab", &n)) return;
    if (n < 1 || n > NTABS) { fail("the tab number has to be 1 to 4%s", ""); return; }
    struct task *t = &tabs[n - 1];
    const char *st = t == current ? "running" : t->wait_key ? "waiting for keys" : t->wake_at ? "sleeping" : "ready";
    V f[3] = { S(str_new(t->title, (u32)strlen(t->title))), S(str_new(st, (u32)strlen(st))), I(n - 1 == active_tab) };
    push(Q(quote_new(f, 3)));
}

static void b_mytab(void) { push(I(current->id + 1)); }
static void b_title(void) { str *s; if (pop_str("title", &s)) set_title(s->s); }

static void b_stats(void) {
    struct omega_stats s;
    omega_stats(&s);
    push(I((i64)s.objects)); push(I((i64)s.gcs)); push(I((i64)s.bindings)); push(I((i64)s.stack)); push(I((i64)s.history));
}

static u8 cmos(u8 r) { outb(0x70, r); return inb(0x71); }
static u8 bcd(u8 v) { return (u8)((v & 0x0F) + (v >> 4) * 10); }

static void b_time(void) {
    while (cmos(0x0A) & 0x80) ;
    u8 s = cmos(0), m = cmos(2), h = cmos(4), d = cmos(7), mo = cmos(8), y = cmos(9), b = cmos(0x0B);
    int pm = h & 0x80;
    h &= 0x7F;
    if (!(b & 0x04)) { s = bcd(s); m = bcd(m); h = bcd(h); d = bcd(d); mo = bcd(mo); y = bcd(y); }
    if (!(b & 0x02) && pm) h = (u8)((h + 12) % 24);
    push(I(2000 + y)); push(I(mo)); push(I(d)); push(I(h)); push(I(m)); push(I(s));
}

static u64 seed;
static void b_rand(void) {
    i64 n;
    if (!pop_int("rand", &n)) return;
    if (n <= 0) { fail("rand needs a number above 0%s", ""); return; }
    if (!seed) { u32 lo, hi; __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi)); seed = ((u64)hi << 32 | lo) | 1; }
    seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
    push(I((i64)(seed % (u64)n)));
}

static void b_put(void) {
    i64 c, x, y, col;
    if (pop_int("put", &col) && pop_int("put", &y) && pop_int("put", &x) && pop_int("put", &c))
        vga_put((int)x, (int)y, (char)c, (u8)col);
}

static void b_text(void) {
    i64 x, y, col;
    str *s;
    if (pop_int("text", &col) && pop_int("text", &y) && pop_int("text", &x) && pop_str("text", &s))
        vga_puts_at((int)x, (int)y, s->s, (u8)col);
}

static void b_getch(void) {
    i64 x, y;
    if (!pop_int("screen@", &y) || !pop_int("screen@", &x)) return;
    if (x < 0 || x >= VW || y < 0 || y >= VH) { push(I(0)); push(I(0)); return; }
    u16 v = current->term.buf[y * VW + x];
    push(I(v & 0xFF)); push(I(v >> 8));
}

static void b_wipe(void) { vga_clear(); }
static void b_cursor(void) { i64 x, y; if (pop_int("cursor", &y) && pop_int("cursor", &x)) vga_setpos((int)x, (int)y); }
static void b_nocursor(void) { vga_cursor(0, VH); }
static void b_where(void) { int x, y; vga_getpos(&x, &y); push(I(x)); push(I(y)); }
static void b_width(void) { push(I(VW)); push(I(VH)); }

static void b_key(void) { push(I(kbd_get())); }
static void b_keyq(void) { push(I(kbd_poll())); }

static void b_waitkey(void) {
    i64 n;
    if (!pop_int("waitkey", &n)) return;
    current->wait_key = 1;
    current->wake_at = ticks() + (u64)n;
    int k;
    while ((k = kbd_poll()) < 0 && ticks() < current->wake_at) yield();
    current->wait_key = 0;
    current->wake_at = 0;
    push(I(k));
}

/* themes */

char logo_name[24] = "subverge";

static int has_logo(const char *n) {
    char w[40];
    snfmt(w, sizeof w, "logo-%s", n);
    return lookup(intern(w, (u32)strlen(w))) != NULL;
}

static void b_theme(void) {
    if (!need(1, "theme")) return;
    V v = pop();
    if (!want(v, T_SYM, "theme")) return;
    for (int i = 0; themes[i].name; i++)
        if (!strcmp(themes[i].name, name_of(v))) {
            /* a theme brings its own logo, if one is defined */
            if (has_logo(themes[i].name)) scopy(logo_name, themes[i].name, sizeof logo_name);
            else if (!strcmp(logo_name, themes[vga_current_theme()].name)) scopy(logo_name, "subverge", sizeof logo_name);
            vga_theme(i);
            persist_save_theme();
            return;
        }
    fail("%s isn't a theme, type themes to see them", name_of(v));
}

static void b_logo(void) {
    if (!need(1, "logo")) return;
    V v = pop();
    if (!want(v, T_SYM, "logo")) return;
    if (strlen(name_of(v)) >= sizeof logo_name || !has_logo(name_of(v))) { fail("logo-%s isn't defined, type logos to see the logos", name_of(v)); return; }
    scopy(logo_name, name_of(v), sizeof logo_name);
    persist_save_theme();
}

static void b_logoname(void) { push(S(str_new(logo_name, (u32)strlen(logo_name)))); }

static void list_logos(bind *d) {
    for (bind *b = d; b; b = b->next) {
        if (b->v.t == T_NONE || strncmp(b->name->s, "logo-", 5)) continue;
        int dup = 0;
        for (bind *c = d; c != b; c = c->next) if (c->name == b->name) dup = 1;
        if (dup) continue;
        const char *n = b->name->s + 5;
        int on = !strcmp(n, logo_name);
        vga_color(on ? C_MOON : C_TEXT);
        kprintf("  %c %s\n", on ? '>' : ' ', n);
    }
}

static void b_logos(void) {
    list_logos(root_dict);
    if (W->dict) list_logos(W->dict);
    vga_color(C_DIM);
    kputs("\n  Switch with 'haelide logo. A logo is a word named logo-<name>.\n");
    vga_color(C_TEXT);
}

static void b_themes(void) {
    for (int i = 0; themes[i].name; i++) {
        int on = i == vga_current_theme();
        vga_color(on ? C_MOON : C_TEXT);
        kprintf("  %c %s\n", on ? '>' : ' ', themes[i].name);
    }
    int x, y;
    vga_getpos(&x, &y);
    vga_color(C_DIM);
    kputs("\n  These colours, in the current theme:\n  ");
    vga_getpos(&x, &y);
    for (int c = 0; c < 16; c++) { vga_put(x + c * 3, y, (char)219, (u8)c); vga_put(x + c * 3 + 1, y, (char)219, (u8)c); }
    kputs("\n\n");
    vga_color(C_DIM);
    kputs("  Switch with 'raw theme. An installed system remembers the choice.\n");
    vga_color(C_TEXT);
}

static void b_themename(void) {
    const char *n = themes[vga_current_theme()].name;
    push(S(str_new(n, (u32)strlen(n))));
}

static void b_rgb(void) {
    i64 n, r, g, b;
    if (!pop_int("rgb", &b) || !pop_int("rgb", &g) || !pop_int("rgb", &r) || !pop_int("rgb", &n)) return;
    if (n < 0 || n > 15) { fail("rgb takes a colour from 0 to 15%s", ""); return; }
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) { fail("rgb takes red, green and blue from 0 to 255%s", ""); return; }
    vga_rgb((int)n, (u32)(r << 16 | g << 8 | b));
}

static void b_sysc(void) { sysinfo(); }

static void b_reboot(void) {
    while (inb(0x64) & 2) ;
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile("hlt");
}

static void b_halt(void) {
    vga_color(C_DIM);
    kputs("  the machine is halted\n");
    __asm__ volatile("cli");
    for (;;) __asm__ volatile("hlt");
}

const struct builtin hw_words[] = {
    { "peek", b_peek }, { "peekd", b_peekd }, { "peekq", b_peekq },
    { "poke", b_poke }, { "poked", b_poked }, { "pokeq", b_pokeq },
    { "in", b_in }, { "out", b_out }, { "ind", b_ind }, { "outd", b_outd },
    { "cpuid", b_cpuid }, { "cpu", b_cpu }, { "rdtsc", b_rdtsc }, { "cr", b_cr },
    { "pci", b_pci }, { "pci@", b_pciread }, { "memmap", b_memmap },
    { "ticks", b_ticks }, { "hz", b_hz }, { "irq", b_irq }, { "idle", b_idle }, { "heap", b_heap },
    { "kernel", b_kernel }, { "sleep", b_sleep }, { "tab", b_tab }, { "mytab", b_mytab },
    { "title", b_title }, { "stats", b_stats }, { "time", b_time }, { "rand", b_rand },
    { "put", b_put }, { "text", b_text }, { "screen@", b_getch }, { "wipe", b_wipe },
    { "cursor", b_cursor }, { "nocursor", b_nocursor }, { "where", b_where }, { "size", b_width },
    { "key", b_key }, { "key?", b_keyq }, { "waitkey", b_waitkey },
    { "theme", b_theme }, { "themes", b_themes }, { "theme@", b_themename }, { "rgb", b_rgb },
    { "logo", b_logo }, { "logos", b_logos }, { "logo@", b_logoname },
    { "sys-c", b_sysc }, { "reboot", b_reboot }, { "halt", b_halt },
    { NULL, NULL }
};
