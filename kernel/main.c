#include <stdint.h>
#include "arch/x86_64/serial.h"
#include "arch/x86_64/multiboot2.h"

/* Phase 0 exit criteria (per AE3301 roadmap v1.0 §61):
 * kernel boots via QEMU, initializes interrupts + a physical memory
 * allocator, and prints verifiable serial output.
 *
 * Phase 0 (boot + serial output) is DONE and verified 100/100 in CI.
 * This file now also implements Phase 1's first atomic step:
 * Multiboot2 memory-map parsing (kernel/arch/x86_64/multiboot2.c).
 * Interrupts and the physical memory allocator itself are still not
 * implemented — this step only parses and prints the memory map;
 * nothing consumes it yet. */

void kernel_main(uint64_t multiboot_info_ptr) {
    serial_init();
    serial_write_str("AE3301 Phase 0: kernel_main reached\n");
    serial_write_str("AE3301 Phase 0: boot OK\n");

    mb2_parse_result_t mb2_result = mb2_parse(multiboot_info_ptr);
    mb2_print_result(&mb2_result);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
