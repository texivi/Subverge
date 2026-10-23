# Tips

- Ctrl-C stops the line a shell is running and undoes it. If a shell doesn't answer, switch to another with F1 to F4. The others keep running, and `reboot` from any of them restarts the machine ([tabs](tabs)).
- `'name see` prints the source of a word, and `'name open` edits it in [scrawl](scrawl). Both work on any word defined in omega, which includes everything in [root](root).
- `"sort" apropos` lists every word whose name or description mentions `sort`. The [index of words](words) has the same information with a page for each word.
- Tab completes a name at the prompt. A `'`, `:` or `->` in front of the name is kept. ([prompt](prompt)).
- `back` undoes the last line that worked, and `redo` undoes the `back`. Both go 64 lines deep ([back](back)).
- A shell's definitions are its own. A word you define in shell 1 doesn't exist in shell 2 until it goes into root ([names](names), [units](units)).
- An edit to root takes effect only if it builds. If it doesn't build, the old version stays in use and the error is shown ([root build](root-build)).
- The first run of a quote is interpreted and the second compiles it to machine code. `'name xray` shows what was generated, and `bench` compares the two ([JIT](jit), [dis](dis)).
- Integers never wrap. `2 1000 ^` is exact, and compiled code switches to bignums at the same point the interpreter does ([bignums](bignums)).
- `0x2a` is a hex literal and `0b101010` a binary one. `bases` prints a number in all of them ([numbers](numbers), [tools](tools)).
- `[ ... ] watch` reruns a quote twice a second until a key is pressed, and `timeit` times one run in ticks and CPU cycles.
- `440 50 beep` plays 440 Hz for half a second on the PC speaker.
- `calc` is an infix calculator with fixed point arithmetic, and it keeps its own history.
- `date` leaves today's date as a string and `cal` prints the month. The clock shows UTC unless the machine was started with a local-time clock.
- `themes` lists the colour themes and `'raw theme` switches to one. An installed disk remembers the choice ([themes](themes)).
- `14 255 0 0 rgb` sets colour 14 to red. This recolours every tab ([themes](themes)).
- `logos` lists the logos that `sys` can draw, and `'haelide logo` picks one ([logos](logos)).
- `'startup open` edits what the first shell runs at boot, and `'user open` changes the name shown by `sys` ([startup](startup)).
- `"build" title` renames the tab that runs it. Programs do this while they run and put `omega` back when they leave.
- `2 tab` returns a list describing tab 2: its title, what it is doing and whether it is in front.
- A line holds 200 characters. Put longer code in a definition with `'name open`.
- Locals set with `{ a b }` or `->name` exist only while a quote runs, so they can't be seen from the prompt after it finishes ([names](names)).
- `1 2 3 empty` clears the stack. `depth` counts it, and `pick`, `tuck`, `2dup`, `nip` and `rot` rearrange it ([stack](stack)).
- `root` lists the programs in root, and `apps` lists the ones meant to be run. Adding one is a line in the `apps` unit ([apps](apps)).
- `hexview` edits live memory. `0xB8000` is the memory of the text screen, which is redrawn from the tab on the next change, so it is a safe place to try it ([memory](memory)).
- The mouse belongs to QEMU's window after you click it, and Ctrl-Alt-G gives it back. A click on a tab title in the top bar switches tab ([ps2](ps2)).
- `docs` opens the index of the help pages and `"name" doc` opens one. Tab selects the next link, Enter follows it, Backspace goes back and `q` leaves. Enter on a line of code closes the viewer and runs the line at the prompt.
- Disks are used only when you run `install`. Until then nothing writes to one, so the live ISO is safe to try with a real disk attached.

See [quirks](quirks) for the things that surprise people, and [troubleshooting](troubleshooting) when something doesn't work.
