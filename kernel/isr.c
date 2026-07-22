#include "isr.h"
#include "idt.h"
#include "pic.h"
#include "io.h"
#include <stdint.h>

#define KB_BUFFER_SIZE 256

void term_putchar(char c);

static const char sc_ascii[] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=','\b', 0,
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
    'z','x','c','v','b','n','m',',','.','/',  0,  '*', 0, ' '
};

static char             kb_buffer[KB_BUFFER_SIZE];
static volatile uint8_t kb_head = 0;
static volatile uint8_t kb_tail = 0;

/* quando 1, scancodes raw vão para o Doom em vez do terminal */
static volatile int doom_kb_mode = 0;

/* declaração do receptor do Doom — implementado em doomgeneric_mf0s.c */
extern void irq1_doom_push(uint8_t sc);

void kb_set_doom_mode(int on) {
    doom_kb_mode = on;
}

void irq1_handler(void) {
    uint8_t sc = inb(0x60);

    if (doom_kb_mode) {
        /* modo Doom: entrega scancode raw (com bit7=release) */
        irq1_doom_push(sc);
    } else {
        /* modo terminal: converte para ASCII e coloca no buffer */
        if (!(sc & 0x80) && sc < sizeof(sc_ascii)) {
            char c = sc_ascii[sc];
            if (c) {
                uint8_t next = (uint8_t)(kb_head + 1);
                if (next != kb_tail) {
                    kb_buffer[kb_head] = c;
                    kb_head = next;
                }
            }
        }
    }

    pic_eoi(1);
}

char kb_getchar(void) {
    while (kb_head == kb_tail);
    char c = kb_buffer[kb_tail];
    kb_tail = (uint8_t)(kb_tail + 1);
    return c;
}

void isr_init(void) {
    extern void irq1_wrapper(void);
    idt_set_entry(33, (uint32_t) irq1_wrapper, 0x08, 0x8E);
    outb(0x21, 0xFD);
}
