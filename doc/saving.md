# Saving

An installed Subverge saves after every line that changes something, without being asked, so a reboot or a power cut loses at most the line that was running.

The live ISO doesn't save anything. To keep your work, run `install` first. On the live ISO `home` returns `-1` and `save` fails with `you haven't installed yet, run install`.

## When saving happens

On an installed disk, these are saved straight away:

- **A line that changed a tab's world.** Only that tab's slot is written. A line that leaves the stack and definitions as they were, such as `words` or `"hi" say`, writes nothing.
- **A `back`**, since it changes the world back.
- **A root edit that builds.** The whole of root is written to its slot. An edit that fails to build writes nothing ([root](root.md)).
- **A theme or logo change**, which rewrites only the header sector ([themes](themes.md), [logos](logos.md)).
- **`install`**, which writes root, all four tabs, the current theme and the current logo to the new drive.

`save` writes root and all four tabs at once. It is rarely needed, since everything is already saved, but it is harmless.

`home` returns the number of the drive Subverge is installed on and saves to.

## Slots

The saved state lives in five slots of 1 MiB at fixed positions on the drive: slot 0 holds root and slots 1 to 4 hold the four tabs. A small header in front of them records each slot's length and checksum and a counter of saves. See [disk layout](disk-layout.md) for the exact positions.

Saving a tab rewrites only that tab's slot and the header. The other slots aren't touched.

## What's in a slot

A slot holds plain omega source. A tab is written as its definitions, oldest first, followed by its stack on one line:

    # saved world
    [ dup * ] :sq2
    [ { a b } a a * b b * + ] :hyp
    "notes" :title
    1 2 3

Only the newest definition of each name is written, and names removed with `forget` are left out, so the saved text is as short as the world allows.

Root is written as each unit's source after its `#>> name` header line, in order, which is the same format as the `root/` files in the source tree.

At boot, root's slot is built, then each tab's slot is run as if it had been typed at that tab's prompt. A world that was saved is therefore always one that omega can read back.

## Values that survive

A value survives only if it can be written as source and read back. These can:

- integers and bignums;
- strings, with escapes for newlines, tabs, escape characters and the omega and aleph glyphs;
- quotes and lists, nested to any depth;
- symbols (`'name`);
- ordinals, written back as arithmetic such as `omega 2 ^ omega +`;
- cardinals, written as `0 aleph` or `2 0 aleph ^`.

Two things change on the way. Identity is lost: two definitions that held the very same quote hold two equal quotes after a reboot ([values and objects](values.md)). And memory from `alloc` isn't saved, so an address kept on the stack points at whatever the heap holds there after the reboot.

## Not saved

- the [back](back.md) history and the prompt history;
- tab titles and screen contents;
- memory from `alloc`, and anything written with `poke` or `out`;
- the JIT's compiled code, which is rebuilt as code runs.

After a reboot every tab starts with an empty history, its saved stack and definitions, and the title `omega`.

## Theme

The theme is saved with the system state and used at the next boot ([themes](themes.md)).
