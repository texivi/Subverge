# Writing programs

A program in Subverge is an omega word. The two examples below are a guessing game that runs at the prompt and a full-screen reaction timer. After them come structuring, testing, installing and sharing programs.

Read [first steps](first-steps.md), [locals](names.md) and the [flow](flow.md) words first.

## Where to write

Short words can be typed at the prompt. Anything longer than a line goes in the editor:

    'guess edit         a definition in this tab
    "games2" unit       a new root unit, shared by every tab

Start with `edit` while the program is changing quickly. `edit` stores a parsed quote, so `back` undoes a bad change and comments are lost. Move the finished program into a [unit](units.md), where comments are kept, and every tab can run it.

## A guessing game

The program picks a number from 1 to 100 and reads guesses until one is right.

    'guess edit

and type:

    "guess" title
    100 rand 1 + 0 1 { secret tries going }
    "  I picked a number from 1 to 100." say
    [ going ] [
      "  guess: " ask { line }
      tries 1 + ->tries
      [ line >num ] try [
        { n }
        n secret = [
          "  right, in " tries str cat " tries" cat say
          0 ->going
        ] [
          n secret < [ "  higher" ] [ "  lower" ] if say
        ] if
      ] when
    ] while
    "omega" title

Save with C-x C-s, leave with C-x C-c, and run it:

    guess

How it works:

- `"guess" title` names the tab while the game runs, and the last line puts the name back.
- `100 rand 1 +` is a number from 1 to 100. `0 1` are the starting values of the guess counter and the loop flag, and `{ secret tries going }` binds all three as locals.
- `[ going ] [ ... ] while` repeats until something sets `going` to 0.
- `ask` prints its argument and reads a line. Ctrl-C at that prompt fails `ask` with `cancelled`, which ends the whole line and with it the game.
- `[ line >num ] try` turns the text into a number. If the text isn't a number, `>num` fails, `try` prints the error in red and leaves `0`, and `when` skips the rest. If it is, `try` leaves the number and `1`, and `when` runs the quote, which takes the number as the local `n`.
- `->tries` and `->going` update locals bound outside the loop. Assignments always go to the innermost live local with that name.

`if` takes two quotes, and `[ "  higher" ] [ "  lower" ] if say` picks a string and then prints it.

## Testing pieces

Words are easiest to test on their own. Split the parts that compute something from the parts that print or read keys:

    [ { n secret } n secret = [ "right" ] [ n secret < [ "higher" ] [ "lower" ] if ] if ] :verdict
    10 50 verdict           "higher"
    50 50 verdict           "right"

A word that only takes values and leaves values can be tried at the prompt, in any order, without starting the program.

To watch a word run, `trace` steps through it one item at a time:

    [ 10 50 verdict ] trace

Space steps, `c` runs the rest and `q` stops. The stack is shown before every step.

To make `rand` and `ask` predictable while testing, define your own versions in the tab:

    [ drop 49 ] :rand
    [ drop "50" ] :ask
    guess
    'rand forget 'ask forget

A tab definition hides a built-in of the same name, so `guess` now picks 50 and always guesses 50. `forget` removes the stand-ins and uncovers the built-ins again ([names](names.md)).

## A full-screen program

The second program measures reaction time. It shows a grey bar, waits a random time, turns the bar red and counts how long it takes you to press a key.

    "react" unit

and type:

    # reaction timer

    [ [ key? -1 = not ] [ ] while ] :drain-keys

    [ { dt }
      'react-best defined? [ react-best ] [ 999999 ] if { best }
      dt best < [ dt :react-best ] when
    ] :react-score

    [ "react" title wipe nocursor
      "Press any key when the bar turns red. q leaves." 2 1 7 text
      1 { going }
      [ going ] [
        "wait for it...        " 2 3 8 text
        32 0 5 80 7 fill
        200 rand 100 + sleep
        drain-keys
        32 0 5 80 0 4 attr fill
        ticks { t0 }
        key { k }
        ticks t0 - 10 * { ms }
        k quit-key? [ 0 ->going ] [
          ms react-score
          ms str " ms, best " cat react-best str cat " ms      " cat 2 3 14 text
          100 sleep drain-keys
        ] if
      ] while
      wipe "omega" title
    ] :react

Save, leave, and run `react`.

### The screen

`wipe` clears the tab's screen and `nocursor` hides the cursor. From then on the program draws at fixed positions instead of printing:

- `"text" x y colour text` writes a string at column `x`, row `y`;
- `ch x y n colour fill` writes `n` copies of character `ch`, here 32, a space.

Rows count from 0 at the top of the tab's area, under the tab bar, and there are 24 of them. A space drawn with a background colour is a solid block, so `32 0 5 80 0 4 attr fill` paints row 5 red: `0 4 attr` is colour 0 on colour 4. Colours are numbers whose actual colour comes from the [theme](themes.md); 4 is red in every theme, 7 is normal text and 14 the accent.

The last line, `wipe "omega" title`, leaves the tab as the program found it. Every bundled program ends this way.

### Timing

`ticks` counts hundredths of a second since boot, so the difference between two readings times ten is milliseconds. `200 rand 100 + sleep` waits between one and three seconds without using the CPU.

### Keys

`drain-keys` throws away keys pressed too early: `key?` returns the next key or `-1` without waiting, and the loop reads until the queue is empty. Without it, a key pressed during the wait would count as an instant reaction. `key` waits for the next key and returns its code, and `quit-key?` is true for `q`, Escape and Ctrl-C ([keyboard](keyboard.md)).

### Keeping a best score

`react-score` keeps the best time in a definition called `react-best`, made at run time with `:react-best`. Definitions made while a program runs go into the tab's [world](world.md) like any other, so:

- the best score is kept between runs, and on an installed disk across reboots;
- each tab has its own best score;
- `back` after a run undoes the new best;
- if the run is interrupted with Ctrl-C, the whole line fails and nothing it defined is kept.

`'react-best defined?` checks whether the word exists yet, so the first run starts from a large number.

## Adding it to apps

`apps` prints a hand-written list ([apps](apps.md)). Open it:

    'apps open

and add a line inside the inner brackets:

    [ "react" "Reaction timer." ]

The program now shows in `apps` in every tab.

## Structure

Programs in Subverge stay readable when they follow the conventions the bundled ones use:

- **Small words.** A screen of code is a lot of omega. Put each step in its own word with a name that says what it does, as `drain-keys` and `react-score` do.
- **Prefixes for helpers.** All words in root share one name space. Name helpers after their program, such as `react-score`, so they don't collide with other units. A unit that redefines an existing word gets a `shadows:` warning only for built-ins, not for other root words.
- **Locals for state, the stack for passing.** Bind a program's state as locals at the top and update it with `->`. Pass values between small words on the stack.
- **One exit path.** End the main loop through its flag, then clean up: restore the title, `wipe`, and `free` any memory from `alloc`.
- **Colours by role.** Use 7 for text, 8 for hints, 12 for errors and 14 for headings, so the program follows the theme.
- **Fail early.** A word that gets bad input should fail with a clear word, such as `>num` on a string that isn't a number, rather than carry on. The line is undone and nothing half-finished is left in the world.

## Speed

Most programs need no tuning. When one does:

- `[ code ] timeit` times one run, and `[ code ] bench` runs it interpreted and compiled ([JIT](jit.md)).
- Loops with their quotes written literally in front of `times`, `for`, `while` and `if` are compiled into jumps. Loops whose quotes come from locals are called each round instead.
- Compiled code looks locals up by name through a call on every use. In the innermost loop, keep values on the stack.
- Drawing is the usual bottleneck in full-screen programs. Redraw only what changed, as `snake` does with the head and tail.

## Sharing a program

A unit is plain text. `'react src say` prints its source, and the same text can be typed into `"react" unit` on another system.
