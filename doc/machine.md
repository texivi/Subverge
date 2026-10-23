# Machine

These words look at the memory map and the drives, reboot or halt the machine, and install Subverge without asking.

## Looking

    memmap              [ base length type ] for each region
    kernel              kernel image bytes, then bytes of RAM
    drives              [ n model megabytes home ] for each drive
    sys-c               the system info screen, written in C

`memmap` is the BIOS memory map, as passed in by GRUB or by Subverge's own [bootloader](boot.md), which collects it with E820 so the kernel can't tell the difference. Type 1 is usable RAM. The other types are reserved, ACPI tables, or memory the firmware keeps for itself, and the kernel leaves them alone. `memls` prints it with names. See [memory](memory.md) for what lives where.

`drives` lists up to four IDE drives, the primary and secondary controller's master and slave, with the model string the drive reports, its size in megabytes, and `1` for the one Subverge is installed on. `disks` prints the same list as a table, and `home` gives the installed drive's number on its own.

## Rebooting and halting

    reboot              restarts the machine
    halt                stops it

`reboot` pulses the reset line through the keyboard controller. On an installed disk it's safe, because every [world](world.md) was [saved](saving.md) after its last line. On the live ISO, everything is lost.

`halt` prints `the machine is halted`, disables interrupts and halts the CPU. Only a reset recovers from it. `try` can't catch it. Close QEMU to get out.

## Installing without asking

    0 install-to

installs Subverge on drive 0 without listing drives or asking for confirmation. It writes the boot sector, stage 2, a fresh copy of the kernel and an empty state header, then saves root and all four tabs into the new slots, and prints `installed`. `install` is the omega program in root that asks for a drive, waits for `yes` and then calls `install-to`.

This is the most destructive word in the system. With a disk image in QEMU it is harmless, but on real hardware it erases whichever drive number you give it. See [disk layout](disk-layout.md) for exactly which sectors it writes.

It refuses drives smaller than 8 MB, and it refuses to run if the kernel's own data section couldn't be copied at boot, because it installs a clean copy of the kernel as it was before anything ran, not the running one with all your changes poked into it.

## Saving by hand

    save

writes root and all four tabs to the installed disk right now. Everything is already saved after each line, so this is rarely needed. It fails on the live ISO with `you haven't installed yet, run install`.

## The rest

See [memory](memory.md) for raw memory, [ports](ports.md) for ports and PCI, [screen](screen.md) for the screen, and [internals](internals.md) for what happens underneath.
