#ifndef MIRROR_H
#define MIRROR_H

/* =============================================================================
 * AI Aura OS — System Mirroring Engine Interface
 * Captures snapshots of OS subsystem state: filesystem, memory, processes,
 * and virtual devices. Provides restore and diff capabilities.
 * =============================================================================*/

#include <stdint.h>
#include "kernel.h"

#define MIRROR_SLOTS        4
#define MIRROR_LABEL_LEN    20

/* What to capture per mirror snapshot */
#define MIRROR_FLAG_MEMORY  (1 << 0)
#define MIRROR_FLAG_PROCS   (1 << 1)
#define MIRROR_FLAG_FS      (1 << 2)
#define MIRROR_FLAG_DEVICES (1 << 3)
#define MIRROR_FLAG_ALL     0x0F

typedef struct {
    char     label[MIRROR_LABEL_LEN];
    uint32_t timestamp;
    uint32_t flags;
    uint32_t mem_used;
    uint32_t mem_free;
    uint32_t proc_count;
    uint32_t plugin_count;
    uint32_t event_pending;
    uint32_t checksum;          /* XOR checksum of all preceding fields */
    uint8_t  valid;
} mirror_snapshot_t;

void          mirror_init(void);
void          mirror_sync(void);                    /* Continuous sync hook */
aura_status_t mirror_capture(uint8_t slot, const char *label, uint32_t flags);
aura_status_t mirror_restore(uint8_t slot);
aura_status_t mirror_verify(uint8_t slot);          /* XOR checksum check */
void          mirror_dump(uint8_t slot);            /* Print snapshot to VGA */
void          mirror_dump_all(void);

#endif /* MIRROR_H */
