#include "fs.h"
#include "multiboot.h"
#include <stdint.h>

/* tabela interna de arquivos carregados pelo GRUB */
static fs_file_t files[FS_MAX_FILES];
static int       file_count  = 0;
static uint32_t  heap_base   = 0;   /* primeiro endereço livre após todos os módulos */

extern uint8_t kernel_end;

void fs_init(uint32_t multiboot_addr) {
    multiboot_info_t *mb = (multiboot_info_t*) multiboot_addr;

    file_count = 0;
    heap_base = ((uint32_t)&kernel_end + 0xFFF) & ~0xFFF;

    /* verifica se o GRUB carregou algum módulo (bit 3 das flags) */
    if (!(mb->flags & MULTIBOOT_INFO_MODULES)) return;

    multiboot_module_t *mods = (multiboot_module_t*) mb->mods_addr;

    for (uint32_t i = 0; i < mb->mods_count && file_count < FS_MAX_FILES; i++) {
        /* nome do arquivo vem da cmdline do módulo */
        char *cmdline = (char*) mods[i].cmdline;

        /* extrai só o nome do arquivo do caminho completo (após última '/') */
        char *name = cmdline;
        for (char *p = cmdline; *p && *p != ' '; p++)
            if (*p == '/') name = p + 1;

        /* copia o nome para a tabela — para no whitespace ou no terminador */
        int j = 0;
        while (name[j] && name[j] != ' ' && j < FS_NAME_MAX - 1) {
            files[file_count].name[j] = name[j];
            j++;
        }
        files[file_count].name[j] = '\0';

        files[file_count].data = (uint8_t*) mods[i].mod_start;
        files[file_count].size = mods[i].mod_end - mods[i].mod_start;

        /* rastreia o endereço mais alto usado pelos módulos, alinhado a 4KB */
        uint32_t mod_end_aligned = (mods[i].mod_end + 0xFFF) & ~0xFFF;
        if (mod_end_aligned > heap_base)
            heap_base = mod_end_aligned;

        file_count++;
    }
}

/* retorna o primeiro endereço livre após todos os módulos GRUB (alinhado a 4KB)
   o heap deve começar aqui para não sobrescrever o WAD */
uint32_t fs_heap_base(void) {
    return heap_base;
}

/* procura um arquivo pelo nome na tabela — retorna NULL se não encontrar */
fs_file_t *fs_open(const char *name) {
    for (int i = 0; i < file_count; i++) {
        int j = 0;
        while (files[i].name[j] && name[j] && files[i].name[j] == name[j])
            j++;
        if (!files[i].name[j] && !name[j])
            return &files[i];
    }
    return 0;
}

/* lê 'size' bytes do arquivo a partir do offset — retorna bytes lidos */
uint32_t fs_read(fs_file_t *f, void *buf, uint32_t size, uint32_t offset) {
    if (!f || offset >= f->size) return 0;
    if (offset + size > f->size) size = f->size - offset;
    uint8_t *dst = (uint8_t*) buf;
    for (uint32_t i = 0; i < size; i++)
        dst[i] = f->data[offset + i];
    return size;
}

/* retorna o tamanho do arquivo em bytes */
uint32_t fs_size(fs_file_t *f) {
    if (!f) return 0;
    return f->size;
}

int fs_count(void) {
    return file_count;
}

fs_file_t *fs_get_file(int index) {
    if (index < 0 || index >= file_count) return 0;
    return &files[index];
}
