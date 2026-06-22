#include "heap.h"
#include "libc.h"

static uint8_t *heap_ptr = 0;
static uint8_t *heap_end = 0;

/* inicializa o heap a partir de 'base' com 'size' bytes disponíveis
   base é calculada em kernel_main via fs_heap_base() para não
   sobrescrever módulos GRUB (WAD, etc.) */
void heap_init(uint32_t base, uint32_t size) {
    heap_ptr = (uint8_t*) base;
    heap_end = (uint8_t*)(base + size);
}

/* aloca 'size' bytes alinhados a 4 bytes — bump allocator sem free
   O Doom usa Zone Memory internamente, então malloc/free viram shims
   que delegam aqui: o Doom faz 1 malloc grande e gerencia o resto */
void* kmalloc(size_t size) {
    size = (size + 3) & ~3;
    if (!heap_ptr || heap_ptr + size > heap_end) return 0;
    void *block = heap_ptr;
    heap_ptr += size;
    return block;
}

/* kmalloc + zera o bloco (equivalente a calloc) */
void* kzalloc(size_t size) {
    void *p = kmalloc(size);
    if (p) kmemset(p, 0, size);
    return p;
}