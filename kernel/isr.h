#ifndef ISR_H
#define ISR_H

#include <stdint.h>

void isr_init(void);
void irq1_handler(void);
char kb_getchar(void);
void kb_set_doom_mode(int on);  /* 1 = envia scancodes raw para o Doom */

#endif