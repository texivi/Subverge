# Boot

The ISO boots with GRUB. Installed disks boot with Subverge's own boot sector and second stage. Both paths end in the kernel's `kmain`, and the kernel can't tell which loader started it.

## From the ISO

The ISO is built with `grub-mkrescue` and loads `/boot/subverge.elf`. GRUB is the only code in the system that wasn't written for Subverge, and it runs only on the ISO.

## From an installed disk

Installed disks don't have GRUB. The first sector of the disk is Subverge's own boot sector, written in nasm, which reads the second stage that follows it. The second stage does the work GRUB would do: it turns on the A20 line, reads the kernel from the disk, collects the memory map, switches to protected mode and starts the kernel. `install` writes both stages together with the kernel, and the kernel finds which drive it was installed on by a marker in the second stage. If a read fails, the boot sector prints `subverge: can't read stage 2` and the second stage prints `subverge: can't read the kernel`, and the machine halts.

## The kernel's entry

The kernel's first code sets up a stack and the page tables, which map the first 4 GiB, turns on 64-bit mode and calls `kmain`. See [internals](internals) for what `kmain` does next.

## Why not UEFI

BIOS booting was enough for QEMU, and the text screen is built on VGA text mode, which UEFI machines often don't have. UEFI booting is on the [roadmap](roadmap).
