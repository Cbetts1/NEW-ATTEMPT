/* =============================================================================
 * AI Aura OS — Memory Manager Implementation
 * Free-list allocator operating on a fixed kernel heap region.
 * =============================================================================*/

#include "memory.h"
#include "vga.h"

/* Block header stored immediately before each allocation */
typedef struct block_hdr {
    uint32_t         magic;
    uint32_t         size;          /* payload size in bytes */
    uint8_t          used;
    struct block_hdr *next;
    struct block_hdr *prev;
} block_hdr_t;

#define BLOCK_MAGIC  0xA3A30000U
#define HDR_SIZE     sizeof(block_hdr_t)

static block_hdr_t *heap_head = NULL;

/* -------------------------------------------------------------------------- */

void memset_k(void *ptr, uint8_t val, size_t len) {
    uint8_t *p = (uint8_t *)ptr;
    while (len--) *p++ = val;
}

void memcpy_k(void *dst, const void *src, size_t len) {
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (len--) *d++ = *s++;
}

/* -------------------------------------------------------------------------- */

void memory_init(void) {
    /* Bootstrap a single free block covering the entire heap */
    heap_head = (block_hdr_t *)HEAP_START;
    heap_head->magic = BLOCK_MAGIC;
    heap_head->size  = (uint32_t)(HEAP_SIZE - HDR_SIZE);
    heap_head->used  = MEM_FREE;
    heap_head->next  = NULL;
    heap_head->prev  = NULL;
    memset_k((uint8_t *)heap_head + HDR_SIZE, 0, heap_head->size);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    /* Align to 8 bytes */
    size = (size + 7) & ~7UL;

    block_hdr_t *cur = heap_head;
    while (cur) {
        if (!cur->used && cur->size >= size) {
            /* Split if there is room for a new header + at least 8 bytes */
            if (cur->size >= size + HDR_SIZE + 8) {
                block_hdr_t *next_blk =
                    (block_hdr_t *)((uint8_t *)cur + HDR_SIZE + size);
                next_blk->magic = BLOCK_MAGIC;
                next_blk->size  = (uint32_t)(cur->size - size - HDR_SIZE);
                next_blk->used  = MEM_FREE;
                next_blk->next  = cur->next;
                next_blk->prev  = cur;
                if (cur->next) cur->next->prev = next_blk;
                cur->next = next_blk;
                cur->size = (uint32_t)size;
            }
            cur->used = MEM_USED;
            return (uint8_t *)cur + HDR_SIZE;
        }
        cur = cur->next;
    }
    return NULL; /* Out of memory */
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_hdr_t *blk = (block_hdr_t *)((uint8_t *)ptr - HDR_SIZE);
    if (blk->magic != BLOCK_MAGIC) return; /* Bad pointer guard */
    blk->used = MEM_FREE;

    /* Coalesce with next block */
    if (blk->next && !blk->next->used) {
        blk->size += HDR_SIZE + blk->next->size;
        blk->next  = blk->next->next;
        if (blk->next) blk->next->prev = blk;
    }
    /* Coalesce with previous block */
    if (blk->prev && !blk->prev->used) {
        blk->prev->size += HDR_SIZE + blk->size;
        blk->prev->next  = blk->next;
        if (blk->next) blk->next->prev = blk->prev;
    }
}

void memory_stats(uint32_t *used, uint32_t *free_bytes) {
    *used       = 0;
    *free_bytes = 0;
    block_hdr_t *cur = heap_head;
    while (cur) {
        if (cur->used) *used       += cur->size;
        else           *free_bytes += cur->size;
        cur = cur->next;
    }
}
