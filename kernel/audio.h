#pragma once

struct song { const char *name, *title, *by; int bpm; const char *tracks[6]; };
extern const struct song songs[];
extern const int nsongs;
