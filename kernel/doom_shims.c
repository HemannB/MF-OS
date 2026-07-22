/*
 * doom_shims.c
 * Implementações bare-metal de todas as funções de libc que o Doom referencia.
 * Compilado com DOOM_CFLAGS junto aos arquivos do doomgeneric.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* ── kernel headers ─────────────────────────────────────────────── */
#include "../kernel/libc.h"
#include "../kernel/heap.h"
#include "../kernel/fs.h"
#include "../kernel/terminal.h"

/* ═══════════════════════════════════════════════════════════════════
   MEMÓRIA
   ═══════════════════════════════════════════════════════════════════ */

void* malloc(size_t size)           { return kmalloc(size); }
void* calloc(size_t n, size_t size) { return kzalloc(n * size); }
void  free(void *p)                 { (void)p; /* bump allocator — sem free */ }
void* realloc(void *p, size_t size) {
    /* realloc simples: aloca novo bloco e copia — sem free do antigo */
    void *n = kmalloc(size);
    if (n && p) kmemcpy(n, p, size);
    return n;
}

void* memcpy (void *d, const void *s, size_t n) { return kmemcpy(d, s, n); }
void* memset (void *d, int c, size_t n)          { return kmemset(d, c, n); }
void* memmove(void *d, const void *s, size_t n)  { return kmemmove(d, s, n); }
int   memcmp (const void *a, const void *b, size_t n) { return kmemcmp(a, b, n); }

/* versões __chk que o GCC emite com _FORTIFY_SOURCE */
void* __memcpy_chk(void *d, const void *s, size_t n, size_t dn)
    { (void)dn; return kmemcpy(d, s, n); }
void* __memset_chk(void *d, int c, size_t n, size_t dn)
    { (void)dn; return kmemset(d, c, n); }
void* __memmove_chk(void *d, const void *s, size_t n, size_t dn)
    { (void)dn; return kmemmove(d, s, n); }

/* ═══════════════════════════════════════════════════════════════════
   STRINGS
   ═══════════════════════════════════════════════════════════════════ */

size_t strlen (const char *s)                       { return kstrlen(s); }
char*  strcpy (char *d, const char *s)              { return kstrcpy(d, s); }
char*  strncpy(char *d, const char *s, size_t n)    { return kstrncpy(d, s, n); }
int    strcmp (const char *a, const char *b)        { return kstrcmp(a, b); }
int    strncmp(const char *a, const char *b, size_t n) { return kstrncmp(a, b, n); }
char*  strcat (char *d, const char *s)              { return kstrcat(d, s); }
char*  strncat(char *d, const char *s, size_t n)    { return kstrncat(d, s, n); }
char*  strchr (const char *s, int c)                { return kstrchr(s, c); }
char*  strrchr(const char *s, int c)                { return kstrrchr(s, c); }
char*  strstr (const char *h, const char *n)        { return kstrstr(h, n); }
int    atoi   (const char *s)                       { return katoi(s); }

/* versões __chk */
char* __strncpy_chk(char *d, const char *s, size_t n, size_t dn)
    { (void)dn; return kstrncpy(d, s, n); }

/* case-insensitive — o Doom usa muito para nomes de lumps */
static char lower(char c) {
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}
int strcasecmp(const char *a, const char *b) {
    while (*a && *b && lower(*a) == lower(*b)) { a++; b++; }
    return (unsigned char)lower(*a) - (unsigned char)lower(*b);
}
int strncasecmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!a[i] && !b[i]) return 0;
        int d = (unsigned char)lower(a[i]) - (unsigned char)lower(b[i]);
        if (d) return d;
    }
    return 0;
}

/* strdup — aloca cópia da string no heap */
char* strdup(const char *s) {
    size_t n = kstrlen(s) + 1;
    char *p = kmalloc(n);
    if (p) kmemcpy(p, s, n);
    return p;
}

/* strtol — converte string para long */
long strtol(const char *s, char **end, int base) {
    while (*s == ' ') s++;
    long sign = 1;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (s[0] == '0') { base = 8; s++; }
        else base = 10;
    }
    long n = 0;
    while (*s) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        else break;
        if (d >= base) break;
        n = n * base + d;
        s++;
    }
    if (end) *end = (char*)s;
    return sign * n;
}

/* strtod — o Doom usa para parsing de config; aproximação inteira serve */
double strtod(const char *s, char **end) {
    return (double) strtol(s, end, 10);
}

/* ═══════════════════════════════════════════════════════════════════
   MATEMÁTICA
   ═══════════════════════════════════════════════════════════════════ */

int    abs (int x)    { return x < 0 ? -x : x; }
long   labs(long x)   { return x < 0 ? -x : x; }
double fabs(double x) { return x < 0.0 ? -x : x; }

/* __divdi3 — divisão de 64 bits, gerada pelo GCC para / em int64 em 32-bit */
long long __divdi3(long long a, long long b) {
    int neg = 0;
    if (a < 0) { a = -a; neg ^= 1; }
    if (b < 0) { b = -b; neg ^= 1; }
    unsigned long long ua = (unsigned long long)a;
    unsigned long long ub = (unsigned long long)b;
    if (ub == 0) return 0;
    unsigned long long q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((ua >> i) & 1);
        if (r >= ub) { r -= ub; q |= (1ULL << i); }
    }
    return neg ? -(long long)q : (long long)q;
}

/* ═══════════════════════════════════════════════════════════════════
   OUTPUT — printf/fprintf/snprintf
   O Doom usa muito para debug. Redirecionamos para o terminal VGA.
   ═══════════════════════════════════════════════════════════════════ */

/* formatador mínimo: %s %d %i %u %x %c %% */
static int kvsnprintf(char *buf, size_t sz, const char *fmt, va_list ap) {
    size_t i = 0;
#define PUT(c) do { if (buf && i+1 < sz) buf[i] = (c); i++; } while(0)
    while (*fmt) {
        if (*fmt != '%') { PUT(*fmt++); continue; }
        fmt++;
        /* flags */
        int zero = 0, left = 0, plus = 0;
        while (*fmt == '0' || *fmt == '-' || *fmt == '+') {
            if (*fmt == '0') zero = 1;
            if (*fmt == '-') left = 1;
            if (*fmt == '+') plus = 1;
            fmt++;
        }
        (void)left; (void)plus;
        /* width */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width*10 + (*fmt++ - '0');
        /* precision */
        int prec = -1;
        if (*fmt == '.') { fmt++; prec = 0; while (*fmt >= '0' && *fmt <= '9') prec = prec*10 + (*fmt++ - '0'); }
        (void)prec;
        /* length */
        if (*fmt == 'l') fmt++;
        if (*fmt == 'l') fmt++;
        char spec = *fmt++;
        if (spec == '%') { PUT('%'); continue; }
        if (spec == 'c') { PUT((char)va_arg(ap, int)); continue; }
        if (spec == 's') {
            const char *s = va_arg(ap, const char*);
            if (!s) s = "(null)";
            int len = (int)kstrlen(s);
            int pad = width - len;
            while (pad-- > 0) PUT(' ');
            while (*s) PUT(*s++);
            continue;
        }
        if (spec == 'd' || spec == 'i') {
            long v = va_arg(ap, long);
            char tmp[24]; int n = 0;
            int neg = v < 0; if (neg) v = -v;
            if (v == 0) tmp[n++] = '0';
            while (v > 0) { tmp[n++] = '0' + (v % 10); v /= 10; }
            if (neg) tmp[n++] = '-';
            int pad = width - n; while (pad-- > 0) PUT(zero ? '0' : ' ');
            while (n > 0) PUT(tmp[--n]);
            continue;
        }
        if (spec == 'u') {
            unsigned long v = va_arg(ap, unsigned long);
            char tmp[24]; int n = 0;
            if (v == 0) tmp[n++] = '0';
            while (v > 0) { tmp[n++] = '0' + (v % 10); v /= 10; }
            int pad = width - n; while (pad-- > 0) PUT(zero ? '0' : ' ');
            while (n > 0) PUT(tmp[--n]);
            continue;
        }
        if (spec == 'x' || spec == 'X') {
            unsigned long v = va_arg(ap, unsigned long);
            const char *hex = (spec == 'x') ? "0123456789abcdef" : "0123456789ABCDEF";
            char tmp[24]; int n = 0;
            if (v == 0) tmp[n++] = '0';
            while (v > 0) { tmp[n++] = hex[v & 0xF]; v >>= 4; }
            int pad = width - n; while (pad-- > 0) PUT(zero ? '0' : ' ');
            while (n > 0) PUT(tmp[--n]);
            continue;
        }
        if (spec == 'f' || spec == 'g' || spec == 'e') {
            (void)va_arg(ap, double); /* ignora floats */
            PUT('?');
            continue;
        }
        if (spec == 'p') {
            unsigned long v = (unsigned long)va_arg(ap, void*);
            PUT('0'); PUT('x');
            char tmp[24]; int n = 0;
            if (v == 0) tmp[n++] = '0';
            while (v > 0) { tmp[n++] = "0123456789abcdef"[v & 0xF]; v >>= 4; }
            while (n > 0) PUT(tmp[--n]);
            continue;
        }
        PUT('?');
    }
#undef PUT
    if (buf && sz > 0) buf[i < sz ? i : sz-1] = '\0';
    return (int)i;
}

static void kputs(const char *s) {
    while (*s) term_putchar(*s++);
}

int vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap) {
    return kvsnprintf(buf, sz, fmt, ap);
}
int snprintf(char *buf, size_t sz, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = kvsnprintf(buf, sz, fmt, ap);
    va_end(ap); return r;
}
int sprintf(char *buf, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = kvsnprintf(buf, (size_t)-1, fmt, ap);
    va_end(ap); return r;
}
int printf(const char *fmt, ...) {
    char buf[512]; va_list ap; va_start(ap, fmt);
    int r = kvsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap); kputs(buf); return r;
}
int vprintf(const char *fmt, va_list ap) {
    char buf[512];
    int r = kvsnprintf(buf, sizeof(buf), fmt, ap);
    kputs(buf); return r;
}

/* versões __chk que o GCC gera com _FORTIFY_SOURCE */
int __printf_chk(int flag, const char *fmt, ...) {
    (void)flag; char buf[512]; va_list ap; va_start(ap, fmt);
    int r = kvsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap); kputs(buf); return r;
}
int __vprintf_chk(int flag, const char *fmt, va_list ap) {
    (void)flag; char buf[512];
    int r = kvsnprintf(buf, sizeof(buf), fmt, ap);
    kputs(buf); return r;
}
int __snprintf_chk(char *buf, size_t sz, int flag, size_t bos, const char *fmt, ...) {
    (void)flag; (void)bos; va_list ap; va_start(ap, fmt);
    int r = kvsnprintf(buf, sz, fmt, ap);
    va_end(ap); return r;
}
int __vsnprintf_chk(char *buf, size_t sz, int flag, size_t bos, const char *fmt, va_list ap) {
    (void)flag; (void)bos; return kvsnprintf(buf, sz, fmt, ap);
}

/* ═══════════════════════════════════════════════════════════════════
   FILE I/O — fopen/fread/fclose/fseek/ftell
   Mapeados para o filesystem do kernel (ramdisk + bump para escrita)
   ═══════════════════════════════════════════════════════════════════ */

/* FILE fake: guarda ponteiro para fs_file_t e offset atual */
typedef struct {
    fs_file_t *f;
    uint32_t   pos;
    int        writable;
    uint8_t   *wbuf;       /* buffer de escrita para save games */
    uint32_t   wsize;
    uint32_t   wcap;
} kFILE;

/* stdout/stderr — ponteiros não-nulos para o terminal (não usamos FILE real) */
static kFILE _stdout_file = {0, 0, 1, 0, 0, 0};
static kFILE _stderr_file = {0, 0, 1, 0, 0, 0};
void *stdout = &_stdout_file;
void *stderr = &_stderr_file;
void *stdin  = 0;

/* número máximo de arquivos abertos simultaneamente */
#define MAX_OPEN_FILES 8
static kFILE open_files[MAX_OPEN_FILES];
static int   file_slots_used = 0;

static kFILE *alloc_file(void) {
    if (file_slots_used >= MAX_OPEN_FILES) return 0;
    return &open_files[file_slots_used++];
}

void* fopen(const char *path, const char *mode) {
    /* extrai nome do arquivo do path (após última '/') */
    const char *name = path;
    for (const char *p = path; *p; p++)
        if (*p == '/') name = p + 1;

    kFILE *fp = alloc_file();
    if (!fp) return 0;

    /* modo escrita — para save games: aloca buffer no heap */
    if (mode[0] == 'w' || mode[0] == 'a') {
        fp->f        = 0;
        fp->pos      = 0;
        fp->writable = 1;
        fp->wcap     = 65536;
        fp->wbuf     = kmalloc(fp->wcap);
        fp->wsize    = 0;
        return fp;
    }

    /* modo leitura — busca no ramdisk */
    fs_file_t *f = fs_open(name);
    if (!f) { file_slots_used--; return 0; }
    fp->f        = f;
    fp->pos      = 0;
    fp->writable = 0;
    fp->wbuf     = 0;
    fp->wsize    = 0;
    fp->wcap     = 0;
    return fp;
}

int fclose(void *stream) {
    (void)stream;
    if (file_slots_used > 0) file_slots_used--;
    return 0;
}

size_t fread(void *buf, size_t sz, size_t n, void *stream) {
    kFILE *fp = (kFILE*)stream;
    if (!fp || !fp->f) return 0;
    uint32_t bytes = (uint32_t)(sz * n);
    uint32_t got = fs_read(fp->f, buf, bytes, fp->pos);
    fp->pos += got;
    return got / sz;
}

size_t fwrite(const void *buf, size_t sz, size_t n, void *stream) {
    kFILE *fp = (kFILE*)stream;
    if (!fp) return 0;
    /* stdout/stderr: imprime no terminal */
    if (fp == &_stdout_file || fp == &_stderr_file) {
        const char *s = (const char*)buf;
        for (size_t i = 0; i < sz*n; i++) term_putchar(s[i]);
        return n;
    }
    if (!fp->writable || !fp->wbuf) return 0;
    uint32_t bytes = (uint32_t)(sz * n);
    if (fp->wsize + bytes > fp->wcap) bytes = fp->wcap - fp->wsize;
    kmemcpy(fp->wbuf + fp->wsize, buf, bytes);
    fp->wsize += bytes;
    fp->pos   += bytes;
    return bytes / sz;
}

int fseek(void *stream, long offset, int whence) {
    kFILE *fp = (kFILE*)stream;
    if (!fp) return -1;
    uint32_t size = fp->f ? fp->f->size : fp->wsize;
    if (whence == 0) fp->pos = (uint32_t)offset;           /* SEEK_SET */
    else if (whence == 1) fp->pos += (uint32_t)offset;     /* SEEK_CUR */
    else fp->pos = size + (uint32_t)offset;                 /* SEEK_END */
    return 0;
}

long ftell(void *stream) {
    kFILE *fp = (kFILE*)stream;
    if (!fp) return -1;
    return (long)fp->pos;
}

int fflush(void *stream) { (void)stream; return 0; }

int fprintf(void *stream, const char *fmt, ...) {
    char buf[512]; va_list ap; va_start(ap, fmt);
    int r = kvsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    kFILE *fp = (kFILE*)stream;
    if (!fp || fp == &_stdout_file || fp == &_stderr_file)
        kputs(buf);
    return r;
}
int __fprintf_chk(void *stream, int flag, const char *fmt, ...) {
    (void)stream; (void)flag; char buf[512]; va_list ap; va_start(ap, fmt);
    int r = kvsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap); kputs(buf); return r;
}
int __vfprintf_chk(void *stream, int flag, const char *fmt, va_list ap) {
    (void)stream; (void)flag; char buf[512];
    int r = kvsnprintf(buf, sizeof(buf), fmt, ap);
    kputs(buf); return r;
}
int vfprintf(void *stream, const char *fmt, va_list ap) {
    (void)stream; char buf[512];
    int r = kvsnprintf(buf, sizeof(buf), fmt, ap);
    kputs(buf); return r;
}
int sscanf(const char *buf, const char *fmt, ...) { (void)buf; (void)fmt; return 0; }
int __isoc99_sscanf(const char *buf, const char *fmt, ...) { (void)buf; (void)fmt; return 0; }

int puts(const char *s) { kputs(s); term_putchar('\n'); return 0; }
int putc(int c, void *stream) { (void)stream; term_putchar((char)c); return c; }
int putchar(int c) { term_putchar((char)c); return c; }

/* ═══════════════════════════════════════════════════════════════════
   SISTEMA
   ═══════════════════════════════════════════════════════════════════ */

void exit(int code) {
    (void)code;
    /* volta para o shell — interrompe o loop do Doom */
    __asm__ volatile ("cli; hlt");
    while(1);
}

int system(const char *cmd) { (void)cmd; return -1; }

int remove(const char *path) { (void)path; return 0; }
int rename(const char *old, const char *new) { (void)old; (void)new; return 0; }
int mkdir(const char *path, ...) { (void)path; return 0; }

/* errno — o Doom checa __errno_location em alguns lugares */
static int _errno = 0;
int* __errno_location(void) { return &_errno; }

/* ctype — o Doom usa toupper para nomes de lumps */
const unsigned short **__ctype_b_loc(void) { return 0; }
const int **__ctype_toupper_loc(void) {
    static int tbl[256];
    static int *ptbl = tbl;
    static int init = 0;
    if (!init) {
        for (int i = 0; i < 256; i++)
            tbl[i] = (i >= 'a' && i <= 'z') ? i - 32 : i;
        init = 1;
    }
    return (const int**)&ptbl;
}
