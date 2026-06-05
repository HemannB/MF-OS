#ifndef PIC_H
#define PIC_H

#include <stdint.h>

#define PIC1_COMMAND  0x20 // Porta de comando do PIC mestre
#define PIC1_DATA     0x21 // Porta de dados do PIC mestre
#define PIC2_COMMAND  0xA0 // Porta de comando do PIC escravo
#define PIC2_DATA     0xA1 // Porta de dados do PIC escravo
#define PIC_EOI       0x20 // Comando para sinalizar o fim de uma interrupção (End of Interrupt)

void pic_init(void); // Função para inicializar o PIC para remapear as interrupções
void pic_eoi(uint8_t irq); // Função para enviar um sinal de fim de interrupção (EOI) ao PIC após o tratamento de uma interrupção
void pic_eoi_irq0(void); // Função para enviar EOI específico para a IRQ0 (timer), já que é a mais comum e pode ser otimizada

#endif