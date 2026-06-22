#include "paging.h"

#define PAGE_PRESENT  0x1
#define PAGE_WRITABLE 0x2
#define PAGE_SIZE     4096

static uint32_t page_directory[1024]   __attribute__((aligned(4096)));
static uint32_t page_tables[128][1024] __attribute__((aligned(4096)));
static uint32_t lfb_table[1024]        __attribute__((aligned(4096)));

void paging_init(void) {
    for (int i = 0; i < 1024; i++)
        page_directory[i] = 0;

    /* identity map 512MB */
    for (int t = 0; t < 128; t++) {
        for (int i = 0; i < 1024; i++) {
            uint32_t phys = (uint32_t)(t * 1024 + i) * PAGE_SIZE;
            page_tables[t][i] = phys | PAGE_PRESENT | PAGE_WRITABLE;
        }
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
}

void paging_map_lfb(void) {
    /* já mapeado em paging_init — noop */
}
