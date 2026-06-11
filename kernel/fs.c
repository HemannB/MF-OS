#include "fs.h"
#include <stdint.h>

/* tabela interna de arquivos carregados pelo GRUB */
static fs_file_t files[FS_MAX_FILES];
static int       file_count = 0;

/* estrutura da Multiboot info — passada pelo GRUB via EBX */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;  /* quantidade de módulos carregados */
    uint32_t mods_addr;   /* endereço da lista de módulos */
} __attribute__((packed)) multiboot_info_t;

/* cada entrada na lista de módulos */
typedef struct {
    uint32_t mod_start;   /* endereço de início do módulo na memória */
    uint32_t mod_end;     /* endereço de fim */
    uint32_t cmdline;     /* nome/linha de comando do módulo */
    uint32_t reserved;
} __attribute__((packed)) multiboot_module_t;

void fs_init(uint32_t multiboot_addr) {
    multiboot_info_t *mb = (multiboot_info_t*) multiboot_addr;

    /* verifica se o GRUB carregou algum módulo (bit 3 das flags) */
    if (!(mb->flags & (1 << 3))) return;

    multiboot_module_t *mods = (multiboot_module_t*) mb->mods_addr;

    for (uint32_t i = 0; i < mb->mods_count && file_count < FS_MAX_FILES; i++) {
        /* nome do arquivo vem da cmdline do módulo */
        char *cmdline = (char*) mods[i].cmdline;
        
        /* extrai só o nome do arquivo do caminho completo */
        char *name = cmdline;
        for (char *p = cmdline; *p; p++)
            if (*p == '/') name = p + 1;

        /* copia o nome para a tabela */
        int j = 0;
        while (name[j] && j < FS_NAME_MAX - 1) {
            files[file_count].name[j] = name[j];
            j++;
        }
        files[file_count].name[j] = '\0';

        files[file_count].data = (uint8_t*) mods[i].mod_start;
        files[file_count].size = mods[i].mod_end - mods[i].mod_start;
        file_count++;
    }
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