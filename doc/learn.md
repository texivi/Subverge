# learn

`learn` is the tutorial built into Subverge. It teaches omega from the stack upwards, one screen per lesson, and each lesson gives you a line to run so you see the result straight away. It assumes you can already program in some language, and it moves quickly.

## Starting

    learn               starts at the first lesson
    14 lesson           starts at lesson 15

Lessons are numbered from 1 on screen, and `lesson` counts from 0 like the rest of omega, so `14 lesson` opens the lesson the header calls `learn 15`.


## How a lesson works

Each lesson shows a title, a short explanation and a suggested line under `Run this, or type your own line:`. The prompt that follows works like the normal [prompt](prompt.md), without history.

- Press Enter on an empty line to skip the exercise.
- Type the suggested line, or anything else, and press Enter to run it.

The lesson then shows the stack after your line. Each lesson starts from an empty stack. If your line fails, the error is printed and the stack is put back as it was before the line, so a mistake never leaves the lesson in a strange state.

After the exercise, any key moves to the next lesson, `b` goes back one, and `q`, Escape or Ctrl-C leaves.

> **Note:** Ctrl-C and C-g at the exercise prompt itself cancel the whole tutorial with the error `cancelled`. Since the tutorial is one line, that also undoes every definition made during it. Leave with `q` after an exercise instead.

## What it keeps

When you leave, `learn` clears the screen and puts back the stack you had before starting. Definitions you made during the lessons are kept, so a word written in the lesson on definitions can be used afterwards.

The whole tutorial runs as a single line, so all the definitions you made in it arrive together as one change to your [world](world.md), and one `back` removes all of them.

## Lessons

| | Lesson | | Lesson |
|---|---|---|---|
| 1 | Operand stack | 44 | Checking the entry |
| 2 | Postfix evaluation | 45 | Root units |
| 3 | Stack words | 46 | Key names |
| 4 | Integers | 47 | Moving along a line |
| 5 | Big numbers | 48 | Deleting text |
| 6 | Truth | 49 | The kill ring |
| 7 | Strings | 50 | Case and swaps |
| 8 | Quotes | 51 | Undo in the line |
| 9 | Definitions | 52 | History |
| 10 | Reflection | 53 | Searching the history |
| 11 | Locals | 54 | Completion |
| 12 | Dynamic scope | 55 | Running a word by name |
| 13 | Conditionals | 56 | Asking about a key |
| 14 | Loops | 57 | Asking about a word |
| 15 | Recursion | 58 | The list of bindings |
| 16 | Tracing | 59 | Binding a key |
| 17 | Lists | 60 | Editing from a binding |
| 18 | Transactions | 61 | Removing a binding |
| 19 | Undo | 62 | Key sequences |
| 20 | Catching errors | 63 | The page viewer |
| 21 | Tabs | 64 | Searching a page |
| 22 | Help | 65 | Moving in the editor |
| 23 | Time | 66 | Killing in the editor |
| 24 | Keys | 67 | The mark and the region |
| 25 | Drawing | 68 | Searching the editor |
| 26 | Themes | 69 | Saving in the editor |
| 27 | Ordinals | 70 | Windows |
| 28 | Cardinals | 71 | Where a window goes |
| 29 | Counting past infinity | 72 | The size of a window |
| 30 | Same size | 73 | Tabs and windows |
| 31 | The hotel | 74 | What phi is |
| 32 | Fractions | 75 | Writing a value |
| 33 | Too many reals | 76 | Looking at a set |
| 34 | Alephs | 77 | Moving and deleting |
| 35 | Hailstones | 78 | Read only sets |
| 36 | Bit parity | 79 | dired |
| 37 | Machine access | 80 | Editing a value |
| 38 | JIT | 81 | phi and the disk |
| 39 | Programs | 82 | Pictures |
| 40 | Small tools | 83 | Credits |
| 41 | Editing a word | 84 | The golden ratio |
| 42 | Writing a program | 85 | Persistence |
| 43 | Registering it in apps | 86 | Where next |

The first part is the language: the stack, values, definitions, locals and their dynamic scope, control flow and lists. The middle covers what makes Subverge different, namely [transactions](transactions.md), [back](back.md), `try`, [tabs](tabs.md), [themes](themes.md), and the ordinals and cardinals. Then the lessons walk through writing a small program, adding it to [apps](apps.md) and putting it in a [root unit](units.md) so it is saved.

Lessons 46 to 84 are about working in the system. They teach the keys of the prompt, the kill ring, the history search, completion, M-x and the help keys ([keys](keys.md)), making your own key bindings, the page viewer and the editor, windows ([tabs](tabs.md)), phi and `dired` ([phi](phi.md)), the gallery and the credits, and `golden`. Some of them show keys that act on the prompt itself, such as M-x and C-h, which are switched off at the prompt of a lesson, so they ask you to try them after you leave.


## Changing the lessons

The tutorial is ordinary omega in the `learn` unit of [root](root.md):

    'learn open

The lessons are a list called `lessons`. Each entry is a list of three strings: the title, the text, and the suggested line, which can be empty for a lesson without an exercise. Lines of text start with two spaces and are separated with `\n`. Adding an entry adds a lesson, and the change takes effect as soon as the unit builds.

The runner, `lesson`, shows how the exercises are made safe. It saves the stack with `depth list`, runs your line with `eval` inside `try`, and spreads the saved stack back if `try` leaves `0`. The same technique works in any program that runs code typed by the user ([catching](catching.md)).
