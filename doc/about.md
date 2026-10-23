# About Subverge

Subverge is a hobby operating system developed in the open at github.com/texivi/Subverge. It is a radical and experimental system.

Its kernel, **ordinal**, its shell, **omega**, and its editor, **scrawl**, are named after ordinals and writing. The word *subverge* is a coinage from *subvert* and *diverge*; the [history](history) page has the details, the inspiration, and a list of releases.

## What the system consists of

The kernel is C and x86-64 assembly. The boot code for ISOs is GRUB, and installed disks boot through Subverge's own master boot record and second stage ([boot](boot)). Everything above the kernel is omega: the utilities, the games, the installer, the lessons of `learn` are source code in [root](root), kept in the kernel image and rebuilt when you edit them.

| Part | What it does |
|---|---|
| ordinal | memory, interrupts, the scheduler, drivers and the JIT |
| help | the pager for these pages and the PS/2 mouse driver |
| omega | the language: parser, interpreter, garbage collector, bignums, ordinals |
| scrawl | the text editor, used by `'name edit` and `'name open` |
| root | the programs written in omega |
| docs | the pages you are reading, linked into the kernel ([where the pages live](docfs)) |

## The help you are reading

These pages are part of the system and are stored in the kernel image, so they match the build you are running and work on the live ISO. `docs` opens the index and `"name" doc` opens a page. The pager is in the kernel and reads a small dialect of markdown. It finds each page by name in a table that was built together with the kernel, and reads nothing from a disk.

## Credits

The [credits](credits) page lists who and what made it.
