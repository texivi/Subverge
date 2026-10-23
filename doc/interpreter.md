# The interpreter

## Reading the line

The [prompt](prompt.md) collects up to 200 printable characters into a buffer. Only bytes from space to `~` can be typed, which is why `\w` and `\A` exist as string escapes for the omega and aleph glyphs. Tab completes names against the tab's definitions, root and the built-ins. When Enter is pressed the line is added to the tab's prompt history, unless it is empty or the same as the previous line, and handed to the evaluator.

## Starting the transaction

Before anything runs, the evaluator copies the committed stack of the tab's [world](world.md) into the working stack and points the working dictionary at the committed one. It then clears the error flag, the `back` counter, the call depth, the local variables, the trace flag and the keep stack. From here until the end of the line, everything happens on the working copies.

## Parsing

The whole line is parsed into one quote before any of it runs. A syntax error such as an unclosed bracket therefore fails the line before the first word executes, which is why `1 2 + [` prints nothing and pushes nothing.

The parser reads tokens separated by spaces, tabs and newlines. `[`, `]`, `{`, `}` and `"` end a token even without a space, so `[1 2]` parses as `[ 1 2 ]`. A `#` where a token could start begins a comment that runs to the end of the line.

Each token becomes one cell:

- `[` starts a nested quote. Nesting deeper than 32 levels fails with `the brackets are nested too deep`, and a single quote can hold at most 256 items.
- `{` reads names up to the matching `}` and becomes a local list. Up to 32 names fit in one list.
- `"` reads a string up to the next unescaped `"`. The escapes are `\n`, `\t`, `\e` (escape), `\w` (the omega glyph) and `\A` (the aleph glyph). A backslash before any other character produces that character.
- A token that reads as a number in decimal, `0x` hex or `0b` binary becomes an integer. If it is valid but doesn't fit in 64 bits, it is parsed again as a [bignum](bignums.md).
- A token starting with `'` becomes a symbol, one starting with `:` a definition, and one starting with `->` a local assignment. These prefixes only count when something follows them, so `:` and `'` on their own are ordinary words.
- Anything else becomes a word.

Names are interned as they are read ([values and objects](values.md)), so parsing a line that mentions `dup` twice creates no new objects for the second `dup`.

## Running a quote

Running a quote first gives the [JIT](jit.md) a chance. If the quote already has current machine code, that code runs and the interpreter is done with it. If this is the quote's second run, it is compiled first. Otherwise the quote is interpreted.

The interpreter keeps a call depth. Each quote it enters adds one, and going past 400 fails with `this nests too deep`. This is what stops unbounded recursion long before the 256 KiB tab stack runs out.

For each cell, in order, the interpreter:

1. checks whether Ctrl-C was pressed in this tab, and fails with `interrupted` if so;
2. gives the [garbage collector](gc.md) a chance to run;
3. if `trace` is active, prints the stack and the cell and waits for a key;
4. executes the cell.

It stops at the first error. Errors are a flag and a message on the tab, so after a failure every word that is still on its way out sees the flag and returns without doing anything.

## Executing a cell

What happens depends on the cell's tag:

- **A word** is resolved and run, as described below.
- **A definition** (`:name`) pops the top value and adds an entry binding `name` to it at the front of the working dictionary.
- **A local assignment** (`->name`) pops the top value. If a local called `name` is already live, its value is replaced. Otherwise a new local is created.
- **A local list** (`{ a b c }`) pops as many values as it has names and binds them in order, so the last name gets the top of the stack.
- **Anything else** (numbers, strings, quotes, symbols) is pushed.

A quote cell is pushed, not run. Only words run things.

## Resolving a word

A word is looked up in this order, and the first match wins:

1. **Locals**, newest first. A local's value is pushed, never run, even if it is a quote.
2. **The tab's definitions**, newest first. If the value is a quote it is run, otherwise it is pushed.
3. **Root's definitions**, newest first, with the same rule.
4. **The built-in** stored on the interned name, if any.

If nothing matches, the line fails with `foo isn't defined`.

This order is why a tab can override a root word without changing root, why `forget` uncovers whatever was underneath, and why a local called `x` hides a definition called `x` inside the quote that bound it. See [names](names.md) for the user-facing consequences, including dynamic scoping.

## Locals and their lifetime

Locals live in a per-tab table of 256 entries, separate from the stack. When a quote finishes, the interpreter cuts the table back to the size it had when the quote started, so every local bound inside the quote disappears with it. A word called from inside the quote can still see those locals while it runs, because lookup searches the whole live table. This is dynamic scoping, and it is a direct consequence of keeping locals in one table rather than in per-quote frames.

## Committing

When the line has finished, three things can happen.

**The line failed.** Any keys still queued in the tab are thrown away, the error is printed in red, and the working copies are abandoned. The committed world was never touched, so nothing needs to be undone.

**The line called `back`.** The working copies are abandoned, and the committed world is replaced by the one from as many lines ago as `back` was called. Anything else the line did to the stack or dictionary is ignored.

**The line succeeded.** If the working dictionary is the same entry as before and the stack holds the same cells, nothing changed, and the line leaves no trace in the history. Otherwise the current world is pushed onto the 64-entry history, a new world is made from the working stack and dictionary, and on an installed disk the tab is [saved](saving.md).

The comparison of stacks is a byte comparison of cells, so pushing a new string that happens to equal the old one still counts as a change.

## Back at the prompt

Before printing the prompt, the evaluator runs a full garbage collection, then prints the stack in grey (shortened from the left to its last 38 characters if it is long) and the `»` sign.
