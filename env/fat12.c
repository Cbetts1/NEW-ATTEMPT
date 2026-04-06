/* =============================================================================
 * AI Aura OS — FAT12 Driver Implementation
 *
 * Supports a standard 1.44 MB floppy geometry:
 *   512 bytes/sector, 18 sectors/track, 2 heads, 80 cylinders
 *   BPB offset 0, FAT1 at sector 1, root dir at sector 19 (2 FATs × 9 sectors)
 * =============================================================================*/

#include "fat12.h"
#include "../adapters/ata.h"
#include "../kernel/memory.h"
#include "../kernel/kstring.h"
#include "../kernel/vga.h"

/* ---------------------------------------------------------------------------
 * Standard 1.44 MB floppy BPB constants (used as fallback if BPB read fails)
 * ---------------------------------------------------------------------------*/
#define BYTES_PER_SECTOR    512u
#define SECTORS_PER_CLUSTER 1u
#define RESERVED_SECTORS    1u
#define NUM_FATS            2u
#define ROOT_ENTRY_COUNT    224u
#define FAT_SIZE_SECTORS    9u      /* sectors per FAT */
#define ROOT_DIR_SECTORS    ((ROOT_ENTRY_COUNT * 32u + BYTES_PER_SECTOR - 1u) / BYTES_PER_SECTOR)

/* Derived layout values */
#define FAT1_LBA            RESERVED_SECTORS                    /* sector 1  */
#define FAT2_LBA            (FAT1_LBA + FAT_SIZE_SECTORS)      /* sector 10 */
#define ROOT_LBA            (FAT2_LBA + FAT_SIZE_SECTORS)      /* sector 19 */
#define DATA_LBA            (ROOT_LBA + ROOT_DIR_SECTORS)      /* sector 33 */

/* FAT12 directory entry (32 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  name[11];      /* 8.3 format, space-padded              */
    uint8_t  attr;          /* file attributes                        */
    uint8_t  reserved[10];  /* unused for our purposes                */
    uint16_t time;
    uint16_t date;
    uint16_t cluster;       /* first cluster number                   */
    uint32_t size;          /* file size in bytes                     */
} fat12_dirent_t;

#define ATTR_VOLUME_ID  0x08
#define ATTR_DIR        0x10
#define ATTR_LFN        0x0F

/* ---------------------------------------------------------------------------
 * Static buffers (avoid repeated kmalloc for small, fixed-size I/O)
 * ---------------------------------------------------------------------------*/
static uint8_t  s_sector_buf[BYTES_PER_SECTOR];     /* generic sector buffer */
static uint8_t  s_fat_buf[FAT_SIZE_SECTORS * BYTES_PER_SECTOR]; /* FAT1 cache */
static int      s_fat_loaded = 0;

/* ---------------------------------------------------------------------------
 * Convert a cluster number to the LBA of its first sector
 * ---------------------------------------------------------------------------*/
static uint32_t cluster_to_lba(uint16_t cluster) {
    return (uint32_t)(DATA_LBA + ((uint32_t)cluster - 2u) * SECTORS_PER_CLUSTER);
}

/* ---------------------------------------------------------------------------
 * FAT12 cluster chain lookup
 * Returns the next cluster for 'cluster', or 0xFFF on end-of-chain.
 * ---------------------------------------------------------------------------*/
static uint16_t fat12_next_cluster(uint16_t cluster) {
    uint32_t offset = (uint32_t)cluster + (uint32_t)(cluster / 2u);
    uint16_t entry;

    if (offset + 1u >= sizeof(s_fat_buf)) return 0xFFF;

    entry = (uint16_t)(s_fat_buf[offset] | ((uint16_t)s_fat_buf[offset + 1u] << 8));
    if (cluster & 1u) {
        entry >>= 4;
    } else {
        entry &= 0x0FFF;
    }
    return entry;
}

/* Write a FAT12 cluster entry back to the in-memory FAT buffer.
 * The FAT buffer must be flushed to disk by the caller. */
static void fat12_set_cluster(uint16_t cluster, uint16_t value) {
    uint32_t offset = (uint32_t)cluster + (uint32_t)(cluster / 2u);
    if (offset + 1u >= sizeof(s_fat_buf)) return;

    if (cluster & 1u) {
        s_fat_buf[offset]     = (uint8_t)((s_fat_buf[offset] & 0x0F) | ((value << 4) & 0xF0));
        s_fat_buf[offset + 1u]= (uint8_t)(value >> 4);
    } else {
        s_fat_buf[offset]     = (uint8_t)(value & 0xFF);
        s_fat_buf[offset + 1u]= (uint8_t)((s_fat_buf[offset + 1u] & 0xF0) | ((value >> 8) & 0x0F));
    }
}

/* Flush both FATs to disk */
static aura_status_t fat12_flush_fat(void) {
    for (uint8_t f = 0; f < NUM_FATS; f++) {
        uint32_t base_lba = (f == 0) ? FAT1_LBA : FAT2_LBA;
        for (uint8_t s = 0; s < FAT_SIZE_SECTORS; s++) {
            const uint8_t *src = s_fat_buf + (uint32_t)s * BYTES_PER_SECTOR;
            if (ata_write_sectors(base_lba + s, 1, src) != AURA_OK) {
                return AURA_ERR;
            }
        }
    }
    return AURA_OK;
}

/* Find a free cluster (value == 0 in FAT).  Returns 0 if none. */
static uint16_t fat12_alloc_cluster(void) {
    /* Clusters 0 and 1 are reserved; data clusters start at 2 */
    for (uint16_t c = 2u; c < 2848u; c++) {
        if (fat12_next_cluster(c) == 0u) return c;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Convert a null-terminated 8.3 name ("AIOS.ENV") to FAT 11-byte format
 * ---------------------------------------------------------------------------*/
static void name_to_fat83(const char *src, uint8_t *dst) {
    /* Fill with spaces */
    for (int i = 0; i < 11; i++) dst[i] = ' ';
    int di = 0;
    for (int i = 0; src[i] && di < 11; i++) {
        char c = src[i];
        if (c == '.') {
            di = 8; /* jump to extension */
        } else {
            /* Uppercase */
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            if (di < 11) dst[di++] = (uint8_t)c;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------*/

aura_status_t fat12_init(void) {
    /* Load FAT1 into the static buffer */
    for (uint8_t s = 0; s < FAT_SIZE_SECTORS; s++) {
        if (ata_read_sectors(FAT1_LBA + s, 1, s_fat_buf + (uint32_t)s * BYTES_PER_SECTOR)
                != AURA_OK) {
            return AURA_ERR;
        }
    }
    s_fat_loaded = 1;
    return AURA_OK;
}

int fat12_read_file(const char *name, uint8_t *buf, uint32_t len) {
    if (!s_fat_loaded || !name || !buf || len == 0) return -1;

    uint8_t fat83[11];
    name_to_fat83(name, fat83);

    /* Walk root directory entries */
    for (uint32_t entry = 0; entry < ROOT_ENTRY_COUNT; entry++) {
        uint32_t sector = ROOT_LBA + (entry * 32u) / BYTES_PER_SECTOR;
        uint32_t off    = (entry * 32u) % BYTES_PER_SECTOR;

        if (ata_read_sectors(sector, 1, s_sector_buf) != AURA_OK) return -1;

        fat12_dirent_t *de = (fat12_dirent_t *)(s_sector_buf + off);

        if (de->name[0] == 0x00) break;          /* end of directory */
        if (de->name[0] == 0xE5) continue;       /* deleted entry    */
        if (de->attr == ATTR_VOLUME_ID) continue;
        if (de->attr == ATTR_DIR)       continue;
        if (de->attr == ATTR_LFN)       continue;

        /* Name match? */
        int match = 1;
        for (int i = 0; i < 11; i++) {
            if (de->name[i] != fat83[i]) { match = 0; break; }
        }
        if (!match) continue;

        /* Found — read data */
        uint32_t to_read = de->size < len ? de->size : len;
        uint32_t lba = cluster_to_lba(de->cluster);
        if (ata_read_sectors(lba, 1, s_sector_buf) != AURA_OK) return -1;
        uint32_t copy = to_read < BYTES_PER_SECTOR ? to_read : BYTES_PER_SECTOR;
        for (uint32_t i = 0; i < copy; i++) buf[i] = s_sector_buf[i];
        return (int)copy;
    }
    return -1; /* Not found */
}

int fat12_write_file(const char *name, const uint8_t *buf, uint32_t len) {
    if (!s_fat_loaded || !name || !buf) return -1;
    if (len > BYTES_PER_SECTOR) len = BYTES_PER_SECTOR; /* cap at one sector */

    uint8_t fat83[11];
    name_to_fat83(name, fat83);

    /* Phase 1: look for an existing entry */
    uint32_t found_entry_sector = 0;
    uint32_t found_entry_off    = 0;
    uint16_t found_cluster      = 0;
    int      found              = 0;
    uint32_t free_entry_sector  = 0;
    uint32_t free_entry_off     = 0;
    int      free_found         = 0;

    for (uint32_t entry = 0; entry < ROOT_ENTRY_COUNT; entry++) {
        uint32_t sector = ROOT_LBA + (entry * 32u) / BYTES_PER_SECTOR;
        uint32_t off    = (entry * 32u) % BYTES_PER_SECTOR;

        if (ata_read_sectors(sector, 1, s_sector_buf) != AURA_OK) return -1;
        fat12_dirent_t *de = (fat12_dirent_t *)(s_sector_buf + off);

        if (de->name[0] == 0x00) {
            /* End of used entries — this slot and beyond are free */
            if (!free_found) {
                free_entry_sector = sector;
                free_entry_off    = off;
                free_found        = 1;
            }
            break;
        }
        if (de->name[0] == 0xE5) {
            if (!free_found) {
                free_entry_sector = sector;
                free_entry_off    = off;
                free_found        = 1;
            }
            continue;
        }
        if (de->attr == ATTR_VOLUME_ID || de->attr == ATTR_DIR ||
            de->attr == ATTR_LFN) continue;

        int match = 1;
        for (int i = 0; i < 11; i++) {
            if (de->name[i] != fat83[i]) { match = 0; break; }
        }
        if (match) {
            found_entry_sector = sector;
            found_entry_off    = off;
            found_cluster      = de->cluster;
            found              = 1;
            break;
        }
    }

    /* Phase 2: allocate a cluster if needed */
    uint16_t cluster;
    if (found) {
        cluster = found_cluster;
        /* Mark old cluster as EOF (reuse it) */
        fat12_set_cluster(cluster, 0xFFF);
    } else {
        if (!free_found) return -1; /* Directory full */
        cluster = fat12_alloc_cluster();
        if (cluster == 0) return -1; /* Disk full */
        fat12_set_cluster(cluster, 0xFFF); /* Mark as EOF */
    }

    /* Phase 3: write data sector */
    /* Copy data into a zeroed sector buffer */
    memset_k(s_sector_buf, 0, BYTES_PER_SECTOR);
    for (uint32_t i = 0; i < len; i++) s_sector_buf[i] = buf[i];
    uint32_t data_lba = cluster_to_lba(cluster);
    if (ata_write_sectors(data_lba, 1, s_sector_buf) != AURA_OK) return -1;

    /* Phase 4: write / update the directory entry */
    uint32_t target_sector = found ? found_entry_sector : free_entry_sector;
    uint32_t target_off    = found ? found_entry_off    : free_entry_off;
    if (ata_read_sectors(target_sector, 1, s_sector_buf) != AURA_OK) return -1;
    fat12_dirent_t *de = (fat12_dirent_t *)(s_sector_buf + target_off);
    for (int i = 0; i < 11; i++) de->name[i] = fat83[i];
    de->attr    = 0x20; /* Archive attribute */
    de->cluster = cluster;
    de->size    = len;
    de->time    = 0;
    de->date    = 0;
    for (int i = 0; i < 10; i++) de->reserved[i] = 0;
    if (ata_write_sectors(target_sector, 1, s_sector_buf) != AURA_OK) return -1;

    /* Phase 5: flush FAT */
    if (fat12_flush_fat() != AURA_OK) return -1;

    return (int)len;
}
