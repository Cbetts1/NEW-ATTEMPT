/* =============================================================================
 * AI Aura OS — System Mirroring Engine Implementation
 * =============================================================================*/

#include "mirror.h"
#include "memory.h"
#include "plugin.h"
#include "eventbus.h"
#include "vga.h"

static mirror_snapshot_t snapshots[MIRROR_SLOTS];
static uint32_t          sync_count = 0;

/* Active live mirror (slot 0 always holds the running state) */
#define LIVE_SLOT 0

/* -------------------------------------------------------------------------- */

static void strncpy_k(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* -------------------------------------------------------------------------- */

void mirror_init(void) {
    for (int i = 0; i < MIRROR_SLOTS; i++) {
        snapshots[i].valid = 0;
    }
    sync_count = 0;
}

void mirror_sync(void) {
    /* Continuously update the live mirror slot */
    extern volatile uint32_t g_tick_count;
    uint32_t used = 0, free_bytes = 0;
    memory_stats(&used, &free_bytes);

    snapshots[LIVE_SLOT].timestamp    = g_tick_count;
    snapshots[LIVE_SLOT].flags        = MIRROR_FLAG_ALL;
    snapshots[LIVE_SLOT].mem_used     = used;
    snapshots[LIVE_SLOT].mem_free     = free_bytes;
    snapshots[LIVE_SLOT].proc_count   = 0; /* populated by scheduler */
    snapshots[LIVE_SLOT].plugin_count = (uint32_t)plugin_count();
    snapshots[LIVE_SLOT].event_pending= eventbus_pending();
    snapshots[LIVE_SLOT].valid        = 1;
    strncpy_k(snapshots[LIVE_SLOT].label, "LIVE", MIRROR_LABEL_LEN);

    sync_count++;
    if ((sync_count & 0xFF) == 0) {
        eventbus_publish(TOPIC_MIRROR_SYNC, sync_count);
    }
}

aura_status_t mirror_capture(uint8_t slot, const char *label, uint32_t flags) {
    if (slot >= MIRROR_SLOTS) return AURA_ERR;

    /* First do a sync to make live slot current */
    mirror_sync();

    snapshots[slot]       = snapshots[LIVE_SLOT];
    snapshots[slot].flags = flags;
    strncpy_k(snapshots[slot].label, label, MIRROR_LABEL_LEN);
    snapshots[slot].valid = 1;
    return AURA_OK;
}

aura_status_t mirror_restore(uint8_t slot) {
    if (slot >= MIRROR_SLOTS || !snapshots[slot].valid) return AURA_ERR;
    /* In a real OS this would restore memory pages, process tables, etc.
     * For AI Aura OS we log the restore event and publish on event bus. */
    eventbus_publish(TOPIC_MIRROR_SYNC, (uint32_t)slot | 0x80000000UL);
    vga_print("[Mirror] Restore from slot ");
    vga_print_dec(slot);
    vga_print(": ");
    vga_println(snapshots[slot].label);
    return AURA_OK;
}

void mirror_dump(uint8_t slot) {
    if (slot >= MIRROR_SLOTS) return;
    if (!snapshots[slot].valid) {
        vga_print("[Mirror] Slot ");
        vga_print_dec(slot);
        vga_println(" is empty.");
        return;
    }
    vga_print("[Mirror] Slot "); vga_print_dec(slot);
    vga_print(" label=");        vga_println(snapshots[slot].label);
    vga_print("  tick=");        vga_print_dec(snapshots[slot].timestamp);
    vga_print("  mem_used=");    vga_print_dec(snapshots[slot].mem_used);
    vga_print("  mem_free=");    vga_print_dec(snapshots[slot].mem_free);
    vga_print("  plugins=");     vga_print_dec(snapshots[slot].plugin_count);
    vga_print("  events=");      vga_print_dec(snapshots[slot].event_pending);
    vga_putchar('\n');
}

void mirror_dump_all(void) {
    for (int i = 0; i < MIRROR_SLOTS; i++) {
        mirror_dump((uint8_t)i);
    }
}
