# dis

Subverge has an x86-64 disassembler in the kernel. It is what `xray` and `jitdis` use to show the code the [JIT](jit.md) produced, and `dis` and `disr` point it at any address you like.

## Using it

    addr n dis          n instructions starting at addr
    addr len disr       every instruction in len bytes from addr
    'fib jitdis         compile fib and disassemble all of it
    'fib xray           fib's source, then its disassembly

`dis` and `disr` take any address, not only compiled code. `0x100000 20 dis` disassembles the first 20 instructions of the kernel itself. The disassembler can't tell code from data, so an address in the middle of a string or a table decodes into instructions that don't mean anything.

`xray` and `jitdis` compile the word first if it hasn't been compiled yet, so you don't need to run it twice beforehand.

## Reading the output

Each line shows one instruction: its address, its bytes in hex, and the instruction in Intel syntax with the destination first. Bytes that don't fit in the column end in `+`. Relative jumps and calls are printed with their absolute target, so you can follow a jump with another `dis` at that address.

Every compiled quote starts by saving registers and finding the omega state of the shell or program that runs it, then comes the code for each word, and it ends by writing the state back and returning. Try `'sq xray` to see it for `[ dup * ]`.

## What it decodes

It decodes the general-purpose integer instructions: moves, arithmetic, logic, shifts and rotates, `lea`, `imul`, `idiv`, conditional jumps, `set`cc and `cmov`cc, `call`, `ret`, `push` and `pop`, and the string instructions. It also knows the system instructions the kernel uses, such as `lgdt`, `lidt`, `invlpg`, `rdmsr`, `wrmsr`, `cpuid`, `rdtsc`, `cli`, `sti`, `hlt` and `iretq`. That covers everything the JIT generates and everything in the kernel, which is compiled without SSE.

SSE, AVX and x87 instructions aren't decoded. Opcodes it doesn't know are printed as `?`.

## Looking at the kernel

The kernel is loaded at 1 MiB, and `kernel` gives the size of the image. The first 16 bytes at `0x100000` are the Multiboot header, which is data. `_start` follows at `0x100010`, but it starts out as 32-bit code, since GRUB and QEMU enter the kernel in 32-bit protected mode ([boot](boot.md)). The disassembler always decodes 64-bit code, so the first part of `_start` comes out wrong. Everything after the switch to long mode, which is almost all of the kernel, decodes correctly:

    0x100010 40 dis

See [memory](memory.md) for the other fixed addresses.
