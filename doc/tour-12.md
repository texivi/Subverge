# Part 12: Keeping your work

On the live ISO everything is lost when the machine stops. An installed system saves each shell's world and your edits to root after every line that succeeds. It stores them as omega source at fixed places on the disk ([saving](saving), [disk layout](disk-layout)).

`disks` lists the drives the kernel found, and `home` tells which one Subverge was installed on:

    disks
    home .

`install` lists the drives, asks for one, and erases it when you type `yes`.

> **Warning:** `install` erases the whole drive you pick, partition table included. Check the name it prints before typing `yes`.

An installed disk skips the first-boot questions and remembers the theme you chose.

That is the end of the tours. Go on with [first steps](first-steps), [learn](>learn) or the [index](index).

[Back: Part 11](tour-11)   [The tours](tour)
