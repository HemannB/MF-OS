#define DOOM_LIBC_SHIMS
#include "../kernel/libc.h"
#include "../kernel/vga13h.h"
#include "../kernel/timer.h"
#include "../kernel/isr.h"
#include "../kernel/terminal.h"
#include "doomgeneric.h"
#include "doomkeys.h"
#include "i_video.h"

static const unsigned char sc_to_doom[128] = {
    [0x01]=KEY_ESCAPE,[0x1C]=KEY_ENTER,[0x39]=' ',
    [0x4B]=KEY_LEFTARROW,[0x4D]=KEY_RIGHTARROW,
    [0x48]=KEY_UPARROW,[0x50]=KEY_DOWNARROW,
    [0x1D]=KEY_RCTRL,[0x38]=KEY_RALT,
    [0x2A]=KEY_RSHIFT,[0x36]=KEY_RSHIFT,
    [0x02]='1',[0x03]='2',[0x04]='3',[0x05]='4',
    [0x06]='5',[0x07]='6',[0x08]='7',[0x09]='8',
    [0x0A]='9',[0x0B]='0',
    [0x10]='q',[0x11]='w',[0x12]='e',[0x13]='r',
    [0x14]='t',[0x15]='y',[0x16]='u',[0x17]='i',
    [0x18]='o',[0x19]='p',
    [0x1E]='a',[0x1F]='s',[0x20]='d',[0x21]='f',
    [0x22]='g',[0x23]='h',[0x24]='j',[0x25]='k',[0x26]='l',
    [0x2C]='z',[0x2D]='x',[0x2E]='c',[0x2F]='v',
    [0x30]='b',[0x31]='n',[0x32]='m',
    [0x3B]=KEY_F1,[0x3C]=KEY_F2,[0x3D]=KEY_F3,[0x3E]=KEY_F4,
    [0x3F]=KEY_F5,[0x40]=KEY_F6,[0x41]=KEY_F7,[0x42]=KEY_F8,
    [0x43]=KEY_F9,[0x44]=KEY_F10,[0x57]=KEY_F11,[0x58]=KEY_F12,
};

#define SC_BUFFER_SIZE 64
static volatile uint8_t sc_buf[SC_BUFFER_SIZE];
static volatile uint8_t sc_head = 0;
static volatile uint8_t sc_tail = 0;

void irq1_doom_push(uint8_t sc) {
    uint8_t next = (uint8_t)((sc_head + 1) % SC_BUFFER_SIZE);
    if (next == sc_tail) return;

    sc_buf[sc_head] = sc;
    sc_head = next;
}

void DG_Init(void) {
    terminal_set_graphics(1);
    kb_set_doom_mode(1);
}

void DG_DrawFrame(void) {
    if (!I_VideoBuffer) return;
    kmemcpy(vga_get_backbuffer(), I_VideoBuffer,
            DOOMGENERIC_RESX * DOOMGENERIC_RESY);
    vga_swap();
}

uint32_t DG_GetTicksMs(void) { return timer_ticks() * 10; }

void DG_SleepMs(uint32_t ms) {
    uint32_t target = DG_GetTicksMs() + ms;
    while (DG_GetTicksMs() < target)
        __asm__ volatile ("hlt");
}

int DG_GetKey(int *pressed, unsigned char *doomKey) {
    if (sc_head == sc_tail) return 0;
    uint8_t sc  = sc_buf[sc_tail];
    sc_tail     = (uint8_t)((sc_tail + 1) % SC_BUFFER_SIZE);
    uint8_t raw = sc & 0x7F;
    *pressed    = !(sc & 0x80);
    unsigned char dk = (raw < 128) ? sc_to_doom[raw] : 0;
    if (!dk) return 0;
    *doomKey = dk;
    return 1;
}

void DG_SetWindowTitle(const char *title) { (void)title; }

static const char *doom_argv[] = {
    "doom", "-iwad", "doom1.wad", "-nosound", "-nomusic", "-nomouse", "-warp", "1", "1", 0
};

void doom_main(void) {
    /* Inicializa VBE antes do Doom para garantir que a janela
       do QEMU já está em 320x200 quando DG_DrawFrame for chamado */
    vga_init_mode13h();
    doomgeneric_Create(3, (char**)doom_argv);
}
