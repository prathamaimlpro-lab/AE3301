#ifndef AE3301_SERIAL_H
#define AE3301_SERIAL_H

#include <stdint.h>

/* Minimal 16550 UART driver for COM1 (port 0x3F8). This is the
 * standard PC serial port I/O sequence — same init sequence used
 * across essentially every hobby-OS and documented on the OSDev
 * wiki's "Serial Ports" page. Used for Phase 0 boot output only;
 * replaced/extended by the real Observability Fabric (v1.0 §36)
 * once the kernel has IPC and a proper logging subsystem. */

#define COM1_PORT 0x3F8

void serial_init(void);
void serial_write_char(char c);
void serial_write_str(const char *s);

#endif
