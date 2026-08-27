/* AE3301 Phase 0 boot stub — GAS/AT&T syntax
 * Multiboot2 entry (loaded by GRUB in 32-bit protected mode),
 * sets up minimal identity-mapped paging + long mode, then jumps
 * to the 64-bit kernel entry point (kernel_main in kernel.c).
 *
 * Reference: Multiboot2 Specification, GNU GRUB Manual.
 * Long-mode transition sequence follows the standard OSDev pattern:
 * enable PAE -> load PML4 -> set EFER.LME -> enable paging (CR0.PG)
 * -> far jump to 64-bit code segment.
 */

.set MB2_MAGIC,        0xe85250d6
.set MB2_ARCH_I386,    0
.set MB2_HEADER_LEN,   (mb2_header_end - mb2_header_start)
.set MB2_CHECKSUM,     -(MB2_MAGIC + MB2_ARCH_I386 + MB2_HEADER_LEN)

.section .multiboot, "a"
.align 8
mb2_header_start:
    .long MB2_MAGIC
    .long MB2_ARCH_I386
    .long MB2_HEADER_LEN
    .long MB2_CHECKSUM
    /* end tag */
    .word 0     /* type */
    .word 0     /* flags */
    .long 8     /* size */
mb2_header_end:

.section .bss
.align 16
stack_bottom:
    .skip 16384         /* 16 KiB boot stack */
stack_top:

.align 4096
pml4_table:  .skip 4096
pdpt_table:  .skip 4096
pd_table:    .skip 4096

.section .rodata
gdt64:
    .quad 0                                   /* null descriptor */
gdt64_code:
    .quad 0x00AF9A000000FFFF                  /* 64-bit code segment, exec/read, ring0 */
gdt64_data:
    .quad 0x00AF92000000FFFF                  /* 64-bit data segment, read/write */
gdt64_end:

gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .quad gdt64

.section .text
.code32
.global _start
.type _start, @function
_start:
    cli
    mov $stack_top, %esp
    mov %ebx, %edi          /* preserve multiboot2 info pointer (arg for later use) */

    /* --- Build identity-mapped paging structures for first 1 GiB
     * using a single 2 MiB page in PD (present+writable+huge). This
     * is deliberately minimal: Phase 0 only needs enough mapping to
     * reach kernel_main() and print to serial; real memory management
     * (kernel/mm) replaces this before Phase 1. */
    mov $pdpt_table, %eax
    or $0x3, %eax            /* present + writable */
    mov %eax, pml4_table

    mov $pd_table, %eax
    or $0x3, %eax
    mov %eax, pdpt_table

    /* Map the first 1 GiB in 2 MiB pages (512 entries) */
    mov $0, %ecx
fill_pd:
    mov %ecx, %eax
    shl $21, %eax             /* eax = ecx * 2MiB */
    or $0x83, %eax            /* present + writable + huge(2MB) */
    mov %eax, pd_table(,%ecx,8)
    inc %ecx
    cmp $512, %ecx
    jl fill_pd

    /* Load CR3 with PML4 */
    mov $pml4_table, %eax
    mov %eax, %cr3

    /* Enable PAE (CR4 bit 5) */
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    /* Set EFER.LME (long mode enable), MSR 0xC0000080 */
    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    /* Enable paging + protected mode (CR0.PG bit31, CR0.PE bit0) */
    mov %cr0, %eax
    or $0x80000001, %eax
    mov %eax, %cr0

    /* Load 64-bit GDT and far-jump into long mode code segment */
    lgdt gdt64_ptr
    ljmp $0x08, $long_mode_entry

.code64
long_mode_entry:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    mov $stack_top, %rsp
    /* multiboot2 info pointer was in edi (32-bit); zero-extended into rdi automatically */
    call kernel_main

halt_loop:
    cli
    hlt
    jmp halt_loop

.size _start, . - _start

/* Mark stack non-executable so the linker doesn't default the whole
 * image to an executable stack (NX hygiene — security-by-design
 * applies even to Phase 0 boot code, not just the eventual kernel). */
.section .note.GNU-stack,"",@progbits
