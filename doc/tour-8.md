# Part 8: Big numbers and infinity

Integers that don't fit in 64 bits turn into bignums, up to 2^65536, and compiled code does the same:

    2 100 ^ .
    2 1000 ^ str len .

`omega`, the first infinite ordinal, is a value, and so is `aleph0`, the size of the natural numbers:

    omega 1 + .
    1 omega + .
    omega 2 * .
    2 omega * .
    omega omega ^ .

The first line is the number after every natural number. The second is `ω`, because adding one element in front of ω changes nothing. Ordinals add and multiply in a fixed order, and the order matters.

`count` counts upward through the finite numbers and on past them. Click to run it, and press `q` to leave:

[count](>count)   [ord-demo](>ord-demo)   [diagonal](>diagonal)   [pairing](>pairing)   [supertask](>supertask)

See [numbers](numbers), [bignums](bignums), [ordinals](ordinals), [cardinals](cardinals) and [aleph-null](aleph-null).

[Next: Part 9](tour-9)   [Back: Part 7](tour-7)   [The tours](tour)
