#include "cpu.h"
#include "task.h"
#include "vga.h"

struct idt_e { u16 off_lo, sel; u8 ist, flags; u16 off_mid; u32 off_hi, zero; } __attribute__((packed));
struct dptr { u16 lim; u64 base; } __attribute__((packed));

static struct idt_e idt[256];
static struct dptr ip;
static void (*irqs[16])(void);
static volatile u64 irqn[16], last_rip, idle_ticks, tick_count;

extern void idt_load(struct dptr *);
extern u64 isr_table[49];

static const char *const exc[32] = {
    "divide by zero", "debug", "nmi", "breakpoint", "overflow", "bound range", "invalid opcode",
    "no fpu", "double fault", "coprocessor overrun", "bad tss", "segment not present", "stack fault",
    "general protection fault", "page fault", "reserved", "fpu error", "alignment check", "machine check",
    "simd error", "virtualization", "control protection", "reserved", "reserved", "reserved", "reserved",
    "reserved", "reserved", "hypervisor injection", "vmm communication", "security", "reserved"
};

void cpu_init(void) {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xF8); outb(0xA1, 0xEF);   /* timer, keyboard, cascade, mouse */

    for (int i = 0; i < 49; i++) {
        u64 a = isr_table[i];
        idt[i] = (struct idt_e){ (u16)a, 0x08, 0, 0x8E, (u16)(a >> 16), (u32)(a >> 32), 0 };
    }
    ip = (struct dptr){ sizeof idt - 1, (u64)idt };
    idt_load(&ip);
}

void irq_set(int irq, void (*fn)(void)) { irqs[irq] = fn; }

void panic(const char *msg) {
    __asm__ volatile("cli");
    u8 a = ATTR(C_BG, C_LRED);
    for (int y = 0; y <= VH; y++) for (int x = 0; x < VW; x++) hw_put(x, y, ' ', a);
    const char *lines[3] = { "panic", msg, "halted" };
    for (int i = 0; i < 3; i++)
        for (int x = 0; lines[i][x] && x < VW - 2; x++) hw_put(2 + x, 1 + i * 2, lines[i][x], a);
    hw_cursor(0, VH + 1);
    for (;;) __asm__ volatile("hlt");
}

u64 isr_handler(struct regs *r) {
    if (r->int_no < 32) {
        char m[96];
        u64 cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        if (r->int_no == 14) snfmt(m, sizeof m, "page fault at 0x%lx (rip 0x%lx)", cr2, r->rip);
        else snfmt(m, sizeof m, "%s at rip 0x%lx", exc[r->int_no], r->rip);
        return task_fault(r, m);
    }
    if (r->int_no == 48) return schedule(r);   /* yield */
    int irq = (int)r->int_no - 32;
    last_rip = r->rip;
    irqn[irq]++;
    if (irqs[irq]) irqs[irq]();
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
    return irq == 0 ? schedule(r) : (u64)r;
}

static void cpuid(u32 leaf, u32 *a, u32 *b, u32 *c, u32 *d) {
    __asm__ volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(leaf), "c"(0));
}

void cpu_brand(char *out, int cap) {
    u32 r[12], a, b, c, d;
    char *s = (char *)r;
    cpuid(0x80000000, &a, &b, &c, &d);
    if (a >= 0x80000004) {
        for (u32 i = 0; i < 3; i++) cpuid(0x80000002 + i, &r[i * 4], &r[i * 4 + 1], &r[i * 4 + 2], &r[i * 4 + 3]);
        s[47] = 0;
    } else {
        cpuid(0, &a, &r[0], &r[2], &r[1]);
        s[12] = 0;
    }
    while (*s == ' ') s++;
    scopy(out, s, (size_t)cap);
}

/* PIT timer */

static void on_tick(void) {
    tick_count++;
    if (*(volatile u8 *)(last_rip - 1) == 0xF4) idle_ticks++;
    if (tick_count % HZ == 0) draw_tabbar();
}

u64 ticks(void) { return tick_count; }
u64 idle_count(void) { return idle_ticks; }
u64 irq_count(int irq) { return irqn[irq]; }

void timer_init(u32 hz) {
    u32 div = 1193182 / hz;
    outb(0x43, 0x36);
    outb(0x40, (u8)(div & 0xFF));
    outb(0x40, (u8)(div >> 8));
    irq_set(0, on_tick);
}
