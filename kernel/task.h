#pragma once
#include "lib.h"
#include "vga.h"

#define NTABS 4

struct regs {
    u64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 int_no, err;
    u64 rip, cs, rflags, rsp, ss;
};

struct term {
    u16 buf[VW * VH];
    int cx, cy, curx, cury, activity;
    u8 attr;
};

struct omega;
struct scrawl_state;
struct calc_state;

struct task {
    u64 rsp;
    int id;
    void (*entry)(void);
    u8 *stack;
    int wait_key;
    u64 wake_at;
    volatile int kb[64];
    volatile int kh, kt;
    volatile int brkflag;
    struct term term;
    char title[24];
    char fault[96];
    struct omega *om;
    struct scrawl_state *ed;
    struct calc_state *calc;
};

extern struct task *current;
extern struct task tabs[NTABS];
extern int active_tab;

void sched_init(void);
void task_start(struct task *t, void (*entry)(void));
u64 schedule(struct regs *r);
u64 task_fault(struct regs *r, const char *msg);
void yield(void);
void sleep_until(u64 tick);
void set_title(const char *t);
void switch_tab(int n);
void draw_tabbar(void);

#ifdef HOST
static inline u64 irq_save(void) { return 0; }
static inline void irq_restore(u64 f) { (void)f; }
#else
static inline u64 irq_save(void) { u64 f; __asm__ volatile("pushfq; pop %0; cli" : "=r"(f) : : "memory"); return f; }
static inline void irq_restore(u64 f) { __asm__ volatile("push %0; popfq" : : "r"(f) : "memory", "cc"); }
#endif
