# Part 10: The JIT

A quote is interpreted the first time it runs and compiled to x86-64 machine code the second time. `xray` shows the code:

    [ dup * ] :sq
    'sq xray

Each item of the quote became inline code or a call back into the interpreter. `bench` runs a quote with the JIT off and then on and prints both times:

    [ -32768 0 2816 40 mandel-at ] bench

`jit` says whether it is on, and `jit-off` makes everything run in the interpreter. The results are the same either way:

    jit
    jit-off
    jit-on

`mandel` zooms into the Mandelbrot set, and its inner loop is almost nothing but arithmetic:

[mandel](>mandel)

See [JIT](jit) and [dis](dis).

[Next: Part 11](tour-11)   [Back: Part 9](tour-9)   [The tours](tour)
