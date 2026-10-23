# Demo Index

These are arranged in increasing order of difficulty, more or less, and can be used as a tutorial if you start at the top and work down. Click a program to run it, or type its name in a shell.

## Start here

- [first steps](first-steps), a short session at the prompt
- [tour](tour), the same session typed out for you
- [learn](>learn), 47 lessons on omega from the stack upwards

## Small tools

- [clock](>clock) shows the time and date
- [stopwatch](>stopwatch) counts hundredths and records laps
- [timer](>90~timer) counts down from 90 seconds
- [calc](>calc) is an infix calculator
- [todo](>todo) keeps a list
- [convert](>convert) converts units
- [password](>password) makes passwords
- [cal](>cal) prints the month
- [colors](>colors) draws the sixteen colours
- [themes](>themes) lists the themes

## Games

- [snake](>snake)
- [tiles](>tiles)
- [pong](>pong)
- [life](>life)

## Animations

- [rain](>rain)
- [stars](>stars)
- [bounce](>bounce)
- [sorting](>sorting)
- [sieve](>sieve)
- [mandel](>mandel), which zooms into the Mandelbrot set and shows what the JIT does for a tight loop
- [fortune](>fortune)

## Infinity

These act out arguments from Vsauce's video on counting past infinity ([demos](demos), [ordinals](ordinals), [cardinals](cardinals)).

- [count](>count) counts from 0 past ω and on to ω₁
- [ord-demo](>ord-demo) lists ordinal and cardinal arithmetic as facts
- [pairing](>pairing) sets the naturals against the evens
- [hotel](>hotel) is Hilbert's hotel with one new guest and then infinitely many buses
- [rationals](>rationals) numbers every positive fraction
- [ulam](>ulam) draws the primes on a square spiral
- [diagonal](>diagonal) is Cantor's diagonal argument
- [supertask](>supertask) halves a line forever

## Reading the machine

- [top](>top) shows the CPU, the heap, the shells and the interrupts live
- [irqmon](>irqmon) counts interrupts as they arrive
- [sys](>sys) prints the system information
- [pcils](>pcils) lists the PCI devices
- [memls](>memls) lists the firmware's memory map
- [cpuinfo](>cpuinfo) shows what the CPU reports
- [disks](>disks) lists the IDE drives
- [hexview](>0xB8000~hexview) edits live memory, starting at the text screen

## Looking inside

Click a line to type it into the first shell.

    'mb-iter see
    'mb-iter xray
    [ -32768 0 2816 40 mandel-at ] bench

`see` prints the source of a word, `xray` the machine code the JIT made for it ([dis](dis)), and `bench` times a quote with the JIT off and then on ([jit](jit)). `'name open` edits any word in [scrawl](scrawl).

The [tools](tools), [games](games) and [demos](demos) pages describe each one.

## Starting a program

A line in this index can be selected and run with Enter. The same names work at the prompt of a shell.
