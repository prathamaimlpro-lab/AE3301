/* AE3301 Phase 0 linker script.
 * Kernel loaded at the conventional 1 MiB physical address expected
 * by GRUB/Multiboot2 for a non-ELF-loaded, BIOS-booted kernel. */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text)
    }

    /* Page-align so rodata/data/bss can get their own PT_LOAD segment
     * with correct (non-executable) permissions instead of inheriting
     * .text's RWX segment — closes the RWX-LOAD-segment warning as a
     * real fix rather than a suppressed one. */
    . = ALIGN(4096);

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data)
    }

    . = ALIGN(4096);

    .bss : {
        *(COMMON)
        *(.bss)
    }
}
