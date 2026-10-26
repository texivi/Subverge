#include "disk.h"
#include "kbd.h"
#include "line.h"
#include "mem.h"
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

extern char _kernel_start[], _data_start[], _data_end[], _kernel_end[];
extern char _binary_build_mbr_bin_start[], _binary_build_stage2_bin_start[];
extern void _start(void);

u32 omega_source(struct omega *om, char *buf, u32 cap);
u32 root_source(char *buf, u32 cap);

#define NSLOTS (1 + NTABS)
struct state_header { char magic[8]; u32 len[NSLOTS]; u32 sum[NSLOTS]; u64 saves; u32 theme; char logo[24]; u32 xlen, xsum; u8 pad[512 - 8 - 8 * NSLOTS - 8 - 4 - 24 - 8]; };
static int home = -1;
static struct state_header hdr;
#define PHI_SLOT NSLOTS

static u32 *lenp(int s) { return s < NSLOTS ? &hdr.len[s] : &hdr.xlen; }
static u32 *sump(int s) { return s < NSLOTS ? &hdr.sum[s] : &hdr.xsum; }
static u32 slot_cap(int s) { return (s < NSLOTS ? SLOT_SECTORS : PHI_SECTORS) * 512u - 1; }
_Static_assert(sizeof(struct state_header) == 512, "header is one sector");

static char snapshot[256 * 1024];
static u64 snapshot_len;

int persist_home(void) { return home; }

void persist_snapshot(void) {
    snapshot_len = (u64)(_data_end - _data_start);
    if (snapshot_len <= sizeof snapshot) memcpy(snapshot, _data_start, snapshot_len);
    else snapshot_len = 0;
}

static u32 checksum(const char *p, u32 n) {
    u32 h = 2166136261u;
    while (n--) h = (h ^ (u8)*p++) * 16777619u;
    return h;
}

static int is_home(int n) {
    char s[512];
    return !disk_rw(n, LBA_STAGE2, 1, s, 0) && !memcmp(s + 8, "SUBVERGE", 8);
}

void persist_init(void) {
    disk_init();
    for (int n = 0; n < 4 && home < 0; n++)
        if (drives[n].present && is_home(n)) home = n;
    if (home < 0) return;
    if (disk_rw(home, LBA_STATE, 1, &hdr, 0) || memcmp(hdr.magic, "SVSTATE1", 8)) {
        memset(&hdr, 0, sizeof hdr);
        memcpy(hdr.magic, "SVSTATE1", 8);
    }
    vga_theme((int)hdr.theme);
    if (hdr.logo[0]) scopy(logo_name, hdr.logo, sizeof logo_name);
}

void persist_save_theme(void) {
    if (home < 0) return;
    u64 f = irq_save();
    hdr.theme = (u32)vga_current_theme();
    scopy(hdr.logo, logo_name, sizeof hdr.logo);
    struct state_header copy = hdr;
    irq_restore(f);
    disk_rw(home, LBA_STATE, 1, &copy, 1);
}

char *persist_load(int slot, u32 *len) {
    if (home < 0 || !*lenp(slot) || *lenp(slot) > slot_cap(slot)) return NULL;
    u32 sectors = (*lenp(slot) + 511) / 512;
    char *buf = kmalloc((u64)sectors * 512 + 1);
    if (disk_rw(home, LBA_STATE + 1 + (u64)slot * SLOT_SECTORS, sectors, buf, 0) || checksum(buf, *lenp(slot)) != *sump(slot)) {
        kfree(buf);
        return NULL;
    }
    buf[*lenp(slot)] = 0;
    *len = *lenp(slot);
    return buf;
}

static int save_slot(int slot, char *buf, u32 len) {
    if (home < 0) return -1;
    u32 sectors = (len + 511) / 512;
    memset(buf + len, 0, (u64)sectors * 512 - len);
    if (sectors && disk_rw(home, LBA_STATE + 1 + (u64)slot * SLOT_SECTORS, sectors, buf, 1)) return -1;
    u64 f = irq_save();
    *lenp(slot) = len;
    *sump(slot) = checksum(buf, len);
    hdr.saves++;
    struct state_header copy = hdr;
    irq_restore(f);
    return disk_rw(home, LBA_STATE, 1, &copy, 1);
}

void persist_save_tab(int tab) {
    if (home < 0 || !tabs[tab].om) return;
    char *buf = kmalloc_try(SLOT_SECTORS * 512);
    if (!buf) return;
    u32 len = omega_source(tabs[tab].om, buf, SLOT_SECTORS * 512 - 512);
    if (len) save_slot(1 + tab, buf, len);
    kfree(buf);
}

void persist_save_phi(void) {
    if (home < 0) return;
    char *buf = kmalloc_try(PHI_SECTORS * 512);
    if (!buf) return;
    u32 len = phi_dump(buf, PHI_SECTORS * 512 - 512);
    save_slot(PHI_SLOT, buf, len);
    kfree(buf);
}

void persist_save_root(void) {
    if (home < 0) return;
    char *buf = kmalloc_try(SLOT_SECTORS * 512);
    if (!buf) return;
    u32 len = root_source(buf, SLOT_SECTORS * 512 - 512);
    if (len) save_slot(0, buf, len);
    kfree(buf);
}

/* words */

static void b_drives(void) {
    V out[4];
    u32 k = 0;
    for (int n = 0; n < 4; n++) {
        if (!drives[n].present) continue;
        V f[4] = { I(n), S(str_new(drives[n].model, (u32)strlen(drives[n].model))), I((i64)(drives[n].sectors / 2048)), I(n == home) };
        out[k++] = Q(quote_new(f, 4));
    }
    push(Q(quote_new(out, k)));
}

static void b_home(void) { push(I(home)); }

static void b_ask(void) {
    str *p;
    if (!pop_str("ask", &p)) return;
    static struct history none[NTABS];
    char line[LINE_MAX + 1];
    vga_color(C_MOON);
    kputs(p->s);
    vga_color(C_TEXT);
    int n = readline(line, &none[current->id < 0 ? 0 : current->id]);
    if (n < 0) { fail("cancelled%s", ""); return; }
    push(S(str_new(line, (u32)n)));
}

static void step(const char *what) { vga_color(C_DIM); kprintf("  %s\n", what); vga_color(C_TEXT); }

static void b_install_to(void) {
    i64 n;
    if (!pop_int("install-to", &n)) return;
    if (n < 0 || n > 3 || !drives[n].present) { fail("there is no such drive%s", ""); return; }
    if (drives[n].sectors < MIN_SECTORS) { fail("that drive is too small, it needs at least 10M%s", ""); return; }
    if (!snapshot_len) { fail("the kernel's data couldn't be copied%s", ""); return; }

    u64 text = (u64)(_data_start - _kernel_start), total = text + snapshot_len;
    u32 ksectors = (u32)((total + 511) / 512);
    if (ksectors > LBA_STATE - LBA_KERNEL) { fail("the kernel doesn't fit in its slot%s", ""); return; }
    char *img = kzalloc((u64)ksectors * 512);
    memcpy(img, _kernel_start, text);
    memcpy(img + text, snapshot, snapshot_len);

    char *s2 = kmalloc(8192);
    memcpy(s2, _binary_build_stage2_bin_start, 8192);
    *(u32 *)(s2 + 16) = LBA_KERNEL;
    *(u32 *)(s2 + 20) = ksectors;
    *(u32 *)(s2 + 24) = (u32)(u64)_kernel_start;
    *(u32 *)(s2 + 28) = (u32)(u64)_kernel_end;
    *(u32 *)(s2 + 32) = (u32)(u64)_start;

    int bad = 0;
    bad |= disk_rw((int)n, 0, 1, _binary_build_mbr_bin_start, 1);
    bad |= disk_rw((int)n, LBA_STAGE2, 16, s2, 1);
    bad |= disk_rw((int)n, LBA_KERNEL, ksectors, img, 1);
    kfree(img);
    kfree(s2);

    memset(&hdr, 0, sizeof hdr);
    memcpy(hdr.magic, "SVSTATE1", 8);
    hdr.theme = (u32)vga_current_theme();
    scopy(hdr.logo, logo_name, sizeof hdr.logo);
    bad |= disk_rw((int)n, LBA_STATE, 1, &hdr, 1);
    if (bad) { fail("writing to the drive failed%s", ""); return; }

    home = (int)n;
    persist_save_root();
    for (int i = 0; i < NTABS; i++) persist_save_tab(i);
    persist_save_phi();
    vga_color(C_LGREEN);
    kputs("  installed\n");
    vga_color(C_TEXT);
}

static void b_save(void) {
    if (home < 0) { fail("you haven't installed yet, run install%s", ""); return; }
    persist_save_root();
    for (int i = 0; i < NTABS; i++) persist_save_tab(i);
    persist_save_phi();
    step("saved to the drive");
}

const struct builtin persist_words[] = {
    { "drives", b_drives }, { "home", b_home }, { "ask", b_ask }, { "install-to", b_install_to }, { "save", b_save },
    { NULL, NULL }
};
