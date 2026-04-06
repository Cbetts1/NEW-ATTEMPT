/* =============================================================================
 * AI Aura OS — ATA PIO Driver Implementation
 * =============================================================================*/

#include "ata.h"
#include "../kernel/io.h"
#include "../kernel/vga.h"

/* ---------------------------------------------------------------------------
 * Primary IDE controller register addresses (I/O base 0x1F0)
 * ---------------------------------------------------------------------------*/
#define ATA_PRIMARY_BASE    0x1F0u
#define ATA_REG_DATA        (ATA_PRIMARY_BASE + 0)  /* 16-bit data port       */
#define ATA_REG_ERROR       (ATA_PRIMARY_BASE + 1)  /* error register (r)     */
#define ATA_REG_FEATURES    (ATA_PRIMARY_BASE + 1)  /* features register (w)  */
#define ATA_REG_SECCOUNT    (ATA_PRIMARY_BASE + 2)  /* sector count           */
#define ATA_REG_LBA0        (ATA_PRIMARY_BASE + 3)  /* LBA bits 0–7           */
#define ATA_REG_LBA1        (ATA_PRIMARY_BASE + 4)  /* LBA bits 8–15          */
#define ATA_REG_LBA2        (ATA_PRIMARY_BASE + 5)  /* LBA bits 16–23         */
#define ATA_REG_HDSEL       (ATA_PRIMARY_BASE + 6)  /* drive / LBA bits 24–27 */
#define ATA_REG_STATUS      (ATA_PRIMARY_BASE + 7)  /* status (r)             */
#define ATA_REG_COMMAND     (ATA_PRIMARY_BASE + 7)  /* command (w)            */
#define ATA_PRIMARY_CTRL    0x3F6u                  /* alt-status / control   */

/* ATA status bits */
#define ATA_SR_ERR   0x01
#define ATA_SR_DRQ   0x08
#define ATA_SR_SRV   0x10
#define ATA_SR_DF    0x20
#define ATA_SR_RDY   0x40
#define ATA_SR_BSY   0x80

/* ATA commands */
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30

/* ---------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------------*/

/* Poll BSY until clear; return AURA_ERR on timeout */
static aura_status_t ata_poll_bsy(void) {
    for (uint32_t i = 0; i < 0x100000U; i++) {
        uint8_t st = inb(ATA_REG_STATUS);
        if (!(st & ATA_SR_BSY)) return AURA_OK;
    }
    return AURA_ERR; /* Timeout */
}

/* Poll until DRQ or ERR */
static aura_status_t ata_poll_drq(void) {
    for (uint32_t i = 0; i < 0x100000U; i++) {
        uint8_t st = inb(ATA_REG_STATUS);
        if (st & ATA_SR_ERR) return AURA_ERR;
        if (st & ATA_SR_DF)  return AURA_ERR;
        if (st & ATA_SR_DRQ) return AURA_OK;
    }
    return AURA_ERR;
}

/* Send LBA28 parameters common to read and write commands */
static void ata_setup_lba(uint32_t lba, uint8_t count) {
    outb(ATA_REG_HDSEL,   (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_REG_FEATURES, 0x00);
    outb(ATA_REG_SECCOUNT, count);
    outb(ATA_REG_LBA0,    (uint8_t)( lba        & 0xFF));
    outb(ATA_REG_LBA1,    (uint8_t)((lba >> 8)  & 0xFF));
    outb(ATA_REG_LBA2,    (uint8_t)((lba >> 16) & 0xFF));
}

/* ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------*/

aura_status_t ata_init(void) {
    /* Select master drive on primary channel */
    outb(ATA_REG_HDSEL, 0xA0);

    /* Issue IDENTIFY command */
    outb(ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    /* If status is 0 after IDENTIFY, no drive is present */
    uint8_t st = inb(ATA_REG_STATUS);
    if (st == 0) return AURA_ERR;

    if (ata_poll_bsy() != AURA_OK) return AURA_ERR;

    /* Check for ATAPI signature (non-ATA device) */
    uint8_t cl = inb(ATA_REG_LBA1);
    uint8_t ch = inb(ATA_REG_LBA2);
    if (cl != 0 || ch != 0) return AURA_ERR; /* ATAPI, not ATA */

    if (ata_poll_drq() != AURA_OK) return AURA_ERR;

    /* Consume the 256-word IDENTIFY data */
    for (int i = 0; i < 256; i++) {
        (void)inw(ATA_REG_DATA);
    }

    return AURA_OK;
}

aura_status_t ata_read_sectors(uint32_t lba, uint8_t count, uint8_t *buf) {
    if (!buf || count == 0) return AURA_ERR;

    if (ata_poll_bsy() != AURA_OK) return AURA_ERR;
    ata_setup_lba(lba, count);
    outb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_poll_bsy() != AURA_OK) return AURA_ERR;
        if (ata_poll_drq() != AURA_OK) return AURA_ERR;

        uint16_t *ptr = (uint16_t *)(buf + (uint32_t)s * ATA_SECTOR_SIZE);
        for (int i = 0; i < 256; i++) {
            ptr[i] = inw(ATA_REG_DATA);
        }
    }
    return AURA_OK;
}

aura_status_t ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t *buf) {
    if (!buf || count == 0) return AURA_ERR;

    if (ata_poll_bsy() != AURA_OK) return AURA_ERR;
    ata_setup_lba(lba, count);
    outb(ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_poll_bsy() != AURA_OK) return AURA_ERR;
        if (ata_poll_drq() != AURA_OK) return AURA_ERR;

        const uint16_t *ptr = (const uint16_t *)(buf + (uint32_t)s * ATA_SECTOR_SIZE);
        for (int i = 0; i < 256; i++) {
            outw(ATA_REG_DATA, ptr[i]);
        }

        /* Flush the write cache */
        outb(ATA_REG_COMMAND, 0xE7); /* FLUSH CACHE */
        if (ata_poll_bsy() != AURA_OK) return AURA_ERR;
    }
    return AURA_OK;
}
