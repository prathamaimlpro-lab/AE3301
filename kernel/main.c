#include <stdint.h>
#include "arch/x86_64/serial.h"

/* Phase 0 exit criteria (per AE3301 roadmap v1.0 §61):
 * kernel boots via QEMU, initializes interrupts + a physical memory
 * allocator, and prints verifiable serial output.
 *
 * This file currently implements only the "prints verifiable serial
 * output" portion — interrupts (kernel/arch/x86_64/idt.c) and the
 * physical memory allocator (kernel/mm/pmm.c) are the next two
 * pieces of Milestone 0.1, deliberately not implemented yet so this
 * boots and is testable before more code is added on top of it. */

void kernel_main(uint64_t multiboot_info_ptr) {
    (void)multiboot_info_ptr; /* not yet consumed — Phase 0 doesn't need memory map info yet */

    serial_init();
    serial_write_str("AE3301 Phase 0: kernel_main reached\n");
    serial_write_str("AE3301 Phase 0: boot OK\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
