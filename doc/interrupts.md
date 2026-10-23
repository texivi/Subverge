# Interrupts and the timer

Subverge uses four sources of interrupts: CPU exceptions, the timer, the keyboard and the mouse. Everything else is masked. The disks are driven by polling ([drives](drives)), so their interrupts are never used.

## How an interrupt is handled

Every interrupt goes through the same entry code, which saves the registers on the stack of the task that was running and calls one handler. The timer and the software interrupt `yield` can return the saved registers of a different task, and that is how the scheduler switches tasks ([scheduler](scheduler)). Every task runs in ring 0, so the processor never changes stacks on an interrupt, which is why each task has a stack of its own.

## The timer

The timer ticks 100 times a second. `hz` returns 100 and `ticks` returns the number of ticks since boot. Each tick counts the tick as idle when the processor was halted ([idle](idle)), redraws the clock of the text screen once a second, and calls the scheduler. `sleep` and `waitkey` are built on the tick counter: a task records the tick it wants to wake at and gives up the CPU until then.

## The keyboard and the mouse

The keyboard raises IRQ 1 and the handler turns the scancode into a key for the tab in front ([keyboard](keyboard)). The mouse raises IRQ 12, and the handler turns the bytes of each packet into a pointer position and a state of the buttons, which `mouse` returns ([ps2](ps2)). The timer, the keyboard, the cascade and the mouse are the unmasked lines.

## Exceptions

A CPU exception in a shell or a program doesn't stop the machine. The handler formats a message from the exception, including the faulting address for a page fault. A shell prints the message and `your last line was dropped`, and its [world](world) is unchanged because the line never committed. Names follow the Intel manuals: `divide by zero`, `invalid opcode`, `general protection fault`, `page fault at 0x...`. Division by zero in omega fails with an ordinary error before it reaches the processor, so an exception only appears for code that bypasses the checks, such as a bad `poke` into running code.

An exception while no task is running, in the idle loop or during boot, is a kernel panic. The screen turns red, shows the message and the machine halts.

## Counting interrupts

`0 irq` and `1 irq` return how many timer and keyboard interrupts have arrived since boot. `irqmon` shows both live with their rates per second.

    0 irq .              ticks so far, the same as ticks
    1 irq .              at least two per key, one for press and one for release

## The clock

The time comes from the real-time clock of the PC, not from the timer. `time` reads it and corrects for the formats the clock may use. QEMU's clock runs in UTC unless it is started with `-rtc base=localtime`.

## Turning interrupts off

Kernel code that touches state shared between tasks, such as the heap, the screen buffers or the disk controller, turns interrupts off while it works. A long disk transfer therefore delays the timer, which shows as a short pause in animations while a large world is being saved. The only omega word that turns interrupts off is `halt`, which also stops the machine for good.
