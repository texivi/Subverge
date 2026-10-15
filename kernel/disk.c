#include "disk.h"
#include "task.h"

struct drive drives[4];

static u16 inw(u16 p) { u16 v; __asm__ volatile("inw %1, %0" : "=a"(v) : "Nd"(p)); return v; }
static void outw(u16 p, u16 v) { __asm__ volatile("outw %0, %1" : : "a"(v), "Nd"(p)); }

static void delay(u16 ctl) { for (int i = 0; i < 4; i++) inb(ctl); }

static int wait(struct drive *d, int drq) {
    for (u32 i = 0; i < 2000000; i++) {
        u8 s = inb(d->io + 7);
        if (s & 0x80) continue;
        if (s & 0x21) return -1;
        if (!drq || (s & 0x08)) return 0;
    }
    return -1;
}

static void identify(struct drive *d) {
    outb(d->ctl, 0x02);
    outb(d->io + 6, (u8)(0xA0 | d->slave << 4));
    delay(d->ctl);
    outb(d->io + 2, 0); outb(d->io + 3, 0); outb(d->io + 4, 0); outb(d->io + 5, 0);
    outb(d->io + 7, 0xEC);
    if (inb(d->io + 7) == 0) return;  /* nothing there */
    for (u32 i = 0; i < 1000000 && (inb(d->io + 7) & 0x80); i++) ;
    if (inb(d->io + 4) || inb(d->io + 5)) return;
    if (wait(d, 1)) return;
    u16 id[256];
    for (int i = 0; i < 256; i++) id[i] = inw(d->io);
    for (int i = 0; i < 20; i++) { d->model[i * 2] = (char)(id[27 + i] >> 8); d->model[i * 2 + 1] = (char)id[27 + i]; }
    d->model[40] = 0;
    for (int i = 39; i >= 0 && d->model[i] == ' '; i--) d->model[i] = 0;
    d->lba48 = (id[83] >> 10) & 1;
    d->sectors = d->lba48 ? (u64)id[100] | (u64)id[101] << 16 | (u64)id[102] << 32 | (u64)id[103] << 48
                          : (u64)id[60] | (u64)id[61] << 16;
    d->present = d->sectors > 0;
}

void disk_init(void) {
    static const u16 io[2] = { 0x1F0, 0x170 }, ctl[2] = { 0x3F6, 0x376 };
    for (int i = 0; i < 4; i++) {
        drives[i] = (struct drive){ 0 };
        drives[i].io = io[i / 2];
        drives[i].ctl = ctl[i / 2];
        drives[i].slave = i & 1;
        if (inb(drives[i].io + 7) == 0xFF) continue;
        identify(&drives[i]);
    }
}

int disk_rw(int n, u64 lba, u32 count, void *buf, int write) {
    if (n < 0 || n > 3 || !drives[n].present || lba + count > drives[n].sectors) return -1;
    struct drive *d = &drives[n];
    u16 *p = buf;
    while (count) {
        u32 k = MIN(count, 128u);
        u64 f = irq_save();
        if (wait(d, 0)) { irq_restore(f); return -1; }
        if (d->lba48) {
            outb(d->io + 6, (u8)(0x40 | d->slave << 4));
            outb(d->io + 2, (u8)(k >> 8));
            outb(d->io + 3, (u8)(lba >> 24)); outb(d->io + 4, (u8)(lba >> 32)); outb(d->io + 5, (u8)(lba >> 40));
            outb(d->io + 2, (u8)k);
            outb(d->io + 3, (u8)lba); outb(d->io + 4, (u8)(lba >> 8)); outb(d->io + 5, (u8)(lba >> 16));
            outb(d->io + 7, write ? 0x34 : 0x24);
        } else {
            outb(d->io + 6, (u8)(0xE0 | d->slave << 4 | ((lba >> 24) & 0x0F)));
            outb(d->io + 2, (u8)k);
            outb(d->io + 3, (u8)lba); outb(d->io + 4, (u8)(lba >> 8)); outb(d->io + 5, (u8)(lba >> 16));
            outb(d->io + 7, write ? 0x30 : 0x20);
        }
        for (u32 s = 0; s < k; s++) {
            delay(d->ctl);
            if (wait(d, 1)) { irq_restore(f); return -1; }
            for (int i = 0; i < 256; i++) {
                if (write) outw(d->io, *p++);
                else *p++ = inw(d->io);
            }
        }
        if (write) {
            outb(d->io + 7, d->lba48 ? 0xEA : 0xE7);
            if (wait(d, 0)) { irq_restore(f); return -1; }
        }
        irq_restore(f);
        lba += k;
        count -= k;
    }
    return 0;
}
