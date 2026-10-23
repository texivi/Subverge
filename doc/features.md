# Features

- A postfix language for a shell. Programs and commands are written in the same language, and the system's own programs are written in it too ([omega](omega)).
- A compiler in the shell. A quote is compiled to x86-64 machine code the second time it runs, and `xray` disassembles the result ([JIT](jit), [dis](dis)).
- Every line is a transaction. A failing word undoes the whole line, and `back` undoes up to 64 lines that worked ([transactions](transactions), [back](back)).
- Exact integers. A result that overflows 64 bits becomes a bignum, up to 2^65536, in interpreted and compiled code alike ([bignums](bignums)).
- Infinity as a value. `omega` and `aleph0` add, multiply and compare like other numbers ([ordinals](ordinals), [cardinals](cardinals), [infinity](infinity)).
- Ring 0 at all times. `peek`, `poke`, `in`, `out`, `cpuid` and `pci` reach memory, ports and the PCI bus ([smarties](smarties)).
- An 80 by 25 text screen, with an optional 640 by 480 graphics mode for drawing ([screen](screen)).
- A PS/2 mouse on the text screen, shown as an inverted cell. It switches tabs from the top bar and follows links in the viewer ([mouse](?mouse)).
- A help system built into the kernel: hyperlinked pages that `docs` and `"name" doc` open in a pager, lines you can select and run, and a page for every word ([help](help)).
- Four shells under a preemptive scheduler, each with its own stack and definitions, switched with F1 to F4 ([tabs](tabs), [scheduler](scheduler)).
- An editor, `scrawl`, for the programs in root. An edit only takes effect if it builds, so a broken edit can't break the running system ([scrawl](scrawl), [root](root)).
- Saved after every line. An installed disk keeps each shell's world and your edits to root as omega source ([saving](saving)).
- Its own boot loader for installed disks. GRUB is used only on the ISO ([boot](boot)).
- Colour themes, with the logo drawn by `sys` following them ([themes](themes), [logos](logos)).
- A PC speaker driver, with `beep` ([screen](screen)).
- A garbage collector for omega's objects ([gc](gc)).
- Programs: [tools](tools), [games](games) and [demos](demos). The [demo index](demo-index) lists them all.

For what the system leaves out and why, see [design](design) and [decisions](decisions). For what is planned, see the [roadmap](roadmap).
