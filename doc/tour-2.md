# Part 2: The prompt and the stack

The `»` is the omega prompt. Each line you type there is a program, and it runs when you press Enter. omega is a stack language: values go on a stack, and a word takes its arguments from it and leaves its results there.

    2 3 +

leaves `5`. The prompt shows the stack in grey in front of the `»`.

    5 »

`.` prints the top value and removes it:

    .

Arguments come first and the word last, and evaluation goes strictly from left to right:

    2 3 + 4 *
    .
    2 3 4 * +
    .

`dup`, `drop`, `swap`, `over`, `rot` and `nip` rearrange the stack. `empty` clears all of it:

    1 2 3 swap
    over
    empty

`stats` leaves five numbers on the stack: the live objects, the garbage collections, the names, the stack depth and the history depth of this shell.

Press Tab after the first letters of a name to complete it.

[Next: Part 3](tour-3)   [Back: Part 1](tour-1)   [The tours](tour)
