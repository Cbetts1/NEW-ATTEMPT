/* =============================================================================
 * AI Aura OS — Main Menu header
 * File: kernel/include/menu.h
 * =========================================================================== */
#ifndef AIOS_MENU_H
#define AIOS_MENU_H

/* Maximum entries in a single menu level */
#define MENU_MAX_ENTRIES    16

/* A single menu action callback */
typedef void (*menu_action_t)(void);

typedef struct menu_entry {
    const char   *label;
    menu_action_t action;
} menu_entry_t;

/* Public API */
void menu_init(void);
void menu_run(void);    /* blocks and drives the interactive loop */
void menu_draw(void);   /* render current menu state to VGA       */

#endif /* AIOS_MENU_H */
