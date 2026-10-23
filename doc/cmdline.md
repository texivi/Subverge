# Command Line Overview

The prompt feeds a line at a time to the omega compiler. A line that is not inside a definition runs as soon as you press Enter, and no main is needed. Definitions go into the shell's [world](world) and stay until you remove them.

    2 3 + .

Words follow their arguments, so `+` comes after the numbers it adds. The stack is printed in grey before the `»`, which shows what the next word will see:

    5 »

A bare name runs it:

    sys
    top

A quoted name, `'sq`, passes the name without running it. Words that work on names take them that way:

    'sq see
    'sq help
    'sq open

`see` prints the source of a definition, `help` shows its page and `open` edits it in [scrawl](scrawl). The page for a word is in the [index of words](words).

## Looking for a word

    "sort" apropos

lists every word whose name or description mentions `sort`. Tab completes a name ([prompt](prompt)),. `words` lists what you have defined in the shell, and `root` lists the programs that come with the system.

## Undoing

A line that fails changes nothing. A line that worked can be undone:

    1 2 3
    back
    redo

`back` undoes the last line and `redo` undoes the `back`, up to 64 lines each way ([back](back), [transactions](transactions)).

## Definitions

    [ dup * ] :sq
    7 sq .

The quote on the left becomes the word `sq` in this shell. A definition in the shell hides a root word of the same name in that shell only ([names](names)). To make a word available in every shell, put it in a root unit with `'name open`.

## Code that spans lines

A line holds up to 200 characters. Longer programs belong in [scrawl](scrawl):

    'newword open

opens an editor on the word, and saving builds it. On an installed disk, words defined in a shell are saved with the shell, and edits to root are saved with root ([saving](saving)).

## Running things

Programs are words, so they run like words:

    snake
    90 timer
    0xB8000 hexview

A line of code in a help page can be selected with Tab and run with Enter, which closes the viewer and runs the line at the prompt. `docs` opens the index of pages.

## Stopping

Ctrl-C stops the line the shell is running and undoes it. If the shell doesn't respond, move to another one with F1 to F4 or Alt-1 to Alt-4. The others keep running.

## More

[Prompt](prompt) covers the editing keys and the line limit. [Stack](stack), [quotes](quotes), [names](names), [flow](flow), [strings](strings) and [numbers](numbers) describe the language. [First steps](first-steps) is a short session, and [tour](tour) is the same session typed out for you.
