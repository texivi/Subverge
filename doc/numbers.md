# Numbers

omega's finite numbers are exact integers. Up to 64 bits they are stored directly in the value, and beyond that they become bignums, up to 2^65536. For infinite values, see [ordinals](ordinals.md) and [cardinals](cardinals.md).

## Literals

    42   -7   0x2a   0b101010   -0x10

Integers can be written in decimal, in hex with `0x` or in binary with `0b`, with an optional minus sign. Hex digits can be either case. A literal too big for 64 bits is read as a bignum:

    123456789012345678901234567890 .

`bases` prints a number in decimal, hex, octal and binary ([tools](tools.md)).

## Arithmetic

| Word | Effect | |
|---|---|---|
| `+` | `a b -- c` | adds |
| `-` | `a b -- c` | subtracts |
| `*` | `a b -- c` | multiplies |
| `/` | `a b -- c` | divides, rounding toward zero |
| `mod` | `a b -- r` | the remainder, with the sign of a |
| `^` | `a b -- c` | a to the power b |
| `neg` | `a -- -a` | flips the sign |
| `abs` | `a -- b` | drops the sign |
| `min` | `a b -- c` | the smaller |
| `max` | `a b -- c` | the bigger |
| `sqrt` | `n -- r` | the square root, rounded down |
| `gcd` | `a b -- c` | the greatest common divisor |
| `inc` | `n -- n+1` | adds one |
| `dec` | `n -- n-1` | subtracts one |
| `sq` | `n -- n*n` | squares |

All of them accept bignums. `+`, `*` and `^` also work on [ordinals](ordinals.md) and [cardinals](cardinals.md), which have their own rules.

    17 5 /          3
    -17 5 /         -3
    -17 5 mod       -2
    2 10 ^          1024
    10 sqrt         3
    84 36 gcd       12

## Division and remainders

`/` rounds toward zero and `mod` takes the sign of the dividend, the same as C. Two consequences catch people out:

- An odd test written as `2 mod 1 =` is false for negative odd numbers, since `-3 2 mod` is `-1`. Write `2 mod 0 = not`.
- Converting a negative number of seconds into minutes with `/` rounds toward zero, not down.

Division by zero fails with `division by zero`. Dividing the most negative 64-bit integer by `-1` gives a bignum rather than overflowing.

## Powers and roots

`^` raises to a non-negative whole power, exactly. Negative exponents fail with `negative powers aren't supported`. A power whose result would pass 2^65536 fails before it is computed, so `2 70000 ^` fails at once instead of running out of memory.

`sqrt` rounds down and works on bignums: `2 200 ^ sqrt` is exactly `2 100 ^`. Negative numbers fail. `gcd` always returns a non-negative result, and `0 0 gcd` is `0`.

## Big numbers

Integers never wrap. A result that doesn't fit in 64 bits becomes a bignum, and a bignum result that fits again becomes a plain integer:

    2 100 ^ .                   1267650600228229401496703205376
    2 100 ^ 2 100 ^ - .         0
    2 1000 ^ str len .          302

Code doesn't see the switch. Every arithmetic and comparison word accepts both kinds, and so do `str`, `>num`, `min`, `max`, conditions and saving. Compiled code checks for overflow after every addition, subtraction and multiplication and calls the bignum code when it happens ([JIT](jit.md)).

Words that need a machine integer, such as `times`, `at`, `peek`, `alloc` and the bitwise words, fail with `name needs a number that fits in 64 bits`. In a condition a bignum is always true, since a bignum is never zero.

The limit is 2^65536, about 19,700 decimal digits. Larger results fail with `that number is too big, the limit is 2^65536`. See [bignums](bignums.md) for the representation and the algorithms.

## Comparisons and truth

| Word | Effect | |
|---|---|---|
| `=` | `a b -- 1/0` | equal |
| `<` `>` `<=` `>=` | `a b -- 1/0` | ordering |
| `and` `or` | `a b -- 1/0` | logical |
| `not` | `a -- 1/0` | logical not |

Comparisons leave `1` or `0`. Conditions treat `0` as false and every other number as true. `and` and `or` evaluate both sides, since both are already on the stack, and always leave `1` or `0`.

`=` works on any two values: numbers by value, strings by content, everything else by identity. The ordering words accept numbers, ordinals and cardinals.

## Bits

| Word | Effect | |
|---|---|---|
| `&` | `a b -- c` | and |
| `\|` | `a b -- c` | or |
| `xor` | `a b -- c` | exclusive or |
| `~` | `a -- b` | not |
| `<<` | `a n -- b` | shift left |
| `>>` | `a n -- b` | shift right, filling with zeros |

The bitwise words work on 64-bit integers only. Shift counts are taken modulo 64, and `>>` is a logical shift, so `-1 60 >>` is `15`. Note the difference between `and`, which is logical, and `&`: `12 10 and` is `1`, but `12 10 &` is `8`.

## Random numbers

`n rand` returns a number from 0 to n-1, from a xorshift generator seeded from the CPU's cycle counter at the first call. Use it for games and shuffling, not for secrets: its whole state is one 64-bit number. `rand` fails with `rand needs a number above 0` for zero or less.

    6 rand 1 + .            a dice roll

## Converting

`str` and `hex` turn numbers into text, `>hex` adds the `0x`, and `>num` reads a number back from a string ([strings](strings.md)). `fmt` pads a number into a column.

## Fractions

Floating point is on the [roadmap](roadmap.md). Until then, fractional values are written as fixed point: multiply by a scale, keep integers, and divide when printing.

    1999 3 * .                  5997, which is 59.97 at a scale of 100
    5997 100 / .                59
    5997 100 mod .              97

The bundled programs do this: the unit conversions in `convert` work in hundredths, and `mandel` uses 65536 as 1.0. `calc` is a separate calculator that works with six decimal places ([tools](tools.md)).
