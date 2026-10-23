# Quotes

A quote is code in square brackets that is pushed instead of run. The same value is also a list, and omega has no separate list type: `[ 1 2 3 ]` is a quote that pushes three numbers when run, and a list of three numbers to every list word. For the words that run quotes, see [flow](flow.md).

## Quotes as code

    [ 1 2 + ]           pushes a quote
    [ 1 2 + ] do        runs it, leaving 3

`if`, `times`, `map` and the other control words are ordinary words that take quotes and decide when to run them. A defined word is a quote bound to a name, and calling the name runs the quote:

    [ dup * ] :sq
    'sq get             pushes the quote itself
    'sq get do          the same as sq

Quotes nest freely and are values like any other, so they can be stored in definitions, kept in lists, passed to words and returned from them:

    [ [ 2 * ] [ 1 + ] ] :steps
    5 steps [ do ] each .           11

## Quotes as lists

    [ 1 2 + ] len .                 3
    [ 1 2 + ] 2 at .                '+

Inside a quote, a word is stored as its name. When a list word hands out such an item, it turns it into a symbol, so `[ 1 2 + ] 2 at` gives `'+` instead of running `+`. Definitions and local assignments come out the same way: `[ :x ] first` is `'x`.

A list is written as a quote of values:

    [ 1 "two" [ 3 ] 'four ]

## Building lists

| Word | Effect | |
|---|---|---|
| `range` | `a b -- l` | the integers from a up to b-1 |
| `list` | `... n -- l` | the top n values as a list |
| `append` | `l x -- list` | x added at the end |
| `cat` | `a b -- c` | two lists joined |
| `reverse` | `l -- list` | back to front |
| `sort` | `l -- list` | numbers or strings in order |

    0 5 range                       [ 0 1 2 3 4 ]
    1 2 3 3 list                    [ 1 2 3 ]
    [ 1 2 ] 3 append                [ 1 2 3 ]
    [ 1 ] [ 2 3 ] cat               [ 1 2 3 ]
    [ 5 2 9 2 ] sort                [ 2 2 5 9 ]

`range` holds at most 100000 numbers and fails with `that range is too big` when `b` is below `a`. `0 0 range` is the empty list.

Lists are immutable, and each of these words builds a new list. `append` copies the whole list, so building a long list one item at a time with `append` takes time proportional to the square of its length. For more than a few thousand items, push the items onto the stack and pack them with `depth list`, or use `map` over a `range`.

`sort` takes a list of numbers, bignums included, or a list of strings, and fails with `sort needs a list of numbers or a list of strings` for anything else or a mix. It is a stable merge sort, so equal items keep their order.

## Reading lists

| Word | Effect | |
|---|---|---|
| `len` | `l -- n` | the number of items |
| `at` | `l i -- x` | item i, counting from 0 |
| `first` | `l -- x` | the first item |
| `last` | `l -- x` | the last item |
| `rest` | `l -- list` | everything after the first |
| `spread` | `l -- ...` | every item onto the stack |
| `has` | `l x -- 1/0` | whether x is in the list |
| `index` | `l x -- i` | where x first appears, or -1 |

`at` fails with `that position is outside the list` for an index past either end, and `first` and `last` fail with `that list is empty`. `rest` of an empty list is the empty list.

`has` and `index` compare integers by value and strings by content. Every other value, including bignums and quotes, is compared by identity, so `[ [ 1 ] ] [ 1 ] has` is `0`: the two `[ 1 ]` are different objects.

## Computing over lists

| Word | Effect | |
|---|---|---|
| `each` | `l q --` | runs q with each item pushed |
| `map` | `l q -- list` | the results of q on each item |
| `filter` | `l q -- list` | the items for which q leaves a true value |
| `sum` | `l -- n` | all items added |
| `fold` | `l x q -- y` | q applied to x and each item in turn |

    [ 1 2 3 ] [ . ] each                    prints 1, 2, 3
    [ 1 2 3 ] [ dup * ] map                 [ 1 4 9 ]
    1 11 range [ 2 mod 0 = ] filter         [ 2 4 6 8 10 ]
    [ 1 2 3 4 ] sum                         10
    [ 1 2 3 4 ] 1 [ * ] fold                24
    [ "a" "b" ] "" [ swap cat ] fold        "ba"

`map`'s quote must leave exactly one value for each item, or `map` fails with `map's quote has to leave exactly one value`. `filter`'s quote must leave a number, and items for which it is non-zero are kept. `each` also works on a string, pushing each byte.

`sum` adds with `+`, so it works on bignums and on ordinals: `omega 1 2 list sum` is `ω+1`. Writing `[ omega 1 ]` instead would give a list holding the symbol `'omega`, since words inside a list stay words. The sum of an empty list is 0.

`fold` keeps an accumulator. It starts as `x`, and for each item `q` runs with the accumulator and the item on the stack and leaves the new accumulator. `fold` is written in omega with the locals `l`, `acc` and `q`, and because locals are dynamically scoped, a quote passed to it that uses those three names as its own free variables sees `fold`'s instead ([names](names.md)). Rename them in your quote or use `each`.

All of these check for Ctrl-C between items, and all of them stop at the first failing item.

## Lists of lists

Nested lists work as records and tables:

    [ [ "ann" 31 ] [ "bob" 27 ] ] :people
    people [ first ] map                    [ "ann" "bob" ]
    people [ 1 at 30 > ] filter             [ [ "ann" 31 ] ]
    people [ spread { name age } name say ] each

`pci`, `memmap`, `drives` and `tab` return their results in this form ([machine](machine.md)).

## Limits

A single pair of brackets can hold 256 items as typed, and brackets nest 32 deep. Lists built at run time have no such limit. Unbalanced brackets fail the whole line before anything runs ([errors](errors.md)).
