# Expectations

## It is an experiment

Subverge is a hobby operating system written by one person. It targets QEMU, the only machine it is tested on. Things change between releases, sometimes in ways that break saved roots, and the [roadmap](roadmap.md) lists what is planned rather than what is promised.

## It is its own system

Subverge has its own kernel, shell and programs, and an installed disk holds fixed slots instead of files ([disk layout](disk-layout.md)). It has no processes, users, permissions or file paths. Everything you write is an omega word, kept in a tab or in [root](root.md), and saved as omega source.

## You need to know how to program

These docs and `learn` assume you already program, and they don't explain loops, variables or functions. Some familiarity with Forth or another stack language helps, and if you know Forth, read [omega for Forth programmers](forth.md) first. The [ordinals](ordinals.md) and [cardinals](cardinals.md) pages explain the mathematics they use, and Vsauce's [How To Count Past Infinity](https://www.youtube.com/watch?v=SrU9YDoXE88) covers the background.

## You can break it

Everything runs in ring 0, so a bad write to memory can bring down the whole virtual machine. Most mistakes cost nothing, because a failing line is undone and [back](back.md) can rewind the last 64 lines. Writes to raw memory and I/O ports can't be undone, and they are your responsibility. See [omega for smarties](smarties.md).

## Never install to a real disk

`install` erases the drive you give it, including its partition table. Only use it on a disk image made for that purpose. QEMU can pass a real disk to the guest, and Subverge will install onto it if asked, so check the drive list `install` prints before typing `yes`.

## Asking for help

Run `learn` and read `'word help` before you ask how a word works. When you ask, give the exact line, the exact error and what you expected to happen. See [troubleshooting](troubleshooting.md).

## Releases

Releases have no dates. Planned features are on the [roadmap](roadmap.md).

Continue with [first steps](first-steps.md).
