# Part 12: Keeping your work

On the live ISO everything is lost when the machine stops. An installed system saves each shell's world and your edits to root after every line that succeeds. It stores them as omega source at fixed places on the disk ([saving](saving), [disk layout](disk-layout)).

`disks` lists the drives the kernel found, and `home` tells which one Subverge was installed on:

    disks
    home .

`install` lists the drives, asks for one, and erases it when you type `yes`.

> **Warning:** `install` erases the whole drive you pick, partition table included. Check the name it prints before typing `yes`.

An installed disk skips the first-boot questions and remembers the theme you chose.

The next parts are about working in the system: the keys of the prompt, windows, phi and sound.

[Next: Part 13](tour-13)   [Back: Part 11](tour-11)   [The tours](tour)
