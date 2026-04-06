#ifndef FS_H
#define FS_H

/* =============================================================================
 * AI Aura OS — Virtual Filesystem (VFS) Interface
 * Minimal in-memory filesystem that lives entirely inside the OS image.
 * Provides file creation, read, write, and directory listing.
 * =============================================================================*/

#include <stdint.h>
#include <stddef.h>

#define FS_MAX_FILES    32
#define FS_MAX_DIRS     8
#define FS_NAME_LEN     24
#define FS_DATA_SIZE    512     /* Max bytes per file (embedded) */

typedef enum {
    FS_TYPE_FILE = 0,
    FS_TYPE_DIR  = 1,
} fs_node_type_t;

typedef struct {
    char           name[FS_NAME_LEN];
    fs_node_type_t type;
    uint32_t       size;
    uint8_t        data[FS_DATA_SIZE];
    uint8_t        valid;
} fs_node_t;

void        fs_init(void);
int         fs_create(const char *name, fs_node_type_t type);
int         fs_write(const char *name, const uint8_t *data, uint32_t len);
int         fs_read(const char *name, uint8_t *buf, uint32_t len);
int         fs_delete(const char *name);
fs_node_t  *fs_find(const char *name);
void        fs_list(void);
void        fs_list_vga(void);      /* Print all VFS entries to VGA */

#endif /* FS_H */
