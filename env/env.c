/* =============================================================================
 * AI Aura OS — Environment Registry Implementation
 * =============================================================================*/

#include "env.h"
#include <stddef.h>
#include "../kernel/kstring.h"
#include "fat12.h"

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

void env_dump_vga(void) {
    /* env.c includes vga.h through the translation unit that calls this function.
     * To keep env.c free of a direct VGA dependency, this thin wrapper
     * delegates the actual printing to the extern VGA symbols included by
     * the translation unit that has both headers available.
     * In practice, menu.c includes both vga.h and env.h, so it calls
     * env_dump_vga() which in turn uses the VGA driver compiled into the kernel. */
    extern void vga_print(const char *);
    extern void vga_println(const char *);
    extern void vga_print_dec(uint32_t);

    vga_println("--- Environment Variables ---");
    int shown = 0;
    for (int i = 0; i < ENV_MAX_VARS; i++) {
        if (!env_table[i].set) continue;
        vga_print("  ");
        vga_print(env_table[i].key);
        vga_print(" = ");
        vga_println(env_table[i].value);
        shown++;
    }
    if (!shown) vga_println("  (empty)");
}

/* ---------------------------------------------------------------------------
 * Serialisation format: lines of "KEY=VALUE\n", up to FAT12_MAX_FILE_SIZE.
 * ---------------------------------------------------------------------------*/

int env_save(void) {
    /* Serialise env_table into a flat text buffer */
    static uint8_t buf[512];
    int pos = 0;

    for (int i = 0; i < ENV_MAX_VARS && pos < 511; i++) {
        if (!env_table[i].set) continue;
        const char *k = env_table[i].key;
        const char *v = env_table[i].value;
        int klen = strlen_k(k);
        int vlen = strlen_k(v);
        /* Write "KEY=VALUE\n" */
        if (pos + klen + 1 + vlen + 1 >= 512) break;
        for (int j = 0; j < klen; j++) buf[pos++] = (uint8_t)k[j];
        buf[pos++] = '=';
        for (int j = 0; j < vlen; j++) buf[pos++] = (uint8_t)v[j];
        buf[pos++] = '\n';
    }
    buf[pos] = '\0';

    return fat12_write_file("AIOS.ENV", buf, (uint32_t)pos);
}

int env_load(void) {
    static uint8_t buf[512];
    int n = fat12_read_file("AIOS.ENV", buf, sizeof(buf) - 1u);
    if (n <= 0) return -1;
    buf[n] = '\0';

    /* Parse "KEY=VALUE\n" lines */
    int i = 0;
    while (i < n) {
        /* Find '=' */
        int key_start = i;
        while (i < n && buf[i] != '=' && buf[i] != '\n') i++;
        if (i >= n || buf[i] != '=') { while (i < n && buf[i] != '\n') i++; i++; continue; }
        int key_end = i;
        i++; /* skip '=' */
        int val_start = i;
        while (i < n && buf[i] != '\n') i++;
        int val_end = i;
        if (i < n) i++; /* skip '\n' */

        /* NUL-terminate in place */
        if (key_end < 512) buf[key_end] = '\0';
        if (val_end < 512) buf[val_end] = '\0';
        env_set((char *)(buf + key_start), (char *)(buf + val_start));
        /* Restore for continued parsing (not needed since buf is sequential) */
    }
    return 0;
}
