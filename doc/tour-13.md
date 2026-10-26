# Part 13: The prompt like Emacs

The prompt edits a line with the keys of Emacs. A key is written as Emacs writes it: C- is Control and M- is Alt, so C-a is Ctrl-A and M-f is Alt-F. DEL is Backspace, RET is Enter and TAB is Tab ([keys](keys)).

Type this line at the prompt, without pressing RET:

    1 2 + 3 4 + * 5 6 + *

C-a moves to the start of the line and C-e to the end. M-f and M-b move over words, C-f and C-b over single characters. Press C-a, then M-f twice, and the cursor sits after `2`.

## Killing and yanking

Deleting text with C-k, C-u, C-w, M-d or M-DEL is called killing, because the text is kept on the kill ring. C-k kills from the cursor to the end of the line. C-y yanks it back where the cursor is, and M-y right after C-y swaps the yanked text for an older kill. The ring holds the last eight kills.

Two kills in a row with no other key between them make one piece. Put the cursor at the start of `1 2 +`, press M-d three times, and then C-y: all three words come back together.

DEL and C-d delete one character and keep nothing.

## Small edits

C-t swaps the two characters at the cursor. M-u, M-l and M-c turn the next word into capitals, small letters or a capitalized word. C-_ undoes the last change to the line, and again undoes the one before it. A run of typed letters counts as one change.

## Earlier lines

C-p and C-n step back and forward through the last 16 lines of the tab, and M-< and M-> go to the oldest line and back to the empty one. C-r searches them for the text you type, newest first: type `+`, and the last line with a plus appears. C-r again goes to the next older match, C-s turns around, C-g puts the line back as it was, and any other key leaves the match in the line. RET leaves it there without running it.

## Completion

TAB completes the word before the cursor. Type `them` and press TAB, and it becomes `theme`, the part that `theme`, `theme@` and `themes` share. Press TAB again and the first of them appears, and each further TAB shows the next.

C-c or C-g throws the line away. See [prompt](prompt) for completion, history and the length of a line.

[Next: Part 14](tour-14)   [Back: Part 12](tour-12)   [The tours](tour)
