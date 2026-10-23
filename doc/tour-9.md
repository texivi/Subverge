# Part 9: Looking at the machine

Everything runs in ring 0, so the prompt reaches the machine directly. These lines read it and change nothing:

    sys
    cpuinfo
    memls
    pcils
    disks

`top` shows the CPU, the heap, the shells and the interrupts live, and `irqmon` counts interrupts as they arrive. Leave either with `q`:

[top](>top)   [irqmon](>irqmon)

`peek` and `poke` read and write memory. `0xB8000` is the memory of the text screen, where a poke shows up on the screen:

    0xB8000 peek .

`hexview` is a hex editor on live memory. Typing two hex digits writes a byte at the cursor and can't be undone:

[hexview at the text screen](>0xB8000~hexview)

`in` and `out` talk to I/O ports, and `cpuid` asks the CPU what it is. The pages under [for smarties](smarties) explain each one.

> **Warning:** Writes to memory and ports are outside the transaction. A bad one can stop the machine.

[Next: Part 10](tour-10)   [Back: Part 8](tour-8)   [The tours](tour)
