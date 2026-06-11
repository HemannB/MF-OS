#include "vga13h.h"
#include "io.h"

#define VGA_WIDTH   320                             // largura em pixels 
#define VGA_HEIGHT  200                             // altura em pixels 
#define FRAMEBUFFER ((volatile uint8_t*) 0xA0000)   // endereço do framebuffer VGA modo 13h 

static uint8_t back_buffer[VGA_WIDTH * VGA_HEIGHT];

void vga_init_mode13h(void) {
    // miscellaneous output habilita acesso aos registradores VGA
    outb(0x3C2, 0x63);

    // sequencer 
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);

    // desbloqueia CRTC
    outb(0x3D4, 0x11); outb(0x3D5, 0x0E);

    // CRTC
    static const uint8_t crtc[] = {
        0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,
        0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,
        0x9C,0x0E,0x8F,0x28,0x40,0x96,0xB9,0xA3,0xFF
    };
    for (int i = 0; i < 25; i++) {
        outb(0x3D4, i);
        outb(0x3D5, crtc[i]);
    }

    // graphics controller
    static const uint8_t gc[] = {
        0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0F,0xFF
    };
    for (int i = 0; i < 9; i++) {
        outb(0x3CE, i);
        outb(0x3CF, gc[i]);
    }

    // attribute controller
    inb(0x3DA); // reset flip-flop
    static const uint8_t ac[] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
        0x41,0x00,0x0F,0x00,0x00
    };
    for (int i = 0; i < 21; i++) {
        outb(0x3C0, i);
        outb(0x3C0, ac[i]);
    }
    outb(0x3C0, 0x20); // habilita vídeo
}

void vga_set_palette(uint8_t *palette) {
    // cada cor tem 3 bytes (R, G, B) VGA usa 6 bits por canal
    outb(0x3C8, 0); // começa na cor 0 
    for (int i = 0; i < 256 * 3; i++)
        outb(0x3C9, palette[i] >> 2); // converte 8-bit para 6-bit
}

void vga_swap(void) {
    // copia o back_buffer para o framebuffer VGA de uma vez
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        FRAMEBUFFER[i] = back_buffer[i];
}

void vga_put_pixel(int x, int y, uint8_t color) {
    // escreve um pixel no back_buffer na posição (x, y)
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT)
        back_buffer[y * VGA_WIDTH + x] = color;
}

uint8_t* vga_get_backbuffer(void) {
    return back_buffer;
}