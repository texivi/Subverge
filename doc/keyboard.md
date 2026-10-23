# The keyboard

Subverge reads a PS/2 keyboard directly, through the keyboard controller at ports `0x60` and `0x64`.

## From scancode to key

The controller delivers scancode set 1, which is what PC firmware and QEMU provide by default. Each press and release raises IRQ 1, and the handler reads one byte from port `0x60`.

The handler tracks Shift, Ctrl, Alt and Caps Lock itself. Releases are ignored except for those modifiers. A press is translated through a US layout table, with a second table for Shift. Caps Lock swaps the case of letters only. Ctrl with a letter produces the control code, so Ctrl-A is 1 and Ctrl-Z is 26.

The extended prefix byte `0xE0` is remembered for the next scancode. It is used to tell keypad Enter from the main Enter, and both produce a newline.

Keys outside the US table, such as a second backslash key on international keyboards, produce nothing.

## Keys that never reach a tab

Some keys are handled inside the interrupt handler and never queued:

- **F1 to F4** and **Alt-1 to Alt-4** switch the visible [tab](tabs.md). The switch happens immediately, even if every tab is busy.
- **Ctrl-C** sets the front shell's break flag *and* is queued as code 3. The flag is what stops running code: the interpreter checks it before every step and fails with `interrupted`. The queued code is what the [prompt](prompt.md) and programs like `snake` see.

## The key queues

Each tab has its own queue of 64 entries. Keys always go to the tab in front when they are pressed, so typing into one never affects another.

A queue holds 63 keys before it is full, and keys pressed while it is full are dropped. A program that doesn't read keys for a while, such as a long computation, can therefore lose typing beyond that point.

When a line fails, the keys queued during it are thrown away, so keys pressed while a broken loop was running don't spill into the next prompt.

## Key codes

`key` waits for a key and pushes its code, `key?` pushes the next code or `-1` if the queue is empty, and `n waitkey` waits at most `n` ticks and pushes `-1` on timeout.

Printable keys give their ASCII code. At the prompt, Tab completes names ([prompt](prompt.md)); everywhere else it is code 9. The others:

| Key | Code |
|---|---|
| Enter | 10 |
| Backspace | 8 |
| Tab | 9 |
| Escape | 27 |
| Ctrl-A to Ctrl-Z | 1 to 26 |
| Up | 256 |
| Down | 257 |
| Left | 258 |
| Right | 259 |
| Home | 260 |
| End | 261 |
| Page Up | 262 |
| Page Down | 263 |
| Delete | 264 |

A loop that reads keys:

    [ key dup 113 = not ] [ . ] while drop

prints codes until `q` is pressed. Locals bound in the condition of a `while` are gone by the time the body runs, so the code stays on the stack instead. The `quit-key?` helper in root tests for `q`, Escape and Ctrl-C at once, which is how most of the bundled programs decide to exit.

## Waiting without spinning

A tab that calls `key` on an empty queue marks itself as waiting for a key and yields. The [scheduler](scheduler.md) skips it until a key arrives in its queue, so a tab sitting at the prompt uses no CPU. `waitkey` marks the tab as waiting for either a key or a tick, whichever comes first, and the animated programs use it to run a frame every few ticks while still reacting to keys at once.

## USB keyboards

On real machines a USB keyboard works only if the firmware emulates a PS/2 controller for it, which is often called legacy USB support. In QEMU the default keyboard is PS/2.
