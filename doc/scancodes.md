# Scancodes

The keyboard sends scancodes of set 1. This page lists the ones that Subverge reads. A release is the same code with bit 7 set, and the handler ignores releases of every key except the modifiers.

## The main block

| Code | Key | Code | Key | Code | Key |
|---|---|---|---|---|---|
| 0x01 | Esc | 0x10 | Q | 0x1E | A |
| 0x02 | 1 | 0x11 | W | 0x1F | S |
| 0x03 | 2 | 0x12 | E | 0x20 | D |
| 0x04 | 3 | 0x13 | R | 0x21 | F |
| 0x05 | 4 | 0x14 | T | 0x22 | G |
| 0x06 | 5 | 0x15 | Y | 0x23 | H |
| 0x07 | 6 | 0x16 | U | 0x24 | J |
| 0x08 | 7 | 0x17 | I | 0x25 | K |
| 0x09 | 8 | 0x18 | O | 0x26 | L |
| 0x0A | 9 | 0x19 | P | 0x27 | ; |
| 0x0B | 0 | 0x1A | [ | 0x28 | ' |
| 0x0C | - | 0x1B | ] | 0x29 | ` |
| 0x0D | = | 0x1C | Enter | 0x2B | \\ |
| 0x0E | Backspace | 0x1D | Ctrl | 0x2A | left Shift |
| 0x0F | Tab | | | 0x36 | right Shift |

| Code | Key | Code | Key |
|---|---|---|---|
| 0x2C | Z | 0x32 | M |
| 0x2D | X | 0x33 | , |
| 0x2E | C | 0x34 | . |
| 0x2F | V | 0x35 | / |
| 0x30 | B | 0x38 | Alt |
| 0x31 | N | 0x39 | Space |

## Function keys and the pad

| Code | Key |
|---|---|
| 0x3B to 0x44 | F1 to F10 |
| 0x57, 0x58 | F11, F12 |
| 0x3A | Caps Lock |
| 0x45, 0x46 | Num Lock, Scroll Lock |
| 0x47 | Home |
| 0x48 | Up |
| 0x49 | Page Up |
| 0x4B | Left |
| 0x4D | Right |
| 0x4F | End |
| 0x50 | Down |
| 0x51 | Page Down |
| 0x52 | Insert |
| 0x53 | Delete |

The cursor block has the same codes as the pad. When the keys are on the cursor block, an `0xE0` comes first, and the handler uses the prefix only to tell the pad's Enter from the main one. The Windows key sends `0xE0` and `0x5B`.


## Translation

A key that is not one of the tab keys is translated with the US layout, with Shift and Caps Lock applied. Ctrl with a letter gives the control code, so Ctrl-A is 1 and Ctrl-Z is 26.

See [keyboard](keyboard) for the codes that programs see and [ps2](ps2) for the controller.
