#ifndef AE3301_MULTIBOOT2_H
#define AE3301_MULTIBOOT2_H

#include <stdint.h>
#include <stdbool.h>

/* Multiboot2 memory-map parsing (AE3301 Phase 1, first atomic step).
 *
 * Scope, deliberately narrow: parse the Multiboot2 information
 * structure GRUB hands off in EBX at boot, locate the memory-map
 * tag, and produce a bounds-checked list of physical memory regions.
 * This does NOT allocate memory, manage pages, or implement a PMM —
 * it only produces the raw region list a future PMM would consume.
 *
 * Reference: Multiboot2 Specification (memory map tag, type 6).
 */

#define MB2_MAX_MEMORY_REGIONS 64

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type; /* 1 = available RAM; any other value = reserved/
                     * non-usable per the Multiboot2 spec (ACPI
                     * reclaimable, NVS, defective, etc.) — this
                     * parser does not need to distinguish those
                     * further, a future PMM can. */
} mb2_memory_region_t;

typedef struct {
    bool valid;             /* the Multiboot2 structure itself parsed
                              * without hitting a bounds/size violation */
    bool memory_map_found;
    uint32_t region_count;
    mb2_memory_region_t regions[MB2_MAX_MEMORY_REGIONS];
} mb2_parse_result_t;

/* Parses the Multiboot2 information structure at the given address.
 * During Phase 0/1, the first 1 GiB is identity-mapped (see boot.S),
 * so this physical address is directly dereferenceable.
 *
 * Never trusts bootloader-provided sizes: every tag and every memory
 * map entry is bounds-checked against the structure's own declared
 * total_size before being read. On any malformed/out-of-bounds
 * value, parsing stops and result.valid is left false (or, for a
 * missing terminator only, true with whatever well-formed tags were
 * already parsed — see multiboot2.c for the exact rule). */
mb2_parse_result_t mb2_parse(uint64_t mb2_info_addr);

/* Prints the parse result over the existing serial driver. No new
 * logging infrastructure — reuses serial_write_str exactly as
 * Phase 0 does. */
void mb2_print_result(const mb2_parse_result_t *result);

#endif
