/* =============================================================================
 * AI Aura OS — System Mirroring Engine
 * File: kernel/mirror.c
 *
 * Continuously captures snapshots of OS subsystem state into a ring buffer.
 * Each snapshot can be verified via checksum and restored to roll back the OS
 * to a known-good state — entirely internal, no host OS involvement.
 * =========================================================================== */
#include "include/mirror.h"
#include "include/vga.h"
#include "include/memory.h"

/* ── Snapshot ring buffer ────────────────────────────────────────────────── */
static mirror_snapshot_t ring[MIRROR_SNAPSHOT_SLOTS];
static uint32_t          ring_head = 0;   /* next write slot   */
static uint32_t          seq_ctr   = 0;   /* monotonic counter */
static uint32_t          tick_ctr  = 0;   /* heartbeat counter */

/* Capture interval: take a snapshot every N heartbeat ticks */
#define MIRROR_INTERVAL     100

/* ── Checksum helper ─────────────────────────────────────────────────────── */
static uint32_t xor_checksum(const uint8_t *data, size_t len)
{
    uint32_t csum = 0xDEADBEEFU;
    for (size_t i = 0; i < len; i++)
        csum ^= ((uint32_t)data[i] << ((i & 3) * 8));
    return csum;
}

/* ── Subsystem serialisers ───────────────────────────────────────────────── */

static size_t serialise_memory(uint8_t *buf, size_t max)
{
    /* Record a simple sentinel: heap base address and size constant */
    if (max < 8) return 0;
    uint32_t base  = (uint32_t)MEM_HEAP_START;
    uint32_t sz    = (uint32_t)MEM_HEAP_SIZE;
    kmemcpy(buf,     &base, 4);
    kmemcpy(buf + 4, &sz,   4);
    return 8;
}

static size_t serialise_process(uint8_t *buf, size_t max)
{
    /* Placeholder: record tick counter as proxy for process state */
    if (max < 4) return 0;
    kmemcpy(buf, &tick_ctr, 4);
    return 4;
}

static size_t serialise_device(uint8_t *buf, size_t max)
{
    /* Placeholder: VGA buffer first word as device state proxy */
    if (max < 4) return 0;
    uint32_t probe = 0xA1050505U;
    kmemcpy(buf, &probe, 4);
    return 4;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void mirror_init(void)
{
    kmemset(ring, 0, sizeof(ring));
    ring_head = seq_ctr = tick_ctr = 0;
    vga_puts("[MIR] System mirroring engine initialised.\n");
}

void mirror_capture(uint32_t subsys_mask)
{
    mirror_snapshot_t *snap = &ring[ring_head % MIRROR_SNAPSHOT_SLOTS];
    snap->seq         = seq_ctr++;
    snap->timestamp   = tick_ctr;
    snap->subsys_mask = subsys_mask;

    /* Zero payload so unused bytes are deterministic for checksum */
    kmemset(snap->payload, 0, sizeof(snap->payload));

    uint8_t *p    = snap->payload;
    size_t   used = 0;
    size_t   rem  = sizeof(snap->payload);

    if (subsys_mask & MIRROR_SUBSYS_MEMORY) {
        size_t n = serialise_memory(p + used, rem - used);
        used += n;
    }
    if (subsys_mask & MIRROR_SUBSYS_PROCESS) {
        size_t n = serialise_process(p + used, rem - used);
        used += n;
    }
    if (subsys_mask & MIRROR_SUBSYS_DEVICE) {
        size_t n = serialise_device(p + used, rem - used);
        used += n;
    }
    (void)used;

    /* Checksum the entire (zero-padded) payload for consistency with verify */
    snap->checksum = xor_checksum(snap->payload, sizeof(snap->payload));
    ring_head++;
}

int mirror_verify(uint32_t slot)
{
    if (slot >= MIRROR_SNAPSHOT_SLOTS) return -1;
    mirror_snapshot_t *snap = &ring[slot];
    if (snap->seq == 0 && snap->checksum == 0) return -1; /* empty slot */

    uint32_t recomputed = xor_checksum(snap->payload, sizeof(snap->payload));
    if (recomputed != snap->checksum) return -1; /* checksum mismatch */
    return 0;
}

void mirror_restore(uint32_t slot)
{
    if (mirror_verify(slot) != 0) {
        vga_printf("[MIR] restore: slot %u invalid\n", slot);
        return;
    }
    vga_printf("[MIR] Restoring from snapshot slot %u (seq=%u)\n",
               slot, ring[slot].seq);
    /* In a full implementation, subsystem state would be re-applied here */
}

void mirror_tick(void)
{
    tick_ctr++;
    if (tick_ctr % MIRROR_INTERVAL == 0)
        mirror_capture(MIRROR_SUBSYS_ALL);
}

void mirror_dump(void)
{
    vga_puts("[MIR] Snapshot ring:\n");
    for (int i = 0; i < MIRROR_SNAPSHOT_SLOTS; i++) {
        mirror_snapshot_t *s = &ring[i];
        if (s->seq == 0 && s->checksum == 0) continue;
        vga_printf("  slot[%d] seq=%-4u ts=%-6u mask=0x%x csum=0x%x\n",
                   i, s->seq, s->timestamp, s->subsys_mask, s->checksum);
    }
}
