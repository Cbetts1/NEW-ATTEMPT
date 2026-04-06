#ifndef ATA_H
#define ATA_H

/* =============================================================================
 * AI Aura OS — ATA PIO Disk Driver
 *
 * Minimal ATA PIO mode driver for the primary IDE controller.
 * Supports reading and writing 512-byte sectors using programmed I/O (PIO).
 *
 * The floppy disk image (AIOS.img, 1.44 MB = 2880 sectors) is typically
 * accessed via the floppy controller, but QEMU also exposes it as a drive
 * accessible through this interface when passed with `-drive`.
 *
 * For QEMU floppy images this driver accesses the primary master drive.
 * =============================================================================*/

#include <stdint.h>
#include "../kernel/kernel.h"

#define ATA_SECTOR_SIZE     512u

/* Read one or more 512-byte sectors from the primary master drive.
 * lba    : 28-bit Logical Block Address of the first sector
 * count  : number of sectors to read (1–255)
 * buf    : caller-supplied buffer (must be at least count*512 bytes) */
aura_status_t ata_read_sectors(uint32_t lba, uint8_t count, uint8_t *buf);

/* Write one or more 512-byte sectors to the primary master drive.
 * lba    : 28-bit Logical Block Address of the first sector
 * count  : number of sectors to write (1–255)
 * buf    : data to write (must be at least count*512 bytes) */
aura_status_t ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t *buf);

/* Initialise and identify the primary master drive.
 * Returns AURA_OK if a drive is present and responds correctly. */
aura_status_t ata_init(void);

#endif /* ATA_H */
