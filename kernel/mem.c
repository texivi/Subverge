#include "mem.h"
#include "cpu.h"
#include "task.h"

typedef struct blk { u64 size; u32 free, pad; struct blk *next; u64 pad2; } blk;

static blk *head;
static u64 used, total;

void heap_init(u64 start, u64 size) {
    u64 a = (start + 15) & ~15ull;
    size -= a - start;
    head = (blk *)a;
    head->size = size - sizeof(blk);
    head->free = 1;
    head->next = NULL;
    total = size;
    used = 0;
}

static void merge(blk *b) {
    while (b->next && b->next->free && (u8 *)(b + 1) + b->size == (u8 *)b->next) {
        b->size += sizeof(blk) + b->next->size;
        b->next = b->next->next;
    }
}

void *kmalloc_try(u64 n) {
    n = (n + 15) & ~15ull;
    if (!n) n = 16;
    u64 f = irq_save();
    for (blk *b = head; b; b = b->next) {
        if (!b->free) continue;
        merge(b);
        if (b->size < n) continue;
        if (b->size >= n + sizeof(blk) + 16) {
            blk *nb = (blk *)((u8 *)(b + 1) + n);
            nb->size = b->size - n - sizeof(blk);
            nb->free = 1;
            nb->next = b->next;
            b->next = nb;
            b->size = n;
        }
        b->free = 0;
        used += b->size + sizeof(blk);
        irq_restore(f);
        return b + 1;
    }
    irq_restore(f);
    return NULL;
}

void *kmalloc(u64 n) {
    void *p = kmalloc_try(n);
    if (!p) panic("ran out of memory");
    return p;
}

void *kzalloc(u64 n) {
    void *p = kmalloc(n);
    memset(p, 0, n);
    return p;
}

void kfree(void *p) {
    if (!p) return;
    u64 f = irq_save();
    blk *b = (blk *)p - 1;
    if (b->free) panic("double free");
    b->free = 1;
    used -= b->size + sizeof(blk);
#ifdef GC_STRESS
    memset(p, 0xDD, b->size);
#endif
    merge(b);
    irq_restore(f);
}

void *krealloc(void *p, u64 n) {
    if (!p) return kmalloc(n);
    blk *b = (blk *)p - 1;
    if (b->size >= n) return p;
    void *q = kmalloc(n);
    memcpy(q, p, b->size);
    kfree(p);
    return q;
}

u64 heap_used(void) { return used; }
u64 heap_total(void) { return total; }
