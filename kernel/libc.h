#ifndef LIBC_H
#define LIBC_H

#include <stdint.h>
#include <stddef.h>

/* ── memória ── */
void* kmemcpy (void *dst, const void *src, size_t n);
void* kmemset (void *dst, int c, size_t n);
void* kmemmove(void *dst, const void *src, size_t n);
int   kmemcmp (const void *a, const void *b, size_t n);

/* ── strings ── */
size_t kstrlen (const char *s);
char*  kstrcpy (char *dst, const char *src);
char*  kstrncpy(char *dst, const char *src, size_t n);
int    kstrcmp (const char *a, const char *b);
int    kstrncmp(const char *a, const char *b, size_t n);
char*  kstrcat (char *dst, const char *src);
char*  kstrncat(char *dst, const char *src, size_t n);
char*  kstrchr (const char *s, int c);
char*  kstrrchr(const char *s, int c);
char*  kstrstr (const char *haystack, const char *needle);
int    katoi   (const char *s);

/* ── Aliases para o Doom — o código do Doom chama memcpy, strcmp, etc.
      Mapeamos para nossas implementações via macros para não poluir o
      namespace global quando compilando arquivos do kernel puro.      ──
   Esses defines são ativados APENAS quando DOOM_LIBC_SHIMS está definido,
   o que o Makefile fará somente para os .c do doomgeneric. ── */
#ifdef DOOM_LIBC_SHIMS
#  define memcpy   kmemcpy
#  define memset   kmemset
#  define memmove  kmemmove
#  define memcmp   kmemcmp
#  define strlen   kstrlen
#  define strcpy   kstrcpy
#  define strncpy  kstrncpy
#  define strcmp   kstrcmp
#  define strncmp  kstrncmp
#  define strcat   kstrcat
#  define strncat  kstrncat
#  define strchr   kstrchr
#  define strrchr  kstrrchr
#  define strstr   kstrstr
#  define atoi     katoi
#endif

#endif /* LIBC_H */