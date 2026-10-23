# Internals

The sections below follow Subverge from power-on to the prompt, and then cover how the parts fit together while it runs.

## The pieces

| Piece | What it is | More |
|---|---|---|
| MBR and stage 2 | the bootloader for installed disks | [boot](boot.md) |
| `kmain` | brings everything up in order | below |
| interrupts | IDT, the two PICs, the PIT at 100 Hz | [scheduler](scheduler.md) |
| scheduler | four tasks, one per tab, preemptive | [scheduler](scheduler.md) |
| heap | one first-fit allocator for everything | [memory](memory.md) |
| VGA | text mode, a buffer per tab, the font and palette, and graphics mode | [screen](screen.md) |
| keyboard | PS/2, a key queue per task | [keyboard](keyboard.md) |
| mouse | PS/2, on IRQ 12, drawn as an inverted cell | [ps2](ps2.md) |
| ATA | PIO, polled, four drives at most | [disk layout](disk-layout.md) |
| persistence | the state header and the five slots | [saving](saving.md) |
| omega | the interpreter, worlds, transactions, GC | [gc](gc.md) |
| ordinals | Cantor normal form and cardinal arithmetic | [ordinals](ordinals.md) |
| root | the units, built at boot and rebuilt on edit | [root](root.md) |
| JIT | quotes to x86-64, plus the disassembler | [JIT](jit.md), [dis](dis.md) |
| help | the table every word must be in | [help](help.md) |
| scrawl | the editor | [scrawl](scrawl.md) |

## Boot, in order

1. The ISO's GRUB, or an installed disk's own [bootloader](boot.md), loads the kernel at 1 MiB and jumps to it in 32-bit protected mode with Multiboot information.
2. `boot.asm` builds an identity map of the first 4 GiB from 2 MiB pages, turns on PAE and long mode, loads a 64-bit GDT and calls `kmain`.
3. `kmain` first copies the kernel's `.data` section aside, before anything's had a chance to change it, so that `install` can later write out a clean kernel.
4. VGA, the font and palette, and the CPU tables come up: the IDT, the remapped PICs.
5. The Multiboot info is read for the memory size, the memory map and the loader's name.
6. The heap is set up after the kernel, up to 64 MiB.
7. The scheduler, then persistence, which looks for an installed drive and reads its state header.
8. Every built-in word is registered, ordinals are initialised, and root is built from the units linked into the kernel, or from the root slot if there is one and it builds.
9. Four tasks are started, one per tab, the PIT is set to 100 Hz, the keyboard and the mouse are turned on, and interrupts are enabled.
10. `kmain` becomes the idle task, running `hlt` in a loop.

When each tab's task first runs, it loads its saved world from its slot if there is one, tab 1 runs `startup`, and all four wait at a prompt.

## Design

Most of the design follows from two decisions. Everything runs in ring 0 in a single address space, so the kernel and omega work on the same data structures directly. And nothing in an omega world is modified in place, which is what makes [transactions](transactions.md), [back](back.md) and [saving](saving.md) fast. 
