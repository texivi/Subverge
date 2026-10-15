#include "cpu.h"
#include "mem.h"
#include "sv.h"
#include "vga.h"

static const char *const moon[7] = {
    "   _.--._", " .'  .-'", "/   /  ", "|   |  ", "\\   \\  ", " '.  '-.__.-'", "   '-.____.-'"
};
static const char *const cat[7] = { "", "", "/\\_/\\", "( -.-)", "(___)~", "", "" };

#define STATS_X 18

static void uptime_str(char *o, int cap) {
    u64 s = ticks() / HZ;
    snfmt(o, (size_t)cap, "%luh %lum %lus", s / 3600, s / 60 % 60, s % 60);
}

static void stat(const char *label, const char *val) {
    vga_color(C_MOON);
    kprintf("%-9s", label);
    vga_color(C_TEXT);
    kputs(val);
}

void sysinfo(void) {
    char up[24], cpu[49], mem[32], kern[32];
    u64 used_k = (boot.kernel_bytes + heap_used()) / 1024;
    uptime_str(up, sizeof up);
    cpu_brand(cpu, sizeof cpu);
    if (used_k < 1024) snfmt(mem, sizeof mem, "%luK / %luM", used_k, boot.mem_k / 1024);
    else snfmt(mem, sizeof mem, "%lu.%luM / %luM", used_k / 1024, used_k % 1024 * 10 / 1024, boot.mem_k / 1024);
    snfmt(kern, sizeof kern, "%s %s %s", KERNEL_NAME, KERNEL_VERSION, ARCH);

    kputc('\n');
    for (int i = 0; i < 7; i++) {
        int x, y;
        kputs("  ");
        vga_color(C_MOON);
        kputs(moon[i]);
        vga_color(C_CAT);
        kputs(cat[i]);
        vga_getpos(&x, &y);
        vga_setpos(STATS_X, y);
        switch (i) {
        case 0:
            vga_color(C_CAT);
            kprintf("%s@%s", USER_NAME, HOST_NAME);
            break;
        case 1: stat("os", OS_NAME " " OS_VERSION); break;
        case 2: stat("kernel", kern); break;
        case 3: stat("uptime", up); break;
        case 4: stat("shell", SHELL_NAME " " SHELL_VERSION); break;
        case 5: stat("cpu", cpu); break;
        case 6: stat("memory", mem); break;
        }
        kputc('\n');
    }
    static const u8 dots[8] = { C_DIM, C_CAT, C_TEAL, C_PINK, C_BLUE, C_MOON, C_LGREEN, C_LRED };
    int x, y;
    kputc('\n');
    vga_getpos(&x, &y);
    vga_setpos(STATS_X, y);
    for (int i = 0; i < 8; i++) {
        vga_color(dots[i]);
        kputs("\x07 ");
    }
    vga_color(C_TEXT);
    kputs("\n\n");
}
