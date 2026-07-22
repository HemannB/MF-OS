#include "paging.h"

#define PAGE_PRESENT  0x1
#define PAGE_WRITABLE 0x2
#define PAGE_SIZE     4096
#define TABLE_SIZE    (1024u * PAGE_SIZE)
#define MAX_IDENTITY_TABLES 128u
#define MAX_IDENTITY_BYTES  (MAX_IDENTITY_TABLES * TABLE_SIZE)
#define FALLBACK_MEMORY_BYTES (16u * 1024u * 1024u)

static uint32_t page_directory[1024]   __attribute__((aligned(4096)));
static uint32_t page_tables[MAX_IDENTITY_TABLES][1024] __attribute__((aligned(4096)));
static uint32_t lfb_table[1024]        __attribute__((aligned(4096)));
static uint32_t identity_limit = 0;

uint32_t paging_init(uint32_t memory_bytes) {
    if (!memory_bytes) memory_bytes = FALLBACK_MEMORY_BYTES;
    if (memory_bytes > MAX_IDENTITY_BYTES) memory_bytes = MAX_IDENTITY_BYTES;
    identity_limit = memory_bytes & ~(PAGE_SIZE - 1);

    for (int i = 0; i < 1024; i++)
        page_directory[i] = 0;

    /* Mapeia somente a RAM reportada pelo bootloader, limitada a 512MB. */
    for (uint32_t t = 0; t < MAX_IDENTITY_TABLES; t++) {
        for (int i = 0; i < 1024; i++) {
            uint32_t phys = (t * 1024u + (uint32_t)i) * PAGE_SIZE;
            page_tables[t][i] = phys < identity_limit
                ? phys | PAGE_PRESENT | PAGE_WRITABLE
                : 0;
        }
        if (t * TABLE_SIZE < identity_limit)
            page_directory[t] = (uint32_t)page_tables[t] | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /* mapeia LFB do Bochs VBE: 0xFD000000 → entry 1012 no page directory */
    for (int i = 0; i < 1024; i++)
        lfb_table[i] = (0xFD000000 + i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    page_directory[1012] = (uint32_t)lfb_table | PAGE_PRESENT | PAGE_WRITABLE;

    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(page_directory) : "eax"
    );

    return identity_limit;
}

uint32_t paging_identity_limit(void) {
    return identity_limit;
}

void paging_map_lfb(void) {
    /* já mapeado em paging_init — noop */
}
