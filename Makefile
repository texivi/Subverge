CC      := gcc
LD      := ld
AS      := nasm
OBJCOPY := objcopy
CFLAGS  := -m64 -std=gnu11 -O2 -Wall -Wextra -ffreestanding -fno-pie -fno-stack-protector \
           -fno-asynchronous-unwind-tables -fcf-protection=none -mgeneral-regs-only -mno-red-zone \
           -nostdlib -Ikernel
LDFLAGS := -m elf_x86_64 -T linker.ld -nostdlib -z max-page-size=0x1000

SRC  := $(wildcard kernel/*.c)
OBJ  := build/boot.o build/rootfs.o build/docfs.o build/pics.o build/bootbins.o $(SRC:kernel/%.c=build/%.o)
ELF  := build/ordinal.elf
IMG  := build/subverge.elf

all: $(IMG)

# elf32 for multiboot
$(IMG): $(ELF)
	objcopy -O elf32-i386 $< $@

$(ELF): $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# root units
build/rootfs.bin: $(sort $(wildcard root/*.om)) | build
	cat $^ > $@

build/rootfs.o: build/rootfs.bin
	$(LD) -m elf_x86_64 -r -b binary -z noexecstack -o $@ $<
	$(OBJCOPY) --rename-section .data=.rodata.blob,alloc,load,readonly,data,contents $@

# help pages
build/docfs.bin: $(sort $(wildcard doc/*.md)) | build
	@for f in $^; do printf '#>> %s\n' "$$(basename $$f .md)"; cat $$f; printf '\n'; done > $@

build/docfs.o: build/docfs.bin
	$(LD) -m elf_x86_64 -r -b binary -z noexecstack -o $@ $<
	$(OBJCOPY) --rename-section .data=.rodata.blob,alloc,load,readonly,data,contents $@

# pictures
build/pics.bin: $(sort $(wildcard art/*.jpg)) | build
	@for f in $^; do printf '#>> %s %s\n' "$$(basename $$f .jpg)" "$$(stat -c %s $$f)"; cat $$f; done > $@

build/pics.o: build/pics.bin
	$(LD) -m elf_x86_64 -r -b binary -z noexecstack -o $@ $<
	$(OBJCOPY) --rename-section .data=.rodata.blob,alloc,load,readonly,data,contents $@

# installer blobs
build/mbr.bin: boot/mbr.asm | build
	$(AS) -f bin $< -o $@

build/stage2.bin: boot/stage2.asm | build
	$(AS) -f bin $< -o $@

build/bootbins.o: build/mbr.bin build/stage2.bin
	$(LD) -m elf_x86_64 -r -b binary -z noexecstack -o $@ build/mbr.bin build/stage2.bin
	$(OBJCOPY) --rename-section .data=.rodata.blob,alloc,load,readonly,data,contents $@

build/boot.o: boot/boot.asm | build
	$(AS) -f elf64 $< -o $@

build/%.o: kernel/%.c kernel/*.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(IMG)
	qemu-system-x86_64 -kernel $(IMG) -m 256M

# blank drive
disk:
	truncate -s 64M build/disk.img

run-install: $(IMG)
	qemu-system-x86_64 -kernel $(IMG) -m 256M -drive file=build/disk.img,format=raw,if=ide

# installed boot
run-disk:
	qemu-system-x86_64 -m 256M -drive file=build/disk.img,format=raw,if=ide

clean:
	rm -rf build

.PHONY: all run clean disk run-install run-disk

# linux harness
HOSTSRC := kernel/lib.c kernel/big.c kernel/omega.c kernel/words.c kernel/ord.c kernel/root.c kernel/hw.c \
           kernel/line.c kernel/scrawl.c kernel/calc.c kernel/sysinfo.c kernel/mem.c kernel/vga.c kernel/sched.c kernel/jit.c kernel/dis.c kernel/help.c kernel/doc.c kernel/mouse.c kernel/gfx.c kernel/img.c
host: build/rootfs.o build/docfs.o build/pics.o $(HOSTSRC) test/host.c kernel/*.h
	gcc -DHOST -g -O1 -fno-builtin -no-pie -w -Ikernel -o build/host $(HOSTSRC) test/host.c build/rootfs.o build/docfs.o build/pics.o

stress: build/rootfs.o build/docfs.o build/pics.o $(HOSTSRC) test/host.c kernel/*.h
	gcc -DHOST -DGC_STRESS -g -O1 -fno-builtin -no-pie -w -Ikernel -o build/stress $(HOSTSRC) test/host.c build/rootfs.o build/docfs.o build/pics.o

qdrive: build/qdrive
build/qdrive: test/qdrive.c
	gcc -O2 -Wall -Wextra -o build/qdrive test/qdrive.c

jit-test: build/subverge.elf build/qdrive
	build/qdrive test/jit.qd -- qemu-system-x86_64 -kernel build/subverge.elf -m 256 -no-reboot

doccheck:
	python3 test/doclinks.py

.PHONY: host stress qdrive jit-test doccheck

build/iso/boot/grub/grub.cfg:
	mkdir -p build/iso/boot/grub
	printf 'set timeout=0\nmenuentry "Subverge" {\n\tmultiboot /boot/subverge.elf\n\tboot\n}\n' > $@

iso: build/subverge.elf build/iso/boot/grub/grub.cfg
	cp build/subverge.elf build/iso/boot/subverge.elf
	grub-mkrescue -o build/subverge.iso build/iso 2>&1 | tail -1

.PHONY: iso

run-iso: iso
	qemu-system-x86_64 -cdrom build/subverge.iso -boot d -m 256M

.PHONY: run-iso
