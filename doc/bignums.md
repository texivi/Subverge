# Bignums

An integer that doesn't fit in 64 bits becomes a bignum, and every arithmetic word keeps working on it. The sections below cover how bignums are stored and computed. For the words and the rules, see [numbers](numbers.md).

## Small and big

A number that fits in a signed 64-bit integer is stored directly in the value cell, with no allocation ([values and objects](values.md)). When a result needs more, it becomes a bignum, and whenever a bignum result fits in 64 bits again, it is turned back into a plain integer. Every number therefore has exactly one representation:

    2 64 ^ 2 64 ^ - .           0, a plain integer
    2 63 ^ 1 - .                9223372036854775807, still plain
    2 63 ^ .                    a bignum

Code that only ever sees small numbers never touches the bignum code, and `=` can compare a bignum with a plain integer by type alone: if one is big and the other isn't, they are different numbers.

## Representation

A bignum is a heap object holding a sign and an array of 32-bit digits, least significant first, with no leading zero digits. The [garbage collector](gc.md) frees it like any other object. The digit array holds at most 2048 digits, which is 65,536 bits; that is where the limit of 2^65536 comes from.

## Cost

Arithmetic on bignums is the schoolbook kind, so multiplying two numbers whose product is near the limit takes a fraction of a second in QEMU. Printing is quadratic in the length too, so `2 65535 ^ .` pauses briefly before its nearly 20,000 digits appear.

## The limit

A result that would need more than 65,536 bits fails with `that number is too big, the limit is 2^65536`. For powers the check is made before any work, from the bit length of the base and the exponent, so `2 100000 ^` fails at once instead of computing a huge intermediate result. The same limit applies to literals and to `>num`.

## Where they work

Arithmetic, `neg`, `abs`, `min`, `max`, `sqrt`, `gcd`, comparisons, conditions, `str`, `>num`, `sort`, `sum` and saving all take bignums.

Words that need a machine integer, such as `times`, `at`, `range`, `alloc`, `peek` and the bitwise words, fail with `name needs a number that fits in 64 bits`. In conditions a bignum is always true, since it is never zero.

`has` and `index` compare bignums by identity, not by value, so look them up with `=` in a `filter` instead.

With ordinals, a bignum works on the left of `+` and `*`, where it is absorbed: `2 100 ^ omega +` is ω. On the right it fails with `that number is too big to mix with an ordinal that way`.

## Bignums and the JIT

Compiled code does `+`, `-`, `*` and `neg` in registers and checks the CPU's overflow flag after each one. On overflow, it jumps to a slow path that calls the ordinary word, which builds the bignum, so compiled code and the interpreter always agree. Once a value is a bignum, every inline operation on it takes the slow path, since the fast path only handles plain integers. See [JIT](jit.md).

## Saving

Bignums are saved as their decimal digits and parsed back at boot ([saving](saving.md)). A tab holding many large numbers can grow past its 1 MiB slot, since each one takes up to about 20 KB of text.
