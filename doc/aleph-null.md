# ℵ₀

ℵ₀ (`aleph0`) is the size of the set of natural numbers 0, 1, 2, 3 and so on. It is the smallest infinite size. This page covers what it means for two infinite sets to have the same size, which sets have size ℵ₀, and what happens when ℵ₀ is added, multiplied and compared. The size of a set is a [cardinal](cardinals.md), and a position in a list is an [ordinal](ordinals.md).

## Size by pairing

For finite sets, two sets have the same size when counting them gives the same number. Counting doesn't finish for a set that never ends, so the definition is replaced by one that agrees with counting on finite sets and also works on infinite ones: two sets have the same size when every member of one can be given exactly one partner in the other, with nobody left over on either side.

Seven chairs and seven people have the same size under both definitions. Only the second one applies to the natural numbers and the even numbers. Give each natural number n the partner 2n:

```text
0 1 2 3 4  5 ...
0 2 4 6 8 10 ...
```

Every natural number has one partner, and every even number is the partner of exactly one natural number. By the definition, the natural numbers and the even numbers have the same size, although the even numbers are only a part of the natural numbers. A part of a finite set is always smaller than the whole. A part of an infinite set can be as large as the whole.

`pairing` prints the first rows of this match, with the squares and the whole numbers next to it ([demos](demos.md)).

## The size of the naturals

ℵ₀ is the size of the natural numbers, and by the definition above it is also the size of every set that can be paired off with them. Such a set is countable. Its members can be written in a list where each one has a position: the member paired with 0 first, the one paired with 1 second, and so on. Every member is reached after finitely many steps, although the list has no last position.

    aleph0 .                ℵ₀
    omega card .            ℵ₀

`card` turns an ordinal into the size of the set of positions before it. The first infinite ordinal ω is the position after every natural number, so the positions before it are the natural numbers, and their size is ℵ₀.

## Sets of size ℵ₀

All of these can be paired off with the natural numbers:

- The even numbers, the odd numbers, the squares and the multiples of any number, as above.
- The whole numbers 0, 1, −1, 2, −2 and so on, taking the positive and the negative ones in turns.
- Pairs of natural numbers. The word `pair` gives each pair its own number and `unpair` goes back ([countable-sets](countable-sets.md)).
- The positive fractions. The word `rational` gives the n-th one ([countable-sets](countable-sets.md)).
- Finite lists of natural numbers, and with them every text that can be typed, since a text is a finite list of character codes.

Several of these look larger than the natural numbers. There are infinitely many fractions between any two fractions. Under the definition of size they are all the same.

## Arithmetic

Adding two sizes gives the size of the two sets put side by side, and multiplying gives the size of the set of pairs. For infinite sizes the result is the larger of the two:

    5 aleph0 + .            ℵ₀
    aleph0 aleph0 + .       ℵ₀
    aleph0 aleph0 * .       ℵ₀
    aleph0 3 ^ .            ℵ₀

Five more members, or as many more again, or a whole copy of the set for each member of the set, all leave a set that can be paired off with the natural numbers. The [hotel](hilbert-hotel.md) goes through the first three.

Subtraction and division have no single answer. ℵ₀ − ℵ₀ could be 0, 5 or ℵ₀, depending on which part is taken away, and omega doesn't define them (`cardinals only support + * and ^`).

## Order and position

Sizes and positions are different questions with different answers. In a list, ω is the position after all of 0, 1, 2 and so on, ω+1 is the position after that, and ω·2 is the position after a second copy of the naturals. These are all different positions. Each has only countably many positions before it, so each has the size ℵ₀:

    omega 1 + card .        ℵ₀
    omega 2 * card .        ℵ₀
    omega omega ^ card .    ℵ₀

The split shows in addition too. A member added in front of a list of the naturals gives a list that has the same shape, so `1 omega +` is `ω`. A member added after all of them gives a list with a last position, so `omega 1 +` is `ω+1`, and the two orders are different. The sizes of both are ℵ₀.

    1 omega + omega = .     1
    omega 1 + omega = .     0

## What is larger

Some sets can't be paired off with the natural numbers, whatever the pairing. The real numbers are one, and so is the set of all subsets of the natural numbers. Both have the size 2^ℵ₀, which is larger than ℵ₀:

    aleph0 continuum < .    1

The proof is in [uncountable-sets](uncountable-sets.md). The next infinite size after ℵ₀ is ℵ₁, and whether ℵ₁ equals 2^ℵ₀ is not settled by the usual axioms of set theory ([cardinals](cardinals.md)).

## In omega

| Word | Effect |
|---|---|
| `aleph0` | pushes ℵ₀ |
| `n aleph` | pushes ℵₙ |
| `n card` | the size of an ordinal |
| `a b pair` | one number for the two |
| `n unpair` | the two numbers back |
| `n rational` | the n-th positive fraction |
| `hotel`, `rationals`, `pairing`, `diagonal` | demos |

The [learn](learn.md) lessons from "Same size" to "Alephs" go through the same material with lines to run.
