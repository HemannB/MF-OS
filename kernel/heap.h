#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

/* heap_init recebe a base dinâmica (após módulos GRUB) e o tamanho desejado.
   Para o Doom: base = fs_heap_base(), size = 16MB (0x1000000) */
void  heap_init(uint32_t base, uint32_t size);
void* kmalloc(size_t size);
void* kzalloc(size_t size);  /* kmalloc + zera o bloco */
size_t kmalloc_size(const void *ptr);

#endif
