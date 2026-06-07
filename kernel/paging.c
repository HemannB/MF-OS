#include "paging.h"

#define PAGE_PRESENT    0x1   // página presente na memória
#define PAGE_WRITABLE   0x2   // página com permissão de escrita
#define PAGE_SIZE       4096  // tamanho de uma página (4KB)

// Tabelas de páginas alinhadas a 4KB, necessárias para o sistema de paginação funcionar corretamente
static uint32_t page_directory[1024] __attribute__((aligned(4096))); 
// A tabela de páginas para o primeiro diretório de páginas, que mapeia os primeiros 4MB de memória física para os primeiros 4MB de memória virtual
static uint32_t page_table[1024]     __attribute__((aligned(4096)));


//Função para inicializar o sistema de paginação, configurando as tabelas de páginas e ativando a paginação no processador
void paging_init(void) {
    // Preenche a tabela de páginas para mapear os primeiros 4MB de memória física para os primeiros 4MB de memória virtual, marcando cada página como presente e com permissão de escrita
    for (int i = 0; i < 1024; i++) {
        page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    // Configura o diretório de páginas para apontar para a tabela de páginas criada, marcando-a como presente e com permissão de escrita
    page_directory[0] = (uint32_t) page_table | PAGE_PRESENT | PAGE_WRITABLE;

    // Marca as entradas restantes do diretório de páginas como não presentes, indicando que não há mapeamento para essas regiões de memória
    for (int i = 1; i < 1024; i++) {
        page_directory[i] = 0;
    }

    // Ativa a paginação no processador, carregando o endereço do diretório de páginas para o registrador CR3 e configurando o bit de habilitação de paginação no registrador CR0
    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(page_directory)
        : "eax"
    );
}