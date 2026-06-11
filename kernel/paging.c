#include "paging.h"

#define PAGE_PRESENT    0x1   // página presente na memória
#define PAGE_WRITABLE   0x2   // página com permissão de escrita
#define PAGE_SIZE       4096  // tamanho de uma página (4KB)

// Tabelas de páginas alinhadas a 4KB, necessárias para o sistema de paginação funcionar corretamente
static uint32_t page_directory[1024] __attribute__((aligned(4096))); 
// A tabela de páginas para o primeiro diretório de páginas, que mapeia os primeiros 4MB de memória física para os primeiros 4MB de memória virtual
/* 4 page tables = 16MB de memória mapeada */
static uint32_t page_table_0[1024] __attribute__((aligned(4096))); /* 0MB - 4MB  */
static uint32_t page_table_1[1024] __attribute__((aligned(4096))); /* 4MB - 8MB  */
static uint32_t page_table_2[1024] __attribute__((aligned(4096))); /* 8MB - 12MB */
static uint32_t page_table_3[1024] __attribute__((aligned(4096))); /* 12MB - 16MB */

//Função para inicializar o sistema de paginação, configurando as tabelas de páginas e ativando a paginação no processador
void paging_init(void) {
    // mapeia 16MB com identity mapping — 4 page tables de 4MB cada
    for (int i = 0; i < 1024; i++) {
        page_table_0[i] = (i * PAGE_SIZE)              | PAGE_PRESENT | PAGE_WRITABLE;
        page_table_1[i] = (i * PAGE_SIZE + 0x400000)   | PAGE_PRESENT | PAGE_WRITABLE;
        page_table_2[i] = (i * PAGE_SIZE + 0x800000)   | PAGE_PRESENT | PAGE_WRITABLE;
        page_table_3[i] = (i * PAGE_SIZE + 0xC00000)   | PAGE_PRESENT | PAGE_WRITABLE;
    }

    // aponta as 4 primeiras entradas do page directory para as page tables
    page_directory[0] = (uint32_t) page_table_0 | PAGE_PRESENT | PAGE_WRITABLE;
    page_directory[1] = (uint32_t) page_table_1 | PAGE_PRESENT | PAGE_WRITABLE;
    page_directory[2] = (uint32_t) page_table_2 | PAGE_PRESENT | PAGE_WRITABLE;
    page_directory[3] = (uint32_t) page_table_3 | PAGE_PRESENT | PAGE_WRITABLE;

    // zera o resto do page directory 
    for (int i = 4; i < 1024; i++)
        page_directory[i] = 0;

    // carrega CR3 e ativa paginação no CR0 
    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(page_directory)
        : "eax"
    );
}