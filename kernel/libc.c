#include "libc.h"

/* ── memória ─────────────────────────────────────────────────────── */

void* kmemcpy(void *dst, const void *src, size_t n) {
    uint8_t       *d = (uint8_t*)       dst;
    const uint8_t *s = (const uint8_t*) src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

void* kmemset(void *dst, int c, size_t n) {
    uint8_t *d = (uint8_t*) dst;
    for (size_t i = 0; i < n; i++) d[i] = (uint8_t) c;
    return dst;
}

void* kmemmove(void *dst, const void *src, size_t n) {
    uint8_t       *d = (uint8_t*)       dst;
    const uint8_t *s = (const uint8_t*) src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    }
    return dst;
}

int kmemcmp(const void *a, const void *b, size_t n) {
    const uint8_t *pa = (const uint8_t*) a;
    const uint8_t *pb = (const uint8_t*) b;
    for (size_t i = 0; i < n; i++) {
        if (pa[i] < pb[i]) return -1;
        if (pa[i] > pb[i]) return  1;
    }
    return 0;
}

/* ── strings ─────────────────────────────────────────────────────── */

size_t kstrlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

char* kstrcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char* kstrncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}

int kstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int kstrncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!a[i] && !b[i]) return 0;
        if ((unsigned char)a[i] != (unsigned char)b[i])
            return (unsigned char)a[i] - (unsigned char)b[i];
    }
    return 0;
}

char* kstrcat(char *dst, const char *src) {
    char *d = dst;
    while (*d) d++;
    while ((*d++ = *src++));
    return dst;
}

char* kstrncat(char *dst, const char *src, size_t n) {
    char *d = dst;
    while (*d) d++;
    for (size_t i = 0; i < n && src[i]; i++) *d++ = src[i];
    *d = '\0';
    return dst;
}

char* kstrchr(const char *s, int c) {
    for (; *s; s++)
        if ((unsigned char)*s == (unsigned char)c) return (char*)s;
    if (c == '\0') return (char*)s;
    return 0;
}

char* kstrrchr(const char *s, int c) {
    const char *last = 0;
    for (; *s; s++)
        if ((unsigned char)*s == (unsigned char)c) last = s;
    if (c == '\0') return (char*)s;
    return (char*)last;
}

char* kstrstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return 0;
}

int katoi(const char *s) {
    int n = 0, sign = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (*s++ - '0');
    return sign * n;
}