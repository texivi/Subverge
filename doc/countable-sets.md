# Countable sets

A set is countable when its members can be paired off with the natural numbers, which means they can be put in a list where each member has a position. The size of such a set is [ℵ₀](aleph-null.md). This page builds the lists for the whole numbers, for pairs of numbers and for fractions, and shows how the words `pair`, `unpair` and `rational` compute positions in them.

## Whole numbers

The whole numbers need a list that reaches in both directions. Start at 0 and alternate: 0, 1, −1, 2, −2, 3, and so on. The number at position n is −n/2 when n is even and (n+1)/2 when n is odd. `pairing` prints this column ([demos](demos.md)).

    pairing

Every whole number is somewhere in the list: k > 0 at position 2k−1, and k < 0 at position −2k.

## Pairs

A table with a row for every natural number and a column for every natural number can't be read row by row, since the first row never ends and the second row would never be reached. It can be read along diagonals. The first diagonal is the single cell (0, 0). The second has two cells, the third has three, and each diagonal is finite, so each cell is reached after finitely many steps.

```text
pair   0   1   2   3
0      0   2   5   9
1      1   4   8  13
2      3   7  12  18
3      6  11  17  24
```

`a b pair` gives the position of the cell in row a and column b in this order:

    0 0 pair .              0
    1 0 pair .              1
    0 1 pair .              2
    3 4 pair .              32

The formula is (a+b)(a+b+1)/2 + b. The first part counts the cells on the diagonals before the one the cell is on, and b is the place of the cell along its own diagonal. This is Cantor's pairing function. Two different cells never get the same position, and every position belongs to some cell.

`n unpair` goes the other way and leaves the row and then the column, with the column on top of the stack:

    32 unpair . .           4 and then 3

Both words work on any natural number, so `n unpair pair` is always `n`.

The pairs of natural numbers have the size ℵ₀ · ℵ₀, and the list shows that this is ℵ₀:

    aleph0 aleph0 * .       ℵ₀

## Fractions

A positive fraction is a pair of natural numbers, so the fractions can be listed by listing pairs and dropping the ones that repeat a value, since 1/2 and 2/4 are the same number. The Calkin-Wilf order lists every positive fraction exactly once. The first terms are

```text
1/1  1/2  2/1  1/3  3/2  2/3  3/1  1/4  4/3  3/5
```

`n rational` leaves the n-th fraction as a numerator and a denominator, with the denominator on top. Positions start at 1.

    1 rational . .          1 and 1
    7 rational . .          1 and 3, which is 3/1
    100 rational . .        19 and 7, which is 7/19

`rationals` prints the first fifteen ([demos](demos.md)).

The numerator of the n-th fraction is the n-th term of Stern's diatomic sequence, `n fusc`, and the denominator is the next term. The sequence follows two rules. Its value at 2n is its value at n, and its value at 2n+1 is the sum of its values at n and at n+1. `fusc` applies the rules to the binary digits of n, so a position near a million takes about twenty steps.

    5 fusc .                3
    6 fusc .                2
    7 fusc .                3

A fraction can sit between any two others, so the list doesn't keep the fractions in order of size. A list that went from the smallest upwards could not exist, since there is no smallest positive fraction.

## Larger collections

Countable sets can be combined without leaving ℵ₀. The union of two countable sets is countable. So is the union of countably many countable sets, by the pairing above with the number of the set as the row. A finite list of natural numbers can be given one number by prime factorisation, as 2^a · 3^b · 5^c and so on, and a text is a finite list of numbers, so every text that can be typed has a position in one list. Every omega program is a text, and the set of omega programs has the size ℵ₀.

Because the programs are countable, most real numbers have no program that prints their digits. There are 2^ℵ₀ real numbers and only ℵ₀ programs ([uncountable-sets](uncountable-sets.md)).
