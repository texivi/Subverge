# Uncountable sets

Some sets can't be paired off with the natural numbers, however the pairing is chosen. Such a set is uncountable, and its size is larger than [ℵ₀](aleph-null.md). This page gives Cantor's argument for the real numbers and for the subsets of a set, and names the words that act it out.

## The diagonal argument

Take any list of real numbers between 0 and 1, written as infinite decimals, one at each position 0, 1, 2 and so on:

```text
0   0.5 1 4 1 5 ...
1   0.3 3 3 3 3 ...
2   0.7 1 8 2 8 ...
3   0.2 5 0 0 0 ...
```

Build a new number from the digits on the diagonal: digit 0 of row 0, digit 1 of row 1, and so on. They are 5, 3, 8, 0. Give the new number digit k equal to 1 when the diagonal digit k is not 1, and 2 when it is. Here that gives 0.1 1 1 1 ... The rule gives a digit that differs from the diagonal digit at every place, and it never uses 0 or 9, so the number has only one decimal expansion.

The new number is between 0 and 1. It is not at position 0, because its digit 0 differs from the digit 0 of row 0. It is not at position 1, because its digit 1 differs from the digit 1 of row 1. For every position k it differs from the number there at digit k, so it is nowhere in the list.

Every list of real numbers therefore misses at least one real number. A list is what a pairing with the natural numbers gives, so no pairing reaches all of the real numbers. `diagonal` draws a table of digits and flips the diagonal on screen ([demos](demos.md)):

    diagonal

## Subsets

The argument works for any set. A subset of the natural numbers can be written as a row of yes and no, with one place for each natural number. A list of subsets is a table of yes and no. The subset that says no where the diagonal says yes, and yes where it says no, is not in the table.

For a finite set the number of subsets is 2 to the power of the size of the set:

    [ 1 2 3 ] powerset .
    [ 1 2 3 ] powerset len .        8
    [ 1 2 3 4 ] powerset len .      16

Each new member doubles the number of subsets, since every old subset can be taken with or without it. For the natural numbers the same count gives 2^ℵ₀, and the diagonal argument shows that it is larger than ℵ₀.

    2 aleph0 ^ .            2^ℵ₀
    aleph0 2 aleph0 ^ < .   1

Cantor's theorem is the general form: for every set, the set of its subsets is larger than the set. It applies to the subsets of the subsets too, so no size is the largest. ℵ₀ < 2^ℵ₀ < 2^(2^ℵ₀), and so on.

## The continuum

The real numbers can be paired off with the subsets of the natural numbers, so the real numbers have the size 2^ℵ₀. That size is the continuum, and `continuum` is the word for it.

    2 aleph0 ^ continuum = .        1

The interval from 0 to 1, a line, a square and a cube all have the size of the continuum.

## Between ℵ₀ and the continuum

ℵ₁ is the size right after ℵ₀. The continuum hypothesis says that 2^ℵ₀ is ℵ₁, so that no set has a size strictly between the natural numbers and the real numbers. The usual axioms of set theory, ZFC, can neither prove nor disprove it. omega leaves the comparison without a result:

    aleph1 continuum =

fails with `ZFC can't decide this one`. Comparisons that come out the same under both answers still work, such as `aleph1 continuum <=`, which is `1` ([cardinals](cardinals.md)).

## Programs and numbers

There are ℵ₀ texts, so there are ℵ₀ omega programs ([countable-sets](countable-sets.md)). A real number that a program can print digit by digit is a number that has a finite description. Most real numbers have none, since 2^ℵ₀ is larger than ℵ₀. The numbers π, e and √2 have programs, and nearly all other numbers in the interval do not.
