#include "timer.h"
#include "idt.h"
#include "pic.h"
#include "process.h"

static volatile uint32_t ticks = 0;

/* O scheduler só troca o frame quando process_run() está ativo. */
uint32_t timer_handler(uint32_t current_esp) {
    ticks++;
    return process_schedule_from_irq(current_esp);
}

uint32_t timer_ticks(void) {
    return ticks;
}

void timer_init(void) {
    /* PIT canal 0, modo 3 (square wave), ~100Hz (divisor 11931) */
    uint16_t divisor = 11931;

    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x36),              "Nd"((uint16_t)0x43));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(divisor & 0xFF)),  "Nd"((uint16_t)0x40));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(divisor >> 8)),    "Nd"((uint16_t)0x40));

    extern void irq0_wrapper(void);
    idt_set_entry(32, (uint32_t) irq0_wrapper, 0x08, 0x8E);

    /* habilita IRQ0 no Master PIC */
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xFC), "Nd"((uint16_t)0x21));
}
