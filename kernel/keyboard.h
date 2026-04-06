#ifndef KEYBOARD_H
#define KEYBOARD_H

/* =============================================================================
 * AI Aura OS — PS/2 Keyboard Driver
 * Polls port 0x60 / 0x64 for scancodes and converts to ASCII characters.
 * =============================================================================*/

#include <stdint.h>

void keyboard_init(void);
void keyboard_poll(void);          /* Call from scheduler to buffer keypresses */
char keyboard_getchar(void);       /* Returns next buffered char, or 0 if none */
void keyboard_push(char c);        /* Inject a character into the ring buffer   */

#endif /* KEYBOARD_H */
