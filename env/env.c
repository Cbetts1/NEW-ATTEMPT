/* =============================================================================
 * AI Aura OS — Environment Registry Implementation
 * =============================================================================*/

#include "env.h"
#include <stddef.h>

static env_var_t env_table[ENV_MAX_VARS];

static int strncmp_k(const char *a, const char *b, int n) {
    while (n-- > 0) {
        if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
        if (*a == '\0') return 0;
        a++; b++;
    }
    return 0;
}

static void strncpy_k(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

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
    /* Called from VGA context — not included here to avoid circular deps */
}
