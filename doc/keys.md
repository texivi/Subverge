# Key Map

## Everywhere

| Key | Action |
|---|---|
| F1 to F4, Alt-1 to Alt-4 | show tab 1, 2, 3 or 4 |
| Ctrl-C | stop the line the tab is running |

The keys are handled in the keyboard interrupt, so they work while every tab is busy.

## In the pages

`docs` opens the index and `"name" doc` opens a page.

| Key | Action |
|---|---|
| Tab | the next link |
| Enter | follow the link, or run the selected code line |
| Space, Page Down | scroll a screen |
| Page Up | scroll a screen back |
| Up, Down | scroll a line |
| Home, End | the top or the bottom |
| Backspace | back to the last page |
| q or Esc | leave the viewer |

Enter on a code line closes the viewer and runs the line at the prompt.

## At the prompt

| Key | Action |
|---|---|
| Enter | run the line |
| Left, Right, Home, End | move the cursor |
| Backspace, Delete | delete a character |
| Up, Down | earlier lines |
| Tab | complete the name before the cursor |
| Ctrl-C | throw the line away |

See [prompt](prompt) for the details.

## In programs

`q`, Escape and Ctrl-C leave most programs. The arrow keys, Space and the letters shown on screen do the rest, and [games](games) lists each program's keys. [scrawl](scrawl) has its own keys.

## The mouse

A PS/2 mouse works on the text screen. The pointer is an inverted cell. A click on a tab title in the top bar switches to that tab. In the page viewer a click follows a link or runs a code line, the wheel scrolls and a right click goes back. `mouse` returns the pointer cell and the buttons, and `mouse?` is 1 when a mouse was found ([ps2](ps2)).

## Key codes in programs

The codes that `key` returns are listed on the [keyboard](keyboard) page.
