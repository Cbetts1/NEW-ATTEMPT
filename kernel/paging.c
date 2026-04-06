/* =============================================================================
 * AI Aura OS — Paging Implementation
 * =============================================================================*/

#include "paging.h"
#include "memory.h"
#include "vga.h"
#include <stddef.h>

/* ---------------------------------------------------------------------------
 * Page directory — 1024 entries, each covering 4 MB (PSE, 4 KB granularity).
 * We use 4 KB pages to allow guard pages, so we also need a page table for
 * the first 4 MB region.
 *
 * The page directory and the single page table are placed at known physical
 * addresses just above the kernel heap guard region so that no kmalloc()
 * call is needed (paging_init() runs before the heap is populated with
 * user data).
 *
 * We choose:
 *   Page directory : 0x280000 (right after 512 KB heap region at 0x200000)
 *   Page table 0   : 0x281000 (4 KB aligned, immediately after)
 * ---------------------------------------------------------------------------*/

#define PDIR_ADDR       0x00280000UL
#define PTAB_ADDR       0x00281000UL

/* Number of 4 KB pages in the first 4 MB */
#define PAGES_4M        1024u

/* Heap bounds from memory.h (HEAP_START / HEAP_SIZE) */
/* Guard page just below HEAP_START */
#define HEAP_GUARD_LO   (HEAP_START - PAGE_SIZE)
/* Guard page just above HEAP_END */
#define HEAP_GUARD_HI   HEAP_END

/* ---------------------------------------------------------------------------
 * Zero a region of memory using the kernel memset_k from memory.h
 * ---------------------------------------------------------------------------*/
extern void memset_k(void *ptr, uint8_t val, size_t len);

void paging_init(void) {
    uint32_t *pdir = (uint32_t *)PDIR_ADDR;
    uint32_t *ptab = (uint32_t *)PTAB_ADDR;

    /* Clear the page directory and page table */
    memset_k(pdir, 0, 1024u * sizeof(uint32_t));
    memset_k(ptab, 0, 1024u * sizeof(uint32_t));

    /* Build page table 0: identity-map the first 4 MB at 4 KB granularity */
    for (uint32_t i = 0; i < PAGES_4M; i++) {
        uint32_t phys = i * PAGE_SIZE;

        /* Guard page below heap: mark not-present */
        if (phys == HEAP_GUARD_LO) {
            ptab[i] = phys; /* no PTE_PRESENT → not mapped */
            continue;
        }
        /* Guard page above heap: mark not-present */
        if (phys == HEAP_GUARD_HI) {
            ptab[i] = phys;
            continue;
        }

        ptab[i] = phys | PTE_PRESENT | PTE_WRITABLE;
    }

    /* Wire page table 0 into PDE 0 (covers virtual 0x000000–0x3FFFFF) */
    pdir[0] = (uint32_t)ptab | PTE_PRESENT | PTE_WRITABLE;

    /* All other PDEs remain 0 (not present) — the kernel fits in the first 4 MB */

    /* Load CR3 with the physical address of the page directory */
    __asm__ volatile (
        "mov %0, %%cr3\n"
        : : "r"(pdir)
    );

    /* Enable PSE (large page support) and paging in CR4/CR0 */
    uint32_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1u << 4); /* CR4.PSE */
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4));

    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1u << 31); /* CR0.PG */
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));

    vga_println("[OK] Paging enabled (4 MB identity map, heap guard pages active)");
}
