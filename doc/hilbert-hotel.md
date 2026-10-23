# The hotel

Hilbert's hotel has a room for every positive whole number: room 1, room 2, room 3, and so on without end. This page uses it to show how the sizes in [aleph-null](aleph-null.md) behave when a set that is already full takes in more members. `hotel` prints the first rows of the first and the last step ([demos](demos.md)).

    hotel

## A full hotel

Every room has a guest. In an ordinary hotel a new guest would be turned away. Here the guest in room n moves to room n+1, for every n at once. Room 1 is empty afterwards and every old guest still has a room, since n+1 is a room whenever n is. The new guest gets room 1.

```text
guest   before   after
1       1        2
2       2        3
3       3        4
```

This is the pairing of the rooms with the rooms from 2 up, and it is the reason `aleph0 1 +` is `aleph0`. A set that is full can take one more member and keep its size.

For k new guests, every guest moves from room n to room n+k, which leaves rooms 1 to k empty. Any whole number k works.

## Infinitely many new guests

When infinitely many guests arrive, moving up by a fixed number doesn't help. Moving every guest from room n to room 2n does. Every even room is taken by an old guest, and no old guest is without a room. All the odd rooms are empty, there are infinitely many of them, and the new guests take rooms 1, 3, 5 and so on.

This is `aleph0 aleph0 +` being `aleph0`: two copies of the natural numbers side by side are as many as one.

## Infinitely many buses

Infinitely many buses arrive, each with infinitely many seats, all taken. A guest is named by the bus b and the seat s, both counted from 0. Cantor's pairing from [countable-sets](countable-sets.md) turns each (b, s) into a number, and no two guests get the same one:

    0 0 pair .              0
    0 1 pair .              2
    1 0 pair .              1
    2 1 pair .              7

In an empty hotel the guest from bus b and seat s takes room (b s pair) + 1. In a full hotel the old guests first move from room n to room 2n, which frees the odd rooms, and the guest from bus b and seat s takes room 2·(b s pair) + 1. Every odd room is used once.

All of this is `aleph0 aleph0 *` being `aleph0`. `hotel` prints the first buses and seats of the empty case.

The hotel stays countable, and its guests can be listed in the order of their rooms.

## Where it stops

A hotel with a room for every natural number has the size ℵ₀, and a set of the size 2^ℵ₀ doesn't fit in it. If one guest came for each real number, no assignment of rooms would work, since the real numbers can't be paired off with the room numbers ([uncountable-sets](uncountable-sets.md)).

## The hotel and ordinals

The first move, from room n to room n+1, is the same operation as putting a new member in front of a list of positions. It shows the difference between `1 omega +` and `omega 1 +`:

    1 omega + .             ω
    omega 1 + .             ω+1

A member put in front moves every other member one place up, and the result is again a list of the natural numbers, so it ends where they end. A member put after all of them has no room number among the natural numbers. It gets the position ω, the first position after every natural number. In position order ω+1 is larger than ω, and in size the two are the same.

    1 omega + omega = .     1
    omega 1 + omega = .     0
    omega 1 + card .        ℵ₀
