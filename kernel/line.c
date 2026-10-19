#include "kbd.h"
#include "line.h"
#include "vga.h"

static void remember(struct history *h, const char *b) {
    if (!*b) return;
    if (h->n && !strcmp(h->h[(h->n - 1) % LINE_HIST], b)) return;
    scopy(h->h[h->n++ % LINE_HIST], b, LINE_MAX + 1);
}

static int stop(char c) { return c == ' ' || c == '[' || c == ']' || c == '{' || c == '}' || c == '"'; }

int readline(char *b, struct history *h) {
    int len = 0, pos = 0, at = h->n, px, py;
    int cyc = -1, cstart = 0;
    char typed[64];
    b[0] = 0;
    vga_getpos(&px, &py);
    int room = MIN(LINE_MAX, VW - px - 1);
    for (;;) {
        vga_fill(px, py, VW - px, ' ', ATTR(C_TEXT, C_BG));
        vga_puts_at(px, py, b, ATTR(C_TEXT, C_BG));
        vga_cursor(px + pos, py);
        int k = kbd_get();
        if (k != '\t') cyc = -1;
        switch (k) {
        case '\t': {
            if (!h->complete) break;
            char m[64];
            int lcp, cnt;
            if (cyc < 0) {
                int s = pos;
                while (s > 0 && !stop(b[s - 1])) s--;
                if (b[s] == '\'' || b[s] == ':') s++;
                else if (b[s] == '-' && b[s + 1] == '>') s += 2;
                if (s >= pos || pos - s > 62) break;
                memcpy(typed, b + s, (size_t)(pos - s));
                typed[pos - s] = 0;
                cnt = complete_name(typed, pos - s, 0, m, sizeof m, &lcp);
                if (!cnt) break;
                cstart = s;
                if (cnt == 1) { int l = (int)strlen(m); m[l] = ' '; m[l + 1] = 0; }
                else if (lcp > pos - s) m[lcp] = 0;
                else cyc = 1 % cnt;
            } else {
                cnt = complete_name(typed, (int)strlen(typed), cyc, m, sizeof m, &lcp);
                if (!cnt) { cyc = -1; break; }
                cyc = (cyc + 1) % cnt;
            }
            int ml = (int)strlen(m), tail = len - pos;
            if (cstart + ml + tail > room) { cyc = -1; break; }
            memmove(b + cstart + ml, b + pos, (size_t)tail + 1);
            memcpy(b + cstart, m, (size_t)ml);
            len = cstart + ml + tail;
            pos = cstart + ml;
            break;
        }
        case '\n':
            vga_setpos(px + len, py);
            kputc('\n');
            remember(h, b);
            return len;
        case CTRL('c'):
            vga_setpos(px + len, py);
            vga_color(C_DIM);
            kputs("^C\n");
            vga_color(C_TEXT);
            return -1;
        case '\b':
            if (pos) { memmove(b + pos - 1, b + pos, (size_t)(len - pos + 1)); pos--; len--; }
            break;
        case K_DEL:
            if (pos < len) { memmove(b + pos, b + pos + 1, (size_t)(len - pos)); len--; }
            break;
        case K_LEFT: if (pos) pos--; break;
        case K_RIGHT: if (pos < len) pos++; break;
        case K_HOME: pos = 0; break;
        case K_END: pos = len; break;
        case K_UP:
        case K_DOWN:
            if (k == K_UP && at > 0 && at > h->n - LINE_HIST) at--;
            else if (k == K_DOWN && at < h->n) at++;
            else break;
            scopy(b, at == h->n ? "" : h->h[at % LINE_HIST], (size_t)room + 1);
            len = pos = (int)strlen(b);
            break;
        default:
            if (k >= ' ' && k <= '~' && len < room) {
                memmove(b + pos + 1, b + pos, (size_t)(len - pos + 1));
                b[pos++] = (char)k;
                len++;
            }
        }
    }
}
