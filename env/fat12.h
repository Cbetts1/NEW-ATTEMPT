#ifndef FAT12_H
#define FAT12_H

/* =============================================================================
 * AI Aura OS — FAT12 Filesystem Driver
 *
 * Provides a minimal read/write interface to a FAT12 volume on the first
 * drive (typically a 1.44 MB floppy image).  Files are limited to a single
 * cluster for simplicity; this is sufficient for small configuration files
 * such as the serialised env registry.
 * =============================================================================*/

#include <stdint.h>
#include "../kernel/kernel.h"

/* Maximum bytes for a FAT12 read/write — one cluster on a 1.44 MB floppy */
#define FAT12_MAX_FILE_SIZE    512u    /* one sector */
#define FAT12_FILENAME_LEN     11      /* 8.3 format, space-padded */

/* Initialise the FAT12 layer (reads the BPB from sector 0).
 * Must be called after ata_init(). */
aura_status_t fat12_init(void);

/* Read a file by its 8.3 filename into buf (up to len bytes).
 * name : null-terminated 8.3 name, e.g. "AIOS.ENV"
 * Returns number of bytes read, or -1 on error. */
int fat12_read_file(const char *name, uint8_t *buf, uint32_t len);

/* Write / overwrite a file in the root directory.
 * name : null-terminated 8.3 name, e.g. "AIOS.ENV"
 * If the file already exists its content is replaced.
 * Returns 0 on success, -1 on error. */
int fat12_write_file(const char *name, const uint8_t *buf, uint32_t len);

#endif /* FAT12_H */
