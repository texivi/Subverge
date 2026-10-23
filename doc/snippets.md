# Snippets

Each snippet below solves one common task with a line or a definition that can be typed at the prompt as it stands. The results are shown after the code.

## Numbers

Factorial, with bignums past 20:

    [ { n } 1 n 1 + range 1 [ * ] fold ] :fact
    20 fact                 2432902008176640000
    25 fact                 15511210043330985984000000

Primes by trial division:

    [ { n } n 2 < [ 0 ] [ 2 n range [ n swap mod 0 = ] filter len 0 = ] if ] :prime?
    0 30 range [ prime? ] filter        [ 2 3 5 7 11 13 17 19 23 29 ]

This tests every divisor below `n`, which is fine for small numbers. `sieve` draws a faster method ([games](games.md)).

The mean of a list, rounded toward zero:

    [ { l } l sum l len / ] :mean
    [ 2 4 9 ] mean          5

A fraction in lowest terms:

    [ { a b } a b gcd { g } a g / b g / ] :simplify
    12 18 simplify          2 3

A number in binary, as a string:

    [ { n } "" { s } [ n 0 > ] [ n 2 mod str s cat ->s n 2 / ->n ] while s ] :bin
    10 bin                  "1010"

`255 bases` prints decimal, hex, octal and binary in one go, and `hex` gives hex directly.

## Strings

Words of a sentence, ignoring repeated spaces:

    "a b  c" " " split [ len 0 > ] filter       [ "a" "b" "c" ]

Counting the occurrences of a word:

    "one two three two" " " split [ "two" = ] filter len     2

Counting a character, by its code:

    [ { s c } 0 s [ c = [ 1 + ] when ] each ] :count-char
    "banana" 97 count-char  3

The same with `split`: `"banana" "a" split len 1 -` is also `3`.

A palindrome test, ignoring case:

    [ { s } s lower s lower reverse = ] :palindrome?
    "Level" palindrome?     1

Removing a character everywhere:

    "abcabc" "b" "" replace     "acac"

Reading numbers from a line of text:

    "3,1,2" "," split [ >num ] map sort         [ 1 2 3 ]
    "  42 " >num 1 +                            43

Splitting a key from a value:

    "key: value" ": " split     [ "key" "value" ]

Building output from values:

    [ 1 2 3 ] [ str ] map " + " join            "1 + 2 + 3"
    "x = " 42 str cat say                       x = 42

## Lists

Removing duplicates, keeping the first of each:

    [ { l } [ ] { out } l [ { x } out x has not [ out x append ->out ] when ] each out ] :unique
    [ 1 2 2 3 3 3 ] unique      [ 1 2 3 ]

Locals only live for the line or quote that binds them, so code that uses them across several steps belongs in a definition.

Scaling every item:

    [ { l n } l [ n * ] map ] :scale
    [ 1 2 3 ] 10 scale      [ 10 20 30 ]

A list of records and a lookup:

    [ [ "ann" 31 ] [ "bob" 27 ] ] :people
    people [ first "bob" = ] filter first 1 at  27

Totals with a local accumulator, when `sum` and `fold` don't fit:

    0 { total } [ 1 2 3 ] [ total + ->total ] each total        6

## The stack

The whole stack as a list, without losing it:

    depth list dup . spread

Clearing it:

    empty

## Loops

Counting with a local:

    0 { i } [ i 5 < ] [ i 1 + ->i ] while i     5

Repeating until a key is pressed:

    1 { going } [ going ] [ "." write 10 sleep key? -1 = not [ 0 ->going ] when ] while

Timing code:

    [ 100000 [ ] times ] timeit

## Screen

A line in a colour, then back to normal text:

    12 color "careful" say 7 color

Text at a fixed position, without moving the cursor:

    "here" 30 10 14 text

A full-screen loop that redraws twice a second until a key is pressed:

    [ time hms 36 12 14 text ] watch

A box of `#` characters:

    [ { x y w h } 0 h [ { i } 35 x y i + w 7 fill ] for ] :box
    10 5 20 4 box

## Themes and the logo

Switch the colours, and the logo with them:

    'haelide theme
    'dusk theme

Change one colour until the next theme switch:

    14 255 120 0 rgb

Draw `sys` with a logo of your own:

    [ [ " /\\ /\\" " (o.o)" "  >^<" ] ] :logo-fox
    'fox logo
    sys

Change the name before the `@` for this tab:

    [ "me" ] :user
    sys

See [themes](themes.md) and [logos](logos.md).

## Help pages

    docs                        open the index
    "tour" doc                  open a page

## Time

    time                    year month day hour minute second
    date say                2026-10-19
    ticks hz /              seconds since boot
    uptime say              the same, as hours, minutes and seconds
    300 sleep               wait three seconds

## Definitions and root

A word that every tab sees and that survives reboots:

    "mine" unit

then write the definitions in the editor, save with Ctrl-S and leave with Ctrl-Q ([units](units.md)).

Trying a change to a root word in one tab only:

    'top edit

makes a private copy that hides the root one in this tab. `'top forget` removes the copy again ([names](names.md)).

## Machine

The first 16 bytes of the screen memory, which hold the tab bar:

    0 16 range [ 0xB8000 + peek ] map

The CPU's vendor string:

    0 cpuid { a b c d } b reg>str d reg>str cat c reg>str cat

All PCI devices from one vendor:

    pci [ 3 at 0x8086 = ] filter

For the rest of the machine words, see [omega for smarties](smarties.md).
