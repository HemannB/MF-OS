#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT_INFO_MEMORY  (1u << 0)
#define MULTIBOOT_INFO_MODULES (1u << 3)

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t reserved;
} __attribute__((packed)) multiboot_module_t;

static inline uint32_t multiboot_memory_bytes(const multiboot_info_t *info) {
    if (!info || !(info->flags & MULTIBOOT_INFO_MEMORY)) return 0;
    uint64_t total = ((uint64_t)info->mem_upper + 1024u) * 1024u;
    return total > UINT32_MAX ? UINT32_MAX : (uint32_t)total;
}

#endif
