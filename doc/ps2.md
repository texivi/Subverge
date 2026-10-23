# The PS/2 controller

The keyboard and the mouse are attached to one chip, the keyboard controller, which has a channel for each. It is reached through port `0x60` for data and port `0x64` for the status and for commands. A program has to wait until the controller is ready before it writes and until it has a byte before it reads. The kernel gives up if the controller never answers.

## The keyboard

The keyboard sends scancodes of set 1. Each press sends one code and each release sends the same code with the top bit set ([scancodes](scancodes)). The keyboard raises IRQ 1, and the handler reads exactly one byte each time, since the controller doesn't raise the interrupt again until the byte is taken ([interrupts](interrupts)).

## The mouse

The mouse raises IRQ 12. At boot the kernel turns on the second channel and asks the mouse to send packets of movement and buttons. A mouse that answers with a wheel gets four-byte packets, and one that doesn't gets three. If the mouse doesn't answer, the system runs without it and `mouse?` is `0` ([mouse](mouse)). The pointer is drawn on the text screen as an inverted cell.

## The reset line

The controller also holds the line that resets the processor, and `reboot` pulses it ([machine](machine)).

## Real machines

Many machines have no PS/2 ports and emulate the controller for a USB keyboard and mouse. The emulation is often called legacy support and has to be turned on in the firmware. Where it isn't there the system starts and can't be typed at ([why not more](why-not-more)).
