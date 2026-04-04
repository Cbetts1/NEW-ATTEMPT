/* =============================================================================
 * AI Aura OS — System Mirroring Engine header
 * File: kernel/include/mirror.h
 * Role : Continuously snapshots and validates FS, memory, process-table, and
 *        device states so the OS can self-heal or restore from a known-good
 *        mirror without relying on any external system.
 * =========================================================================== */
#ifndef AIOS_MIRROR_H
#define AIOS_MIRROR_H

#include <stdint.h>
#include <stddef.h>

/* How many snapshots the ring-buffer holds */
#define MIRROR_SNAPSHOT_SLOTS   8

/* Subsystem bitmask flags */
#define MIRROR_SUBSYS_MEMORY    (1 << 0)
#define MIRROR_SUBSYS_FS        (1 << 1)
#define MIRROR_SUBSYS_PROCESS   (1 << 2)
#define MIRROR_SUBSYS_DEVICE    (1 << 3)
#define MIRROR_SUBSYS_ALL       (0x0F)

/* Snapshot header */
typedef struct mirror_snapshot {
    uint32_t  seq;              /* monotonically increasing sequence number */
    uint32_t  timestamp;        /* heartbeat tick at capture time           */
    uint32_t  subsys_mask;      /* which subsystems are included            */
    uint32_t  checksum;         /* simple XOR checksum of payload bytes     */
    uint8_t   payload[256];     /* compact serialised state blob            */
} mirror_snapshot_t;

/* Public API */
void mirror_init(void);
void mirror_capture(uint32_t subsys_mask);  /* take a snapshot now          */
int  mirror_verify(uint32_t slot);          /* verify snapshot integrity    */
void mirror_restore(uint32_t slot);         /* restore system from snapshot */
void mirror_tick(void);                     /* called every heartbeat       */
void mirror_dump(void);                     /* print snapshot ring via VGA  */

#endif /* AIOS_MIRROR_H */
