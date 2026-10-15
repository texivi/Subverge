#pragma once
#include "lib.h"

void cpu_init(void);
void irq_set(int irq, void (*fn)(void));
void panic(const char *msg) __attribute__((noreturn));
void cpu_brand(char *out, int cap);

/* timer */
void timer_init(u32 hz);
u64 ticks(void);
u64 idle_count(void);
u64 irq_count(int irq);
#define HZ 100
