# AE3301 Phase 0 build system.
#
# Deliberately simple (Make, not a hermetic build system yet) — the
# v1.0 roadmap's "reproducible builds" goal applies once there's
# enough surface area (multiple libraries, generated files) to make
# reproducibility non-trivial. A single-target freestanding kernel
# does not need that machinery yet; Make is adopted here for Phase 0
# and revisited (Meson/Ninja per roadmap v0.1 §14/§28) once the
# userland/driver framework exists and the build graph grows.

CC      := gcc
AS      := gcc
LD      := ld

CFLAGS  := -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
           -mcmodel=kernel -mgeneral-regs-only -Wall -Wextra -std=gnu11 -O2
ASFLAGS := -m64
LDFLAGS := -n -T kernel/linker.ld -nostdlib

SRCS_C  := kernel/main.c kernel/arch/x86_64/serial.c kernel/arch/x86_64/multiboot2.c
SRCS_S  := kernel/arch/x86_64/boot.S
OBJS    := $(SRCS_C:.c=.o) $(SRCS_S:.S=.o)

KERNEL_ELF := build/ae3301.elf
ISO        := build/ae3301.iso

.PHONY: all clean iso run

all: $(KERNEL_ELF)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJS)
	mkdir -p build
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(OBJS)

iso: $(KERNEL_ELF)
	mkdir -p build/isodir/boot/grub
	cp $(KERNEL_ELF) build/isodir/boot/ae3301.elf
	cp tools/grub.cfg build/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) build/isodir

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -serial stdio -display none -no-reboot -m 256M

clean:
	rm -f $(OBJS)
	rm -rf build
