# History

## TempleOS

Subverge started from TempleOS. Like TempleOS, its shell is also its compiler, everything runs in ring 0, and one language is used for the whole system.

None of the code came from TempleOS. Its shell is a stack language rather than a C dialect, code is compiled the second time it runs rather than ahead of time, and each line is a [transaction](transactions.md), so a typo in ring 0 doesn't take the system down. It also treats infinity as a value.

## Counting past infinity


Subverge was also inspired by Vsauce's video [How To Count Past Infinity](https://www.youtube.com/watch?v=SrU9YDoXE88). Ordinals and cardinals are values in omega because of it, and the kernel and the shell are named after ordinals: ordinal, and omega for ω. The [ordinals](ordinals.md) and [cardinals](cardinals.md) pages start with the same ideas as the video.

## The name

I made up the word **subverge** from *subvert* and *diverge*, after the Latin *sub-*, "under", and *vergere*, "to bend" or "to turn".

> **subverge** (verb): to turn away from an established norm in a way that deliberately undermines it; literally, to dip beneath an edge or surface.
>
> **subvergent** (adjective): quietly, deliberately non-conforming beneath dominant conventions.
>
> **subvergence** (noun): the act of subverging.

The kernel is called **ordinal**, the shell **omega**, and the editor **scrawl**.

## Code

All of Subverge was written for it, in C, assembly and omega. The only outside code is GRUB, which boots the ISO.

## Releases

The current release is **Subverge 0.9.5**, with ordinal 1.0.5 and omega 1.0.5. See the [roadmap](roadmap.md) for what each release added.

| Release | ordinal | omega |
|---|---|---|
| 0.9.5 | 1.0.5 | 1.0.5 |
| 0.9.4 | 1.0.4 | 1.0.4 |
| 0.9.3 | 1.0.3 | 1.0.3 |
| 0.9.2 | 1.0.2 | 1.0.2 |
| 0.9.1 | 1.0.1 | 1.0.1 |
| 0.9.0 | 1.0.0 | 1.0.0 |
| 0.8.0 | 0.10.0 | 0.9.0 |

Every release is on the [releases page](https://github.com/texivi/Subverge/releases) with its ISO.

