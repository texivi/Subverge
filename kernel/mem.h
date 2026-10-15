#pragma once
#include "lib.h"

void heap_init(u64 start, u64 size);
void *kmalloc(u64 n);
void *kmalloc_try(u64 n);
void *kzalloc(u64 n);
void kfree(void *p);
void *krealloc(void *p, u64 n);
u64 heap_used(void);
u64 heap_total(void);
