#include "heap.h"

#define HEAP_START 0x200000   // Endereço inicial do heap (2MB)
#define HEAP_SIZE  0x100000   // Tamanho do heap (1MB)

static uint8_t *heap_ptr; // Ponteiro que aponta (avá) para o próximo endereço livre no heap
static uint8_t *heap_end; // Ponteiro que aponta (avá²) para o limite do heap

// Função para inicializar o heap, definindo os ponteiros de início e fim do heap
void heap_init(void) {
    heap_ptr = (uint8_t*) HEAP_START; // O ponteiro do heap começa no endereço definido por HEAP_START
    heap_end = (uint8_t*) (HEAP_START + HEAP_SIZE); // O ponteiro do fim do heap é o endereço inicial mais o tamanho do heap, definindo o limite do heap
}

// Função para alocar memória no heap, retornando um ponteiro para o bloco alocado ou 0 se não houver memória suficiente
void* kmalloc(size_t size) {
    size = (size + 3) & ~3; // Alinha o tamanho para múltiplos de 4 bytes, garantindo que o ponteiro retornado seja alinhado corretamente para a maioria dos tipos de dados
    /*
        size = 5:
        5 + 3 = 8
        8 em binário:  0000 1000
        ~3 em binário: 1111 1100
        AND:           0000 1000 = 8  <-- Alinhado
        size = 7:
        7 + 3 = 10
        10 em binário: 0000 1010
        ~3 em binário: 1111 1100
        AND:           0000 1000 = 8  <- Alinhado 
        size = 8:
        8 + 3 = 11
        11 em binário: 0000 1011
        ~3 em binário: 1111 1100
        AND:           0000 1000 = 8  <- Deu pra entender.....
    */
    if (heap_ptr + size > heap_end) return 0; // Verifica se há espaço suficiente no heap para alocar o bloco solicitado; se não houver, retorna 0 (indicando falha na alocação)

    void *block = heap_ptr; // Armazena o endereço do bloco que será alocado, que é o endereço atual do ponteiro do heap
    heap_ptr += size; // Avança o ponteiro do heap pelo tamanho do bloco alocado, preparando-o para a próxima alocação
    return block;
}