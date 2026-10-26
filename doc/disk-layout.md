# Disk layout

An installed drive has no partitions and no allocation table. `install` writes the bootloader, the kernel and the saved state at fixed sectors, listed below, so a disk image can be read with a hex editor without any other tool. Named values are kept in [phi](phi.md), which has a slot of its own.

## The map

Sectors are 512 bytes.

    LBA 0           MBR
    LBA 1-16        stage 2
    LBA 64          the kernel, as many sectors as it needs
    LBA 4096        the state header, "SVSTATE1"
    LBA 4097        slot 0, root, 2048 sectors
    LBA 6145        slot 1, tab 1
    LBA 8193        slot 2, tab 2
    LBA 10241       slot 3, tab 3
    LBA 12289       slot 4, tab 4
    LBA 14337       slot 5, phi, 1024 sectors
    LBA 15361       the end of everything Subverge uses

About 7.5 MiB in all. `install` refuses any drive smaller than 8M. Anything past the end is ignored.

The kernel gets the sectors from 64 up to 4095, which is just under 2 MiB. If a kernel you've built doesn't fit, `install` fails with `the kernel doesn't fit in its slot`. The current one is 1.8 MB, most of it the help pages and the pictures.

## The kernel copy

`install` doesn't write the running kernel. At boot, before anything else runs, the kernel copies its own `.data` section aside, and `install` writes the code from memory followed by that untouched copy of the data. So an installed kernel boots fresh, without whatever state the live system had built up, and without anything you poked into it.

## The state header

The sector at LBA 4096 holds the marker `SVSTATE1`, the length and a checksum of each slot, the number of saves so far, the [theme](themes), the name of the [logo](logos) and the length and checksum of the phi slot, which come after the logo name. If the marker is wrong, the header is treated as empty and every slot as unused. Disks installed before 0.9.0 have `dusk` as their theme, disks installed before 0.9.1 have no logo name, so `sys` draws the `subverge` logo, and disks installed before 0.9.6 have no phi slot, so phi starts empty. A slot's data is written first and the header second, so a save that is interrupted leaves the old header pointing at a slot whose checksum no longer matches, and that slot starts fresh.

## The slots

Each slot is 2048 sectors, 1 MiB, and holds plain omega source with a zero byte after it, padded to a whole sector. Slot 0 is root, every unit after its `#>> name` line. Slots 1 to 4 are the [worlds](world.md) of tabs 1 to 4. Slot 5 is phi, 1024 sectors, 512 KiB, in a text format that is described on its [page](phi.md). See [saving](saving.md) for what's in them and when they're written.

A slot whose length is 0, too big, or whose checksum doesn't match is treated as empty.

## Finding the home drive

At boot the kernel reads sector 1 of each of the four IDE drives, and the first one with `SUBVERGE` at byte 8 is home. If two drives both have Subverge installed, the first one is home and the other one is not used.

## Looking at it

On the host, with the VM shut down:

    $ dd if=disk.img bs=512 skip=4096 count=1 | xxd | head -4
    $ dd if=disk.img bs=512 skip=6145 count=4 | strings

shows you the header and the start of tab 1's world. Don't write to the image while QEMU has it open.
