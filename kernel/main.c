#include "cpu.h"
#include "disk.h"
#include "kbd.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

extern char _kernel_start[], _kernel_end[];
struct bootinfo boot = { 0, 0, "unknown" };
struct mmap_entry { u64 base, len; u32 type; };
extern struct mmap_entry boot_mmap[32];
extern int boot_nmmap;

static void read_multiboot(u32 magic, u32 *mb) {
    boot.mem_k = 64 * 1024;
    if (magic != 0x2BADB002) return;
    if (mb[0] & 1) boot.mem_k = (u64)mb[1] + mb[2] + 384;
    if (mb[0] & (1 << 9)) boot.loader = (const char *)(u64)mb[16];
    if (mb[0] & (1 << 6)) {
        u8 *p = (u8 *)(u64)mb[12], *end = p + mb[11];
        while (p < end && boot_nmmap < 32) {
            u32 size = *(u32 *)p;
            boot_mmap[boot_nmmap++] = (struct mmap_entry){ *(u64 *)(p + 4), *(u64 *)(p + 12), *(u32 *)(p + 20) };
            p += size + 4;
        }
    }
}

void persist_snapshot(void);

void kmain(u32 magic, u64 mbaddr) {
    persist_snapshot();
    vga_init();
    font_init();
    cpu_init();
    read_multiboot(magic, (u32 *)mbaddr);
    boot.kernel_bytes = (u64)(_kernel_end - _kernel_start);

    u64 heap_start = (u64)_kernel_end, ram_end = 0x100000 + (boot.mem_k - 1024) * 1024;
    heap_init(heap_start, MIN(ram_end - heap_start, 64ull * 1024 * 1024));

    sched_init();
    persist_init();
    omega_init_world();
    register_words(core_words);
    register_words(hw_words);
    register_words(gfx_words);
    register_words(img_words);
    register_words(ord_words);
    register_words(root_words);
    register_words(persist_words);
    register_words(jit_words);
    register_words(dis_words);
    register_words(help_words);
    register_words(doc_words);
    register_words(mouse_words);
    ord_init();
    root_boot();

    for (int i = 0; i < NTABS; i++) task_start(&tabs[i], omega_main);
    switch_tab(0);
    timer_init(HZ);
    kbd_init();
    mouse_init();

    __asm__ volatile("sti");
    for (;;) __asm__ volatile("hlt");
}
