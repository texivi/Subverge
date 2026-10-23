# How infinity works

The [ordinals](ordinals.md) and [cardinals](cardinals.md) pages cover what you can do with infinite values. The sections below cover how the kernel stores them and how each operation is computed.

## Storing an ordinal

Every ordinal below ε₀ can be written in exactly one way as a sum of powers of ω, largest first, each with a whole-number coefficient:

    ω^a·c + ω^b·d + ... + n

This is Cantor normal form. The kernel stores an infinite ordinal as a list of terms in this order, and each term is a pair: an exponent and a coefficient. The exponent is itself an ordinal, stored the same way, or nothing for a finite term. The coefficient is a 64-bit unsigned number.

    omega 2 * 3 +           ω·2+3       [ (1, 2) (0, 3) ]
    omega omega ^           ω^ω         [ (ω, 1) ]
    omega 2 ^ omega +       ω²+ω        [ (2, 1) (1, 1) ]

Because the exponents are ordinals too, an ordinal is a small tree. `omega omega omega ^ ^` is a term whose exponent is a term whose exponent is ω.

A finite result is never kept as an ordinal. When an operation ends up with no infinite terms, it returns a plain integer, so `5 ordinal?` is `0` and finite ordinals work with every integer word.

## ε₀ and ω₁

ε₀ can't be written in Cantor normal form with smaller exponents, since ε₀ = ω^ε₀. The kernel keeps `epsilon0` and `omega1` as two fixed atoms. Inside a calculation, an atom acts like a single term ω^x·1 whose exponent is the atom itself. That is correct for both: ω^ε₀ = ε₀ and ω^ω₁ = ω₁. So `epsilon0 1 +` is the list `[ (ε₀, 1) (0, 1) ]`, printed as `ε₀+1`, and `omega epsilon0 ^` simplifies straight back to `ε₀`.

ω₁ compares above every ordinal that can be built from `omega` and `epsilon0`, and ε₀ compares above every ordinal built from `omega` alone. omega has no way to write an ordinal between them other than by building on ε₀.

## Comparing

Two ordinals are compared term by term from the front. The one whose first exponent is larger is larger. If the exponents are equal, the larger coefficient wins, and if those are equal too, the next term decides. When one list runs out first, it is the smaller ordinal.

    omega 2 ^ omega 5 * <       compares ω² with ω·5: exponent 2 beats 1, so 0
    omega 3 * omega 2 * 9 + >   compares ω·3 with ω·2+9: coefficient 3 beats 2, so 1

Exponents are compared with the same rule, so comparison recurses into the tree. Every integer is below every infinite ordinal, and negative integers compare below all of them.

## Adding

`a b +` keeps every term of `a` whose exponent is larger than the first exponent of `b`, drops every term of `a` whose exponent is smaller, and appends all of `b`. A term of `a` with the same exponent as the first term of `b` has its coefficient added to it.

    omega 5 + omega +           ω+5 then ω: the 5 has exponent 0 < 1 and is dropped, so ω·2
    omega 2 ^ omega 3 * + omega 2 * +     ω²+ω·3 then ω·2: ω·3 merges into ω·5, so ω²+ω·5

This is the absorption rule from the [ordinals](ordinals.md) page, done on lists.

## Multiplying

`a b *` is computed one term of `b` at a time, and the pieces are added up left to right.

- A finite term `n` of `b` gives `a` with its first coefficient multiplied by `n`. The rest of `a` is unchanged.
- An infinite term ω^e·c of `b` gives the single term ω^(x+e)·c, where x is the first exponent of `a`. Everything after the first term of `a` disappears.

<!-- -->

    omega 1 + 3 *               ω·3+1
    omega 1 + omega *           ω²
    omega 2 ^ omega 1 + *       ω³+ω²

The last one is the infinite term ω giving ω^(2+1) = ω³, plus the finite term 1 giving ω².

## Powers

A finite base to an infinite power becomes a power of ω. `2 omega ^` is ω, and in general n^(ω^e) is ω^(ω^(e−1)) for a finite exponent e. A finite part at the end of the exponent multiplies the result by n to that power:

    2 omega ^ .                 ω
    2 omega 3 + ^ .             ω·8
    3 omega 2 ^ ^ .             ω^ω

An infinite base is raised by splitting the exponent into its infinite part and its finite part n. The infinite part is done in one step with the rule ω^x to the power y = ω^(x·y), and the finite part is done by multiplying n more times. A finite part above 64 fails with `that exponent is too big to write out`, since every multiplication can double the number of terms.

## Limits

An ordinal has at most 64 terms, and a result that would have more keeps its first 64. Every coefficient has to fit in 64 bits, and an operation whose coefficient would overflow fails with `that coefficient doesn't fit in 64 bits`. A bignum can't be a coefficient, so `omega 2 100 ^ *` fails, while `2 100 ^ omega +` is ω, since the bignum is absorbed ([limits](limits.md)).

## Cardinals

A cardinal fits in a single integer. ℵₖ is stored as k, and 2^ℵₖ as −k−1, so ℵ₀ is 0 and the continuum is −1. Cardinals are never allocated, which makes them as fast as integers.

Addition and multiplication take the larger of the two, and comparisons follow what ZFC can prove. ℵⱼ against ℵₖ and 2^ℵⱼ against 2^ℵₖ compare by index. ℵₖ < 2^ℵₖ always holds. ℵₖ₊₁ ≤ 2^ℵₖ always holds, and whether they are equal fails with the ZFC error. Anything further apart fails the same way. A finite number compares below every cardinal and is absorbed by addition, and `0 *` gives 0.

## Printing and saving

Ordinals and cardinals print with ω, ℵ and subscript digits, which the kernel adds to the [screen](screen.md) font in characters 128 and up. Exponents that are a single term without a coefficient print without brackets, as in `ω^ω`, and `²` is used for a square. Anything longer gets brackets, as in `ω^(ω·2)`.

When a tab is [saved](saving.md), these glyphs can't be read back, so ordinals are written as omega code that rebuilds them. `ω·2+3` is saved as `omega 2 * 3 +`, ε₀ as `epsilon0`, and the continuum as `2 0 aleph ^`. Loading the tab runs that code and gets exactly the same value.
