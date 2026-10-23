# Welcome to Subverge

Subverge is an x86-64 operating system for programming and experiments. Everything on it runs in ring 0, the shell is a programming language, and the programs that come with it are written in that language and can be changed while the system runs. The kernel is called **ordinal** and the shell is called **omega**.

It is written for people who already program. Nothing in these pages explains what a loop or a variable is, and `learn` assumes the same. If you know Forth, read [omega for Forth programmers](forth) first.

## The machine

Subverge runs in QEMU, and QEMU is the only machine it is tested on. It boots from an ISO into an 80 by 25 text screen with four tabs. A PS/2 mouse works there, and `gfx` starts a 640 by 480 graphics mode on QEMU's standard VGA display. See [screen](screen). The pages are read inside the system with `docs` ([help](help)).

A live ISO saves nothing. To keep your work, attach a blank disk image and run `install`. An installed disk doesn't hold files. It holds fixed slots for the root units, for each shell's saved world and for the boot code ([disk layout](disk-layout)), and it writes them after every line that succeeds ([saving](saving)).

## The language

At the prompt you type [omega](omega), a postfix language related to Forth. The shell is also the compiler: a line is parsed, run, and compiled to x86-64 machine code the second time it runs ([JIT](jit)). Programs and commands are written in the same language, so a command you use often becomes a word with `[ ... ] :name`.

    2 3 +
    [ dup * ] :sq
    7 sq

Every line is a transaction. When a word on the line fails, the whole line is undone, and [back](back) undoes up to 64 lines that worked ([transactions](transactions)). Raw writes to memory and I/O ports are not part of that and can't be taken back.

Integers don't wrap. A result that doesn't fit in 64 bits becomes a bignum, up to 2^65536 ([numbers](numbers), [bignums](bignums)). Infinity is a value too: `omega 1 +` is a valid line and gives `ω+1` ([ordinals](ordinals), [cardinals](cardinals)).

## Ring 0

`peek` and `poke` read and write any address, `in` and `out` talk to I/O ports, `cpuid` and `pci` read the CPU and the PCI bus, and `xray` disassembles the code the JIT made ([memory](memory), [ports](ports), [dis](dis)). A bad write can stop the whole virtual machine. That is part of what the system is for, and it is why the [expectations](expectations) page is the first thing to read.

## Four shells

Four shells run at the same time under a preemptive [scheduler](scheduler). Each has its own stack, its own definitions and its own screen, and they share only [root](root), the heap and the hardware. F1 to F4 switch between them ([tabs](tabs)).

## What to run first

The programs that come with Subverge cover most of what the system does. Type its name in a shell. In the page viewer, Enter on a line of code runs it at the prompt.

- [snake](>snake), [tiles](>tiles), [pong](>pong) and [life](>life)
- [mandel](>mandel), [stars](>stars), [rain](>rain) and [bounce](>bounce)
- [count](>count), [sorting](>sorting) and the other [demos of infinity](demos)
- [calc](>calc), [clock](>clock), [stopwatch](>stopwatch), [todo](>todo) and [convert](>convert)
- [top](>top), [irqmon](>irqmon) and [hexview](>0xB8000~hexview), which read the machine
- [learn](>learn), a guided tour of the language in 47 lessons

The [demo index](demo-index) lists every program with what it shows. [Tools](tools) and [games](games) describe them one by one.

## Reading on

Start with [first steps](first-steps), a short session at the prompt. The [tour](tour) types the same lines for you. [Key map](keys) lists every key, [command line](cmdline) explains the prompt and [tips](tips) collects things worth knowing. For the reasons behind the system, see [design](design) and [decisions](decisions). The [index of words](words) has a page for each word, and [help](help) explains how it is organised.

[Take the tour](tour)

Subverge is released under the GPL v3.
