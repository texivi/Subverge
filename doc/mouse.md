# Mouse

A PS/2 mouse works on the text screen. The pointer is one cell drawn with its colours inverted, and it moves in whole cells. The controller and the packets are described in [the PS/2 controller](ps2.md).

## What the mouse does

- A click on a tab title in the top bar switches to that [tab](tabs.md).
- In the help viewer a click follows a link or runs a line of code, the wheel scrolls and a right click goes back ([help](help.md)).

## Words

| Word | Effect | |
|---|---|---|
| `mouse` | `-- x y buttons` | the pointer cell and the buttons |
| `mouse?` | `-- 1/0` | 1 when a mouse was found at boot |

`x` and `y` count from the top-left cell of the tab's area, so `y` is one less than the screen row, and `y` is `-1` over the top bar. `buttons` is the sum of 1 for the left button, 2 for the right button and 4 for the middle button:

    mouse? .            1
    mouse . . .         buttons, then y, then x

A mouse that reports a wheel is detected at boot. One that doesn't answer is left unused, and `mouse?` is `0`. Under QEMU the pointer follows the host mouse after the first click in the QEMU window, and Ctrl-Alt-G releases it.
