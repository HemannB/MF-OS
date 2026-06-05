#include "gdt.h"

static gdt_entry_t gdt[3]; // GDT com 3 entradas: null, code e data
static gdt_ptr_t   gdt_ptr; // Ponteiro para a GDT que será passado para lgdt

// Função para preencher uma entrada da GDT
static void gdt_set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // Preenche os pedaços da base e do limite de acordo com o formato da GDT e os campos de acesso e granularidade (herança do 286)
    gdt[i].base_low    = (base & 0xFFFF);
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].base_high   = (base >> 24) & 0xFF;
    gdt[i].limit_low   = (limit & 0xFFFF);
    gdt[i].granularity = (limit >> 16) & 0x0F;
    gdt[i].granularity |= (gran & 0xF0);
    gdt[i].access      = access;
}

// Função para montar as 3 entradas da GDT e carregar na CPU
void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1; // O limite é o tamanho total da GDT - 1, pois o limite é baseado em bytes e começa do zero
    gdt_ptr.base  = (uint32_t) &gdt; // O endereço base da GDT é o endereço do array de entradas

    gdt_set_entry(0, 0, 0,          0x00, 0x00); // Segmento nulo: base 0, limite 0, acesso 0x00 (não presente), granularidade 0x00
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Segmento de código: base 0, limite máximo, acesso 0x9A (presente, ring 0, código executável, leitura permitida), granularidade 0xCF (4K granularity, 32-bit)
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Segmento de dados: base 0, limite máximo, acesso 0x92 (presente, ring 0, dados, escrita permitida), granularidade 0xCF (4K granularity, 32-bit)

    gdt_flush((uint32_t) &gdt_ptr); // Carrega a GDT usando a função em assembly que chama lgdt com o endereço do ponteiro da GDT
}