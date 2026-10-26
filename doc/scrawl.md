# scrawl

scrawl is the text editor in Subverge. It is used for anything longer than one line: definitions, [root units](units.md) and new programs. It has no modes, so typing always inserts text, and the commands are Ctrl and Alt keys, the same ones as at the prompt ([keys](keys.md) explains how keys are written).

## Opening it

scrawl is never started directly. It is opened by one of three words, which decide what happens to the text when you quit:

    'sq edit            a definition in this tab
    'top open           the root unit that defines top
    "mine" unit         a new, empty root unit

`edit` works on your tab's [world](world.md), and `open` and `unit` work on [root](root.md). 

While scrawl is open, the tab's title becomes `edit` followed by the name, and the tab is otherwise busy. The other tabs keep running, and each tab can have its own scrawl open.

## The screen

The top row shows `scrawl`, the name being edited, `[+]` when there are unsaved changes, and the cursor's line and column on the right. Each line of text has its line number in the gutter on the left, and rows past the end of the text show `~`. The bottom row lists the main keys, and is replaced by messages such as `saved, 412 bytes`, `not found: foo` or `mark set`.

Lines longer than the screen scroll sideways as the cursor moves along them.

## Keys

| Key | Action |
|---|---|
| C-f, C-b, Left, Right | move by character, wrapping at line ends |
| C-n, C-p, Down, Up | move by line |
| C-a, C-e, Home, End | start or end of the line |
| M-f, M-b | forward or back one word |
| M-<, M-> | start or end of the text |
| C-v, M-v, Page Down, Page Up | one screen down or up |
| C-l | puts the cursor line in the middle of the screen |
| RET | splits the line, keeping the current line's indentation |
| C-o | opens a blank line after the cursor |
| DEL | deletes before the cursor, joining lines at the start of one |
| C-d, Delete | deletes under the cursor, joining lines at the end of one |
| TAB | inserts spaces up to the next multiple of 4 |
| C-t | swaps the two characters at the cursor |
| C-k | kills to the end of the line, or the line break when the line is empty |
| M-d, M-DEL | kills the next word or the word before |
| C-SPC | sets the mark |
| C-w, M-w | kills or copies from the mark to the cursor |
| C-y | yanks the last kill |
| C-x C-x | swaps the cursor and the mark |
| C-s, C-r | searches forward or backward |
| M-g | goes to a line number |
| C-g | clears the mark |
| C-x C-s | saves |
| C-x C-c, C-q | quits |

Only printable ASCII can be typed, as at the [prompt](prompt.md). Use `\w` and `\A` in strings for the omega and aleph glyphs.

## Killing and yanking

Text that is killed goes into the tab's kill buffer, replacing what was there, and C-y inserts it at the cursor. Kills that follow one another with nothing in between are added to the same piece, so C-k twice at the start of a line takes the line and its break. A kill can span several lines, so a block is moved by marking it, killing it with C-w and yanking it where it belongs.

C-SPC sets the mark where the cursor is. The region is the text between the mark and the cursor, and C-w kills it and M-w copies it without removing anything. Both clear the mark afterward. C-x C-x puts the cursor at the mark and the mark where the cursor was, which shows where the region starts.

The kill buffer belongs to the tab and survives between editing sessions, so text killed in one definition can be yanked into another.

## Lines

M-g asks for a line number on the bottom row and moves to the start of that line. Numbers past the end go to the last line, and ESC cancels. The line numbers in the gutter are the ones it uses, so an error that names a line can be found directly.

## Searching

C-s asks for text on the bottom row. RET searches forward from just after the cursor, wrapping around past the end, and moves the cursor to the first match. ESC and C-g cancel. An empty answer repeats the last search, so C-s RET steps through every match. C-r does the same toward the start of the text. The search is case-sensitive.

## Saving and quitting

C-x C-s records the current text as the saved version. Nothing outside the editor changes yet. C-x C-c, or C-q, quits, and what happens next depends on how scrawl was opened, using the last saved version:

- **`edit`** parses the text and redefines the word in your tab.
- **`open` and `unit`** rebuild root from that unit onward. If the rebuild fails, root keeps working as before and your text is kept as a draft ([how root builds](root-build.md)).

If the text has unsaved changes, the first quit shows `unsaved. quit again to discard`, and a second one quits without them. Quitting without ever saving changes nothing at all.

## edit in detail

`'name edit` opens the body of a definition without its outer brackets:

    [ { a b } a a * b b * + ] :hyp
    'hyp edit

shows `{ a b } a a * b b * +`. On quit, the text is parsed back into a quote and defined as `hyp` in your tab. A few consequences:

- **A new name** opens an empty editor, and quitting defines the word. Use it for anything longer than a line.
- **A root word** opens its root definition, and quitting defines a private copy in your tab that hides the root one. Use `open` to change root itself.
- **A value that isn't a quote**, such as a string, is opened as its plain text, and saved back as a string.
- **Text that doesn't parse**, for example with an unclosed bracket, is kept as a string, and scrawl reports `that didn't parse` with the reason. The word then pushes that string when run. Open it again with `edit` to fix it.

Since `edit` redefines the word, the change is an ordinary change to the world: `back` undoes it, and on an installed disk it is [saved](saving.md).

Comments don't survive `edit`. The text is parsed into a quote, and printing the quote back for the next `edit` produces the code without them. Code whose comments matter belongs in a root unit, where the source is kept as written.

## Size

`edit` shows at most 16 KiB of a definition's text. Root units have no limit of their own beyond the 1 MiB slot that all of root is saved in.

## On the screen

`scrawl` uses the whole window of its tab, which is 80 by 24 characters when the tab has the screen to itself and smaller in a [split window](tabs.md). The mouse isn't used inside the editor. The tab keys F1 to F4 and Alt-1 to Alt-4 still switch tabs, so another tab can show a help page while you write a unit.
