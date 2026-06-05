#ifndef ISR_H
#define ISR_H

#include <stdint.h>

void isr_init(void); // Função para inicializar as ISRs (Interrupt Service Routines) e configurar a IDT (Interrupt Descriptor Table) com os handlers de interrupção apropriados
void irq1_handler(void); // Handler para a IRQ1 (teclado), que será chamado quando uma interrupção de teclado ocorrer, lendo o scancode da tecla pressionada e processando-a para interagir com o terminal
char kb_getchar(void); // Função para ler um caracter do teclado usando o controlador de teclado do PC (porta 0x60 para dados e 0x64 para status)

#endif