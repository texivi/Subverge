#pragma once
#include "lib.h"

struct drive {
    int present, slave, lba48;
    u16 io, ctl;
    u64 sectors;
    char model[41];
};
extern struct drive drives[4];

void disk_init(void);
int disk_rw(int n, u64 lba, u32 count, void *buf, int write);

#define LBA_STAGE2   1
#define LBA_KERNEL   64
#define LBA_STATE    8192
#define SLOT_SECTORS 2048
#define PHI_SECTORS  1024
#define MIN_SECTORS  (LBA_STATE + 1 + SLOT_SECTORS * 5 + PHI_SECTORS)

int persist_home(void);
void persist_init(void);
char *persist_load(int slot, u32 *len);
void persist_save_tab(int tab);
void persist_save_root(void);
void persist_save_phi(void);
void persist_save_theme(void);
