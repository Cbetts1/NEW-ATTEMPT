/* =============================================================================
 * AI Aura OS — Environment Registry Implementation
 * =============================================================================*/

#include "env.h"
#include <stddef.h>
#include "../kernel/kstring.h"

static env_var_t env_table[ENV_MAX_VARS];

void env_init(void) {
    for (int i = 0; i < ENV_MAX_VARS; i++) {
        env_table[i].set = 0;
    }
    env_set(ENV_KEY_OS_NAME,    "AI Aura OS");
    env_set(ENV_KEY_OS_VERSION, "1.0.0");
}

int env_set(const char *key, const char *value) {
    if (!key || !value) return -1;
    /* Update existing */
    for (int i = 0; i < ENV_MAX_VARS; i++) {
        if (env_table[i].set &&
            strncmp_k(env_table[i].key, key, ENV_KEY_LEN) == 0) {
            strncpy_k(env_table[i].value, value, ENV_VAL_LEN);
            return 0;
        }
    }
    /* Insert new */
    for (int i = 0; i < ENV_MAX_VARS; i++) {
        if (!env_table[i].set) {
            strncpy_k(env_table[i].key,   key,   ENV_KEY_LEN);
            strncpy_k(env_table[i].value, value, ENV_VAL_LEN);
            env_table[i].set = 1;
            return 0;
        }
    }
    return -1; /* Full */
}

const char *env_get(const char *key) {
    if (!key) return NULL;
    for (int i = 0; i < ENV_MAX_VARS; i++) {
        if (env_table[i].set &&
            strncmp_k(env_table[i].key, key, ENV_KEY_LEN) == 0) {
            return env_table[i].value;
        }
    }
    return NULL;
}

void env_dump(void) {
    /* Dumping is performed by the caller using the VGA driver to avoid a
     * circular dependency between the environment layer and the display layer.
     * Callers should call env_get() for individual keys, or iterate the
     * env_table[] array directly where the VGA driver is available. */
}
