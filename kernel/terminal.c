#include "terminal.h"
#include "io.h"
#include <stddef.h>

#define VGA_ADDR  ((volatile uint16_t*) 0xB8000)
#define VGA_COLS  80
#define VGA_ROWS  25

static size_t  term_row;
static size_t  term_col;
static uint8_t term_color;
static int     graphics_mode = 0;  /* 1 = Doom rodando, suprime texto */

static inline uint8_t vga_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t) c | ((uint16_t) color << 8);
}

static void cursor_enable(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 13);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

static void cursor_update(void) {
    uint16_t pos = term_row * VGA_COLS + term_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void term_scroll(void) {
    for (size_t r = 1; r < VGA_ROWS; r++)
        for (size_t c = 0; c < VGA_COLS; c++)
            VGA_ADDR[(r - 1) * VGA_COLS + c] = VGA_ADDR[r * VGA_COLS + c];
    for (size_t c = 0; c < VGA_COLS; c++)
        VGA_ADDR[(VGA_ROWS - 1) * VGA_COLS + c] = vga_entry(' ', term_color);
    term_row = VGA_ROWS - 1;
}

void term_init(void) {
    graphics_mode = 0;
    term_row   = 0;
    term_col   = 0;
    term_color = vga_color(VGA_LIGHT_GREEN, VGA_BLACK);
    for (size_t r = 0; r < VGA_ROWS; r++)
        for (size_t c = 0; c < VGA_COLS; c++)
            VGA_ADDR[r * VGA_COLS + c] = vga_entry(' ', term_color);
    cursor_enable();
}

/* ativa/desativa modo gráfico — quando ativo, term_putchar não escreve
   em 0xB8000, evitando corrupção do framebuffer VGA do Doom */
void terminal_set_graphics(int on) {
    graphics_mode = on;
}

void term_set_color(vga_color_t fg, vga_color_t bg) {
    term_color = vga_color(fg, bg);
}

void term_putchar(char c) {
    outb(0xE9, (uint8_t)c);  /* QEMU/Bochs debug console */
    if (graphics_mode) return;  /* suprimido durante o Doom */

    if (c == '\n') {
        term_col = 0;
        if (++term_row == VGA_ROWS) term_scroll();
        return;
    }
    if (c == '\r') { term_col = 0; return; }
    if (c == '\b' && term_col > 0) {
        term_col--;
        VGA_ADDR[term_row * VGA_COLS + term_col] = vga_entry(' ', term_color);
        return;
    }

    VGA_ADDR[term_row * VGA_COLS + term_col] = vga_entry(c, term_color);
    if (++term_col == VGA_COLS) {
        term_col = 0;
        if (++term_row == VGA_ROWS) term_scroll();
    }
    cursor_update();
}

void term_print(const char *s) {
    while (*s) term_putchar(*s++);
}

void term_println(const char *s) {
    term_print(s);
    term_putchar('\n');
}

void term_print_uint(uint32_t n) {
    if (n == 0) { term_putchar('0'); return; }
    char buf[12];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i - 1; j >= 0; j--) term_putchar(buf[j]);
}
