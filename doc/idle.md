# The idle loop

When no task can run the processor has nothing to do, and the kernel runs the idle task, which stops the processor until the next interrupt. The next interrupt is at most ten milliseconds away, so the idle task uses almost no power and, in an emulator, almost no CPU of the host.

A task that waits for a key or a timer can't run, so a system with every shell at its prompt spends nearly all its time idle. The idle task runs whenever no task is ready ([scheduler](scheduler)).

## Measuring it

The timer counts the ticks in which the processor was idle, and `idle` gives the count:

    idle .

`top` divides the change since its last look by the change in all ticks to show a percentage. The figure is coarse, since it can't see what happened inside a tick. It is good enough to tell a system that waits from one that works.

A shell in a tight loop makes the figure rise at once, and a shell at its prompt makes no difference. See [interrupts](interrupts) for the timer and [tools](tools) for `top`.
