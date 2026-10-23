# Debugging Overview

Everything runs in ring 0 and the tools for finding out what went wrong are words at the prompt. This page lists them in the order they are usually needed.

## The line failed

A line that fails prints its error in red under the line and is undone. The error says which word failed and why, and [errors](errors) lists the messages. The stack is as it was before the line, so the line can be run again after a change.

    1 2 3
    4 5 0 /

If an error message is not enough, `'name see` prints the source of the word that failed, and `'name help` prints its page ([help](help)).

## The line did nothing

A line that fails is undone as a whole, including the parts that ran before the failure. If a line seems to do nothing, check for a red error above it. `back` shows nothing was recorded, since a line that failed has no entry in the history ([back](back)).

## The stack is not what you expected

The prompt shows the stack in grey before the `»`. `depth` counts it, `.` prints and removes the top value, and `'dup help` shows the stack effect of any word. A word written as `a b -- c` takes two values and leaves one ([help](help)).

A value that looks right and isn't often has the wrong type. `type` leaves the name of a value's type.

## The program loops

Ctrl-C stops the line and undoes it. A line that doesn't respond to Ctrl-C is one that is inside a single long built-in word, and the other shells are not affected. Switch to one with F1 to F4 or Alt-1 to Alt-4, and use `reboot` if the stuck shell has to go ([tabs](tabs)).

## What the machine is doing

`top` shows the CPU model, the idle percentage, the heap, the state of each shell and the interrupts, and updates twice a second. `irqmon` counts interrupts as they arrive.

| Word | Shows |
|---|---|
| `top` | the machine, live |
| `irqmon` | the interrupts, live |
| `sys` | the versions and the memory |
| `cpuinfo` | what the CPU reports |
| `memls` | the memory map from the firmware |
| `pcils` | the PCI devices |
| `disks` | the drives |

## The compiled code

A quote is compiled on its second run. If a word gives a different answer the second time, compare it with the interpreter: `jit-off` makes everything run in the interpreter, `jit-on` turns the compiler back on, and the results must be identical. `'name xray` prints the machine code of a word, and `[ ... ] bench` times a quote both ways ([JIT](jit), [dis](dis)).

## Memory

`hexview` is an editor for live memory, and `peek` and `poke` read and write a byte. `alloc` and `free` take and give back memory, and a program that forgets to free leaks it. The words that allocate are described on the [memory](memory) page. A write to memory or to a port can't be undone and is not part of the transaction ([transactions](transactions)).

## A fault

A fault in one shell, such as an access to an address that isn't mapped, stops that shell's line and no other. The shell prints the fault and `your last line was dropped`, and starts again at its prompt with the world it had before the line ([tabs](tabs)). A fault that happens while no task is running, in the idle loop, is a panic: the screen turns red, prints the message and the system halts.

See [troubleshooting](troubleshooting) for common problems and [limits](limits) for what the system can hold.
