#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Descritor de segmento da GDT
typedef struct {
    uint16_t limit_low; // Limite inferior do segmento
    uint16_t base_low; // Base inferior do segmento
    uint8_t  base_mid; // Base do segmento (bits 16-23)
    uint8_t  access; // Acesso ao segmento
    uint8_t  granularity; // Granularidade
    uint8_t  base_high; // Base do segmento (bits 24-31)
} __attribute__((packed)) gdt_entry_t; // Garante que a estrutura seja empacotada sem preenchimento

// Estrutura de ponteiro para a GDT
typedef struct {
    uint16_t limit; // Limite da GDT
    uint32_t base; // Endereço base da GDT
} __attribute__((packed)) gdt_ptr_t; // Garante que a estrutura seja empacotada sem preenchimento

void gdt_init(void); // Função para inicializar a GDT

void gdt_flush(uint32_t gdt_ptr); // Função em assembly para carregar a GDT na CPU com a instrução lgdt e recarregar os registradores de segmento.

#endif
