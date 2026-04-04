#ifndef MEMORY_H
#define MEMORY_H

/* =============================================================================
 * AI Aura OS — Memory Manager Interface
 * Simple bump/free-list allocator for kernel use
 * =============================================================================*/

#include <stdint.h>
#include <stddef.h>

/* Kernel heap: 512 KB starting at 2 MB physical */
#define HEAP_START  0x00200000UL
#define HEAP_SIZE   0x00080000UL   /* 512 KB */
#define HEAP_END    (HEAP_START + HEAP_SIZE)

/* Memory region flags */
#define MEM_FREE    0
#define MEM_USED    1

void   memory_init(void);
void  *kmalloc(size_t size);
void   kfree(void *ptr);
void   memory_stats(uint32_t *used, uint32_t *free_bytes);
void   memset_k(void *ptr, uint8_t val, size_t len);
void   memcpy_k(void *dst, const void *src, size_t len);

#endif /* MEMORY_H */
