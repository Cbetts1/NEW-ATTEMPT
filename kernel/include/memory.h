/* =============================================================================
 * AI Aura OS — Memory Manager header
 * File: kernel/include/memory.h
 * =========================================================================== */
#ifndef AIOS_MEMORY_H
#define AIOS_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* Physical memory layout constants */
#define MEM_HEAP_START      0x300000    /* 3 MB — start of kernel heap */
#define MEM_HEAP_SIZE       (4 * 1024 * 1024)  /* 4 MB heap */
#define MEM_BLOCK_SIZE      16          /* minimum allocation granularity */

/* Memory region types reported by the registry */
typedef enum {
    MEM_REGION_FREE   = 0,
    MEM_REGION_USED   = 1,
    MEM_REGION_KERNEL = 2,
    MEM_REGION_MMIO   = 3
} mem_region_type_t;

/* A single memory region descriptor */
typedef struct mem_region {
    uint32_t         base;
    uint32_t         length;
    mem_region_type_t type;
} mem_region_t;

/* Public API */
void  memory_init(void);
void *kmalloc(size_t size);
void  kfree(void *ptr);
void *kmemset(void *dest, int val, size_t n);
void *kmemcpy(void *dest, const void *src, size_t n);
void  memory_dump(void);          /* debug: print heap stats via VGA */

#endif /* AIOS_MEMORY_H */
