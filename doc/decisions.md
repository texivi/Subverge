# Decisions

Each of these is a choice the system makes and what follows from it. They go with the rules on the [design](design) page.

## omega is a stack language

A line is split on spaces and brackets and evaluated from left to right, so precedence never comes up and a word's arguments are the values before it. Programs, commands and saved state share one syntax, which means the saved form of a [world](world) is its own source ([saving](saving)). `calc` is the one place that parses infix, for arithmetic you want to type as usual.

## Everything runs in ring 0

The prompt reaches all memory, every I/O port and every disk, and a call into the kernel is a function call. A line can do anything the hardware allows, and so can a mistake: a bad `poke` or `out` is outside the transaction and can stop the machine ([smarties](smarties), [expectations](expectations)).

## Every line is a transaction

A line works on a copy of the world. If every word succeeds the copy becomes the new world and the old one goes into the history, and if a word fails the copy is thrown away. The same history gives `back` and `redo`, 64 worlds deep ([transactions](transactions), [back](back)). Printed text, the screen, the machine and root edits that already built are outside it.

## Named values live in phi

An installed drive has no partitions and no allocation table. The kernel, root, each shell's world and phi are written at fixed sectors, as text, so an image can be read with a hex editor ([disk layout](disk-layout)). Named values that are not definitions are kept in phi, a tree of sets in memory that is written to its own slot after every change ([phi](phi)).

## The JIT compiles on the second run

The first run of a quote is interpreted, and the second compiles it to x86-64. Compiled code and the interpreter give the same results, including the switch to bignums on overflow, so `jit-off` can be used to check one against the other ([JIT](jit)).

## Root is rebuilt from source

The programs in root are omega source that is built at boot and again after every edit. An edit that fails to build leaves the old root in place. A new root becomes visible to all four shells at once ([root build](root-build)).

## Four shells

The kernel runs exactly four shells at the same time under a timer-driven scheduler. Each has its own world, history and key queue and a 256 KiB kernel stack. They share root, the heap and the hardware ([tabs](tabs), [scheduler](scheduler)).

## The help is part of the kernel

The pages are text in the kernel image, found by name in a table built together with it. They match the build that is running, and they are available on the live ISO with no disk attached. `docs` and `"name" doc` open them in a pager that draws on the text screen ([help](help)).

## The machine is QEMU

Every driver is written for the hardware QEMU provides: the PIT and PICs, a PS/2 keyboard and mouse, IDE disks through the legacy ports, and a VGA card with an optional Bochs graphics mode.

## What is not there yet

UEFI booting, USB, AHCI, networking, sound beyond the PC speaker, floating point numbers and more than one CPU core are on the [roadmap](roadmap).
