#include "cpu.h"
#include "mem.h"
#include "sv.h"
#include "task.h"

#define KSTACK (256 * 1024)

struct task tabs[NTABS];
static struct task idle_task;
struct task *current = &idle_task;
int active_tab;

static u64 frame_for(struct task *t) {
    u64 top = ((u64)t->stack + KSTACK) & ~15ull;
    struct regs *r = (struct regs *)(top - 16 - sizeof(struct regs));
    memset(r, 0, sizeof *r);
    r->rip = (u64)t->entry;
    r->cs = 0x08;
    r->rflags = 0x202;          /* interrupts on */
    r->rsp = top - 8;
    r->ss = 0x10;
    return (u64)r;
}

void task_start(struct task *t, void (*entry)(void)) {
    t->entry = entry;
    if (!t->stack) t->stack = kmalloc(KSTACK);
    t->rsp = frame_for(t);
}

void sched_init(void) {
    idle_task.id = -1;
    for (int i = 0; i < NTABS; i++) {
        tabs[i].id = i;
        tabs[i].term.attr = ATTR(C_TEXT, C_BG);
        tabs[i].term.h = VH;
        for (int k = 0; k < VW * VH; k++) tabs[i].term.buf[k] = ' ' | (u16)ATTR(C_TEXT, C_BG) << 8;
        scopy(tabs[i].title, SHELL_NAME, sizeof tabs[i].title);
    }
}

static int runnable(struct task *t) {
    if (!t->entry) return 0;
    if (!t->wait_key && !t->wake_at) return 1;
    if (t->wait_key && t->kh != t->kt) return 1;
    if (t->wake_at && ticks() >= t->wake_at) return 1;
    return 0;
}

u64 schedule(struct regs *r) {
    current->rsp = (u64)r;
    int start = current->id < 0 ? NTABS - 1 : current->id;
    for (int k = 1; k <= NTABS; k++) {
        struct task *t = &tabs[(start + k) % NTABS];
        if (runnable(t)) { current = t; return t->rsp; }
    }
    current = &idle_task;
    return idle_task.rsp;
}

u64 task_fault(struct regs *r, const char *msg) {
    if (current->id < 0) panic(msg);
    (void)r;
    scopy(current->fault, msg, sizeof current->fault);
    current->wait_key = 0;
    current->wake_at = 0;
    current->rsp = frame_for(current);
    return current->rsp;
}

#ifdef HOST
void host_yield(void);
void yield(void) { host_yield(); }
#else
void yield(void) { __asm__ volatile("int $48"); }
#endif

void sleep_until(u64 tick) {
    current->wake_at = tick;
    while (ticks() < tick) yield();
    current->wake_at = 0;
}
