# Values and objects

## The value cell

Every value on the stack, in a quote, in a local or in a definition is a cell of three machine words: a type tag, a 64-bit integer and a pointer. A cell is 24 bytes, and the [stack](stack.md) is an array of 1024 of them.

Which fields are used depends on the tag:

| Tag | Integer field | Pointer field |
|---|---|---|
| integer | the value | unused |
| bignum | unused | the digits |
| string | unused | the string object |
| quote | unused | the quote object |
| symbol (`'name`) | unused | the interned name |
| word | unused | the interned name |
| definition (`:name`) | unused | the interned name |
| local assignment (`->name`) | unused | the interned name |
| local list (`{ a b }`) | unused | a quote holding the names |
| ordinal | unused | the ordinal object |
| cardinal | which aleph | unused |

Integers that fit in 64 bits live entirely inside the cell, so arithmetic on them never allocates. That is also why the [JIT](jit.md) can compile `+` into a single `add` with an overflow check: it only has to test the tag of the two top cells and add their integer fields in place.

Cardinals are encoded in the integer field alone. `aleph0` is 0, `aleph1` is 1, and so on. A negative number `-k-1` means 2 to the power of aleph k, so `continuum`, which is 2^aleph0, is stored as -1. See [how infinity works](infinity.md) for how they are compared.

## type

`type` gives the kind of a value as a string:

| Value | `type` |
|---|---|
| `42`, `2 100 ^` | `number` |
| `"hi"` | `string` |
| `[ 1 2 ]`, `[ dup * ]` | `list` |
| `'sq` | `symbol` |
| `omega` | `ordinal` |
| `aleph0` | `cardinal` |

Integers and bignums both give `number`. A word taken out of a list is a symbol, so `[ dup * ] first type` gives `symbol`.

## Words are values

A quote is a list of cells, and the source text is gone after parsing. `[ dup * ]` becomes a quote of two cells, both tagged *word*, pointing at the interned names `dup` and `*`. Running the quote means walking those cells in order.

Because code and data share one representation, a quote can be used as a list (`[ 1 2 3 ] first`) and a list can be run (`[ 1 2 3 ] do` pushes 1, 2 and 3). When a list operation hands you an element that is a word, such as `[ dup * ] first`, it is converted to a symbol first, so taking it out of the list doesn't run it.

## Interned names

Every name the parser sees is interned: looked up in a table of 8192 entries and created there if it's new. Two occurrences of `sq` therefore point at the same object, and comparing names is a pointer comparison. Name lookup in a dictionary never compares strings.

The interned name object also carries the address of the built-in it names, if there is one. Looking up `dup` does not search a table of built-ins: the interpreter checks the tab's definitions, then root's, and if neither binds the name it calls the function stored on the name itself. A definition with the same name hides the built-in without touching it, and `forget` uncovers it again.

Interned names are never freed. They are allocated outside the collected heap, so a name stays in the table even when nothing uses it anymore. The table fills only if a session creates thousands of distinct names, for example with `>sym` in a loop, and filling it stops the machine with `symbol table full`.

## Objects

Strings, quotes, bignums, ordinals and dictionary entries are heap objects. Each has a small header with a link to the next object, a kind, a mark bit and a *pinned* flag. Every tab keeps a list of the objects it has allocated, and its collector walks that list.

Strings are length-prefixed and also zero-terminated, so the length is known without scanning, and the bytes can be handed to the screen code directly. A string can contain any byte except that the zero terminator ends it for words like `find`.

Quotes store their length, their cells, and some JIT bookkeeping: how many times the quote has run, a pointer to its machine code, the length of that code, the JIT version it was compiled under, and whether compiling it was tried and failed. A quote is compiled on its second run ([JIT](jit.md)).

A dictionary entry holds a name, a value and a link to the next entry. A tab's definitions are a singly linked list of these entries, newest first. Defining a word pushes a new entry at the front; it never modifies the old one. This is what makes keeping 64 old worlds cheap ([how worlds work](worlds.md)).

## Pinned objects

Objects created while [root](root.md) is being built are pinned. They are not added to any tab's object list, are never collected, and are shared by all four tabs. When root is rebuilt, the old root's objects are left in place, so a quote that a tab copied out of root stays valid.

## Equality

`=` compares integers and bignums by value and strings by content. Every other kind of value is compared by identity:

    [ 1 2 ] [ 1 2 ] = .          0
    [ 1 2 ] dup = .              1
    "ab" "a" "b" cat = .         1

Two quotes with the same contents are different objects, so they are not equal. `has` uses the same rule.

## What survives a reboot

Saving writes a tab back out as omega source ([saving](saving.md)), so a value survives only if it can be printed as source and parsed back. Integers, bignums, strings, quotes, symbols, ordinals and cardinals all can. Identity does not survive: two definitions that held the same quote object hold two equal copies after a reboot.

A raw address from `alloc` is saved as the integer it is, and after a reboot it points at whatever the heap happens to hold there.
