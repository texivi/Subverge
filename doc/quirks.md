# Quirks

- A name can't be a number, so `2048` can't be defined as a word. Anything that parses as a number is a number ([values](values)).
- Locals bound in the condition of a `while` are gone by the time the body runs. Keep the value on the stack instead ([flow](flow)).
- A tab's key queue holds 63 keys. Keys pressed while a long computation runs are dropped once it is full, and keys queued during a failing line are thrown away ([keyboard](keyboard)).
- A line holds 200 characters, less when the stack takes up room before the `»`. Longer code goes in a definition.
- `ask` and `calc` read a line with the prompt's editing keys but without history and without completion.
- Tab titles set with `title` aren't saved and aren't part of a [world](world). A program that fails halfway leaves its title behind.
- The mouse belongs to QEMU's window after the first click. Ctrl-Alt-G releases it, and keys QEMU grabs itself, such as Ctrl-Alt-1 and Ctrl-Alt-2, never reach Subverge.
- The first run of a quote is interpreted, and the compiled version is used from the second. A very short quote can be slower on the second run than it looks, since compiling has a cost ([JIT](jit)).
- Booting a new ISO with an old installed disk attached loads the root saved on that disk, so `sys` shows the old version until you run `install` again.
- `rand` is seeded from the CPU's cycle counter, which is fine for games and not for secrets ([numbers](numbers)).
- The clock shows UTC unless the machine was started with a local-time clock.
- A write with `poke`, `out` or `hexview` can't be undone by `back`, because it isn't part of the world ([smarties](smarties)).
- A failing line is undone completely, including definitions made earlier on the same line.
- Everything a line prints before it fails stays on the screen, even though the line itself is undone.
- Characters outside ASCII, such as `ω` and `ℵ₀`, are drawn by redefining unused characters of the VGA font, and they can't be typed. Use `omega`, `aleph0` and the `\w` and `\A` escapes ([prompt](prompt)).
- An installed disk holds your tabs and root. It doesn't hold the help pages, which come from the kernel and always match the version that is running.
