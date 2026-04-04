#ifndef ENV_H
#define ENV_H

/* =============================================================================
 * AI Aura OS — Environment Definitions
 * Defines the OS-wide environment constants and the virtual filesystem root.
 * =============================================================================*/

#include <stdint.h>

/* Environment key-value registry (max 16 entries) */
#define ENV_MAX_VARS    16
#define ENV_KEY_LEN     16
#define ENV_VAL_LEN     32

typedef struct {
    char    key[ENV_KEY_LEN];
    char    value[ENV_VAL_LEN];
    uint8_t set;
} env_var_t;

void        env_init(void);
int         env_set(const char *key, const char *value);
const char *env_get(const char *key);
void        env_dump(void);

/* Well-known environment keys */
#define ENV_KEY_OS_NAME     "OS_NAME"
#define ENV_KEY_OS_VERSION  "OS_VERSION"
#define ENV_KEY_BOOT_DRIVE  "BOOT_DRIVE"
#define ENV_KEY_MEM_TOTAL   "MEM_TOTAL"

#endif /* ENV_H */
