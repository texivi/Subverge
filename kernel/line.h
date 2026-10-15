#pragma once
#include "lib.h"

#define LINE_MAX 200
#define LINE_HIST 16

struct history { char h[LINE_HIST][LINE_MAX + 1]; int n; };

int readline(char *b, struct history *h);
