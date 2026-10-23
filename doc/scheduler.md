# Scheduler

The four [tabs](tabs.md) share one CPU. The scheduler switches to the next task a hundred times a second.

## Tasks

There are five tasks: the four tabs and the idle task. Each tab has its own 256 KiB stack, from the heap, and runs `omega_main`, which is the prompt loop: read a line, run it, repeat. The idle task is `kmain` itself, which ends in `hlt` in a loop.

The four tabs match the four tab slots in the [disk layout](disk-layout.md).

## Switching

The PIT fires IRQ 0 a hundred times a second. Every interrupt, timer or not, goes through one common handler that pushes all the registers onto the current task's stack. For the timer, the handler calls `schedule` with a pointer to those saved registers, which:

1. stores that pointer as the current task's saved `rsp`
2. looks at the tabs in order, starting after the current one, for the first one that's runnable
3. returns that task's saved `rsp`, or the idle task's if none are

and the handler then switches to whatever stack it was given and pops the registers off it. The return from the interrupt *is* the context switch. The registers stay on that stack, and no FPU state is saved, because the kernel is compiled to never touch SSE.

## Runnable

A tab is runnable unless it's waiting. It can be waiting for a key, in which case it's runnable once its key queue isn't empty, or sleeping until a tick, in which case it's runnable once that tick has passed. A tab doing neither is always runnable, so a tab in a tight loop gets its full share, and the others still get theirs.

Only the tab in front receives keys. Keys typed while you're on tab 1 go into tab 1's queue and nowhere else ([keyboard](keyboard.md)).

## Yielding

A task that has nothing to do right now, because it's waiting for a key or sleeping, gives up the CPU early with `int 48`, which goes through the same handler and the same `schedule` as the timer does. A tab waiting at the prompt therefore uses no CPU time.

## Faults

If a tab's code causes a CPU exception, the fault handler doesn't panic. It records the message on the tab, clears whatever it was waiting for, builds a brand new stack frame that starts `omega_main` from the top, and returns that. The tab's world isn't touched, because a line's effects only reach the world when the line finishes, which it didn't. The new `omega_main` sees the recorded fault, prints it, prints `your last line was dropped`, and goes back to the prompt. See [transactions](transactions.md).

A fault in the idle task, or in the kernel when no task is running, is a real panic, and the machine stops.

## Sharing

All tasks share one heap, one screen, one keyboard controller and one disk. The allocator and the disk code disable interrupts around the parts that can't be interrupted. Everything else per tab is separate: the omega state, the stack, the screen buffer, the key queue and the prompt history.

The [JIT](jit.md) needs extra care, since compiled code can still be running in one tab when another tab invalidates it. See [JIT](jit.md).
