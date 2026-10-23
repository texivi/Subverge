# How worlds work

A world is a tab's stack and its definitions, which is what a [transaction](transactions.md) protects.

## What a world holds

A world holds the stack as a quote that never changes, and a list of definitions. Defining a word with `:name` doesn't change any existing definition. It adds a new one to the front of the list, so older worlds still see the list as it was. Each tab keeps its current world and the 64 before it.

## Running a line

Before a line runs, the stack of the current world is copied, and the line works on that copy. The whole line is parsed first, so a parse error means nothing ran at all. When a word fails, the line stops there.

When the line ends, one of four things happens:

1. If something failed, the copy is thrown away and the error is printed. The world never changed.
2. If the line used `back`, the world from that many lines ago becomes the current one, and everything else the line did is thrown away.
3. If the stack and the definitions are the same as before, nothing is recorded. This is why `back` skips lines like `words`.
4. Otherwise the copy becomes the new world, and the old one goes into the history. The oldest of the 64 is dropped.

On an installed disk, the tab is [saved](saving.md) right after 2 and 4, before the prompt comes back.

## Fast undo

Nothing in a world is ever changed in place, so a new world only costs a copy of the stack. The definitions are shared with the worlds before it. Old worlds keep everything they point to alive until they drop out of the history, which is how the [garbage collector](gc.md) knows not to free it.

## What isn't in a world

Memory, ports, the screen, `alloc` and [root](root.md). They are never rolled back.

`try` stops a failure from ending the line, but it doesn't roll anything back either, so whatever its quote did before failing stays. See [catching](catching.md) for examples.

A CPU fault restarts the tab before the line finishes, so the world is left as it was. See [scheduler](scheduler.md).
