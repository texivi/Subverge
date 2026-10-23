# Part 1: The help system

The help pages are stored in the kernel and are read with `docs`, which opens the index, or with `"name" doc`, which opens one page. Pages are linked to each other and to the words of the language.

Tab selects the next link and Shift-Tab the previous one. Enter follows the selected link. Backspace goes back to the page you came from, and `q` or Escape closes the viewer. Up, Down, Page Up, Page Down, Home, End and Space scroll. A mouse works too: a click follows a link, the wheel scrolls, and the right button goes back ([mouse](mouse)).

A line of code shown in a different colour can be run. Select this one and press Enter:

    2 3 +

The viewer closes, the line is typed at the prompt and runs. The result is on the stack, and the prompt shows it in grey before the `»`.

There is a page for every word. Select one to see its stack effect, what it does and a line to try:

[dup](?dup)   [swap](?swap)   [map](?map)   [theme](?theme)

The [index of words](word-index) lists the words by group, and [A to Z](a-to-z) lists them by letter. The page for each group is a link from the word page.

The title bar shows the name of the page, and the right end of the bottom line shows how far down the page you are.

[Next: Part 2](tour-2)   [The tours](tour)
