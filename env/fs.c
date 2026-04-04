/* =============================================================================
 * AI Aura OS — Virtual Filesystem Implementation
 * =============================================================================*/

#include "fs.h"
#include "../kernel/kstring.h"
#include "../kernel/memory.h"

static fs_node_t fs_table[FS_MAX_FILES];

/* -------------------------------------------------------------------------- */

void fs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        fs_table[i].valid = 0;
        fs_table[i].size  = 0;
    }
    /* Create root directory */
    fs_create("/", FS_TYPE_DIR);
}

int fs_create(const char *name, fs_node_type_t type) {
    if (!name) return -1;
    if (fs_find(name)) return -1; /* Already exists */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!fs_table[i].valid) {
            strncpy_k(fs_table[i].name, name, FS_NAME_LEN);
            fs_table[i].type  = type;
            fs_table[i].size  = 0;
            fs_table[i].valid = 1;
            return 0;
        }
    }
    return -1; /* Full */
}

int fs_write(const char *name, const uint8_t *data, uint32_t len) {
    fs_node_t *node = fs_find(name);
    if (!node || node->type != FS_TYPE_FILE) return -1;
    if (len > FS_DATA_SIZE) len = FS_DATA_SIZE;
    memcpy_k(node->data, data, (size_t)len);
    node->size = len;
    return (int)len;
}

int fs_read(const char *name, uint8_t *buf, uint32_t len) {
    fs_node_t *node = fs_find(name);
    if (!node || node->type != FS_TYPE_FILE) return -1;
    if (len > node->size) len = node->size;
    memcpy_k(buf, node->data, (size_t)len);
    return (int)len;
}

int fs_delete(const char *name) {
    fs_node_t *node = fs_find(name);
    if (!node) return -1;
    node->valid = 0;
    node->size  = 0;
    return 0;
}

fs_node_t *fs_find(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_table[i].valid &&
            strncmp_k(fs_table[i].name, name, FS_NAME_LEN) == 0) {
            return &fs_table[i];
        }
    }
    return NULL;
}

void fs_list(void) {
    /* Listing is performed by the caller using the VGA driver to avoid a
     * circular dependency between the VFS layer and the display layer.
     * Iterate fs_table[] directly and call vga_print() from context where
     * the VGA driver is available (e.g. from kernel/menu.c). */
    (void)fs_table;
}
