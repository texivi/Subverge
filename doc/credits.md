# Credits

[texivi](https://github.com/texivi) wrote Subverge.

## Inspiration

The idea came from TempleOS, by Terry A. Davis: an operating system whose shell is also its compiler, whose programs all run in ring 0, and which is small enough for one person to understand. Subverge uses none of its code.

The infinite values in omega came from Vsauce's video [How To Count Past Infinity](https://www.youtube.com/watch?v=SrU9YDoXE88). Ordinals and cardinals are values in the language because of it, and the kernel and the shell are named after them.

## Other software

GRUB boots the ISO and is the only outside code that Subverge runs. It is built with gcc, binutils and nasm, and the ISO is made with `grub-mkrescue` and `xorriso`.

## Fonts and pictures

The text is drawn with the 8 by 16 VGA font that the video card provides, with a few characters added by the kernel for `ω`, `ℵ` and subscript digits.

## The name

*Subverge* is a made-up word from *subvert* and *diverge*. See [history](history).

## Licence

Subverge is released under the GPL v3.

[Help & Index](index)
