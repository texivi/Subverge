# Screen

This page describes the 80 by 25 text screen and the words that draw on it, which every shell and program uses, and the full-screen graphics mode that `gfx` starts.

Subverge normally uses 80 by 25 VGA text mode. Row 0 is the tab bar and the rest belongs to the current [tab](tabs.md). `gfx` starts a separate [graphics mode](#graphics-mode).

## Tabs have their own screens

Each tab has an 80 by 24 buffer of its own, in memory. Everything a tab prints or draws goes into its buffer, and the buffer of the visible tab is copied to the real screen at `0xB8000`. When you switch tabs, the new tab's buffer is shown, exactly as it was left. So `snake` in tab 3 keeps its picture while you're in tab 1, and a tab can draw while you're on another one. If a tab writes to its screen while you're not looking at it, its name in the tab bar gets a `*`.

## Drawing words

| Word | Effect | |
|---|---|---|
| `put` | `ch x y color --` | one character at x, y |
| `text` | `s x y color --` | a string starting at x, y |
| `screen@` | `x y -- ch color` | read one back |
| `wipe` | `--` | clear the tab's screen |
| `cursor` | `x y --` | move the cursor |
| `nocursor` | `--` | hide it |
| `where` | `-- x y` | where it is |
| `size` | `-- w h` | the width and height of the tab's window, `80 24` when it has the whole screen |
| `title` | `s --` | rename the tab |

Coordinates start at `0 0`, the top-left of the tab's area, under the tab bar. `cursor` moves the cursor without showing it, and anything printed afterwards starts there. Despite the name, it isn't the opposite of `nocursor`.

`put` and `text` don't move the cursor and don't scroll. `say` and `write` do both. Programs that draw a full screen use `wipe`, `nocursor`, then `put` and `text`, and never print. See [games](games.md) for the pattern.

## Colours

Text mode has 16 colours, numbered 0 to 15, and `colors` shows them all. The kernel loads them from the current [theme](themes.md) at boot and whenever the theme changes, so they aren't the usual VGA colours. Each number keeps its role in every theme: 0 is the background, 7 normal text, 8 dim text, 12 errors and 14 the accent.

All sixteen work as background colours too. The kernel switches off the VGA blink bit at boot, which would otherwise turn backgrounds 8 to 15 into blinking text on a darker background.

`color` sets the colour for printed text, foreground only. `put` and `text` take a full attribute byte, which is the foreground plus 16 times the background, and `attr` builds one for you:

    14 1 attr           the accent on colour 1
    "hi" 10 5 14 1 attr text

`fill` draws a row of one character, `bar` draws a meter that's some percent full, and `header` draws a title bar across the top. They're in root, under util.


## Characters that weren't there

The kernel adds `ω`, `ℵ` and the subscript digits 0 to 9 to the font, at the codes `0x80` to `0x8B`. The [ordinals](ordinals.md) print with them as `ω` and the [cardinals](cardinals.md) as `ℵ₀`. In a string, `\w` is `ω` and `\A` is `ℵ`. The subscript digits have no escape, so use `130 chr` and up.

## The tab bar

Row 0 is drawn by the kernel, not by any tab, so you can't draw over it with `put`. It shows the four tab titles, highlights the current one, marks the ones with something new with `*`, and shows the clock at the right edge. The clock is the machine's real-time clock, which in QEMU is UTC by default.

## Graphics mode

`gfx` switches the screen to 640 by 480 graphics, and `gfx-off` switches back to the text screen. The drawing words below use the sixteen colours of the current [theme](themes.md). Pictures and bitmaps use any 24-bit colour and are described in [pictures](pictures).

| Word | Effect | |
|---|---|---|
| `gfx` | `--` | start graphics mode, with the screen cleared to colour 0 |
| `gfx-off` | `--` | back to the text screen |
| `dot` | `x y col --` | set one pixel |
| `box` | `x y w h col --` | fill a rectangle |
| `window` | `x y w h title --` | draw a titled frame |
| `text` | `s x y col --` | draw a string at pixel x, y, 8 pixels per character |

Coordinates start at `0 0` in the top-left corner, and anything outside the screen is cut off. Colours are the same numbers as in text mode, 0 to 15. `dot`, `box` and `window` fail with `graphics mode is off, run gfx first` when it isn't running.

    gfx 20 20 100 60 5 box key drop gfx-off

draws a rectangle, waits for a key and goes back to text.


The picture is eight `box` calls, one per line, after `gfx`:

    0 0 640 480 1 box
    60 60 360 220 0 box
    60 60 360 20 5 box
    84 110 160 24 2 box
    84 150 240 24 9 box
    84 190 100 24 14 box
    300 300 240 120 4 box
    330 330 180 60 6 box

The text screen isn't shown in graphics mode, so the prompt and anything the tabs print are hidden. The tabs keep running and keep their screens, and a word that fails shows its error once you're back. F1 to F4 and alt-1 to alt-4 switch tabs and also leave graphics mode, which is the way out when you can't see the prompt. Use `theme` and `rgb` before `gfx`, since they set the text screen's colours.

`bitmap`, `bmp-show` and the other words for full-colour drawing are in [pictures](pictures).

`gfx` needs QEMU's default `-vga std` display, which is the one that has the Bochs display registers at ports `0x1CE` and `0x1CF` ([ports](ports.md)). On anything else it fails with `no Bochs display here, start QEMU with -vga std`.

## Poking it

`poke` can write into `0xB8000` directly. What it writes is overwritten the next time the visible tab is drawn. See [memory](memory.md) for the details.

## Why 80 by 25

VGA text mode is 80 by 25. Every program in [root](root.md) is drawn for it. Graphics mode is a separate screen and doesn't change the text one.
