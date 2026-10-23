# Drives

Subverge talks to disks through the legacy IDE interface, using programmed I/O.

## Finding drives

At boot the kernel probes four positions: master and slave on the primary channel (ports `0x1F0` to `0x1F7`, control `0x3F6`) and on the secondary channel (`0x170` to `0x177`, control `0x376`). They are numbered 0 to 3 in that order, which matches QEMU's `-hda` to `-hdd`.

For each position the kernel checks for a floating bus, selects the drive and sends `IDENTIFY DEVICE`. A position counts as a drive if it answers, isn't an ATAPI device and reports a non-zero size. CD-ROM drives answer as ATAPI and are skipped, which is why the drive the live ISO boots from never shows up in `disks`.

The kernel keeps the model string and the size of each drive.

    drives .             [ [ 0 "QEMU HARDDISK" 64 0 ] ]

Each entry is the drive number, the model, the size in MiB and whether Subverge is installed on it. `disks` prints the same list as a table.

## Reading and writing

Transfers are done by polling, with interrupts off. That keeps the code short and makes a transfer impossible to interrupt halfway, at the cost of stopping the other tasks while it runs. A save of a full slot takes a noticeable fraction of a second in QEMU. A transfer that doesn't become ready is treated as failed. A save writes the slot first and the header that describes it last ([disk layout](disk-layout)), so a save that fails partway leaves the slot to start fresh at the next boot.

## The home drive

After probing, the kernel looks for an installed system by reading sector 1 of each drive and checking for the `SUBVERGE` signature in the stage 2 header. The first drive that has it becomes the home drive, `home` returns its number, and [saving](saving.md) writes there. With no home drive, `home` returns `-1`.

Installing makes the target drive the home drive immediately, without a reboot.

## Supported controllers

The kernel reaches drives through the legacy IDE ports only, so AHCI, NVMe, virtio, USB storage and SCSI disks stay invisible to it. On real hardware a SATA controller has to be set to IDE or legacy mode in the firmware for its disks to appear. DMA is never used.

## Words

For `drives`, `disks`, `home`, `install` and `install-to`, see [machine](machine.md). Sector-level access is kept inside the kernel, so omega code reaches the disk only through saving and installing.
