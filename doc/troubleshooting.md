# Troubleshooting

## Using it

**The mouse doesn't move.** Click inside the QEMU window to give it the pointer, and press Ctrl-Alt-G to take it back. `mouse?` gives 0 when no PS/2 mouse was found ([ps2](ps2.md)). Every action of the mouse has a key ([keys](keys.md)).

**`gfx` says there is no Bochs display.** The display isn't QEMU's standard VGA card. Start QEMU with `-vga std` ([screen](screen.md)).


**The clock is wrong.** The tab bar shows the time from the CMOS clock, and QEMU sets that clock to UTC by default. Add `-rtc base=localtime` to the QEMU command line to see local time.

**`No drives were found.`** The disk is attached as virtio or NVMe, and Subverge only has an IDE driver. Attach it with `-hda` instead.

**Nothing is saved.** You are on the live ISO, which keeps everything in memory. `sys` says which one you are on. Run `install` to keep your work.

**My whole line did nothing.** A word on the line failed, so the line was undone, including the parts that ran before the failure. The red text says which word failed and why ([transactions](transactions.md)).

**`back` went further than I expected.** `back` only counts lines that changed something. A line that only printed, such as `help`, isn't counted, so `back` skips over it to the line before ([back](back.md)).

**A word works at the prompt but not inside another word.** Locals are dynamically scoped. A local in the calling word with the same name as a word you call hides that word, or a word you call reads a local you didn't mean to pass. Rename one of them ([names](names.md)).

**`forget` says the name isn't defined in this tab.** `forget` only removes definitions made in the current tab. A word defined in root has to be changed in root with `'name open` ([names](names.md)).

**My root edit didn't do anything.** The edit didn't build, so root was left as it was and the edit was kept as a draft. `root` marks the unit `(draft)`. Open it again with `'name open` to see the error and fix it ([root](root.md)).

**I used `edit` on `top` and the other tabs didn't change.** `edit` redefines a word in the current tab only, which makes a private copy that hides the root version. Use `'top open` to change root for every tab, and `'top forget` to drop the private copy ([scrawl](scrawl.md)).

**My program isn't in `apps`.** `apps` prints a fixed list. Add your program to it with `'apps open` ([apps](apps.md)).

**The colours are unreadable.** Switch back with `'dusk theme`, or type `themes` to pick another ([themes](themes.md)). In `amber`, `phosphor` and `haelide` every colour is a shade of one hue, so programs that rely on colour to tell things apart are harder to read in them.

**My `rgb` colours are gone after a reboot.** `rgb` changes the palette until the next theme switch or reboot, and isn't saved. Put the `rgb` lines in `startup` to apply them at every boot ([themes](themes.md)).

**The theme resets at every boot.** You are on the live ISO, which always starts with `dusk`. An installed system keeps the theme.

**`sys` shows the cat after I picked a logo.** Either you switched themes, and the logo you picked had the name of the old theme, or the logo was defined in another tab. `sys` runs in tab 1 at boot, so put the logo in a root unit ([logos](logos.md)).

**Tab does nothing at the prompt.** Nothing matches the name before the cursor, or the cursor is right after a space ([prompt](prompt.md)).

**`redo` says `nothing to redo`.** A line changed the world after the `back`, and that clears the redo list ([back](back.md)).

**Ctrl-C doesn't stop it.** The code is stuck in a place that doesn't check for Ctrl-C, such as a long built-in. Switch to another tab with F1 to F4, which keeps working ([tabs](tabs.md)).

**Everything froze.** Something wrote over memory the kernel uses, a port write hung the machine, or `halt` was run. Close QEMU and start it again. On an installed disk, every line up to the last one that succeeded was saved.

