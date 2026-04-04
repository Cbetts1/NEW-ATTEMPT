/* =============================================================================
 * AI Aura OS — Virtual Filesystem Adapter
 * File: adapters/aura_fs.c
 *
 * Provides a minimal in-memory virtual filesystem layer.  All file operations
 * are self-contained within the OS image — no host OS filesystem access.
 * =========================================================================== */
#include "../kernel/include/plugin.h"
#include "../kernel/include/vga.h"
#include "../kernel/include/memory.h"

#define VFS_MAX_FILES   32
#define VFS_NAME_LEN    64
#define VFS_DATA_LEN    512

typedef struct {
    char    name[VFS_NAME_LEN];
    uint8_t data[VFS_DATA_LEN];
    int     used;
    size_t  size;
} vfs_file_t;

static vfs_file_t vfs_table[VFS_MAX_FILES];
static int        vfs_count = 0;

static int fs_init(void)
{
    kmemset(vfs_table, 0, sizeof(vfs_table));
    vfs_count = 0;

    /* Pre-populate a small virtual filesystem */
    static const char readme[] = "AI Aura OS v1.0 — virtual filesystem\n";
    int idx = vfs_count++;
    kmemcpy(vfs_table[idx].name, "/README", 8);
    kmemcpy(vfs_table[idx].data, readme, sizeof(readme) - 1);
    vfs_table[idx].size = sizeof(readme) - 1;
    vfs_table[idx].used = 1;

    vga_puts("[FS ] Virtual filesystem adapter online. Files: ");
    vga_printf("%d\n", vfs_count);
    return 0;
}

static int fs_tick(void)
{
    return 0;
}

static void fs_shutdown(void)
{
    vga_puts("[FS ] VFS adapter shutting down.\n");
}

plugin_descriptor_t aura_fs_adapter = {
    .name     = "aura.fs",
    .version  = 0x0100,
    .type     = PLUGIN_TYPE_ADAPTER,
    .init     = fs_init,
    .tick     = fs_tick,
    .shutdown = fs_shutdown,
};
