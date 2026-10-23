# Ports

These words talk to I/O ports, the CPU and the PCI bus. They run in ring 0, check nothing and can't be undone. `in` and `out` accept any port, including those that reset the machine.

## I/O ports

| Word | Effect | |
|---|---|---|
| `in` | `port -- n` | read a byte |
| `out` | `n port --` | write a byte |
| `ind` | `port -- n` | read 4 bytes |
| `outd` | `n port --` | write 4 bytes |

These map directly to the `in` and `out` instructions.

Notable ports:

| Port | What's there |
|---|---|
| `0x20`, `0xA0` | the two interrupt controllers |
| `0x40` to `0x43` | the PIT, which drives the 100 Hz timer and therefore the [scheduler](scheduler.md) |
| `0x61` | the PC speaker gate, which `beep` uses together with PIT channel 2 |
| `0x60`, `0x64` | the PS/2 controller, for the keyboard and the mouse, which `reboot` also uses |
| `0x80` | the POST port, harmless to write, which is why the examples use it |
| `0x1F0` to `0x1F7`, `0x3F6` | the primary IDE controller, where installed disks live |
| `0x170` to `0x177`, `0x376` | the secondary IDE controller |
| `0x3C0` to `0x3DF` | VGA, including the palette and the font |
| `0x1CE`, `0x1CF` | the Bochs display registers, which `gfx` uses to start graphics mode ([screen](screen.md)) |
| `0x92` | fast A20, which the [bootloader](boot.md) turns on |
| `0xCF8`, `0xCFC` | PCI configuration space |

Reprogramming the PIT changes the scheduling rate and the length of `sleep`. Masking interrupts at the PIC stops the timer or the keyboard. Writing to the IDE ports during a save can corrupt a slot.

## The CPU

    0 cpuid             leaves eax ebx ecx edx for leaf 0
    cpu say             the CPU's brand string
    0 cr                control register 0
    rdtsc               the time stamp counter

`cpuid` takes a leaf and runs the instruction with it in `eax`. `cr` reads `cr0`, `cr2`, `cr3` or `cr4`, and anything else fails with `cr only takes 0, 2, 3 or 4`. No word writes a control register. `cr3` is the address of the page table root, and with `peekq` you can walk it.

`cpuinfo` on the [tools](tools.md) page is built on `cpuid`, and decodes the feature flags with `edx-flags`, `ecx-flags` and `show-flags`.

## Interrupts

    1 irq               how many times IRQ 1 has fired

Only IRQ 0, the timer, IRQ 1, the keyboard, and IRQ 12, the mouse, are unmasked. All other lines are masked, so their counts stay at `0`. The disks are driven by polling, not interrupts. `irqmon` shows the counts live, and `top` shows the rates.

Vector 48 is the software interrupt a tab uses to give up the CPU early. See the [scheduler](scheduler.md).

## PCI

    pci                 every PCI function as [ bus dev fn vendor device class subclass ]
    0 0 0 0 pci@        a dword from a function's configuration space

`pci@` takes the bus, device, function and offset, and reads through `0xCF8` and `0xCFC`. A PCI write is `outd` to `0xCF8` followed by `outd` to `0xCFC`.

`pcils` prints the list with vendor and class names from `vendor` and `class-name`, which only know the vendors QEMU presents.


## An example: the CMOS clock

The real-time clock sits behind two ports. Writing a register number to `0x70` selects it, and reading `0x71` returns its value. Register 0 holds the seconds:

    0 0x70 out 0x71 in hex say

The clock stores its values in BCD, one decimal digit per hex digit, so 42 seconds is the byte `0x42`. `hex` prints that as `42`, while `.` would print `66`. Registers 2 and 4 hold the minutes and hours, and `time` reads all of them the same way, waiting until the clock isn't in the middle of an update ([interrupts](interrupts.md)).

The selection in `0x70` is shared by everything that reads the clock. A read can occasionally get the register that was selected for it instead. Run the line again if a value looks wrong.

See [memory](memory.md) for the other half of talking to hardware, and [machine](machine.md) for the words that act on the whole machine.
