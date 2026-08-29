#include "multiboot2.h"
#include "serial.h"

#define MB2_TAG_TYPE_END        0
#define MB2_TAG_TYPE_MEMORY_MAP 6

#define MB2_MEM_TYPE_AVAILABLE  1

/* Sanity bound on total structure size. Real GRUB output is at most
 * a few KiB; anything absurd is treated as malformed rather than
 * trusted — this is a defensive limit, not a spec value. */
#define MB2_MAX_TOTAL_SIZE      (1u * 1024u * 1024u)

#define MB2_MIN_TOTAL_SIZE      8u   /* total_size(4) + reserved(4) */
#define MB2_TAG_HEADER_SIZE     8u   /* type(4) + size(4) */
#define MB2_MEMMAP_HEADER_SIZE  16u  /* type(4)+size(4)+entry_size(4)+entry_version(4) */
#define MB2_MEMMAP_MIN_ENTRY    24u  /* base(8)+length(8)+type(4)+reserved(4) */

static uint32_t align_up8(uint32_t v) {
    return (v + 7u) & ~7u;
}

static void print_hex64(uint64_t v) {
    char buf[19];
    const char *digits = "0123456789abcdef";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = digits[(v >> ((15 - i) * 4)) & 0xF];
    }
    buf[18] = '\0';
    serial_write_str(buf);
}

static void print_uint32(uint32_t v) {
    char buf[11];
    int i = 10;
    buf[i--] = '\0';
    if (v == 0) {
        buf[i--] = '0';
    } else {
        while (v > 0 && i >= 0) {
            buf[i--] = (char)('0' + (v % 10));
            v /= 10;
        }
    }
    serial_write_str(&buf[i + 1]);
}

mb2_parse_result_t mb2_parse(uint64_t mb2_info_addr) {
    mb2_parse_result_t result;
    result.valid = false;
    result.memory_map_found = false;
    result.region_count = 0;

    if (mb2_info_addr == 0) {
        return result; /* untrusted input: reject null outright */
    }
    if ((mb2_info_addr & 0x7u) != 0) {
        return result; /* Multiboot2 requires 8-byte alignment */
    }

    const uint8_t *base = (const uint8_t *)(uintptr_t)mb2_info_addr;

    uint32_t total_size;
    __builtin_memcpy(&total_size, base, sizeof(total_size));

    if (total_size < MB2_MIN_TOTAL_SIZE || total_size > MB2_MAX_TOTAL_SIZE) {
        return result; /* malformed or implausible — refuse to walk it */
    }

    uint32_t offset = 8; /* past total_size + reserved fields */

    while (offset + MB2_TAG_HEADER_SIZE <= total_size) {
        uint32_t tag_type, tag_size;
        __builtin_memcpy(&tag_type, base + offset, sizeof(tag_type));
        __builtin_memcpy(&tag_size, base + offset + 4, sizeof(tag_size));

        if (tag_size < MB2_TAG_HEADER_SIZE) {
            return result; /* a tag can never be smaller than its own header */
        }
        if (offset + tag_size > total_size) {
            return result; /* tag claims to extend past the structure */
        }

        if (tag_type == MB2_TAG_TYPE_END) {
            result.valid = true;
            return result;
        }

        if (tag_type == MB2_TAG_TYPE_MEMORY_MAP && !result.memory_map_found
            && tag_size >= MB2_MEMMAP_HEADER_SIZE) {

            uint32_t entry_size, entry_version;
            __builtin_memcpy(&entry_size, base + offset + 8, sizeof(entry_size));
            __builtin_memcpy(&entry_version, base + offset + 12, sizeof(entry_version));
            (void)entry_version;

            if (entry_size >= MB2_MEMMAP_MIN_ENTRY) {
                uint32_t entries_bytes = tag_size - MB2_MEMMAP_HEADER_SIZE;
                uint32_t entry_count = entries_bytes / entry_size;
                result.memory_map_found = true;

                for (uint32_t i = 0;
                     i < entry_count && result.region_count < MB2_MAX_MEMORY_REGIONS;
                     i++) {

                    uint32_t entry_offset = offset + MB2_MEMMAP_HEADER_SIZE + i * entry_size;

                    /* Redundant per-entry bound check even though
                     * entry_count was floor-divided to fit — cheap
                     * insurance against an arithmetic mistake above
                     * ever reading past the tag's own bounds. */
                    if (entry_offset + MB2_MEMMAP_MIN_ENTRY > offset + tag_size) {
                        break;
                    }

                    uint64_t e_base, e_len;
                    uint32_t e_type;
                    __builtin_memcpy(&e_base, base + entry_offset, sizeof(e_base));
                    __builtin_memcpy(&e_len, base + entry_offset + 8, sizeof(e_len));
                    __builtin_memcpy(&e_type, base + entry_offset + 16, sizeof(e_type));

                    result.regions[result.region_count].base = e_base;
                    result.regions[result.region_count].length = e_len;
                    result.regions[result.region_count].type = e_type;
                    result.region_count++;
                }
            }
        }

        offset += align_up8(tag_size);
    }

    /* Reached total_size without seeing a terminating tag. Per spec
     * this shouldn't happen, but every tag we did walk was already
     * bounds-checked above, so what we've collected is still valid —
     * we just never saw an explicit end marker. */
    result.valid = true;
    return result;
}

void mb2_print_result(const mb2_parse_result_t *result) {
    if (!result->valid) {
        serial_write_str("AE3301 Phase 1: Multiboot2 info INVALID (parse failed)\n");
        return;
    }

    serial_write_str("AE3301 Phase 1: Multiboot2 info OK\n");

    if (!result->memory_map_found) {
        serial_write_str("AE3301 Phase 1: no memory-map tag found\n");
        return;
    }

    serial_write_str("AE3301 Phase 1: memory-map tag found, entries=");
    print_uint32(result->region_count);
    serial_write_str("\n");

    for (uint32_t i = 0; i < result->region_count; i++) {
        serial_write_str("  region base=");
        print_hex64(result->regions[i].base);
        serial_write_str(" length=");
        print_hex64(result->regions[i].length);
        serial_write_str(" type=");
        print_uint32(result->regions[i].type);
        serial_write_str(result->regions[i].type == MB2_MEM_TYPE_AVAILABLE
                          ? " (usable)\n" : " (reserved)\n");
    }
}
