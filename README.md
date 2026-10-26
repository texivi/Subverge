<p align="center">
  <img src="https://sbvrg.github.io/images/logo.png" alt="Subverge" width="200">
</p>

# Subverge

Subverge is a radical and experimental 64-bit operating system written from scratch. See the [features](https://sbvrg.github.io/features.html).

## Requirements

The virtual machine needs `qemu-system-x86_64` and 256 MB of RAM. Building from source also needs gcc, binutils and nasm, and the ISO needs `grub-mkrescue` and `xorriso`. On Exherbo, QEMU is in the `virtualization` repository:

    # cave resolve -x repository/virtualization
    # cave resolve -x app-virtualization/qemu sys-devel/gcc sys-devel/binutils \
        dev-lang/nasm sys-boot/grub app-cdr/libisoburn

## Running

The ISO is attached to each release on the [releases page](https://github.com/texivi/Subverge/releases). You can also build it yourself:

    $ git clone https://github.com/texivi/Subverge
    $ cd Subverge
    $ make iso
    $ qemu-system-x86_64 -cdrom build/subverge.iso -m 256M

The ISO is a live CD, so nothing you do on it is saved. Type `learn` once it has booted. To hear the songs of `play`, add `-audiodev pa,id=snd0 -device sb16,audiodev=snd0` to the QEMU command line.

## Installing

An installed disk keeps your work between boots. Boot the ISO with a blank disk image attached, run `install`, and then boot the image on its own.

    $ truncate -s 64M disk.img
    $ qemu-system-x86_64 -cdrom build/subverge.iso -boot d -hda disk.img -m 256M
    ... install
    $ qemu-system-x86_64 -hda disk.img -m 256M

`install` erases the whole drive you pick. Only use it on a disk image.

## Documentation

The documentation is on [sbvrg.github.io](https://sbvrg.github.io). Start with the [README](https://sbvrg.github.io/readme.html) and the [expectations](https://sbvrg.github.io/expectations.html).

## Contact

Report bugs on the [issue tracker](https://github.com/texivi/Subverge/issues), or message `texivi` on Discord. [Bugs](https://sbvrg.github.io/bugs.html) explains what to put in a report.

## License

Subverge is released under the GPL v3. The full text is in [LICENSE](LICENSE).
