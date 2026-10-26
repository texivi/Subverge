# Part 15: Your own keys

`bind` runs a quote when a key is typed at the prompt. It takes the name of the key and the quote:

    "M-j" [ "dup " insert ] bind

After this line, M-j types `dup ` into the line you are editing. `insert` types a string at the cursor, and it only works inside a binding.

A key name is written as on the [key map](keys): `"C-k"`, `"M-u"`, `"<up>"`, or a sequence that starts with C-x such as `"C-x C-t"`. Plain printable keys can't be bound, since they type text, and neither can C-h or RET.

## Working on the line

A binding can read and change the line:

| Word | Effect |
|---|---|
| `line@` | the line as a string |
| `line!` | replaces the line with a string |
| `point@` | the cursor position, counted from 0 |
| `point!` | moves the cursor |
| `insert` | types a string at the cursor |

This binding turns the whole line into capitals:

    "M-i" [ line@ upper line! ] bind

Type some letters and press M-i to see it.

## What a binding is

A binding is a definition named `key:` and the key, so `key:M-i` here. It lives in the world of the tab, which means it is saved with it, `back` undoes it, and it is gone when you `forget` it. A binding in a [root unit](units) works in every tab.

A binding runs on a copy of the stack. What it leaves behind is dropped, and an error in it is printed on the line below before the prompt returns.

## Removing and listing

    "M-j" unbind
    "M-i" unbind
    bindings

`bindings` shows the bindings of yours under the heading Yours.

[Next: Part 16](tour-16)   [Back: Part 14](tour-14)   [The tours](tour)
