# Cardinals

Cardinals measure the size of a set, and [ordinals](ordinals.md) measure positions in an ordering. omega keeps cardinals as their own type, separate from ordinals and from integers.

## What they are

Two sets have the same size when their members can be paired off one to one. Pair each natural number n with the even number 2n and every member of both sets gets exactly one partner, so there are as many even numbers as natural numbers. That size is ℵ₀, and omega agrees:

    aleph0 aleph0 + .       ℵ₀
    aleph0 aleph0 * .       ℵ₀

Positions and sizes split apart here. ω, ω+1 and ω·2 are different places in the line, but each one has only countably many places before it, so all three have size ℵ₀:

    omega 2 * card .        ℵ₀

Some sets are bigger. Cantor's diagonal argument shows that the subsets of the natural numbers can't be paired off with the natural numbers, so 2^ℵ₀, the `continuum`, is larger than ℵ₀. `diagonal` runs the argument on screen ([demos](demos.md)). ℵ₁ is the next size after ℵ₀, and whether it equals the continuum is the continuum hypothesis, which omega refuses to answer (see below).

The pages [aleph-null](aleph-null.md), [countable-sets](countable-sets.md), [hilbert-hotel](hilbert-hotel.md) and [uncountable-sets](uncountable-sets.md) go through the same ideas in more detail. Vsauce's [How To Count Past Infinity](https://www.youtube.com/watch?v=SrU9YDoXE88) explains all of this with pictures.

## Literals

| Word | Value | Printed |
|---|---|---|
| `aleph0` | the size of the natural numbers | `ℵ₀` |
| `aleph1` | the next infinite size | `ℵ₁` |
| `n aleph` | the n-th infinite size | `ℵₙ` |
| `continuum` | the size of the real numbers, 2^ℵ₀ | `2^ℵ₀` |

    3 aleph .               ℵ₃
    12 aleph .              ℵ_12

`aleph` takes any non-negative 64-bit number. Above 9 the index is printed after an underscore, since the font has subscript digits only up to 9. The index has to be a finite number, so ℵ_ω and the alephs above it can't be written in omega, even though set theory has them.

The other values omega can represent are powers of two: `2 n aleph ^` is 2^ℵₙ.

## Addition and multiplication

For infinite cardinals, sums and products are the larger of the two:

    aleph0 aleph0 + .       ℵ₀
    aleph0 aleph0 * .       ℵ₀
    3 aleph0 + .            ℵ₀
    aleph0 aleph1 + .       ℵ₁
    aleph0 continuum * .    2^ℵ₀
    0 aleph1 * .            0

Adding a finite number to an infinite cardinal doesn't change it, and multiplying by `0` gives `0`. When the larger of the two can't be determined in set theory, the operation fails with the ZFC error described below: `aleph2 continuum +` is either ℵ₂ or 2^ℵ₀, and ZFC can't say which.

## Powers

Only these powers are supported:

| Form | Result |
|---|---|
| `n aleph_k ^`, with n ≥ 2 finite | 2^ℵₖ |
| `aleph_k n ^`, with n ≥ 1 finite | ℵₖ |

    2 aleph0 ^ .            2^ℵ₀
    10 aleph0 ^ .           2^ℵ₀
    aleph0 3 ^ .            ℵ₀
    2 aleph0 ^ continuum = .    1

Every finite base from 2 up gives the same power, since nᵏ and 2ᵏ have the same size for infinite k. A cardinal raised to a finite power is itself.

Everything else, such as `aleph0 aleph0 ^`, fails with `that cardinal power isn't supported`. Several of these have answers in ZFC (ℵ₀^ℵ₀ is 2^ℵ₀), but omega only implements the two rules above.

## Comparison

    aleph0 aleph1 < .               1
    aleph0 continuum < .            1
    aleph1 continuum <= .           1
    2 aleph0 ^ 2 aleph1 ^ <= .      1

Comparisons follow what ZFC can prove:

- ℵⱼ < ℵₖ exactly when j < k.
- ℵₖ < 2^ℵₖ always, by Cantor's theorem. The `diagonal` demo shows the proof ([demos](demos.md)).
- ℵₖ₊₁ ≤ 2^ℵₖ always, but whether they are equal is undecidable.
- 2^ℵⱼ ≤ 2^ℵₖ for j < k, but whether the inequality is strict is undecidable.
- ℵⱼ compared with 2^ℵₖ for j > k+1 is undecidable either way.

Every finite number is smaller than every infinite cardinal.

## The continuum hypothesis

The continuum hypothesis says that no size lies strictly between the natural numbers and the real numbers, that is, `aleph1` equals `continuum`.

    aleph1 continuum =

fails with

    ZFC can't decide this one (continuum hypothesis, Godel 1940 and Cohen 1963)

The hypothesis is independent of ZFC: Gödel showed in 1940 that ZFC can't disprove it, and Cohen showed in 1963 that ZFC can't prove it. omega doesn't assume an answer. A comparison whose result depends on it, or on the generalised hypothesis for higher alephs, fails with the same message, and since failing fails the line, nothing a program computes ever rests on an assumption it didn't make.

Comparisons that come out the same either way still work. `aleph1 continuum <=` is `1`, and so is `aleph1 continuum > not`.

## Ordinals and cardinals

Ordinals and cardinals can't be mixed directly:

    omega aleph0 +          can't mix cardinals with that

Convert an ordinal with `card` first:

    omega 2 * card .        ℵ₀
    epsilon0 card .         ℵ₀
    omega1 card .           ℵ₁

Finite numbers work with both types.

Subtraction and division fail with `cardinals only support + * and ^`.

## Saving

Cardinals are saved as source, `0 aleph` for ℵ₀ and `2 0 aleph ^` for 2^ℵ₀, and read back exactly ([saving](saving.md)).
