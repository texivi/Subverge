#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;

#define CTRL(c) ((c) & 0x1f)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *d, const void *s, size_t n);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int memcmp(const void *a, const void *b, size_t n);
int strncmp(const char *a, const char *b, size_t n);
const char *strstr(const char *h, const char *n);
void scopy(char *d, const char *s, size_t cap);
int vsnfmt(char *b, size_t n, const char *f, va_list ap);
int snfmt(char *b, size_t n, const char *f, ...);

static inline void outb(u16 p, u8 v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(p)); }
static inline u8 inb(u16 p) { u8 v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(p)); return v; }
