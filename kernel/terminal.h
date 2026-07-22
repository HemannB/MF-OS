#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

typedef enum {
    VGA_BLACK = 0, VGA_BLUE, VGA_GREEN, VGA_CYAN,
    VGA_RED, VGA_MAGENTA, VGA_BROWN, VGA_LIGHT_GREY,
    VGA_DARK_GREY, VGA_LIGHT_BLUE, VGA_LIGHT_GREEN,
    VGA_LIGHT_CYAN, VGA_LIGHT_RED, VGA_LIGHT_MAGENTA,
    VGA_LIGHT_BROWN, VGA_WHITE
} vga_color_t;

void term_init(void);
void term_set_color(vga_color_t fg, vga_color_t bg);
void term_putchar(char c);
void term_print(const char *s);
void term_println(const char *s);
void term_print_uint(uint32_t n);

/* modo gráfico: quando ativo (1), suprime escrita em 0xB8000
   para não interferir com o framebuffer VGA do Doom */
void terminal_set_graphics(int on);

#endif