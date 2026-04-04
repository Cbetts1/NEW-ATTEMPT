/* =============================================================================
 * AI Aura OS — User / Admin Registry Implementation
 * =============================================================================*/

#include "user.h"
#include <stddef.h>

static user_t user_table[USER_MAX];

/* ---------------------------------------------------------------------------*/

static void str_copy(char *dst, const char *src, int max) {
    int i = 0;
    while (i < max - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int str_eq(const char *a, const char *b) {
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return (*a == '\0' && *b == '\0');
}

/* ---------------------------------------------------------------------------*/

void user_init(void) {
    int i;
    for (i = 0; i < USER_MAX; i++) user_table[i].active = 0;

    /* Default administrator account */
    str_copy(user_table[0].name, "admin",  USER_NAME_LEN);
    str_copy(user_table[0].pass, "admin",  USER_PASS_LEN);
    user_table[0].is_admin = 1;
    user_table[0].active   = 1;
}

int user_authenticate(const char *name, const char *pass) {
    int i;
    for (i = 0; i < USER_MAX; i++) {
        if (user_table[i].active &&
            str_eq(user_table[i].name, name) &&
            str_eq(user_table[i].pass, pass)) {
            return i;
        }
    }
    return -1;
}

user_t *user_get(int idx) {
    if (idx < 0 || idx >= USER_MAX) return NULL;
    return user_table[idx].active ? &user_table[idx] : NULL;
}

user_t *user_find(const char *name) {
    int i;
    for (i = 0; i < USER_MAX; i++) {
        if (user_table[i].active && str_eq(user_table[i].name, name))
            return &user_table[i];
    }
    return NULL;
}
