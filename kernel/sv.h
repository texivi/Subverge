#pragma once
#include "lib.h"

#define OS_NAME     "Subverge"
#define HOST_NAME   "subverge"
#define KERNEL_NAME "ordinal"
#define SHELL_NAME  "omega"
#define OS_VERSION     "0.8.0"
#define KERNEL_VERSION "0.10.0"
#define SHELL_VERSION  "0.9.0"
#define ARCH        "x86_64"
#define USER_NAME   "user"

struct bootinfo {
    u64 mem_k;
    u64 kernel_bytes;
    const char *loader;
};
extern struct bootinfo boot;

void sysinfo(void);
char *scrawl(const char *name, const char *text, u32 len, u32 *outlen);
void omega_main(void);

struct omega_stats { u64 objects, gcs, bindings, stack, history; };
void omega_stats(struct omega_stats *s);

void calc(void);
void font_init(void);
void omega_init_world(void);
void eval_line(const char *line);
