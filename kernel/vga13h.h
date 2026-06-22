#ifndef VGA13H_H
#define VGA13H_H

#include <stdint.h>

void     vga_init_mode13h(void);
void     vga_set_palette(uint8_t *palette);
void     vga_swap(void);
void     vga_put_pixel(int x, int y, uint8_t color);
uint8_t* vga_get_backbuffer(void);
void     vga_test(void);

#endif
