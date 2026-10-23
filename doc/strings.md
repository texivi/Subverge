# Strings

Strings are immutable sequences of bytes, written in double quotes. Every word that seems to change a string returns a new one and leaves the original alone.

## Literals

    "hello"
    "two\nlines"
    "a \"quoted\" word"
    "tab\there"

| Escape | Character |
|---|---|
| `\n` | newline |
| `\t` | tab |
| `\e` | escape, 27 |
| `\"` | a double quote |
| `\\` | a backslash |
| `\w` | the omega glyph |
| `\A` | the aleph glyph |

A backslash before any other character gives that character. The [prompt](prompt.md) only accepts printable ASCII, so `\w` and `\A` are the way to put the glyphs in a string. The screen font shows bytes above 127 as the PC code page 437 characters, and `ascii` prints the whole table ([tools](tools.md)).

A string literal has to close on the same line, since a line ends the code. Longer strings are built with `cat` or written in [scrawl](scrawl.md).

## Printing

    "hi" say        hi, then a newline
    "hi" write      hi, no newline
    "hi" .          "hi", in quotes
    nl              a newline on its own
    65 emit         A, one character from its code

`n color` sets the colour for the text that follows, 0 to 15 ([themes](themes.md)). `put` and `text` write at a fixed position without moving the cursor ([screen](screen.md)).

## Building and taking apart

| Word | Effect | |
|---|---|---|
| `cat` | `a b -- c` | joins two strings |
| `len` | `s -- n` | length in bytes |
| `at` | `s i -- n` | the byte at position i, from 0 |
| `slice` | `s a n -- str` | n bytes starting at a |
| `split` | `s sep -- l` | cuts at every sep |
| `join` | `l sep -- s` | the items as text, with sep between |
| `reverse` | `s -- str` | back to front |
| `chr` | `n -- s` | a one-byte string |

    "sub" "verge" cat               "subverge"
    "subverge" 3 5 slice            "verge"
    "a,b,,c" "," split              [ "a" "b" "" "c" ]
    [ "a" "b" "c" ] "-" join        "a-b-c"
    [ 1 2 3 ] ", " join             "1, 2, 3"
    "abc" reverse                   "cba"

`slice` never fails: a start before 0 counts as 0, and a length that runs past the end, or a negative one, takes the rest of the string. `split` keeps empty fields, so splitting on a separator that appears twice in a row gives an empty string between them. It cuts into at most 256 parts. An empty separator returns the whole string as the only item.

`join` writes each item as `say` would, so strings appear without quotes and numbers as digits. The result can be up to 64 KiB.

`at` and `each` give byte values, not one-character strings:

    "hi" 0 at                       104
    "hi" [ chr say ] each           h and i on separate lines

## Searching

| Word | Effect | |
|---|---|---|
| `find` | `s sub -- i` | where sub first starts, or -1 |
| `starts?` | `s pre -- 1/0` | whether s begins with pre |
| `ends?` | `s suf -- 1/0` | whether s ends with suf |
| `replace` | `s a b -- str` | every a replaced by b |

    "hello" "l" find                2
    "hello" "z" find                -1
    "notes.om" ".om" ends?          1
    "a-b-c" "-" " " replace         "a b c"

`replace` works left to right on non-overlapping matches, so `"aaa" "aa" "b" replace` gives `"ba"`. An empty search string fails with `replace needs something to look for`. All searching is case-sensitive; compare `lower` copies for a case-insensitive match.

## Case and spaces

| Word | Effect | |
|---|---|---|
| `upper` | `s -- str` | a to z become capitals |
| `lower` | `s -- str` | A to Z become small letters |
| `trim` | `s -- str` | drops spaces, tabs and newlines at both ends |
| `pad` | `s n -- str` | spaces up to n wide, negative pads on the left |

Only ASCII letters change case. `pad` returns the string unchanged when it is already at least `n` wide, and widths above 4096 are ignored. `fmt` pads any value, which is how the bundled programs line up columns:

    "7" -3 pad                      "  7"
    42 6 fmt                        "42    "

## Converting

| Word | Effect | |
|---|---|---|
| `str` | `v -- s` | any value as text |
| `>num` | `s -- n` | reads a number |
| `hex` | `n -- s` | hex digits without `0x` |
| `>hex` | `n -- s` | hex with `0x` |
| `>sym` | `s -- 'x` | a name from a string |

`str` writes a value the way `say` prints it, so `"hi" str` is `"hi"` itself and `[ 1 2 ] str` is `"[ 1 2 ]"`. For values other than bignums the result is cut at 511 bytes.

`>num` reads exactly one number written as source: decimal, `0x` hex, `0b` binary, negative, or a bignum. Spaces around the number are fine, and anything else fails with `not a number: ...`:

    "how many? " ask >num

`hex` of a negative number shows its 64-bit two's complement, so `-1 hex` is `"ffffffffffffffff"`.

## Comparing and sorting

`=` compares strings by content. `<` and `>` take numbers only, but `sort` orders a list of strings by byte value:

    [ "pear" "Fig" "apple" ] sort   [ "Fig" "apple" "pear" ]

Capitals sort before small letters, since that is their order in ASCII. Sort a `lower` copy, or build pairs of a key and a value, when that matters.

`has` and `index` find a string in a list by content.

## Running a string

    "1 2 +" eval                    3

`eval` parses the string and runs it in the current context, so it sees the caller's locals and definitions, and a syntax error in it fails the line. It is how `learn` runs what you type into its exercises.

## Reading input

`ask` prints a prompt and reads one line with the usual editing keys, without history or completion. Ctrl-C fails it with `cancelled`.

    "your name? " ask { n } "hi, " n cat say
