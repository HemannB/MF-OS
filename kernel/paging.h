#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

// Retorna o limite efetivamente mapeado por identity mapping.
uint32_t paging_init(uint32_t memory_bytes);
uint32_t paging_identity_limit(void);


void paging_map_lfb(void);
#endif
