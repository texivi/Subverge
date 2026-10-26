# Part 14: Asking the system

The C-h keys ask the system about itself. C-h waits for a second key.

## What does this key do?

C-h k waits for a key and says what it does at the prompt, in the page viewer and in the editor. The word behind it is `describe-key`. Run it and press C-k when it asks:

    describe-key

It prints the name of the key, the command it runs at the prompt and the line that says what the command does, and then the same key in the other places that use it.

## What does this word do?

C-h f asks for a word, with completion, and shows its help, which is what `'name help` does:

    'filter help

C-h a asks for text and lists every word whose help mentions it. The word is `apropos`:

    "line" apropos

C-h i opens the [help pages](index).

## Every key

C-h b opens a page that lists every key at the prompt, in the viewer and in the editor, and every binding of yours. It is the page viewer, so C-s searches it, and `q` leaves:

    bindings

## M-x

M-x asks for a word on a new line, with completion, runs it, and brings back the line you were editing. It works at the prompt and in the viewer. Leave this page, type `M-x` and then `sy` and TAB, and the word `sys` completes.

M-x and C-h are switched off at the prompt of the lessons in `learn`, since that prompt belongs to the lesson. Use them at the normal prompt.

[Next: Part 15](tour-15)   [Back: Part 13](tour-13)   [The tours](tour)
