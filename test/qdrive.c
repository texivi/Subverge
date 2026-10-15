/* qemu driver */
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int mfd = -1;
static pid_t qpid;
static char sock[108];

static void msleep(long ms) { struct timespec t = { ms / 1000, (ms % 1000) * 1000000L }; nanosleep(&t, NULL); }

static void die(const char *m) {
    fprintf(stderr, "qdrive: %s\n", m);
    if (qpid > 0) kill(qpid, SIGKILL);
    unlink(sock);
    exit(1);
}

static void until_prompt(void) {
    char buf[4096], tail[8] = { 0 };
    for (int idle = 0; idle < 200;) {
        struct pollfd p = { mfd, POLLIN, 0 };
        if (poll(&p, 1, 50) <= 0) { idle++; continue; }
        ssize_t n = read(mfd, buf, sizeof buf);
        if (n <= 0) die("monitor closed");
        for (ssize_t i = 0; i < n; i++) {
            memmove(tail, tail + 1, 6);
            tail[6] = buf[i];
            if (!memcmp(tail, "(qemu) ", 7)) return;
        }
    }
    die("monitor timeout");
}

static void mon(const char *c) {
    char line[512];
    int n = snprintf(line, sizeof line, "%s\n", c);
    if (write(mfd, line, (size_t)n) != n) die("monitor write");
    until_prompt();
}

static void key(const char *k) {
    char c[96];
    snprintf(c, sizeof c, "sendkey %s", k);
    mon(c);
    msleep(45);
}

static const char *keyname(char c, char *tmp) {
    static const char *map[][2] = {
        { " ", "spc" }, { "[", "bracket_left" }, { "]", "bracket_right" }, { "'", "apostrophe" },
        { ":", "shift-semicolon" }, { ";", "semicolon" }, { "+", "shift-equal" }, { "=", "equal" },
        { "-", "minus" }, { "*", "shift-8" }, { "<", "shift-comma" }, { ">", "shift-dot" },
        { ".", "dot" }, { ",", "comma" }, { "/", "slash" }, { "\"", "shift-apostrophe" },
        { "{", "shift-bracket_left" }, { "}", "shift-bracket_right" }, { "^", "shift-6" },
        { "&", "shift-7" }, { "|", "shift-backslash" }, { "!", "shift-1" }, { "?", "shift-slash" },
        { "#", "shift-3" }, { "@", "shift-2" }, { "_", "shift-minus" }, { "\\", "backslash" },
        { "(", "shift-9" }, { ")", "shift-0" }, { "%", "shift-5" }, { "~", "shift-grave_accent" },
    };
    for (size_t i = 0; i < sizeof map / sizeof *map; i++) if (map[i][0][0] == c) return map[i][1];
    if (isupper((unsigned char)c)) { sprintf(tmp, "shift-%c", tolower(c)); return tmp; }
    tmp[0] = c; tmp[1] = 0;
    return tmp;
}

static void type(const char *s) { char t[16]; for (; *s; s++) key(keyname(*s, t)); }

/* png out */

static uint32_t crctab[256];
static void crcinit(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crctab[n] = c;
    }
}
static uint32_t crc(uint32_t c, const uint8_t *p, size_t n) { while (n--) c = crctab[(c ^ *p++) & 0xFF] ^ (c >> 8); return c; }
static void be32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = (uint8_t)v; }

static void chunk(FILE *f, const char *type, const uint8_t *d, uint32_t n) {
    uint8_t h[8];
    be32(h, n);
    memcpy(h + 4, type, 4);
    fwrite(h, 1, 8, f);
    if (n) fwrite(d, 1, n, f);
    uint32_t c = crc(0xFFFFFFFFu, (const uint8_t *)type, 4);
    c = crc(c, d, n) ^ 0xFFFFFFFFu;
    be32(h, c);
    fwrite(h, 1, 4, f);
}

static int ppm_to_png(const char *ppm, const char *png) {
    FILE *f = fopen(ppm, "rb");
    if (!f) return -1;
    int w, h, mx;
    if (fscanf(f, "P6 %d %d %d", &w, &h, &mx) != 3 || mx != 255) { fclose(f); return -1; }
    fgetc(f);
    size_t row = (size_t)w * 3, raw_n = (row + 1) * (size_t)h;
    uint8_t *raw = malloc(raw_n);
    for (int y = 0; y < h; y++) {
        raw[y * (row + 1)] = 0;
        if (fread(raw + y * (row + 1) + 1, 1, row, f) != row) { fclose(f); free(raw); return -1; }
    }
    fclose(f);

    size_t blocks = raw_n / 65535 + 1, zn = 2 + raw_n + blocks * 5 + 4;
    uint8_t *z = malloc(zn), *q = z;
    *q++ = 0x78; *q++ = 0x01;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < raw_n; i++) { a = (a + raw[i]) % 65521; b = (b + a) % 65521; }
    for (size_t off = 0; off < raw_n || off == 0;) {
        size_t n = raw_n - off > 65535 ? 65535 : raw_n - off;
        *q++ = off + n >= raw_n;
        *q++ = n & 0xFF; *q++ = n >> 8; *q++ = ~n & 0xFF; *q++ = (~n >> 8) & 0xFF;
        memcpy(q, raw + off, n);
        q += n;
        off += n;
        if (!n) break;
    }
    be32(q, b << 16 | a);
    q += 4;

    FILE *o = fopen(png, "wb");
    if (!o) { free(raw); free(z); return -1; }
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, o);
    uint8_t ih[13];
    be32(ih, (uint32_t)w); be32(ih + 4, (uint32_t)h);
    ih[8] = 8; ih[9] = 2; ih[10] = ih[11] = ih[12] = 0;
    chunk(o, "IHDR", ih, 13);
    chunk(o, "IDAT", z, (uint32_t)(q - z));
    chunk(o, "IEND", NULL, 0);
    fclose(o);
    free(raw);
    free(z);
    return 0;
}

static void shot(const char *png) {
    char ppm[600], c[700];
    snprintf(ppm, sizeof ppm, "%s.ppm", png);
    snprintf(c, sizeof c, "screendump %s", ppm);
    mon(c);
    if (ppm_to_png(ppm, png)) die("screendump failed");
    unlink(ppm);
    printf("shot %s\n", png);
}

/* script */

static void run(FILE *s) {
    char line[1024];
    while (fgets(line, sizeof line, s)) {
        line[strcspn(line, "\n")] = 0;
        char *arg = strchr(line, ' ');
        if (arg) *arg++ = 0; else arg = "";
        if (!line[0] || line[0] == '#') continue;
        if (!strcmp(line, "type")) type(arg);
        else if (!strcmp(line, "cmd")) {
            char *ms = strstr(arg, "  ~");
            long w = 600;
            if (ms) { w = atol(ms + 3); *ms = 0; }
            type(arg); key("ret"); msleep(w);
        }
        else if (!strcmp(line, "key")) key(arg);
        else if (!strcmp(line, "wait")) msleep(atol(arg));
        else if (!strcmp(line, "shot")) shot(arg);
        else if (!strcmp(line, "mon")) mon(arg);
        else if (!strcmp(line, "quit")) break;
        else { fprintf(stderr, "qdrive: bad line: %s\n", line); }
    }
}

int main(int argc, char **argv) {
    int dash = 0;
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--")) { dash = i; break; }
    if (argc < 4 || dash != 2) { fprintf(stderr, "usage: qdrive script -- qemu args...\n"); return 2; }
    crcinit();
    snprintf(sock, sizeof sock, "/tmp/qdrive.%d.sock", (int)getpid());
    char monarg[160];
    snprintf(monarg, sizeof monarg, "unix:%s,server,nowait", sock);

    int qn = argc - dash - 1;
    char **qa = calloc((size_t)qn + 6, sizeof *qa);
    for (int i = 0; i < qn; i++) qa[i] = argv[dash + 1 + i];
    qa[qn] = "-monitor"; qa[qn + 1] = monarg; qa[qn + 2] = "-display"; qa[qn + 3] = "none";

    qpid = fork();
    if (qpid == 0) {
        int nul = open("/dev/null", 1);
        dup2(nul, 1); dup2(nul, 2);
        execvp(qa[0], qa);
        _exit(127);
    }

    struct sockaddr_un a = { .sun_family = AF_UNIX };
    memcpy(a.sun_path, sock, strlen(sock) + 1);
    for (int t = 0; t < 100; t++) {
        mfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (!connect(mfd, (struct sockaddr *)&a, sizeof a)) break;
        close(mfd);
        mfd = -1;
        msleep(50);
    }
    if (mfd < 0) die("no monitor");
    until_prompt();

    FILE *s = !strcmp(argv[1], "-") ? stdin : fopen(argv[1], "r");
    if (!s) die("no script");
    run(s);

    if (write(mfd, "quit\n", 5) < 0) {}
    msleep(200);
    kill(qpid, SIGKILL);
    waitpid(qpid, NULL, 0);
    unlink(sock);
    return 0;
}
