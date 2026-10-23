# Part 4: Undoing mistakes

Each line is a transaction. If a word on it fails, the whole line is undone, and the stack and the definitions are as they were before it:

    1 2 3
    [ 7 ] :seven 4 5 0 /

The division fails, `seven` isn't defined, and the stack is still `1 2 3`. The error is printed in red under the line.

Lines that worked can be undone too:

    [ 1 + ] :inc2
    back

`back` undoes up to 64 lines, and `redo` undoes a `back`:

    redo

If something runs too long, Ctrl-C stops it and undoes the line.

Printed text, the screen and the machine are outside the transaction. A write with `poke` or `out` happens at once and stays, even in a line that fails afterwards.

See [transactions](transactions), [back](back) and [world](world).

[Next: Part 5](tour-5)   [Back: Part 3](tour-3)   [The tours](tour)
