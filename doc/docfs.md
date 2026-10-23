# Where the help pages live

The help pages aren't files. Each is a text file in the `doc/` directory of the source, and the build links them into the kernel, so the pager finds them in memory and reads nothing from a disk. The pages therefore match the build that is running and are there on the live ISO.

## Reading a page

`docs` opens the index of pages and `"name" doc` opens one by name, as in `"prompt" doc`. In the pager, Up, Down, PgUp, PgDn, Home and End scroll, and Space pages down. Tab selects the next link, Enter follows the selected link, Backspace goes back, and `q` or Esc leaves. Enter on a line of code closes the viewer and runs the line at the prompt.

With a PS/2 mouse, a click follows a link or runs a code line, the wheel scrolls, and a right click goes back. `mouse?` is 1 when a mouse was found, and `mouse` leaves the pointer cell and the buttons (1 left, 2 right, 4 middle).

## Changing a page

A page can't be changed in a running system. To change one, edit its file and build the kernel again. An installed disk doesn't hold the pages, since they aren't state: it holds root, the worlds of the shells and the theme, and the pages come from whatever kernel is booted ([saving](saving)).

## Generated pages

The index of words, the pages of the words and the pages of the groups are made when they are asked for, from the table that the `help` word uses ([help](help)).
