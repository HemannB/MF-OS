#include "pic.h"
#include "io.h"
#include <stdint.h>

// Função para incializar o PIC configurando com ICW (Initialization Command Words) para remapear as interrupções e mascarar todas as IRQs inicialmente
void pic_init(void) {
    // ICW1 — inicia a sequência de configuração
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2 — define os offsets das IRQs
    outb(PIC1_DATA, 0x20);    // Master: IRQ0-7 → 0x20-0x27
    outb(PIC2_DATA, 0x28);   // Slave:  IRQ8-15 → 0x28-0x2F

    // ICW3 — conecta Master e Slave
    outb(PIC1_DATA, 0x04);   // Master: Slave está na IRQ2
    outb(PIC2_DATA, 0x02);   // Slave: identidade 2

    // ICW4 — modo 8086
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // mascara todas as IRQs por enquanto
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

// Função para enviar um sinal de fim de interrupção (EOI) ao PIC após o tratamento de uma interrupção, indicando que o PIC pode enviar a próxima interrupção
void pic_eoi(uint8_t irq) {
    //Se a IRQ veio do Slave PIC (IRQ >= 8), precisa enviar EOI para os dois — Slave e Master. Se veio do Master, só para o Master.
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

// Função otimizada para enviar EOI específico para a IRQ0 (timer), já que é a mais comum e pode ser otimizada para evitar a verificação de qual PIC enviar o EOI
void pic_eoi_irq0(void) {
    outb(PIC1_COMMAND, PIC_EOI);
}

