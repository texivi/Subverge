# Part 5: Flow, lists and strings

Conditions and loops take quotes:

    5 3 > [ "bigger" say ] [ "smaller" say ] if
    10 [ "hi" say ] times
    0 5 [ . ] for
    3 [ dup 0 > ] [ dup . 1 - ] while drop

A quote is also a list:

    0 5 range .
    [ 1 2 3 ] [ sq ] map .
    [ 1 2 3 4 ] [ 2 mod 0 = ] filter .
    [ 3 1 2 ] len .

Strings:

    "subverge" upper say
    "a,b,c" "," split .
    "x = " 42 str cat say

See [flow](flow), [quotes](quotes) and [strings](strings). Every word on these lines has a page. Click a name to read it: [if](?if), [times](?times), [for](?for), [while](?while), [range](?range), [map](?map), [filter](?filter), [split](?split), [cat](?cat).

[Next: Part 6](tour-6)   [Back: Part 4](tour-4)   [The tours](tour)
