# The language in detail

omega reads a line in three steps: it splits it into tokens, parses the tokens into a quote, and runs the quote. The sections below cover each step, what values look like inside, and how a name is looked up. Read [omega](omega.md) first.

## Lexical structure

A line is split into tokens on whitespace. Each token is, in this order of precedence:

- a string, starting with `"` and running to the next unescaped `"`, which may contain spaces
- `[` or `]`, which open and close a quote, and `{`, which opens a list of local names closed by `}`
- `#`, which ends the line
- a number: an optional `-`, then decimal digits, `0x` and hex digits, or `0b` and binary digits, of any length up to 2^65536
- `'name`, a symbol
- `:name`, a definition
- `->name`, a local assignment
- anything else, a word

Strings recognise `\n \t \e \\ \"`, plus `\w` and `\A`, which are the bytes `0x80` and `0x81`, reprogrammed into the font as omega and aleph. Unknown escapes keep the character after the backslash. Nesting of brackets is limited, past which the parse fails with `the brackets are nested too deep`. The whole line is parsed before any of it runs, so a parse error means nothing on the line happened.

## Values

A value is 24 bytes, made of a type, a 64-bit integer and a pointer to a heap object. These are the types:

| Type | Payload | Notes |
|---|---|---|
| integer | the i64 | when it fits in 64 bits |
| bignum | heap object | only when an integer outgrows 64 bits, see [bignums](bignums.md) |
| string | heap object | immutable bytes with a length, not NUL-dependent |
| quote | heap object | an array of values, immutable once built |
| symbol | interned name | `'x` and `"x" >sym` are the same object |
| word | interned name | only appears inside quotes |
| ordinal | heap object | Cantor normal form, see [how infinity works](infinity.md) |
| cardinal | the i64 | n for ℵn, -n-1 for 2^ℵn |

Heap objects belong to the tab's [collector](gc.md).

## Evaluation

A quote is evaluated left to right. Literals push themselves. A word is resolved and run. `:name` pops one value and binds it in the tab's dictionary. `->name` pops one value and updates the innermost bound local called `name`, and otherwise binds a new local. `{ a b c }` pops as many values as names and binds them so that the last name gets the top.

When a word is run, it's resolved in this order: bound locals, innermost first; the tab's dictionary; root's dictionary; built-ins. A binding whose value is a quote is called. Any other bound value is pushed. A tab binding with no value, which is what `forget` creates, ends the search of the tab's dictionary and lookup carries on in root, so forgetting a tab definition uncovers whatever it shadowed.

Locals live on a single per-tab array of 256 entries, pushed when bound and popped when the binding quote returns. Lookup scans it from the top, which makes scoping dynamic: a callee sees its caller's locals. Exceeding 256 live locals fails with `too many local names`.

Call depth is limited, and exceeding it fails with `this nests too deep`. The data stack holds 1024 values.

See [limits](limits.md) for the numbers and [how worlds work](worlds.md) for what happens after a line runs.
