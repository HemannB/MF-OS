#include "vga13h.h"
#include "io.h"

#define VBE_DISPI_IOPORT_INDEX  0x01CE
#define VBE_DISPI_IOPORT_DATA   0x01CF

#define VBE_DISPI_INDEX_ID          0
#define VBE_DISPI_INDEX_XRES        1
#define VBE_DISPI_INDEX_YRES        2
#define VBE_DISPI_INDEX_BPP         3
#define VBE_DISPI_INDEX_ENABLE      4

#define VBE_DISPI_DISABLED   0x00
#define VBE_DISPI_ENABLED    0x01
#define VBE_DISPI_LFB_ENABLED 0x40
#define VGA_BPP    8

#define VGA_WIDTH  320
#define VGA_HEIGHT 200
#define LFB_ADDR   0xFD000000

static uint8_t  back_buffer[VGA_WIDTH * VGA_HEIGHT];
static uint8_t *lfb = (uint8_t*) LFB_ADDR;

static void vbe_write(uint16_t index, uint16_t value) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA,  value);
}

static uint16_t vbe_read(uint16_t index) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

void vga_init_mode13h(void) {
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES,   VGA_WIDTH);
    vbe_write(VBE_DISPI_INDEX_YRES,   VGA_HEIGHT);
    vbe_write(VBE_DISPI_INDEX_BPP,    VGA_BPP);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

void vga_set_palette(uint8_t *palette) {
    outb(0x3C8, 0);
    for (int i = 0; i < 256 * 3; i++)
        outb(0x3C9, palette[i] >> 2);
}

void vga_swap(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        lfb[i] = back_buffer[i];
}

void vga_put_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT)
        back_buffer[y * VGA_WIDTH + x] = color;
}

uint8_t* vga_get_backbuffer(void) {
    return back_buffer;
}

void vga_test(void) {
    /* primeiro: verifica se VBE respondeu */
    uint16_t id = vbe_read(VBE_DISPI_INDEX_ID);
    /* escreve direto no LFB sem passar pelo back_buffer */
    for (int y = 0; y < VGA_HEIGHT; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            lfb[y * VGA_WIDTH + x] = (uint8_t)((x + y) % 256);
    (void)id;
}

/* escreve cor de teste direto no LFB sem paleta */
void vga_fill(uint8_t color) {
    for (int i = 0; i < 320*200; i++)
        lfb[i] = color;
}
