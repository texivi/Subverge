#pragma once
#include "lib.h"

#define LINE_MAX 200
#define LINE_HIST 16

struct history { char h[LINE_HIST][LINE_MAX + 1]; int n, complete, shell; };

struct keydoc { int key; const char *name, *doc; };
extern const struct keydoc line_keys[];
extern const int n_line_keys;
extern void (*line_redraw)(void);

int readline(char *b, struct history *h);
/* completion lookup */
int complete_name(const char *pre, int n, int nth, char *out, int cap, int *lcp);
void key_name(int k, char *o, int cap);
void keys_edit(char *b, int *len, int *pos, int room);
void keys_describe(int k1, int k2);
void keys_each(const char *ctx, void (*f)(const char *key, const char *doc, void *u), void *u);
void doc_show(const char *name);
int minibuf(const char *label, char *out, int complete);
