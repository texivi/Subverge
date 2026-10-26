# Prompt

The `»` prompt is where omega code is typed. Each tab has its own prompt, its own line history and its own key queue, and whatever is typed at it runs as one [transaction](transactions.md) when Enter is pressed.

## What the prompt shows

Before the `»`, the prompt prints the tab's stack in grey, bottom to top:

    1 2 [ dup * ] »

An empty stack prints nothing before the `»`. A long stack is cut from the left and shown with `..` in front, so the top of the stack, which is what the next word will see, is always visible.

The stack shown is the committed one. While a line is running, nothing is printed until it finishes.

## Editing keys

The prompt edits a line with the keys of Emacs. The full table is on the [keys](keys.md) page. The ones used most:

| Key | Action |
|---|---|
| RET | runs the line |
| C-a, C-e | move to the start or end of the line |
| C-b, C-f, M-b, M-f | move by character or by word |
| DEL, C-d | delete before or at the cursor |
| C-k, C-u, C-w | kill to the end, to the start or the last word |
| C-y, M-y | yank the last kill, or an older one |
| C-p, C-n | step through earlier lines |
| C-r, C-s | search the earlier lines |
| C-_ | undoes the last change to the line |
| TAB | completes the name before the cursor |
| M-x | runs a word by name |
| C-c, C-g | throws the line away and starts a new one |
| F1 to F4, Alt-1 to Alt-4 | switch [tab](tabs.md) |

The arrow keys, Home, End and Delete do what their Emacs equivalents do. Characters are inserted at the cursor, so a typo in the middle of a line can be fixed without retyping the rest.

M-x asks for a word on a new line, with completion, runs it as if it had been typed, and brings the prompt back with the line you were editing. C-h starts the help keys, such as C-h k to see what a key does. You can bind keys of your own, see [keys](keys.md).

## Completion

Tab completes the name that ends at the cursor. The candidates are your tab's definitions, root's words and the built-ins.

- With one match, Tab writes the rest of the name and a space.
- With several matches that share a longer beginning, Tab extends the name to it: `them` becomes `theme`, the part `theme`, `theme@` and `themes` have in common.
- Otherwise Tab writes the first match in alphabetical order, and each further Tab replaces it with the next, wrapping around: `ro` gives `root`, `rot`, `row`, then `root` again.

A `'`, `:` or `->` in front of the name is kept, so `'th` completes to `'theme`, `'themes` and `'theme@` in turn. Locals aren't offered, since they only exist while code runs.

`ask` and `calc` don't complete.

## Line length

A line holds up to 200 characters, or less when the prompt itself is wide, since the line has to fit on one screen row after the stack and the `»`. Once it is full, further keys are ignored. If the stack display takes up a lot of the row, `empty` or a few `drop`s give the line more room.

Code longer than one line belongs in a definition, written with `'name edit` in [scrawl](scrawl.md), or in a [root unit](units.md).

## History

Up and Down, or C-p and C-n, step through the last 16 lines typed in the tab. C-r searches them for text. Empty lines and a line identical to the one before it are not added. The history is per tab, it is not saved to disk, and it holds text only: recalling a line and pressing Enter runs it again, which is different from undoing it with [back](back.md).

## Characters you can type

Only printable ASCII, from space to `~`, can be typed. omega has a few values that print with other characters, such as `ω` and `ℵ₀`, which the kernel draws by redefining unused characters in the VGA font ([screen](screen.md)). To use them:

    omega .                      ω
    aleph0 .                     ℵ₀
    "\w and \A" say              ω and ℵ

`\w` and `\A` work inside any string. The words `omega` and `aleph0` push the values themselves ([ordinals](ordinals.md), [cardinals](cardinals.md)).

## When a line fails

A failing line prints its error in red, indented under the line, and the prompt comes back with the same stack as before:

    1 2 » 3 0 /
      ! division by zero
    1 2 »

Keys pressed while the failing line was running are discarded, so the next prompt starts clean. See [errors](errors.md).

## When a line runs for a long time

The prompt doesn't come back until the line finishes. C-c, or C-g, interrupts it with the error `interrupted`, which also undoes the line. If the line doesn't respond to Ctrl-C, switch to another tab with F1 to F4 or Alt-1 to Alt-4. The other tabs keep running and their prompts keep working ([tabs](tabs.md)).

## Programs that read keys

Programs started from the prompt, such as `snake` or `top`, read keys from the same queue. When they exit, the prompt returns. `ask` prints a question and reads one line with the same editing keys, but without history:

    "name? " ask say

## Help pages

`docs` opens the help pages and `"name" doc` opens one page. A line of code shown in a page can be selected with Tab and run with Enter, which closes the viewer and runs the line at the prompt ([help](help.md)).
