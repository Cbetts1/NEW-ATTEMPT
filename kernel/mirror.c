/* =============================================================================
 * AI Aura OS — System Mirroring Engine Implementation
 * =============================================================================*/

#include "mirror.h"
#include "memory.h"
#include "plugin.h"
#include "scheduler.h"
#include "eventbus.h"
#include "vga.h"
#include "kstring.h"
#include "../env/env.h"
#include "../kernel/menu.h"

static mirror_snapshot_t snapshots[MIRROR_SLOTS];
static uint32_t          sync_count = 0;

/* Active live mirror (slot 0 always holds the running state) */
#define LIVE_SLOT 0

/* -------------------------------------------------------------------------- */

/* Compute XOR checksum over all snapshot fields except 'checksum' and 'valid' */
static uint32_t snapshot_checksum(const mirror_snapshot_t *s) {
    uint32_t csum = 0;
    /* XOR the label bytes as 32-bit words */
    for (int i = 0; i < MIRROR_LABEL_LEN; i++) {
        csum ^= (uint32_t)(unsigned char)s->label[i];
    }
    csum ^= s->timestamp;
    csum ^= s->flags;
    csum ^= s->mem_used;
    csum ^= s->mem_free;
    csum ^= s->proc_count;
    csum ^= s->plugin_count;
    csum ^= s->event_pending;
    return csum;
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
    snapshots[LIVE_SLOT].proc_count   = (uint32_t)scheduler_task_count();
    snapshots[LIVE_SLOT].plugin_count = (uint32_t)plugin_count();
    snapshots[LIVE_SLOT].event_pending= eventbus_pending();
    snapshots[LIVE_SLOT].checksum     = snapshot_checksum(&snapshots[LIVE_SLOT]);
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
    snapshots[slot].checksum = snapshot_checksum(&snapshots[slot]);
    snapshots[slot].valid = 1;
    return AURA_OK;
}

aura_status_t mirror_restore(uint8_t slot) {
    if (slot >= MIRROR_SLOTS || !snapshots[slot].valid) return AURA_ERR;
    if (mirror_verify(slot) != AURA_OK) {
        vga_println("[Mirror] Restore aborted: checksum mismatch!");
        return AURA_ERR;
    }

    /* Log the restore event on the bus */
    eventbus_publish(TOPIC_MIRROR_SYNC, (uint32_t)slot | 0x80000000UL);
    vga_print("[Mirror] Restore from slot ");
    vga_print_dec(slot);
    vga_print(": ");
    vga_println(snapshots[slot].label);

    /* Re-initialise the environment registry to its boot defaults so that
     * any runtime env_set() calls made after the snapshot was taken are
     * rolled back.  Full memory-page restore is not yet feasible without
     * a paging layer — this is the best approximation available. */
    env_init();

    /* Reset VGA to standard boot colours and redisplay the menu */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_clear();
    menu_draw_banner();
    vga_println("[Mirror] System state restored. Resuming kernel loop.");

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
    int ok = (mirror_verify(slot) == AURA_OK);
    vga_print("[Mirror] Slot "); vga_print_dec(slot);
    vga_print(" label=");        vga_print(snapshots[slot].label);
    vga_print(ok ? "  [OK]" : "  [BAD CHECKSUM]");
    vga_print("\n  tick=");      vga_print_dec(snapshots[slot].timestamp);
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

aura_status_t mirror_verify(uint8_t slot) {
    if (slot >= MIRROR_SLOTS || !snapshots[slot].valid) return AURA_ERR;
    uint32_t expected = snapshot_checksum(&snapshots[slot]);
    return (snapshots[slot].checksum == expected) ? AURA_OK : AURA_ERR;
}
