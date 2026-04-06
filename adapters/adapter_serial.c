/* =============================================================================
 * AI Aura OS — Serial Port Adapter (COM1)
 * Provides read/write access to the COM1 serial port (0x3F8).
 * Useful for debugging and host communication during emulation.
 * =============================================================================*/

#include "adapter.h"
#include "../kernel/plugin.h"
#include "../kernel/io.h"
#include "../kernel/vga.h"
#include "../kernel/kstring.h"
#include "../kernel/keyboard.h"

#define COM1_PORT       0x3F8
#define COM1_DATA       (COM1_PORT + 0)
#define COM1_IER        (COM1_PORT + 1)
#define COM1_FCR        (COM1_PORT + 2)
#define COM1_LCR        (COM1_PORT + 3)
#define COM1_MCR        (COM1_PORT + 4)
#define COM1_LSR        (COM1_PORT + 5)

/* -------------------------------------------------------------------------- */

static aura_status_t serial_open(void) {
    outb(COM1_IER, 0x00);   /* Disable interrupts */
    outb(COM1_LCR, 0x80);   /* Enable DLAB */
    outb(COM1_DATA, 0x03);  /* Baud divisor low: 38400 baud */
    outb(COM1_IER,  0x00);  /* Baud divisor high */
    outb(COM1_LCR,  0x03);  /* 8N1: 8 bits, no parity, 1 stop bit */
    outb(COM1_FCR,  0xC7);  /* Enable FIFO, clear, 14-byte threshold */
    outb(COM1_MCR,  0x0B);  /* IRQs enabled, RTS/DSR set */
    return AURA_OK;
}

static aura_status_t serial_close(void) {
    outb(COM1_IER, 0x00);
    return AURA_OK;
}

static int serial_tx_ready(void) {
    return (int)(inb(COM1_LSR) & 0x20);
}

static int serial_write(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        while (!serial_tx_ready()) {} /* Spin wait */
        outb(COM1_DATA, buf[i]);
    }
    return (int)len;
}

static int serial_rx_ready(void) {
    return (int)(inb(COM1_LSR) & 0x01);
}

static int serial_read(uint8_t *buf, uint32_t len) {
    uint32_t count = 0;
    while (count < len && serial_rx_ready()) {
        buf[count++] = inb(COM1_DATA);
    }
    return (int)count;
}

static aura_status_t serial_ioctl(uint32_t cmd, void *arg) {
    (void)cmd; (void)arg;
    return AURA_OK;
}

/* Mirror each VGA character out the serial port */
static void serial_mirror_char(char c) {
    uint8_t b = (uint8_t)c;
    /* Translate bare '\n' to '\r\n' for serial terminals */
    if (c == '\n') {
        uint8_t cr = '\r';
        serial_write(&cr, 1);
    }
    serial_write(&b, 1);
}

/* -------------------------------------------------------------------------- */

static adapter_t serial_adapter = {
    .name  = "serial-com1",
    .flags = 0,
    .ops   = {
        .open  = serial_open,
        .close = serial_close,
        .read  = serial_read,
        .write = serial_write,
        .ioctl = serial_ioctl,
    },
    .open  = 0,
};

/* Plugin lifecycle wrappers */
static aura_status_t serial_plugin_init(void) {
    aura_status_t r = serial_open();
    if (r == AURA_OK) {
        serial_adapter.open = 1;
        const uint8_t banner[] = "AI Aura OS serial console ready\r\n";
        serial_write(banner, sizeof(banner) - 1);
        /* Enable VGA→serial mirroring */
        vga_set_serial_hook(serial_mirror_char);
        vga_println("[serial] COM1 adapter initialized.");
    }
    return r;
}

static aura_status_t serial_plugin_shutdown(void) {
    vga_set_serial_hook((void *)0);
    serial_close();
    serial_adapter.open = 0;
    return AURA_OK;
}

/* Serial poll: drain incoming COM1 bytes and inject them into the keyboard
 * ring buffer so the OS can be controlled from a serial console (e.g. via
 * `make run-serial` on QEMU or a real COM1 terminal). */
void serial_poll(void) {
    uint8_t buf[16];
    int n = serial_read(buf, sizeof(buf));
    for (int i = 0; i < n; i++) {
        char c = (char)buf[i];
        /* Normalise carriage-return to newline for compatibility */
        if (c == '\r') c = '\n';
        keyboard_push(c);
    }
}

aura_status_t adapter_register(adapter_t *adp) {
    plugin_desc_t desc = {0};
    strncpy_k(desc.name, adp->name, PLUGIN_NAME_LEN);
    desc.version[0]= '1'; desc.version[1] = '\0';
    desc.type      = PLUGIN_TYPE_ADAPTER;
    desc.init      = serial_plugin_init;
    desc.tick      = NULL;
    desc.shutdown  = serial_plugin_shutdown;
    desc.priv      = adp;
    aura_status_t r = plugin_register(&desc);
    if (r == AURA_OK) plugin_activate(desc.name);
    return r;
}

/* Called from kernel_main() to register the serial adapter */
void adapter_serial_register(void) {
    adapter_register(&serial_adapter);
}
