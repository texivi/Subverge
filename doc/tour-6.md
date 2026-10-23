# Part 6: Tabs

There are four shells. The top line has a title for each of them, and a `*` after a title means that shell printed something while you were elsewhere. F1 to F4 and Alt-1 to Alt-4 show a shell, and a click on a title with the mouse does too. Each shell has its own stack, its own definitions and its own history.

Start something that runs for a while:

    0 100000000 [ 1 + ] times .

Press F2 to show shell 2 and type the line there, then press F1. Shell 2 goes on computing while you are in shell 1, and its title gets a `*` when it prints. Ctrl-C in a shell stops the line it is running.

See [tabs](tabs), [keys](keys) and [scheduler](scheduler).

[Next: Part 7](tour-7)   [Back: Part 5](tour-5)   [The tours](tour)
