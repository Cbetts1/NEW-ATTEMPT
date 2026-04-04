#ifndef MENU_H
#define MENU_H

/* =============================================================================
 * AI Aura OS — Main Menu Interface
 * Text-based interactive OS menu rendered via the VGA driver.
 * =============================================================================*/

void menu_run(void);        /* Initial display (call once at boot) */
void menu_draw_banner(void);
void menu_tick(void);       /* Keyboard-driven menu handler — call from scheduler */

#endif /* MENU_H */
