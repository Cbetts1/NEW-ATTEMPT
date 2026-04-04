#ifndef KEYBOARD_H
#define KEYBOARD_H

/* =============================================================================
 * AI Aura OS — PS/2 Keyboard Driver (polled, no IRQ)
 * =============================================================================*/

#include <stdint.h>

/* Special key codes returned by kb_getchar() in the range 0x80–0x8F */
#define KEY_UP        ((char)0x80)
#define KEY_DOWN      ((char)0x81)
#define KEY_LEFT      ((char)0x82)
#define KEY_RIGHT     ((char)0x83)
#define KEY_F1        ((char)0x84)
#define KEY_F2        ((char)0x85)
#define KEY_F3        ((char)0x86)
#define KEY_F4        ((char)0x87)
#define KEY_F5        ((char)0x88)
#define KEY_DEL       ((char)0x89)

/* Standard ASCII codes used elsewhere */
#define KEY_ESC       '\x1B'
#define KEY_ENTER     '\r'
#define KEY_TAB       '\t'
#define KEY_BACKSPACE '\b'

void kb_init(void);
int  kb_haskey(void);    /* 1 if keystroke is waiting, 0 otherwise (non-blocking) */
char kb_getchar(void);   /* block until a key is pressed, return ASCII or KEY_* */

#endif /* KEYBOARD_H */
