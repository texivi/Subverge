# JIT

A quote is compiled to x86-64 the second time it runs, and the machine code runs from then on. See [dis](dis.md) for reading the output.

## Looking at it

    jit                 is it on, and what has it compiled
    jit-off  jit-on     turn it off and on
    'fib compile        compile now, leaves address and length
    'fib addr           the address of its code
    addr n dis          disassemble n instructions at addr
    addr len disr       disassemble a range of bytes
    'fib jitdis         compile and disassemble all of it
    'fib xray           source, then machine code
    [ ... ] bench       time a quote with the JIT off, then on

`xray` is the most useful of these. It shows the word's source and then every instruction the JIT made for it, so you can see which words were inlined and which became calls.


`bench` runs a quote with the JIT off and then on and shows both times. On a recursive `fib`, it's about 80 ticks interpreted and 4 compiled, twenty times faster.


## What it generates

The JIT makes call-threaded x86-64. Each item in a quote becomes either inline code or a call back into the interpreter. Simple words such as `+ - * & | xor`, the comparisons and `dup over drop swap nip not neg` are inlined, each with a check that falls back to the real built-in when the values aren't plain integers. Arithmetic that overflows also falls back, which is how bignums keep working in compiled code.

`fib` goes from about 80 ticks to 4, since it is all comparisons, arithmetic and an `if` with literal quotes. A loop that spends its time in `say` speeds up much less, since `say` is still `say`. Compiled loops still check for Ctrl-C and let the [garbage collector](gc.md) run.

## When it compiles

A quote counts how many times it's been run. On the second run it's compiled, and the code is attached to the quote. Quotes that can't be compiled are marked so it doesn't try again, and `compile` on one fails with `foo can't be compiled`.

## Invalidation

Compiled code assumes that `+` still means `+`. When a definition shadows a built-in or [root](root.md) is rebuilt, older compiled code is no longer run and the quote is compiled again the next time it is needed. Old code is freed only when no task is inside compiled code, since another [tab](tabs.md) might be running it.

A root rebuild retires the code of every unit it replaced. A quote freed by the collector retires its code the same way.

## Turning it off

`jit-off` makes everything run in the interpreter. Results are identical either way, so any difference in behaviour is a JIT bug. Keep the `xray` output of the word.

## mandel

`mandel` is the best real program to watch it on, because its inner loop, `mb-iter`, is almost nothing but arithmetic. Try `'mb-iter xray` and `[ -32768 0 2816 40 mandel-at ] bench`.
