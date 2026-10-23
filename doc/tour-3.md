# Part 3: Words

A quote is code in square brackets that isn't run yet. `:name` takes the quote on the stack and gives it a name:

    [ dup * ] :sq
    7 sq .

`sq` now works like any word. `see` prints the source of a word and `words` lists everything you have defined in this shell:

    'sq see
    words

The quote mark in `'sq` passes the name instead of running the word.

Words defined at the prompt belong to the shell. Press F2 to show shell 2. It is a separate session with its own stack, and a word you defined in shell 1 isn't there. Press F1 to come back.

The programs that come with the system are root words. They are visible in every shell. `root` lists the units they are defined in, and `'clock see` prints one:

    root
    'clock see

A word of your own that has the same name as a root word hides it in your shell only.

See [names](names), [quotes](quotes) and [stack](stack).

[Next: Part 4](tour-4)   [Back: Part 2](tour-2)   [The tours](tour)
