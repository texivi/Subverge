# omega for smarties

These pages are for people who already write omega and want to know how it works underneath, or want the words that read and write the machine directly. They assume you know what ring 0 and a page table are. If you can't write a loop in omega yet, go through [first steps](first-steps.md) and `learn` first.

## The language

- [The language in detail](language.md)
- [Values and objects](values.md)
- [The interpreter](interpreter.md)
- [How worlds work](worlds.md)
- [Bignums](bignums.md)
- [How infinity works](infinity.md)
- [Limits](limits.md)
- [Index of words](words)
- [Reading the pages](help)

## The machine

These words act on the real machine as soon as they run. Transactions and `back` can't undo them, and nothing checks what you pass them.

- [Memory](memory.md)
- [Ports, the CPU and PCI](ports.md)
- [The screen](screen.md)
- [The keyboard](keyboard.md)
- [Drives](drives.md)
- [Drives, rebooting and installing](machine.md)
- [The JIT](jit.md) and [the disassembler](dis.md)

## The system

- [Internals](internals.md)
- [Interrupts and the timer](interrupts.md)
- [How root builds](root-build.md)
- [Booting](boot.md)
- [Disk layout](disk-layout.md)
- [The scheduler](scheduler.md)
- [The garbage collector](gc.md)
