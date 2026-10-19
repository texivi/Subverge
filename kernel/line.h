#pragma once
#include "lib.h"

#define LINE_MAX 200
#define LINE_HIST 16

struct history { char h[LINE_HIST][LINE_MAX + 1]; int n, complete; };

int readline(char *b, struct history *h);
/* nth match for a prefix; returns the count, *lcp the common length */
int complete_name(const char *pre, int n, int nth, char *out, int cap, int *lcp);
