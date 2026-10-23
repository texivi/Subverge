# Part 7: Editing a program

`'name edit` opens the editor, scrawl, on a word. Type the program and save with Ctrl-S, then quit with Ctrl-Q:

    'countdown edit

Type this:

    { n }
    n 0 > [
      n .
      30 sleep
      n 1 - countdown
    ] when

Then run it:

    5 countdown

The word calls itself, takes its argument as the local `n` and stops when `n` reaches 0.

The programs in root are edited the same way, with `open`. An edit takes effect only if it builds, so a mistake can't break the running system:

    'clock open

Leave the editor with Ctrl-Q. The old version stays in use if the edit didn't build.

See [scrawl](scrawl), [root](root) and [units](units).

[Next: Part 8](tour-8)   [Back: Part 6](tour-6)   [The tours](tour)
