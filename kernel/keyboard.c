/* =============================================================================
 * AI Aura OS — PS/2 Keyboard Driver Implementation
 * Polls the PS/2 keyboard controller (port 0x60 / 0x64) for scancodes and
 * converts make-codes to ASCII using a US QWERTY layout.  Characters are
 * stored in a small ring buffer for consumption by the menu or other tasks.
 * =============================================================================*/

#include "keyboard.h"
#include "io.h"

#define KBD_DATA_PORT   0x60    /* read scancode / write command */
#define KBD_STATUS_PORT 0x64    /* status register (bit 0 = output buffer full) */

#define KB_BUFFER_SIZE  32

/* US QWERTY scancode-to-ASCII table (set 1 make codes 0x00–0x3A) */
static const char scancode_ascii[] = {
    0,    0,   '1', '2', '3', '4', '5', '6', '7', '8',  /* 0x00–0x09 */
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', /* 0x0A–0x13 */
    't', 'y', 'u', 'i', 'o',  'p', '[', ']', '\n', 0,   /* 0x14–0x1D */
    'a', 's', 'd', 'f', 'g',  'h', 'j', 'k', 'l',  ';', /* 0x1E–0x27 */
    '\'','`',  0,  '\\','z',  'x', 'c', 'v', 'b',  'n', /* 0x28–0x31 */
    'm', ',', '.', '/',  0,   '*',  0,  ' '               /* 0x32–0x39 */
};
#define SCANCODE_TABLE_SIZE ((int)(sizeof(scancode_ascii)))

/* Shift-modified table for the same range */
static const char scancode_ascii_shift[] = {
    0,    0,   '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O',  'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G',  'H', 'J', 'K', 'L',  ':',
    '"', '~',  0,  '|', 'Z',  'X', 'C', 'V', 'B',  'N',
    'M', '<', '>', '?',  0,   '*',  0,  ' '
};

/* Scancode constants for modifier tracking */
#define SC_LSHIFT_PRESS   0x2A
#define SC_RSHIFT_PRESS   0x36
#define SC_LSHIFT_RELEASE 0xAA
#define SC_RSHIFT_RELEASE 0xB6
#define SC_CAPSLOCK_PRESS 0x3A

/* Key state */
static int shift_held  = 0;
static int caps_active = 0;

/* Ring buffer */
static char     kb_buf[KB_BUFFER_SIZE];
static uint8_t  kb_head = 0;   /* write position */
static uint8_t  kb_tail = 0;   /* read  position */
static uint8_t  kb_count = 0;

/* -------------------------------------------------------------------------- */

static void kb_push(char c) {
    if (kb_count >= KB_BUFFER_SIZE) return; /* drop if full */
    kb_buf[kb_head] = c;
    kb_head = (uint8_t)((kb_head + 1) % KB_BUFFER_SIZE);
    kb_count++;
}

/* -------------------------------------------------------------------------- */

void keyboard_init(void) {
    shift_held  = 0;
    caps_active = 0;
    kb_head = kb_tail = kb_count = 0;
}

void keyboard_poll(void) {
    /* Drain all pending scancodes */
    while (inb(KBD_STATUS_PORT) & 0x01) {
        uint8_t sc = inb(KBD_DATA_PORT);

        /* Track modifier keys */
        if (sc == SC_LSHIFT_PRESS  || sc == SC_RSHIFT_PRESS)  { shift_held = 1; continue; }
        if (sc == SC_LSHIFT_RELEASE|| sc == SC_RSHIFT_RELEASE) { shift_held = 0; continue; }
        if (sc == SC_CAPSLOCK_PRESS) { caps_active = !caps_active; continue; }

        /* Ignore release codes (bit 7 set) and extended prefix (0xE0) */
        if (sc & 0x80) continue;
        if (sc == 0xE0) continue;

        if (sc >= SCANCODE_TABLE_SIZE) continue;

        int use_shift = shift_held ^ caps_active;
        char c = use_shift ? scancode_ascii_shift[sc] : scancode_ascii[sc];
        if (c) kb_push(c);
    }
}

char keyboard_getchar(void) {
    if (kb_count == 0) return 0;
    char c = kb_buf[kb_tail];
    kb_tail = (uint8_t)((kb_tail + 1) % KB_BUFFER_SIZE);
    kb_count--;
    return c;
}
