# Design

These are the rules the system is built to, and they are followed throughout. The exceptions are listed in [decisions](decisions).

## One language

The shell, the utilities, the games, the installer and the lessons are all omega. The kernel is the only other layer. A command typed at the prompt becomes a program by putting `[ ]` around it and a name after it, and a program becomes part of the system by moving it into [root](root).

## Ring 0

All code runs in ring 0 and nothing is off limits. Memory, I/O ports, control registers and every instruction are available to a line typed at the prompt. A call into the kernel is a call to a function, so system calls are not needed. A bad write can stop the machine. In return the prompt reaches everything the hardware does, with no layers in between.

Mistakes that stay inside omega, such as an error in a word or a wrong stack, are handled by the transaction rule below. Raw memory and port writes are the one place where the system leaves the consequences to the user.

## Every line is a transaction

A line either completes or has no effect. This is what makes ring 0 comfortable at a prompt: a typo, a division by zero or an interrupted loop leaves the stack and the definitions as they were. Undo is built on the same mechanism. The world, which is the stack and the definitions of a shell, is a value that the kernel keeps copies of, 64 deep ([world](world), [transactions](transactions)).

## No files

Subverge has no processes, users, permissions or file paths. Everything you write is an omega word, kept in a shell or in root, and an installed disk stores those words as omega source in fixed slots. Because the saved form is source, a saved world is something you can read, and loading one is running it.

## The system edits itself

Root is the part of the system that is written in omega, and it can be opened from the prompt with `'name open`. An edit is built before it is accepted, so an edit that doesn't build changes nothing, and one that does build is in use at once in all four shells. Nothing needs to be restarted ([units](units), [root build](root-build)).

## Exact numbers

An integer never wraps. Values that overflow 64 bits become bignums, and the compiled code does the same, so the interpreter and the JIT give the same answers. Infinite values are ordinary numbers with ordinary rules, which means the arithmetic of ordinals is part of the shell rather than a library.

## Small enough to know

The whole system, kernel and omega programs, is about ten thousand lines. The pages under [for smarties](smarties) explain each part of the kernel in the order it runs.

## Hardware kept simple

Subverge uses the PC hardware that every emulator provides: the PIT and PIC for time and interrupts, a PS/2 keyboard and mouse, IDE disks through legacy ports, and a VGA display with an optional Bochs graphics mode. Each kind of device has one driver. Planned additions, such as USB, AHCI and networking, are on the [roadmap](roadmap).
