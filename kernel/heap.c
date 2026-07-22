#include "heap.h"
#include "libc.h"

static uint8_t *heap_ptr = 0;
static uint8_t *heap_end = 0;

/* inicializa o heap a partir de 'base' com 'size' bytes disponíveis
   base é calculada em kernel_main via fs_heap_base() para não
   sobrescrever módulos GRUB (WAD, etc.) */
void heap_init(uint32_t base, uint32_t size) {
    if (!base || size > UINT32_MAX - base) {
        heap_ptr = 0;
        heap_end = 0;
        return;
    }
    heap_ptr = (uint8_t*) base;
    heap_end = (uint8_t*)(base + size);
}

/* aloca 'size' bytes alinhados a 4 bytes — bump allocator sem free
   O Doom usa Zone Memory internamente, então malloc/free viram shims
   que delegam aqui: o Doom faz 1 malloc grande e gerencia o resto */
void* kmalloc(size_t size) {
    if (!size || size > SIZE_MAX - sizeof(size_t) - 3) return 0;

    size_t aligned_size = (size + 3) & ~3;
    size_t allocation_size = sizeof(size_t) + aligned_size;
    if (!heap_ptr || heap_ptr > heap_end) return 0;
    if (allocation_size > (size_t)(heap_end - heap_ptr)) return 0;

    size_t *header = (size_t*)heap_ptr;
    *header = size;
    heap_ptr += allocation_size;
    return header + 1;
}

/* kmalloc + zera o bloco (equivalente a calloc) */
void* kzalloc(size_t size) {
    void *p = kmalloc(size);
    if (p) kmemset(p, 0, size);
    return p;
}

size_t kmalloc_size(const void *ptr) {
    if (!ptr) return 0;
    return ((const size_t*)ptr)[-1];
}
