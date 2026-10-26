# Why Not More?

Subverge leaves out much that other systems have. Each item on this page is missing because it is a driver or a layer of its own, and each of them is on the [roadmap](roadmap) or has a reason it isn't.

## Real hardware

Subverge is developed and tested in QEMU only. The drivers are written for the hardware QEMU provides, which is a PC from the late 1990s with a 64-bit CPU: a PIT and two PICs for time and interrupts, a PS/2 keyboard and mouse, IDE disks through the legacy ports and a VGA card, with Bochs graphics for the `gfx` mode. Many real machines provide the same things through compatibility modes in their firmware. A machine that does not, such as one that boots only through UEFI, can't start the system.

## UEFI

The ISO boots through GRUB in BIOS mode and an installed disk boots through Subverge's own boot sector and second stage, which use BIOS disk services ([boot](boot)). UEFI is on the roadmap. It needs a loader that is a program in a different format and a different way of finding the memory map, and it takes away the VGA text mode that the text screen needs.

## USB

A USB keyboard or mouse works only where the firmware emulates a PS/2 controller for it. A USB driver is a stack of layers, one for each of the controllers that PCs have had, and each one is large. It is on the roadmap.

## Disks

Subverge reads and writes IDE disks through the legacy ports and uses programmed I/O, so it is slow and works everywhere QEMU's IDE controller does. Disks behind an AHCI controller, an NVMe controller or a virtio device are not seen. AHCI is on the roadmap ([drives](drives)).

## Networking

There is no network driver. A network stack is a driver and a set of protocols, and the protocols are most of the work. It is on the roadmap.

## Sound

The only sound is the PC speaker, which plays one tone at a time through `beep`. A sound card driver would need the PCI bus, and the format of the sound is not decided. It is on the roadmap.

## Floating point

omega's numbers are exact integers, bignums, ordinals and cardinals. The kernel is compiled without floating point so that it doesn't have to save the registers of the unit when it switches tasks. A floating point number type is on the roadmap.

## More than one CPU

The system uses one core. The scheduler, the heap and the interrupt handlers assume nothing runs in parallel with them. Several cores are on the roadmap.

## Files

The disk holds fixed slots, and the values with names are in [phi](phi), which is a tree of sets in memory with its own slot. Programs and definitions are omega words, and an installed disk saves them as omega source. phi has no permissions and no links between values ([disk layout](disk-layout)).

## Protection

All code runs in ring 0 with access to everything, and a bad write can stop the machine. Nothing on the roadmap changes this ([design](design)).

## Fonts

There is one font, the 8 by 16 font of the VGA card,.

See [decisions](decisions) for what the system chose to do and [expectations](expectations) for what to expect of it.
