# Stack

Each tab has one data stack of up to 1024 values. Words take their arguments from it and leave their results on it, and every value passed between words goes through it. Locals are the only other way to hold a value ([names](names.md)).

## Effects

A stack effect lists what a word takes and what it leaves, with the top of the stack on the right:

    dup     a -- a a
    swap    a b -- b a
    rot     a b c -- b c a

`'word help` shows the effect of any built-in or root word ([help](help.md)). The letters are only names: `n` and `i` are numbers, `s` a string, `l` a list, `q` code, and `a`, `b`, `x` anything.

## Stack words

| Word | Effect | |
|---|---|---|
| `dup` | `a -- a a` | copies the top value |
| `drop` | `a --` | discards the top value |
| `swap` | `a b -- b a` | swaps the top two |
| `over` | `a b -- a b a` | copies the second to the top |
| `rot` | `a b c -- b c a` | moves the third to the top |
| `nip` | `a b -- b` | discards the second |
| `tuck` | `a b -- b a b` | copies the top under the second |
| `2dup` | `a b -- a b a b` | copies the top two |
| `pick` | `... n -- ... x` | copies the value n below the top |
| `depth` | `-- n` | how many values there are |
| `empty` | `... --` | discards everything |

`0 pick` is `dup` and `1 pick` is `over`. `pick` reaches as deep as the stack goes, and fails with `pick needs more values on the stack` when `n` is at least the depth after taking `n` off.

    10 20 30 2 pick         10 20 30 10
    1 2 tuck                2 1 2
    3 4 2dup * rot rot +    12 7

Every stack word moves whole values, so a quote, a string or a bignum costs the same to `dup` as a small integer. Copies share the same object ([values and objects](values.md)).

## Juggling or locals

Code that keeps three or more values in play reads better with locals than with `rot` and `pick`:

    [ rot dup * rot dup * + swap dup * + ] :sumsq3
    [ { a b c } a a * b b * + c c * + ] :sumsq3

Both define the sum of three squares. The first is shorter on the stack and slightly faster when compiled, since the [JIT](jit.md) inlines `dup` and `*` while locals go through the interpreter. The second is easier to read. Mixing them is common: use the stack for values that are consumed at once and locals for values used more than once.

## Whole-stack operations

`depth list` packs the entire stack into one list, and `spread` unpacks a list onto the stack. Together they save and restore the stack:

    depth list { saved }
    ... code that makes a mess ...
    empty saved spread

After `depth list` the stack is empty, so the code in the middle starts from nothing, and `empty saved spread` puts the old values back whatever it left. `learn` runs each exercise this way. To keep the values on the stack while also saving a copy, spread them back straight away with `depth list dup spread` ([catching](catching.md)).

To print the stack without changing it:

    depth list dup . spread

The [prompt](prompt.md) already shows the stack before each `»`, cut from the left when it is long.

## Printing values

| Word | Pops | Prints |
|---|---|---|
| `.` | one value | the value as source, then a newline: strings in quotes, symbols with `'` |
| `say` | one value | the value as text, then a newline: strings without quotes |
| `write` | one value | as text, without a newline |

`.` is for looking at values and `say` for output meant to be read. `"hi" .` prints `"hi"`, and `"hi" say` prints `hi`.

## Errors

A word that finds too few values fails with `name needs more values on the stack`, naming itself. A word that finds the wrong kind of value fails with `name needs a number`, `name needs a string`, `name needs a quote` or `name needs a 'name`. Either way the whole line is undone ([transactions](transactions.md)), so the stack is as it was before the line.

Pushing a 1025th value fails with `the stack is full`. This nearly always means a loop that pushes on every pass without popping, such as `2000 [ 1 ] times`.

## The stack and the world

The stack is part of the tab's [world](world.md): it is saved on installed disks, restored by [back](back.md) and `redo`, and separate in each [tab](tabs.md). Values that can't be written as source don't survive a reboot ([saving](saving.md)).
