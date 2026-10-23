# Startup

At boot, tab 1 runs `startup`, defined in the `sys` unit of [root](root.md). It prints the system information and the `welcome` hints. The other tabs start at a prompt.

## What happens at boot

Once the kernel has started, things happen in this order:

1. Root is built, from the saved root on an installed disk or from the copy inside the kernel ([how root builds](root-build.md)).
2. Each tab starts its shell. On an installed disk it first runs its saved [world](world.md), so its definitions and stack are back before anything else happens.
3. Tab 1 runs `startup` if it is defined, and `sys` otherwise.
4. Every tab shows its prompt.

Because the saved world is loaded first, `startup` can use words defined in tab 1, and a `startup` defined in tab 1 replaces the one in root.

`startup` runs as an ordinary line in tab 1. If it fails, the error is printed and the prompt appears as usual, and nothing it did is kept.

## Changing it

    'startup open

and edit the definition. `[ sys ] :startup` drops the hints, and `[ ] :startup` starts at a bare prompt.

    'user open

changes the `user` in `user@subverge`. `host`, `os` and `kernel-name` set the other lines of the `sys` screen, and `logo` changes the drawing on the left ([logos](logos.md)).

## The hints

`welcome` prints a few lines of hints below the system information: whether the system runs from the live ISO or from an installed drive, and the words to start with. `welcome` runs again at any time, and `[ sys ] :startup` leaves it out at boot.

## Overriding it per tab

On an installed disk, tab 1's saved [world](world.md) is loaded before `startup` runs, and tab definitions take precedence over root ([names](names.md)). Defining `startup` in tab 1 therefore overrides the root version without changing it:

    [ "it's me again" say ] :startup

`'startup see` then shows your version in tab 1 and the root version in tab 2.

## Missing startup

If `startup` is undefined, tab 1 runs `sys` instead. If root failed to build at boot, it shows `sys-c` and the root error in red.
