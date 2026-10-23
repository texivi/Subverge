# Catching

`try` runs a quote and keeps a failure inside it from ending the line. It leaves `1` if the quote finished, or prints the error and leaves `0` if it failed.

    [ 1 0 / ] try .         prints the error, then 0
    [ 1 2 + ] try .         leaves 3, then prints 1

Without `try`, a failure anywhere in a line ends the line and undoes it ([transactions](transactions.md)). With `try`, the line carries on after the quote, and the code after `try` decides what to do from the `1` or `0`.

## No rollback

`try` doesn't roll anything back:

    [ 5 6 0 / ] try

leaves `5 0`. The `5` was pushed before the failure and stays. The `6` was taken by `/` when it failed, and the `0` comes from `try`. Only the line as a whole is a [transaction](transactions.md), so whatever the quote changed before it failed is still there, definitions included.

Restoring the stack after a failure is up to you. A common way is to save it first and put it back on failure:

    depth list ->saved saved spread
    [ risky-word ] try not [ empty saved spread ] when

`depth list` takes every value off the stack and makes one list of them, `->saved` keeps that list in a local, and `saved spread` pushes the values back at once, so the stack is unchanged and a copy is kept. If the quote fails, `empty` clears whatever it left and `saved spread` restores the copy. `learn` runs every exercise like this, so a mistake in a lesson can't mess up the stack you had before (`'lesson see`).

## Failing on purpose

`fail` takes a string and fails with it as the message:

    "the file is empty" fail
      ! the file is empty

It works like a failing built-in. At the prompt it ends and undoes the line, and inside `try` it is caught:

    [ "bad input" fail ] try .      prints the error, then 0

Use it for checks in your own words, so a bad argument stops the line with a message that says what was wrong instead of failing later somewhere less obvious:

    [ { n } n 0 < [ "negative" fail ] when n sqrt ] :root-of
    16 root-of .            4
    -4 root-of
      ! negative

## Interrupts

Ctrl-C raises `interrupted`, which is an ordinary error, so `try` catches it:

    [ 1000000000 [ ] times ] try "after" say

Pressing Ctrl-C there prints `! interrupted` and then `after`. The `stopped` error from `trace` is caught the same way. A program that wants Ctrl-C to always leave should avoid wrapping its main loop in `try`.

## What it can't catch

`try` can't catch a CPU fault, such as a page fault from a bad `peek`. A fault restarts the tab and drops the line ([tabs](tabs.md)). `halt` stops the machine, and nothing runs after it.

## When to use it

Use `try` for failures you expect, where the program has something sensible to do next:

    "12x" [ >num ] try [ "number" say ] [ "not a number" say ] if

Here `>num` fails on text that isn't a number, and the line prints `not a number` instead of ending.

Wrapping code in `try` just in case makes failures harder to find. A line that fails outright points at the word that failed and leaves the world as it was. A line that carries on after a caught failure keeps going with a stack that may be missing values or have extra ones. See [errors](errors.md) for the messages.
