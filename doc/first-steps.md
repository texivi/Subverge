# First steps

## The welcome screen

After booting, the first shell shows the system information and the first-boot questions, and then the prompt:

    »

`»` is the omega prompt. Each line you type there is an omega program, which runs when you press Enter. Tabs 2 to 4 start at the same prompt, and F1 to F4 or Alt-1 to Alt-4 switch between them.

## Reading the pages

The help pages are inside the system. `docs` opens the index and `"first-steps" doc` opens a page by name. In the viewer Up, Down, PgUp, PgDn, Home and End scroll, Space pages down, Tab selects the next link, Enter follows it and Backspace goes back. `q` or Esc leaves. Enter on a line of code closes the viewer and runs the line at the prompt. With a mouse, a click follows a link or runs a code line, the wheel scrolls and a right click goes back ([help](help), [keys](keys)).

The first-boot questions ask whether to install and whether to take the [tour](tour), a twelve page walk through the system that types the lines for you. Answer `n` to both and carry on here.

## The stack

omega is a stack language. Values are pushed onto a stack, and words take their arguments from it and push their results back:

    2 3 +

leaves `5` on the stack. The prompt shows the stack in grey before the `»`, so you can see it change:

    5 »

`.` prints the top value and removes it:

    5 » .
    5
    »

Arguments come first and the operation last, as in Forth or an RPN calculator, and evaluation goes strictly left to right:

    2 3 + 4 *                   20
    2 3 4 * +                   14

`dup`, `drop`, `swap`, `over`, `rot` and `nip` rearrange values ([stack](stack.md)). `empty` clears the whole stack.

## Defining words

A quote is code in square brackets that isn't run yet. `:name` takes the value on top of the stack and gives it a name:

    [ dup * ] :sq
    7 sq .                      49

`sq` now works like any built-in word, in this tab. `see` shows a definition and `words` lists all of yours:

    'sq see
    words

The quote mark in `'sq` passes the name instead of running it. See [names](names.md) for definitions, locals and how names are looked up.

## Undoing mistakes

Each line is a transaction. If any word in it fails, the whole line is undone and the stack and definitions are as they were before:

    1 2 3
    [ 7 ] :seven 4 5 0 /
      ! division by zero

The stack is still `1 2 3`, and `seven` isn't defined. See [transactions](transactions.md).

Lines that worked can be undone too:

    [ 1 + ] :inc2
    back
      rewound 1

`back` undoes up to 64 lines ([back](back.md)). If something runs for too long, Ctrl-C stops it.

## Control flow

Conditions and loops take quotes:

    5 3 > [ "bigger" say ] [ "smaller" say ] if
    10 [ "hi" say ] times
    0 5 [ . ] for               prints 0 to 4
    3 [ dup 0 > ] [ dup . 1 - ] while drop      prints 3 2 1

See [flow](flow.md) for the rest.

## Lists

A quote is also a list:

    0 5 range .                 [ 0 1 2 3 4 ]
    [ 1 2 3 ] [ sq ] map .      [ 1 4 9 ]
    [ 1 2 3 4 ] [ 2 mod 0 = ] filter .     [ 2 4 ]
    [ 3 1 2 ] len .             3

## Strings

    "subverge" upper say        SUBVERGE
    "a,b,c" "," split .         [ "a" "b" "c" ]
    "x = " 42 str cat say       x = 42

See [strings](strings.md) for the rest.

## Big numbers and infinity

Integers that don't fit in 64 bits become bignums, and `omega`, the first infinite ordinal, is a value too:

    2 100 ^ .                   1267650600228229401496703205376
    omega 1 + .                 ω+1
    1 omega + .                 ω

See [numbers](numbers.md) and [ordinals](ordinals.md).

## A small program

Write a word over several lines with the editor:

    'countdown edit

In [scrawl](scrawl.md), type:

    { n }
    n 0 > [
      n .
      30 sleep
      n 1 - countdown
    ] when

Press Ctrl-S to save and Ctrl-Q to quit. Then:

    5 countdown

counts down from 5, a little over three a second. The word calls itself, takes its argument as the local `n`, and stops when `n` reaches 0.

## Tabs

Press F2. Tab 2 is a separate session with its own stack, and `sq` isn't defined there unless you define it again:

    7 sq .                      49, because root has its own sq
    'countdown defined? .       0

Definitions made at a prompt stay in their tab. Root words, such as `sq`, `apps` and `clock`, work in every tab. Press F1 to go back ([tabs](tabs.md)).

## Programs

`apps` lists the programs. Type a name at the prompt to run one. A program uses the whole tab until it ends, and Ctrl-C or the program's own quit key stops it. The other tabs keep running meanwhile.

    "credits" doc

`doc` opens a help page.

## The look

    'amber theme                amber text on black
    'dusk theme                 back to the default
    sys                         the logo and system information again

See [themes](themes.md) and [logos](logos.md).

## Finding your way around

    help                every built-in word, by group
    'flow help          one group
    'times help         one word
    apps                the programs that come with Subverge
    root                the root units and what they define
    learn               the tutorial

Run `learn` next. It teaches the language in short lessons with exercises ([learn](learn.md)). After that, see [omega](omega.md) for the language and [tools](tools.md), [games](games.md) and [demos](demos.md) for the bundled programs.

## Keeping your work

On the live ISO, everything is lost when the machine stops. `install` writes Subverge to a drive and keeps your work: an installed system saves each tab after every line that changes it ([saving](saving.md)).

## Where to go from here

Run `learn` for the language and the [tour](tour) for a walk through the system. [themes](themes) lists the themes and [tips](tips) collects the things that are easy to miss.
