#ifndef KERNEL_H
#define KERNEL_H

/* =============================================================================
 * AI Aura OS — Kernel Core Types and Definitions
 * =============================================================================*/

#include <stdint.h>
#include <stddef.h>

/* OS identity */
#define AURA_OS_NAME     "AI Aura OS"
#define AURA_OS_VERSION  "1.0.0"

/* Boolean */
typedef int bool_t;
#define TRUE  1
#define FALSE 0

/* Result codes */
typedef enum {
    AURA_OK      = 0,
    AURA_ERR     = -1,
    AURA_NOMEM   = -2,
    AURA_NOSLOT  = -3,
    AURA_NOTFOUND= -4,
    AURA_BUSY    = -5,
} aura_status_t;

/* Kernel state */
typedef enum {
    KERNEL_STATE_BOOT  = 0,
    KERNEL_STATE_INIT  = 1,
    KERNEL_STATE_RUN   = 2,
    KERNEL_STATE_PANIC = 3,
} kernel_state_t;

/* Forward declarations */
void kernel_main(void);
void kernel_heartbeat(void);
__attribute__((noreturn)) void kernel_panic(const char *msg);
__attribute__((noreturn)) void kernel_shutdown(void);

/* Global kernel state (defined in kernel.c) */
extern volatile kernel_state_t g_kernel_state;
extern volatile uint32_t       g_tick_count;

#endif /* KERNEL_H */
