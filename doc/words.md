# Words

The everyday words, grouped the way `help` groups them, with a few notes on each group. `'word help` shows the stack effect and an example for any of them, and `'group help` lists a group, such as `'flow help`. See the [index of words](words) for the complete list and [omega for smarties](smarties.md) for the memory, port, screen, machine and compiler words.

## syntax

`[ ]` [quotes](quotes.md), `:name` definitions, `->name` and `{ a b }` locals, `'name` symbols ([names](names.md)), `"text"` [strings](strings.md), `42 0x2a 0b101` [numbers](numbers.md), `#` comments.

These aren't words you look up. The parser recognises them by their first character, so `:sq` is one token that defines `sq`, and `'sq` is one token that pushes the symbol. A space is needed between every token, including after `[` and before `]`.

## stack

`dup drop swap over rot nip tuck 2dup pick depth empty`. See [stack](stack.md).

These only move values around. When a word needs more than three values at once, locals are usually clearer than a chain of `rot` and `over`.

## math and logic

`+ - * / mod ^ neg abs min max sqrt gcd`, `= < > <= >= and or not`, and bitwise `& | xor ~ << >>`. See [numbers](numbers.md). `+ * ^` also work on [ordinals](ordinals.md) and [cardinals](cardinals.md).

`/` rounds towards zero and `mod` takes the sign of the first number. `and`, `or` and `not` work on truth values and leave `1` or `0`, while `&`, `|` and `~` work on the bits.

## flow

`do if when unless times for while each map filter eval trace try`. See [flow](flow.md), and [catching](catching.md) for `try`.

Every one of them takes its code as a quote. `eval` is the exception: it takes a string and runs it as if it had been typed.

## lists and text

`list range append cat len at first last rest has index spread reverse sort sum fold type` on [quotes](quotes.md), and `str hex chr slice pad find split join replace starts? ends? >num upper lower trim` on [strings](strings.md).

Many of them work on both. `len`, `at`, `cat`, `reverse` and `each` treat a string as a list of characters, and `type` says which one you have.

## io

`. say write emit nl color key key? waitkey ask`.

`key` waits for a key and returns its code, `key?` returns `-1` immediately if no key is waiting, and `waitkey` waits up to a given number of ticks. The arrow keys are `256` to `259`, in the order up, down, left, right.

## names

`get forget >sym defined? see words edit`. See [names](names.md).

## world

`help docs doc apropos back redo stats save home root open unit src welcome`. See [help](help.md), [back](back.md), [saving](saving.md), [root](root.md), [units](units.md) and [startup](startup.md).

## infinity

`omega epsilon0 omega1 aleph0 aleph1 aleph continuum card ordinal?`. See [ordinals](ordinals.md) and [cardinals](cardinals.md).

## time

`ticks hz time idle sleep beep rand`. The timer runs at 100 ticks a second, so `100 sleep` waits one second. In QEMU the clock is UTC unless configured otherwise.

## screen

`put text screen@ wipe cursor nocursor where size title tab mytab`, `gfx gfx-off dot box window` for graphics mode, `bitmap bmp-set bmp-get bmp-fill bmp-box bmp-size bmp-show bmp-free pack-rgb picture pictures gallery credits` for pictures, `mouse mouse?` for the pointer, `theme themes theme@ rgb` for colours, and `logo logos logo@` for the `sys` logo. See [screen](screen.md), [pictures](pictures.md), [themes](themes.md) and [logos](logos.md).

## tools, demos and games

The programs in root, described in [tools](tools.md), [demos](demos.md) and [games](games.md).

## util

`sq inc dec kb mb uptime >hex fmt attr fill bar header quit-key?`. Small helpers used by the programs, available to yours as well. `quit-key?` is true for `q`, Esc and Ctrl-C, which is why those keys exit almost every program. For the drawing helpers, see [screen](screen.md).

## helpers

Words used internally by the root programs, listed by `'helpers help`. `user`, `host` and `os` are the strings shown on the `sys` screen ([startup](startup.md)).
