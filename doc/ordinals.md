# Ordinals

omega has transfinite ordinals as ordinary values. `omega` pushes the first infinite ordinal, and ordinals can be added, multiplied, raised to powers, compared, stored, printed and saved like any number. For cardinals, see [cardinals](cardinals.md).

## What they are

Watch Vsauce's [How To Count Past Infinity](https://www.youtube.com/watch?v=SrU9YDoXE88) first. The infinite values in Subverge come from it, and the rest of this page assumes the idea it starts with: a number can say how many things there are, or where something is in a line, and past the finite those are different questions. Ordinals answer the second one. [Cardinals](cardinals.md) answer the first.

The natural numbers 0, 1, 2, ... never end, and `omega` is the position right after all of them:

    1000000 omega < .       1
    2 1000 ^ omega < .      1

No integer reaches it, however big. After ω, counting carries on as usual, and every step is a value you can type:

    omega 1 + .             ω+1
    omega omega + .         ω·2
    omega omega * .         ω²
    omega omega ^ .         ω^ω

The order goes 0, 1, 2, ..., ω, ω+1, ..., ω·2, ..., ω², ..., ω^ω, ..., ε₀, ..., ω₁. Run `count` to watch it go by on screen ([demos](demos.md)).

## Literals

| Word | Value | Printed |
|---|---|---|
| `omega` | the first infinite ordinal | `ω` |
| `epsilon0` | the first ordinal with ω^x = x | `ε₀` |
| `omega1` | the first uncountable ordinal | `ω₁` |

`epsilon0` is the limit of ω, ω^ω, ω^ω^ω and so on, so `omega epsilon0 ^` is `epsilon0` again. `omega1` comes after every ordinal that can be built from `omega` and `epsilon0`, and it is the first one with uncountably many ordinals below it.

Every other ordinal is built from these and the natural numbers with `+`, `*` and `^`. Non-negative integers act as finite ordinals when combined with an ordinal, and a result that is finite comes back as a plain integer.

## Addition

Addition is not commutative. `a b +` is the sequence `a` followed by the sequence `b`:

    omega 1 + .             ω+1
    1 omega + .             ω

Think of `1 omega +` as one element placed in front of a line of ω elements. The new element takes position 0, the old position 0 moves to 1, and so on. Every element still has a natural-number position, and the line still has no last element, so the result has the same shape as ω and is equal to it. `omega 1 +` places one element after the whole line. That element sits at position ω, the line now has a last element, and ω has none, so ω+1 is a different, larger ordinal.

In general, in `a b +`, every part of `a` smaller than the leading part of `b` is absorbed.

    omega 5 + omega + .     ω·2
    omega 2 ^ omega + .     ω²+ω
    omega omega 2 ^ + .     ω²

## Multiplication

`a b *` is `b` copies of `a` placed one after another, and it isn't commutative either:

    omega 2 * .             ω·2, two copies of ω
    2 omega * .             ω, ω copies of a pair
    omega 1 + 2 * .         ω·2+1
    omega omega * .         ω²

`2 omega *` is ω copies of a pair of elements in a row. That line is 0, 1, 2, 3, ... again, so it is ω. `omega 2 *` is a line of ω followed by a second line of ω, which has a limit position in the middle and is ω·2. Multiplying by a finite number on the right multiplies the leading coefficient and keeps the rest. Multiplying by an infinite ordinal on the right adds to the exponents instead.

## Powers

    omega 2 ^ .             ω²
    omega omega ^ .         ω^ω
    2 omega ^ .             ω
    omega 1 + 2 ^ .         ω²+ω+1
    omega epsilon0 ^ epsilon0 = .       1

A finite base raised to an infinite power becomes a power of ω: 2^ω is the limit of 2, 4, 8, ..., which is ω. The last line is the defining property of ε₀: it is the first fixed point of x ↦ ω^x.

A power with a finite part in the exponent is computed by repeated multiplication, and a finite part above 64 fails with `that exponent is too big to write out`. `omega omega 100 + ^` fails this way, while `omega omega ^` is fine.

## Comparison

All comparison words work and follow the ordinal order:

    5 omega < .                     1
    omega omega 1 + < .             1
    1 omega + omega = .             1
    omega 2 ^ omega omega ^ < .     1
    epsilon0 omega1 < .             1

Every finite number, bignums included, is below every infinite ordinal. Negative numbers compare as smaller than all ordinals.

`=` on two ordinals compares them by value, so `1 omega + omega =` is `1` even though they were built differently.

## What doesn't exist

Ordinals have no subtraction, division or negatives, and the attempts fail:

    omega 1 -           ordinals only support + * and ^
    omega -3 +          ordinals are never negative

Bignums mix with ordinals only where they would be absorbed anyway: `2 100 ^ omega +` is ω, but `omega 2 100 ^ +` fails with `that number is too big to mix with an ordinal that way`, since ω followed by 2^100 elements can't be written with a 64-bit coefficient.

## Testing and converting

    omega ordinal? .        1
    5 ordinal? .            0
    omega 2 * card .        ℵ₀
    omega1 card .           ℵ₁

`ordinal?` is true only for infinite ordinals, since finite ones are plain integers. `card` gives the cardinality of an ordinal: `aleph0` for every countable ordinal and `aleph1` for `omega1` and everything built on it ([cardinals](cardinals.md)).

## Printing and saving

On screen, ordinals print in Cantor normal form with the glyphs the kernel adds to the font: `ω`, a middle dot for coefficients, `²` for a square and `^` for other exponents, with brackets where an exponent has several parts:

    omega 2 * 3 + .                 ω·2+3
    omega omega 2 * ^ .             ω^(ω·2)

`.` and `say` print this form, and `str` gives the same text. When a tab is [saved](saving.md), ordinals are written back as omega code, so `ω·2+3` is saved as `omega 2 * 3 +` and read back exactly.

[How infinity works](infinity.md) shows the representation and the algorithms.

## Using them

Ordinals are mostly a demonstration, but they are real values that programs can use. A program can count past the end of a loop, use ω as an "unbounded" marker that compares above every number, or build termination arguments, where each step must decrease an ordinal. `count`, `ord-demo` and `'show see` are short examples.
