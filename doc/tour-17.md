# Part 17: Windows

The screen can show up to four tabs at once, each in a window. Super-Enter opens a window, Super-W closes the one that has the keyboard and Super with an arrow key moves to the window in that direction ([tabs](tabs)).

The same things are on the C-x keys: C-x 3 opens a window beside this one and C-x 2 below it, C-x 1 keeps only this one, C-x 0 closes it and C-x o moves to the next. After C-x the top bar shows `C-x -` and waits for the next key.

## Where a window goes

A new window splits the one that has the keyboard in half and gets the keyboard. The split goes side by side when the window is wider than it is tall as drawn, and stacked otherwise. So the second window is beside the first, the third is below the second and the fourth is beside the third.

Each window has a line at its bottom with the number and title of its tab. The window with the keyboard has it highlighted. Tabs that are not in a window keep running.

## The size of a window

A tab has the size of its window, and `size` says what it is:

    size .

It leaves the width and then the height. The prompt wraps at the width, and the page viewer, the editor and `dired` fit themselves to the size. Programs that draw at fixed places, such as the games, assume 80 by 24 and show only the part that fits, so give them the whole screen with C-x 1.

F1 to F4 show a tab in the window that has the keyboard. A click with the mouse moves to the window under the pointer.

[Next: Part 18](tour-18)   [Back: Part 16](tour-16)   [The tours](tour)
