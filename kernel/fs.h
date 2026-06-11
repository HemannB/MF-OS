#ifndef FS_H
#define FS_H
#include <stdint.h>

#define FS_MAX_FILES 16
#define FS_NAME_MAX  64



typedef struct {
    char     name[FS_NAME_MAX]; /* nome do arquivo */
    uint8_t *data;              /* ponteiro para os dados em memória */
    uint32_t size;              /* tamanho em bytes */
} fs_file_t;

void      fs_init(uint32_t multiboot_addr);
fs_file_t *fs_open(const char *name);
uint32_t   fs_read(fs_file_t *f, void *buf, uint32_t size, uint32_t offset);
uint32_t   fs_size(fs_file_t *f);
int        fs_count(void);
fs_file_t *fs_get_file(int index);

#endif