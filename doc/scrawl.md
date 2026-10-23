# scrawl

scrawl is the text editor in Subverge. It is used for anything longer than one line: definitions, [root units](units.md) and new programs. It has no modes, so typing always inserts text, and every command is a Ctrl key.

## Opening it

scrawl is never started directly. It is opened by one of three words, which decide what happens to the text when you quit:

    'sq edit            a definition in this tab
    'top open           the root unit that defines top
    "mine" unit         a new, empty root unit

`edit` works on your tab's [world](world.md), and `open` and `unit` work on [root](root.md). 

While scrawl is open, the tab's title becomes `edit` followed by the name, and the tab is otherwise busy. The other tabs keep running, and each tab can have its own scrawl open.

## The screen

The top row shows `scrawl`, the name being edited, `[+]` when there are unsaved changes, and the cursor's line and column on the right. Each line of text has its line number in the gutter on the left, and rows past the end of the text show `~`. The bottom row lists the main keys, and is replaced by messages such as `saved, 412 bytes` or `not found: foo`.

Lines longer than the screen scroll sideways as the cursor moves along them.

## Keys

| Key | Action |
|---|---|
| arrows | move by character and line, wrapping at line ends |
| Home, End | start or end of the line |
| Page Up, Page Down | one screen up or down |
| Ctrl-T, Ctrl-B | top or bottom of the text |
| Enter | splits the line, keeping the current line's indentation |
| Backspace | deletes before the cursor, joining lines at the start of one |
| Delete | deletes under the cursor, joining lines at the end of one |
| Tab | inserts spaces up to the next multiple of 4 |
| Ctrl-G | goes to a line number |
| Ctrl-D | duplicates the current line below it |
| Ctrl-K | cuts the current line |
| Ctrl-U | pastes the cut line above the current one |
| Ctrl-F | finds text |
| Ctrl-S | saves |
| Ctrl-Q | quits |

Only printable ASCII can be typed, as at the [prompt](prompt.md). Use `\w` and `\A` in strings for the omega and aleph glyphs.

## Cutting and pasting

Ctrl-K removes the current line and puts it on the clipboard, replacing whatever was there. Ctrl-U inserts a copy of the clipboard above the cursor and keeps the clipboard, so a line can be pasted several times. To move a block of lines, cut them one at a time from the top, move, and paste them back in reverse order, or cut and paste each line separately.

The clipboard belongs to the tab and survives between editing sessions, so a line cut in one definition can be pasted into another.

## Lines

Ctrl-G asks for a line number on the bottom row and moves to the start of that line. Numbers past the end go to the last line, and Escape cancels. The line numbers in the gutter are the ones it uses, so an error that names a line can be found directly.

Ctrl-D copies the current line below itself and moves to the copy, which is quicker than cutting and pasting twice when writing several similar lines.

## Finding

Ctrl-F asks for text on the bottom row. Enter searches forward from just after the cursor, wrapping around past the end, and moves the cursor to the first match. Escape cancels. An empty search repeats the last one, so Ctrl-F Enter steps through every match. The search is case-sensitive.

## Saving and quitting

Ctrl-S records the current text as the saved version. Nothing outside the editor changes yet. Ctrl-Q quits, and what happens next depends on how scrawl was opened, using the last saved version:

- **`edit`** parses the text and redefines the word in your tab.
- **`open` and `unit`** rebuild root from that unit onward. If the rebuild fails, root keeps working as before and your text is kept as a draft ([how root builds](root-build.md)).

If the text has unsaved changes, the first Ctrl-Q shows `unsaved. ^Q again to discard`, and a second Ctrl-Q quits without them. Quitting without ever saving changes nothing at all.

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

`scrawl` uses the tab's text area, which is 80 by 24 characters. The mouse isn't used inside the editor. The tab keys F1 to F4 and Alt-1 to Alt-4 still switch tabs, so another tab can show a help page while you write a unit.
