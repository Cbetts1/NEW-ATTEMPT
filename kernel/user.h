#ifndef USER_H
#define USER_H

/* =============================================================================
 * AI Aura OS — User / Admin Registry
 * Minimal user table; default admin account is created at boot.
 * =============================================================================*/

#include <stdint.h>

#define USER_NAME_LEN   16
#define USER_PASS_LEN   16
#define USER_MAX         4

typedef struct {
    char    name[USER_NAME_LEN];
    char    pass[USER_PASS_LEN];
    uint8_t is_admin;
    uint8_t active;
} user_t;

void    user_init(void);
int     user_authenticate(const char *name, const char *pass); /* returns index ≥0 or -1 */
user_t *user_get(int idx);
user_t *user_find(const char *name);

#endif /* USER_H */
