# omega

omega is the language you type at the prompt, and every program in Subverge is written in it. It is a stack language in the Forth family: values go onto a stack, and words take their arguments from the stack and leave their results there. The kernel is called ordinal and the language is called omega after ω, the first infinite ordinal, and omega has infinite numbers as real values.


If ordinals and alephs are new to you, watch the video linked on the [ordinals](ordinals.md) page first. The infinite values in Subverge come from it, and the [ordinals](ordinals.md) and [cardinals](cardinals.md) pages follow the same order: counting positions first, then sizes.

Each section below is short and links to the page with the details. For the everyday words, see [words](words.md), and for the exact rules and the machine-level words, see [omega for smarties](smarties.md).

## Reading a line

A line is split at spaces into tokens, and the tokens run from left to right. A number or a string is pushed onto the stack. A word is looked up and run.

    2 3 +           leaves 5
    2 3 + 4 *       leaves 20
    2 3 4 * +       leaves 14

`2 3 +` pushes 2, pushes 3, and then `+` takes both and pushes 5. The order of the tokens decides the order of the operations, so omega needs no brackets for arithmetic and has no precedence rules. After each line the prompt shows what is left on the stack, in grey, with the top on the right.

`.` takes the top value and prints it, and `say` prints a string followed by a newline:

    2 3 + .         prints 5
    "hello" say     prints hello

`#` starts a comment that runs to the end of the line.

## The stack

Most short programs only need a handful of stack words:

    5 dup           5 5
    1 2 swap        2 1
    1 2 drop        1
    1 2 over        1 2 1
    1 2 3 rot       2 3 1

The stack holds up to 1024 values, and each [tab](tabs.md) has its own. See [stack](stack.md) for the full list, and for when to use locals instead.

## Values

Every value has a type, and words check the types they get. `+` on two strings fails with an error instead of adding two addresses.

| Type | Written as | More |
|---|---|---|
| integer | `42` `-7` `0x2a` `0b101` | [numbers](numbers.md) |
| string | `"text"` | [strings](strings.md) |
| quote | `[ 1 2 + ]` | [quotes](quotes.md) |
| symbol | `'name` | [names](names.md) |
| ordinal | `omega` `epsilon0` `omega1` | [ordinals](ordinals.md) |
| cardinal | `aleph0` `aleph1` `continuum` | [cardinals](cardinals.md) |

Integers have no fixed size. A result that doesn't fit in 64 bits becomes a bignum, so `2 100 ^ .` prints all 31 digits of 2^100 ([bignums](bignums.md)). `type` gives the name of a value's type, such as `"list"` for a quote.

Comparisons leave `1` for true and `0` for false, and any value other than `0` counts as true in a condition.

## Quotes

Square brackets make a quote. The code inside isn't run, and the whole quote is pushed as one value to be run later:

    [ 2 3 + ]       pushes the quote
    [ 2 3 + ] do    runs it, leaves 5

A quote is also a list. `[ 1 2 3 ] len` is 3, `[ 1 2 3 ] 0 at` is 1, and `[ 1 2 3 ] [ 2 * ] map` is `[ 2 4 6 ]`. omega has no separate list type, and the same words work on code and on data ([quotes](quotes.md)).

## Definitions

A colon in front of a name binds the value below it to that name:

    [ dup * ] :sq
    7 sq            leaves 49
    42 :answer
    answer          leaves 42

A name bound to a quote runs the quote when it is used. A name bound to anything else pushes its value. Definitions made at the prompt belong to your tab's [world](world.md). Words that every tab should see go in [root](root.md), which is also where every program that ships with Subverge lives.

A symbol, written with a quote mark in front, is a name that isn't run. Words that work on names take a symbol, such as `'sq see` to show a definition and `'sq forget` to remove it ([names](names.md)).

## Locals

`{ w h }` takes two values off the stack and binds them to `w` and `h` until the quote it is in finishes:

    [ { w h } w h * ] :area
    3 4 area        leaves 12

`->x` binds a single value. Locals are dynamically scoped, so a word called from inside the quote can see them too. [Names](names.md) explains the rules and the cases where that matters.

## Control flow

Conditionals and loops are ordinary words that take quotes:

    5 0 > [ "yes" ] [ "no" ] if say
    3 [ "again" say ] times
    0 3 [ . ] for
    [ 1 2 3 ] [ . ] each

`if` takes a condition and two quotes and runs one of them. `when` and `unless` take a condition and one quote. `times`, `for`, `while` and `each` repeat a quote. See [flow](flow.md), which also covers `trace` for stepping through code one word at a time.

## Errors and undo

When a word fails, the line stops and everything the line did is undone, including the definitions it made and the values it pushed. This is a [transaction](transactions.md). The error is printed in red:

    1 0 /
      ! division by zero

`back` undoes whole lines that worked, up to the last 64, and `redo` reverses a `back` ([back](back.md)). `try` runs a quote and leaves `0` instead of failing the line, for code that expects a failure ([catching](catching.md)). See [errors](errors.md) for the messages and what causes them.

## Compiling

You never compile anything by hand. A quote runs in the interpreter the first time, and the second time it runs, the kernel compiles it to x86-64 machine code. `'sq xray` shows the code it produced ([JIT](jit.md)).

## Help

`help` lists every word by group, `'group help` lists one group, and `'word help` shows a word's stack effect, what it does and an example. `"text" apropos` searches the help text. The [index of words](words) has a page for each word, and `learn` teaches the language in short lessons with a line to try in each.
