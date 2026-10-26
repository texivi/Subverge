# Part 18: phi

phi holds named values in sets ([phi](phi)). A set holds members, and each member is a value or another set. A value is a string.

    "buy milk" "/notes/todo" phi-write
    "/notes/todo" phi-read .
    "/notes" members
    phi

`phi-write` makes the set `notes` by itself. `members` lists a set, sets first, and `phi` shows everything you made.

## Changing it

    "/notes/todo" "/notes/done" phi-move
    "/notes/done" phi? .
    "/notes/done" phi-delete

`phi?` says what a path is: 0 for nothing, 1 for a set and 2 for a value. `phi-set` makes an empty set and `phi-delete` removes a value or a set with everything in it.

## What comes with the system

`/doc` has the help pages and `/pictures` the built-in pictures. You can read them, and list them, and nothing in them can change:

    "/doc/keys" phi-read len .
    "/pictures" members

## dired

`dired` browses phi one set at a time. n and p move, RET opens a set or edits a value, ^ goes up, + makes a set, c makes a value, R renames, d deletes and q leaves:

    dired

`phi-edit` opens one value in the editor.

phi is not part of the world of a tab, so `back` does not undo what you wrote. On an installed disk it is written after every change and is there after a reboot ([saving](saving)).

[Next: Part 19](tour-19)   [Back: Part 17](tour-17)   [The tours](tour)
