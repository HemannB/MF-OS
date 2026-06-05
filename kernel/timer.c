#include "timer.h"
#include "idt.h"
#include "pic.h"

static volatile uint32_t ticks = 0; // Contador de ticks

// Função chamada a cada interrupção do timer
void timer_handler(void) {
    ticks++;
}

// Função para retornar o número de ticks desde a inicialização do timer
uint32_t timer_ticks(void) {
    return ticks;
}

void timer_init(void) {
    // PIT frequência base: 1193180 Hz divisor = 1193180 / 100 = 11931 → ~100Hz (10ms por tick)
    uint16_t divisor = 1193;

    // ICW: canal 0, modo de operação 3 (square wave), acesso de baixo byte seguido de alto byte 
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x36), "Nd"((uint16_t)0x43));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(divisor & 0xFF)), "Nd"((uint16_t)0x40));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(divisor >> 8)), "Nd"((uint16_t)0x40));

    extern void irq0_wrapper(void);
    idt_set_entry(32, (uint32_t) irq0_wrapper, 0x08, 0x8E);

    /* habilita IRQ0 no Master PIC — bit 0 = 0 */
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xFC), "Nd"((uint16_t)0x21));
}