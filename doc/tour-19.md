# Part 19: Sound and pictures

## Songs

The kernel has a Sound Blaster 16 driver, a synthesizer and six songs ([sound](sound)):

    songs
    sound? .

`sound?` is 1 when a card was found. `play` starts a song and returns at once:

    "canon" play

The song goes on while you do anything else, in any tab, and the top bar shows `play`. `playing?` says whether a song is going and `stop` ends it:

    playing? .
    stop

## Pictures

`gallery` shows the built-in pictures: photographs, drawings and screenshots of Subverge. The arrows, SPC and n go on, p goes back and q leaves:

    gallery

`credits` shows the people on four pages:

    credits

## The golden ratio

`golden` draws golden rectangles with their spiral, a sunflower of seeds at the golden angle and the digits of φ. SPC moves to the next view and q leaves:

    golden

That is the end of the tours. Go on with [first steps](first-steps), [learn](>learn) or the [index](index).

[Back: Part 18](tour-18)   [The tours](tour)
