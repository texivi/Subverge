# Part 16: The viewer and the editor

## The page viewer

The pages you are reading are in the page viewer,. SPC, C-v and Page Down scroll a screen, and M-v and Page Up scroll back. C-n and C-p move a line. TAB and n select the next link, BTAB and p the one before, and RET follows it. `l` or DEL goes back, `t` goes to the index and `g` asks for a page by name.

C-s asks for text and jumps to the next line that has it, and C-r searches backward. An empty answer repeats the last search. M-x runs a word and closes the viewer, `c` shows the credits and `G` the gallery, and `q` leaves.

    "keys" doc

## The editor

scrawl uses the same keys for moving: C-f, C-b, C-n, C-p, C-a, C-e, M-f, M-b, M-< and M->, C-v and M-v for a screen. C-l puts the cursor line in the middle.

Kills go into the kill buffer. C-k kills to the end of the line, and on an empty line it takes the line break, so two C-k take a whole line. M-d and M-DEL kill words, and C-y yanks.

C-SPC sets the mark. The text between the mark and the cursor is the region: C-w kills it, M-w copies it and C-x C-x swaps the cursor and the mark. C-g clears the mark.

C-s and C-r search, and M-g goes to a line number. C-x C-s saves and C-x C-c leaves. Open a word and try them:

    'countdown edit

[Next: Part 17](tour-17)   [Back: Part 15](tour-15)   [The tours](tour)
