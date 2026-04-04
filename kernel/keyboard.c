/* =============================================================================
 * AI Aura OS — PS/2 Keyboard Driver
 * Polled mode: reads scan code set 1 from port 0x60.
 * No IRQ handler — the main loop polls kb_haskey() / kb_getchar().
 * =============================================================================*/

#include "keyboard.h"
#include "io.h"
#include <stdint.h>

#define KBD_DATA    0x60
#define KBD_STATUS  0x64
#define KBD_READY   0x01   /* output-buffer-full bit in status register */

static uint8_t kb_shift = 0;   /* 1 when left- or right-shift is held */
static uint8_t kb_ext   = 0;   /* 1 after receiving 0xE0 extended prefix */

/* ---------------------------------------------------------------------------
 * Scan code set 1 → ASCII, lowercase (84 entries: 0x00–0x53)
 * Indices match the make-code byte from the keyboard controller.
 * ---------------------------------------------------------------------------*/
static const uint8_t sc_lower[0x54] = {
/*00*/ 0,  27, '1','2','3','4','5','6','7','8','9','0','-','=',  8,  9,
/*10*/'q','w', 'e','r','t','y','u','i','o','p','[',']', 13,  0, 'a','s',
/*20*/'d','f', 'g','h','j','k','l',';','\'','`',  0,'\\','z','x','c','v',
/*30*/'b','n', 'm',',','.','/',  0, '*',  0, ' ',  0,  0,  0,  0,  0,  0,
/*40*/ 0,  0,   0,  0,  0,  0,  0, '7','8','9','-','4','5','6','+','1',
/*50*/'2','3', '0','.'
};

/* Shifted map */
static const uint8_t sc_upper[0x54] = {
/*00*/ 0,  27, '!','@','#','$','%','^','&','*','(',')','_','+',  8,  9,
/*10*/'Q','W', 'E','R','T','Y','U','I','O','P','{','}', 13,  0, 'A','S',
/*20*/'D','F', 'G','H','J','K','L',':','"', '~',  0, '|','Z','X','C','V',
/*30*/'B','N', 'M','<','>','?',  0, '*',  0, ' ',  0,  0,  0,  0,  0,  0,
/*40*/ 0,  0,   0,  0,  0,  0,  0, '7','8','9','-','4','5','6','+','1',
/*50*/'2','3', '0','.'
};

/* ---------------------------------------------------------------------------*/

void kb_init(void) {
    kb_shift = 0;
    kb_ext   = 0;
    /* Drain any stale bytes in the controller's output buffer */
    while (inb(KBD_STATUS) & KBD_READY) {
        (void)inb(KBD_DATA);
    }
}

int kb_haskey(void) {
    return (int)(inb(KBD_STATUS) & KBD_READY);
}

/* Block until the controller has a byte ready, then read it */
static uint8_t kb_read(void) {
    while (!(inb(KBD_STATUS) & KBD_READY)) {}
    return inb(KBD_DATA);
}

char kb_getchar(void) {
    for (;;) {
        uint8_t sc = kb_read();

        /* Extended-key prefix: set flag and fetch next byte */
        if (sc == 0xE0) { kb_ext = 1; continue; }

        /* Key-release event (bit 7 set) */
        if (sc & 0x80) {
            uint8_t make = sc & 0x7F;
            if (make == 0x2A || make == 0x36) kb_shift = 0;
            kb_ext = 0;
            continue;
        }

        /* Modifier: shift press */
        if (sc == 0x2A || sc == 0x36) { kb_shift = 1; kb_ext = 0; continue; }

        /* Arrow keys arrive as extended (0xE0 prefix) */
        if (kb_ext) {
            kb_ext = 0;
            switch (sc) {
                case 0x48: return KEY_UP;
                case 0x50: return KEY_DOWN;
                case 0x4B: return KEY_LEFT;
                case 0x4D: return KEY_RIGHT;
                case 0x53: return KEY_DEL;
                default:   continue;
            }
        }

        /* Function keys F1–F5 (no ext prefix in set 1) */
        if (sc >= 0x3B && sc <= 0x3F) {
            return (char)(KEY_F1 + (sc - 0x3B));
        }

        /* Regular printable key */
        if (sc < 0x54) {
            char c = (char)(kb_shift ? sc_upper[sc] : sc_lower[sc]);
            if (c) return c;
        }
        /* Ignore anything else */
    }
}
