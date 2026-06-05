#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Estrutura de uma entrada na IDT (Interrupt Descriptor Table)
typedef struct {
    uint16_t base_low; // Endereço base do handler de interrupção (parte baixa)
    uint16_t selector; // Seletor de segmento de código (normalmente 0x08 para o kernel)
    uint8_t  zero; // Reservado, deve ser zero
    uint8_t  flags; // Flags de configuração da interrupção (tipo, privilégio, etc.)
    uint16_t base_high; // Endereço base do handler de interrupção (parte alta)
} __attribute__((packed)) idt_entry_t; // Garantir que a estrutura seja empacotada (sem padding)

// Estrutura do ponteiro para a IDT, usada para carregar a IDT com a instrução lidt
typedef struct {
    uint16_t limit; // Limite da IDT (tamanho total da tabela de descritores - 1)
    uint32_t base; // Endereço base da IDT (ponteiro para o início da tabela de descritores)
} __attribute__((packed)) idt_ptr_t; // Garantir que a estrutura seja empacotada (sem padding)

void idt_init(void);
void idt_set_entry(uint8_t n, uint32_t base, uint16_t selector, uint8_t flags);
void idt_flush(uint32_t idt_ptr);
#endif