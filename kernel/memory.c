/* =============================================================================
 * AI Aura OS — Memory Manager
 * File: kernel/memory.c
 *
 * Implements a simple free-list (boundary-tag) allocator over a statically
 * defined heap region.  No OS calls, no libc — fully self-contained.
 * =========================================================================== */
#include "include/memory.h"
#include "include/vga.h"

/* ── Block header ────────────────────────────────────────────────────────── */
typedef struct block_header {
    uint32_t            magic;      /* 0xA10CA110 when valid                */
    uint32_t            size;       /* usable bytes (excluding header)      */
    uint8_t             free;       /* 1 = free, 0 = allocated              */
    struct block_header *next;
    struct block_header *prev;
} block_header_t;

#define BLOCK_MAGIC     0xA10CA110U
#define HDR_SIZE        sizeof(block_header_t)

/* Heap lives in BSS — a static buffer of MEM_HEAP_SIZE bytes */
static uint8_t  heap_storage[MEM_HEAP_SIZE] __attribute__((aligned(16)));
static block_header_t *heap_head = (void *)0;

/* ── internal helpers ────────────────────────────────────────────────────── */

static void split_block(block_header_t *blk, size_t needed)
{
    if (blk->size <= needed + HDR_SIZE + MEM_BLOCK_SIZE)
        return;  /* not worth splitting */

    block_header_t *newblk = (block_header_t *)((uint8_t *)blk + HDR_SIZE + needed);
    newblk->magic = BLOCK_MAGIC;
    newblk->size  = blk->size - needed - HDR_SIZE;
    newblk->free  = 1;
    newblk->next  = blk->next;
    newblk->prev  = blk;

    if (blk->next)
        blk->next->prev = newblk;

    blk->next = newblk;
    blk->size = needed;
}

static void coalesce(block_header_t *blk)
{
    /* merge with next */
    if (blk->next && blk->next->free) {
        blk->size += HDR_SIZE + blk->next->size;
        blk->next  = blk->next->next;
        if (blk->next)
            blk->next->prev = blk;
    }
    /* merge with previous */
    if (blk->prev && blk->prev->free) {
        blk->prev->size += HDR_SIZE + blk->size;
        blk->prev->next  = blk->next;
        if (blk->next)
            blk->next->prev = blk->prev;
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void memory_init(void)
{
    heap_head        = (block_header_t *)heap_storage;
    heap_head->magic = BLOCK_MAGIC;
    heap_head->size  = MEM_HEAP_SIZE - HDR_SIZE;
    heap_head->free  = 1;
    heap_head->next  = (void *)0;
    heap_head->prev  = (void *)0;

    vga_puts("[MEM] Memory manager initialised. Heap: ");
    vga_printf("%u KB\n", (unsigned)(MEM_HEAP_SIZE / 1024));
}

void *kmalloc(size_t size)
{
    if (size == 0)
        return (void *)0;

    /* align to MEM_BLOCK_SIZE */
    size = (size + MEM_BLOCK_SIZE - 1) & ~(size_t)(MEM_BLOCK_SIZE - 1);

    block_header_t *cur = heap_head;
    while (cur) {
        if (cur->magic != BLOCK_MAGIC) {
            vga_puts("[MEM] HEAP CORRUPTION DETECTED\n");
            return (void *)0;
        }
        if (cur->free && cur->size >= size) {
            split_block(cur, size);
            cur->free = 0;
            return (uint8_t *)cur + HDR_SIZE;
        }
        cur = cur->next;
    }

    vga_puts("[MEM] kmalloc: out of heap memory\n");
    return (void *)0;
}

void kfree(void *ptr)
{
    if (!ptr) return;

    block_header_t *blk = (block_header_t *)((uint8_t *)ptr - HDR_SIZE);
    if (blk->magic != BLOCK_MAGIC) {
        vga_puts("[MEM] kfree: invalid pointer\n");
        return;
    }
    blk->free = 1;
    coalesce(blk);
}

void *kmemset(void *dest, int val, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    while (n--) *d++ = (uint8_t)val;
    return dest;
}

void *kmemcpy(void *dest, const void *src, size_t n)
{
    uint8_t       *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dest;
}

void memory_dump(void)
{
    vga_puts("[MEM] Heap dump:\n");
    block_header_t *cur = heap_head;
    int idx = 0;
    while (cur) {
        vga_printf("  [%d] addr=0x%x size=%u free=%d\n",
                   idx++,
                   (uint32_t)((uint8_t *)cur + HDR_SIZE),
                   cur->size,
                   (int)cur->free);
        cur = cur->next;
    }
}
