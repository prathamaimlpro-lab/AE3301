#include "serial.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    /* disable interrupts */
    outb(COM1_PORT + 3, 0x80);    /* enable DLAB (set baud rate divisor) */
    outb(COM1_PORT + 0, 0x03);    /* divisor low byte: 38400 baud */
    outb(COM1_PORT + 1, 0x00);    /* divisor high byte */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* enable FIFO, clear, 14-byte threshold */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs disabled, RTS/DSR set */
}

static int transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_write_char(char c) {
    while (!transmit_empty()) { }
    outb(COM1_PORT, (uint8_t)c);
}

void serial_write_str(const char *s) {
    while (*s) {
        if (*s == '\n') serial_write_char('\r');
        serial_write_char(*s++);
    }
}
