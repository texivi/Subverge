# FAQ

## General

### Is Subverge a Linux distribution?

No. The kernel, ordinal, was written for Subverge, and so were the bootloader, the shell, the editor and every program. Subverge is *built* on Linux, with gcc and nasm, but none of Linux ends up in the ISO apart from GRUB, which only boots the live system.

### Is it a TempleOS fork?

No. Subverge took a few ideas from TempleOS, mainly a shell that is also the compiler and running everything in ring 0, but none of its code. The language, the kernel and the way lines are undone are different. See [history](history.md).

### Why is it called Subverge?

The name comes from *subvert* and *diverge*. See [history](history.md).

### Can I use it every day?

No. It runs in QEMU, has no network and no protection between programs, and a bad `poke` can take the whole machine down. It is a system to explore and program in. Read the [expectations](expectations.md).

### Is there a roadmap?

Yes, see the [roadmap](roadmap.md).

## Running it

### Where are my files?

Subverge keeps what you write in three places. Your definitions are in your tab's [world](world.md), programs are in [root](root.md), and named values such as notes are in [phi](phi.md). An installed disk [saves](saving.md) all three.

### Why was my work lost after a reboot?

The live ISO doesn't save anything. Run `install` to keep your work. An installed system saves after every line that succeeds, so the most you can lose is the line you were typing.

### I booted a new ISO and sys still shows the old version

You booted with an installed disk attached. The live system loads the root and tabs saved on that disk, and the saved root still has the old `sys`. The kernel and the built-in words are new. Run `install` on the same drive to replace the kernel and keep your work.

## Help pages

### How do I read the help pages?

`docs` opens the index and `"name" doc` opens a page. Tab selects the next link, Enter follows it, Backspace goes back, and `q` or Esc leaves ([where the pages live](docfs)).

### How do I switch between the shells?

Press F1 to F4 or Alt-1 to Alt-4 for shell 1 to 4. With a mouse, click a tab title in the top bar ([tabs](tabs)).

### The mouse doesn't move.

QEMU gives the mouse to the guest after the first click in its window, and Ctrl-Alt-G takes it back. `mouse?` is 1 when the system found a mouse. Everything the mouse does has a key as well ([keys](keys)).

### How do I change the colours?

`themes` lists the themes and `'paper theme` switches to one. See [themes](themes).

## The system

### Is it POSIX-compliant?

No, and it never will be. POSIX describes processes, files, file descriptors and system calls, and Subverge is built from other parts. Programs are omega words, and memory, I/O ports and PCI devices are reached with words such as `peek`, `in` and `pci@`. See [omega for smarties](smarties.md).

### Does it have its own bootloader?

Installed disks use Subverge's own [bootloader](boot.md). The ISO uses GRUB.

### Why does everything run in ring 0?

Every word at the prompt can read and write the machine directly, the same way TempleOS works. Separating programs from the kernel would need user mode, system calls and paging per process, and Subverge leaves that layer out. [Transactions](transactions.md) undo most mistakes, and the ones they can't undo are writes to the hardware.

### Is it self-hosting?

No. Subverge has no C compiler or assembler of its own, so the kernel is built on Linux. The programs in root are omega source and can be edited and rebuilt inside Subverge ([root](root.md)).

### Is it multi-user?

No. The user name on the `sys` screen is a string, and `'user open` changes it.

### Can I change the colours?

Yes. `themes` lists the themes and `'haelide theme` switches to one. `n r g b rgb` changes a single colour, and `logos` lists the logos `sys` can draw. See [themes](themes.md) and [logos](logos.md).

### Why is the screen so small?

Subverge uses 80 by 25 VGA text mode, with the top row taken by the tab bar. `gfx` starts a 640 by 480 graphics mode that draws dots, boxes, text and titled frames. See [screen](screen.md).

### Does it have networking or sound?

Sound: yes, through the PC speaker. `beep` plays a tone with a pitch and a length ([ports](ports.md)). Anything beyond the speaker is on the [roadmap](roadmap.md).

Networking: not yet. It is on the [roadmap](roadmap.md).

## The language

### Why is the shell a programming language?

So that a command and a program are the same thing. A line you type at the prompt can be put in a quote, given a name and saved, and it becomes a program. See [omega](omega.md).

### Why is everything written backwards?

omega is a postfix language, so you write `2 3 +` instead of `2 + 3`. The arguments go on the stack first and the word that uses them comes last. It needs no brackets or precedence rules. See [stack](stack.md).

### Why are locals dynamically scoped?

A word can read the locals of the word that called it, which is sometimes useful and sometimes causes surprising bugs. See [names](names.md).

### How big can a number get?

Up to 2^65536, which is about 19,700 digits. A result past that fails with an error ([bignums](bignums.md)). The infinite values, such as `omega` and `aleph0`, are a different kind of value: they compare above every integer, but they are positions and sizes rather than very large integers ([ordinals](ordinals.md)).

### What is omega, the value?

`omega` pushes ω, the first position after all the natural numbers. It is the reason the shell is called omega. Watch Vsauce's [How To Count Past Infinity](https://www.youtube.com/watch?v=SrU9YDoXE88), then see [ordinals](ordinals.md).

### Why does omega 1 - fail?

Ordinals only support `+`, `*` and `^`. ω has no position directly before it, so ω−1 isn't an ordinal. See [ordinals](ordinals.md).

### Why did my whole line do nothing?

One of its words failed, so the whole line was undone. See [transactions](transactions.md) and [errors](errors.md).

### How do I undo a line?

Type `back`. See [back](back.md).

### How do I stop a program that won't stop?

Press Ctrl-C. It stops the code running in the current tab and undoes the line. If the tab doesn't respond, switch to another [tab](tabs.md) with F1 to F4 or Alt-1 to Alt-4. The tabs are scheduled separately, so the others keep working.

## The project

### What's the license?

[GPL v3](https://www.gnu.org/licenses/gpl-3.0.html).
