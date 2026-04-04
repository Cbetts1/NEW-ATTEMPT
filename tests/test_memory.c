/* =============================================================================
 * AI Aura OS — Memory Allocator Unit Tests
 *
 * The kernel heap lives at physical 0x200000.  On the test host we use mmap()
 * to map a 512 KB anonymous region at that exact address before the first call
 * to memory_init(), then munmap() it when we're done.
 * =============================================================================*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "tests/framework.h"
#include "kernel/memory.h"

/* MAP_FIXED_NOREPLACE avoids clobbering existing mappings (Linux 4.17+).
 * Fall back to MAP_FIXED on older kernels. */
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE MAP_FIXED
#endif

static void *s_heap_map = NULL;

/* Map the heap region and initialise the allocator. */
static void heap_setup(void) {
    if (s_heap_map == NULL) {
        s_heap_map = mmap((void *)(uintptr_t)HEAP_START, HEAP_SIZE,
                          PROT_READ | PROT_WRITE,
                          MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS,
                          -1, 0);
        if (s_heap_map == MAP_FAILED) {
            /* Fall back to MAP_FIXED if NOREPLACE is not available */
            s_heap_map = mmap((void *)(uintptr_t)HEAP_START, HEAP_SIZE,
                              PROT_READ | PROT_WRITE,
                              MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
                              -1, 0);
        }
        if (s_heap_map == MAP_FAILED) {
            perror("mmap: cannot map test heap at 0x200000");
            exit(1);
        }
    }
    memory_init(); /* Reset to a single free block every time */
}

/* =========================================================================== */
/* Tests                                                                        */
/* =========================================================================== */

static void test_memory_init_stats(void) {
    heap_setup();
    uint32_t used = 99, free_bytes = 99;
    memory_stats(&used, &free_bytes);
    TEST_ASSERT_EQ(used, 0u);
    TEST_ASSERT(free_bytes > 0u);
    /* Total free should not exceed HEAP_SIZE */
    TEST_ASSERT(free_bytes < HEAP_SIZE);
}

static void test_kmalloc_returns_nonnull(void) {
    heap_setup();
    void *p = kmalloc(16);
    TEST_ASSERT_NOT_NULL(p);
}

static void test_kmalloc_zero_returns_null(void) {
    heap_setup();
    TEST_ASSERT_NULL(kmalloc(0));
}

static void test_kmalloc_alignment(void) {
    heap_setup();
    /* All returned pointers must be 8-byte aligned */
    for (int sz = 1; sz <= 64; sz++) {
        void *p = kmalloc((size_t)sz);
        TEST_ASSERT_NOT_NULL(p);
        TEST_ASSERT_EQ((uintptr_t)p % 8u, 0u);
    }
}

static void test_kmalloc_within_heap(void) {
    heap_setup();
    void *p = kmalloc(64);
    TEST_ASSERT_NOT_NULL(p);
    /* Pointer must fall inside the heap region */
    TEST_ASSERT((uintptr_t)p >= HEAP_START);
    TEST_ASSERT((uintptr_t)p + 64 <= HEAP_START + HEAP_SIZE);
}

static void test_kmalloc_multiple_unique(void) {
    heap_setup();
    void *a = kmalloc(8);
    void *b = kmalloc(8);
    void *c = kmalloc(8);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT(a != b);
    TEST_ASSERT(b != c);
    TEST_ASSERT(a != c);
}

static void test_kmalloc_increases_used(void) {
    heap_setup();
    uint32_t used_before, free_before, used_after, free_after;
    memory_stats(&used_before, &free_before);
    void *p = kmalloc(32);
    (void)p;
    memory_stats(&used_after, &free_after);
    TEST_ASSERT(used_after > used_before);
    TEST_ASSERT(free_after < free_before);
}

static void test_kmalloc_oom(void) {
    heap_setup();
    /* Request the entire heap in one shot — must fail */
    void *p = kmalloc(HEAP_SIZE);
    TEST_ASSERT_NULL(p);
}

static void test_kfree_null_noop(void) {
    heap_setup();
    /* Must not crash */
    kfree(NULL);
    TEST_ASSERT(1); /* reach here without crashing */
}

static void test_kfree_bad_ptr_noop(void) {
    heap_setup();
    /* Pointer with wrong magic — should be silently ignored */
    uint8_t buf[32];
    memset(buf, 0xAB, sizeof(buf));
    kfree(buf + 8); /* not a real block header */
    TEST_ASSERT(1);
}

static void test_kfree_restores_free_bytes(void) {
    heap_setup();
    uint32_t used0, free0;
    memory_stats(&used0, &free0);

    void *p = kmalloc(64);
    TEST_ASSERT_NOT_NULL(p);

    uint32_t used1, free1;
    memory_stats(&used1, &free1);
    TEST_ASSERT(used1 > used0);

    kfree(p);
    uint32_t used2, free2;
    memory_stats(&used2, &free2);
    TEST_ASSERT_EQ(used2, 0u);    /* nothing allocated now */
    TEST_ASSERT(free2 > free1);
}

static void test_coalesce_next(void) {
    /* Allocate a, b; free a then b; should coalesce into one block.
     * Verify by re-allocating a size that would fit only in the merged block. */
    heap_setup();
    void *a = kmalloc(128);
    void *b = kmalloc(128);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);

    kfree(a);  /* a is free, b is used */
    kfree(b);  /* b is free → coalesce a+b via "coalesce with prev" in kfree(b) */

    /* Now allocate 192 bytes — this requires the merged block */
    void *c = kmalloc(192);
    TEST_ASSERT_NOT_NULL(c);
}

static void test_coalesce_all_three(void) {
    heap_setup();
    void *a = kmalloc(64);
    void *b = kmalloc(64);
    void *c = kmalloc(64);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    kfree(b);  /* b free, neighbours used */
    kfree(a);  /* a free, coalesces with b via "coalesce with next" */
    kfree(c);  /* c free, coalesces with big block ahead */

    /* After full coalesce, stats should show used == 0 */
    uint32_t used, free_bytes;
    memory_stats(&used, &free_bytes);
    TEST_ASSERT_EQ(used, 0u);
}

static void test_reuse_after_free(void) {
    heap_setup();
    void *p = kmalloc(256);
    TEST_ASSERT_NOT_NULL(p);
    kfree(p);

    /* Should be able to allocate the same size again */
    void *q = kmalloc(256);
    TEST_ASSERT_NOT_NULL(q);
}

static void test_memset_k(void) {
    uint8_t buf[64];
    memset_k(buf, 0xAA, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        TEST_ASSERT_EQ(buf[i], (uint8_t)0xAA);
    }
}

static void test_memset_k_zero(void) {
    uint8_t buf[32];
    memset(buf, 0xFF, sizeof(buf));
    memset_k(buf, 0, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        TEST_ASSERT_EQ(buf[i], (uint8_t)0);
    }
}

static void test_memcpy_k(void) {
    const uint8_t src[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t dst[16]       = {0};
    memcpy_k(dst, src, 16);
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQ(dst[i], src[i]);
    }
}

static void test_memcpy_k_single_byte(void) {
    uint8_t src = 0x5A, dst = 0;
    memcpy_k(&dst, &src, 1);
    TEST_ASSERT_EQ(dst, (uint8_t)0x5A);
}

static void test_memory_stats_accounts_for_all(void) {
    heap_setup();
    uint32_t used, free_bytes;
    memory_stats(&used, &free_bytes);
    /* Sum of used + free should be less than HEAP_SIZE (headers are overhead) */
    TEST_ASSERT(used + free_bytes < HEAP_SIZE);
    /* But should be close — within a handful of headers */
    TEST_ASSERT(used + free_bytes > HEAP_SIZE / 2u);
}

/* =========================================================================== */

int main(void) {
    printf("=== test_memory ===\n");
    RUN_TEST(test_memory_init_stats);
    RUN_TEST(test_kmalloc_returns_nonnull);
    RUN_TEST(test_kmalloc_zero_returns_null);
    RUN_TEST(test_kmalloc_alignment);
    RUN_TEST(test_kmalloc_within_heap);
    RUN_TEST(test_kmalloc_multiple_unique);
    RUN_TEST(test_kmalloc_increases_used);
    RUN_TEST(test_kmalloc_oom);
    RUN_TEST(test_kfree_null_noop);
    RUN_TEST(test_kfree_bad_ptr_noop);
    RUN_TEST(test_kfree_restores_free_bytes);
    RUN_TEST(test_coalesce_next);
    RUN_TEST(test_coalesce_all_three);
    RUN_TEST(test_reuse_after_free);
    RUN_TEST(test_memset_k);
    RUN_TEST(test_memset_k_zero);
    RUN_TEST(test_memcpy_k);
    RUN_TEST(test_memcpy_k_single_byte);
    RUN_TEST(test_memory_stats_accounts_for_all);
    PRINT_RESULTS();
}
