# How root builds

[Root](root.md) is ordinary omega source, run in a special mode.

## The source

The root source is one text with unit headers in it. A line that starts with `#>> ` followed by a name begins a new unit, and everything up to the next header belongs to it. Since `#` starts a comment, the headers are also valid omega, and the whole text could be pasted at a prompt.

The kernel carries the original root, concatenated from the files in `root/` at build time. An installed system also has a saved root in slot 0 ([disk layout](disk-layout.md)). At boot the kernel tries the saved root first, and falls back to the built-in one if it is missing, damaged or fails to build.

## Building

A build runs each unit's source, in order, as one quote. It uses the tab that asked for it, with three changes:

- **Pinning is on.** Every object created during the build is pinned: it is kept out of the tab's object list and is never collected ([values and objects](values.md)). Root's definitions are shared by all four tabs, so they can't belong to any one of them.
- **The dictionary starts empty**, or at the end of the last unit before the edited one. A unit's definitions go on top of everything before it, so later units see earlier ones.
- **The tab's stack, locals and trace state are saved** before the build and restored after it, so a build can't disturb whatever the tab was doing.

The [JIT](jit.md) is off for the length of the build.

When a unit finishes, the build records where its definitions end in the dictionary and which names it defined. Those records are what `root` prints and what `'name open` searches to find the unit that defines `name`.

If any unit fails, the build stops there, the tab's state is restored, and the error is reported with the unit's name in front of it.

## Rebuilding after an edit

When you leave [scrawl](scrawl.md) after `'name open`, the kernel rebuilds from the edited unit onward. Units before it are not run again: their part of the dictionary is reused as it is, and the new definitions are built on top of the point where the unit before the edited one ended.

A rebuild either succeeds completely or changes nothing:

- **If it fails**, the old root stays in place and keeps working. Your edited text is kept as a draft attached to the unit, `root` marks it `(draft)`, and the next `open` of that unit shows the draft, so you can fix it without retyping. The error has the form `root unchanged, unit: message (your edit is kept: open it again)`.
- **If it succeeds**, the new dictionary replaces root in one step. Every tab sees the change on its next lookup, the draft is dropped, and root is [saved](saving.md).

The replacement is a single pointer change, so a tab running a root word at that moment finishes with the old definition and picks up the new one on its next lookup.

## What a rebuild invalidates

Compiled code may have inlined decisions based on the old root, such as treating `+` as the built-in. After a successful rebuild:

- machine code attached to the replaced definitions is released;
- the global JIT version is increased, so every compiled quote in every tab is recompiled the next time it runs twice.

Old root objects are not freed. A tab may have copied a quote out of the old root with `get`, and it stays valid.

## Shadowing built-ins

After a rebuild the kernel looks through the new root for definitions whose names are also built-ins. If it finds any, it prints `shadows:` followed by the names in orange. This isn't an error. It is a warning that every tab will now run your definition instead of the built-in, including inside other root words that use the name.

## Order

New units from `"name" unit` are appended at the end, so they can use every other unit, and editing one only rebuilds that unit. Because rebuilding starts at the edited unit, editing `core`, the first unit, rebuilds all of root, and a mistake there can make every later unit fail. The draft mechanism keeps that recoverable.

## When the saved root fails at boot

If the saved root doesn't build at boot, for example after a kernel upgrade removed a built-in that a unit used, the kernel reports `the saved root didn't build, using the built-in one` and runs the built-in root instead. The saved copy on disk is left alone until the next successful root edit replaces it, so the old source can still be recovered by booting the previous kernel.

If the built-in root fails too, tab 1 shows the system information screen and the root error in red, and the tabs start with only the built-in words.
