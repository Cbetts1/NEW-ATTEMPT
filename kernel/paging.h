#ifndef PAGING_H
#define PAGING_H

/* =============================================================================
 * AI Aura OS — x86 Paging Subsystem
 *
 * Sets up a minimal page directory covering the first 4 MB of physical memory
 * with an identity mapping (virtual address == physical address).  This lets
 * the kernel run with CR0.PG=1 while all existing absolute addresses remain
 * valid.
 *
 * Guard pages are placed around the kernel heap so that a buffer overrun or
 * underrun triggers a #PF (exception 14) instead of silently corrupting other
 * data structures.
 * =============================================================================*/

#include <stdint.h>

/* Page size and shift */
#define PAGE_SIZE       4096u
#define PAGE_SHIFT      12

/* Page directory / table entry flags */
#define PTE_PRESENT     (1u << 0)
#define PTE_WRITABLE    (1u << 1)
#define PTE_USER        (1u << 2)
#define PTE_PS          (1u << 7)   /* 4 MB page (PSE) */

/* Enable paging: identity-map first 4 MB, install guard pages */
void paging_init(void);

#endif /* PAGING_H */
