# omega for Forth programmers

omega is a stack language with postfix syntax and Forth's stack words, so a Forth programmer can read most of it at once. Underneath it works differently: values are typed and garbage collected, code is quotes, names are bound late, and every line is a transaction. This page lists the differences that matter in practice.

## Side by side

| Forth | omega |
|---|---|
| `: sq dup * ;` | `[ dup * ] :sq` |
| `x @` / `x !` on a variable | `x` / `->x` on a local, or a definition |
| `flag IF a ELSE b THEN` | `flag [ a ] [ b ] if` |
| `flag IF a THEN` | `flag [ a ] when` |
| `10 0 DO I . LOOP` | `0 10 [ . ] for` |
| `BEGIN test WHILE body REPEAT` | `[ test ] [ body ] while` |
| `5 0 DO body LOOP` | `5 [ body ] times` |
| `' sq EXECUTE` | `'sq get do`, or `[ sq ] do` |
| `S" text" EVALUATE` | `"text" eval` |
| `>R ... R>` | `{ a }` and `a`: locals |
| `CR` | `nl` |
| `.S` | the [prompt](prompt.md) shows the stack |
| `HEX` | `0x` literals, `hex`, `>hex` |
| `\ comment`, `( comment )` | `# comment` |
| `c@` `c!` | `peek` `poke` |
| `@` `!` on 64-bit cells | `peekq` `pokeq` |
| `ALLOCATE` `FREE` | `alloc` `free` |
| `RECURSE` | the word's own name |
| `FORGET x` | `'x forget`, which only removes the newest definition |

The stack words `dup drop swap over rot nip tuck pick 2dup depth` behave as in Forth. `-rot`, `?dup`, `2drop` and `2swap` aren't built in; define them if you want them, for example `[ rot rot ] :-rot`.

## Truth

Comparisons leave `1` for true, not `-1`, and `0` for false. Conditions treat any non-zero number as true. Code that uses a flag as a bit mask, such as `flag AND` to keep or clear a value, needs `neg` first, since `1 neg` is `-1`.

`and`, `or` and `not` are logical and always return `1` or `0`. The bitwise words are `& | xor ~ << >>`.

## Values

Forth cells are untyped. omega values carry a type: integers, bignums, strings, quotes, symbols, ordinals and cardinals ([values and objects](values.md)). `+` on two strings fails with `+ needs numbers` instead of adding two addresses.

Integers don't wrap. Results past 64 bits become bignums, up to 2^65536, so double-cell arithmetic and `*/` aren't needed ([numbers](numbers.md)). `/` and `mod` truncate toward zero, so `-7 2 mod` is `-1` and `-7 2 /` is `-3`.

Strings are values, not address and length pairs. `"text"` pushes one value, `cat` joins two, `len` gives the length, and the garbage collector frees them ([strings](strings.md)). For raw memory, `alloc` returns a plain address that `peek` and `poke` use and that you `free` yourself.

## Code is quotes

Forth compiles words between `:` and `;`. omega parses a whole line into a quote, a list of values, and `:name` binds the value on top of the stack to a name. A quote is data until something runs it:

    [ dup * ]               a quote on the stack
    [ dup * ] :sq           bound to sq
    [ dup * ] do            run once

Control words are ordinary words that take quotes, in the style of Joy and Factor, so omega needs no immediate words, compile state or `POSTPONE`. A quote is also a list, and the list words work on it ([quotes](quotes.md)).

Definitions can bind any value, not just code: `42 :answer` makes `answer` push 42.

## Late binding

Forth binds a name when a word is compiled, so redefining a word leaves earlier callers with the old one. omega looks names up when they run:

    [ 1 ] :one
    [ one 1 + ] :two
    [ 10 ] :one
    two                     11

Redefining `one` changes `two`. This is what lets a tab override a root word, or a test replace `rand` with a fixed value ([writing programs](programming.md)). It also means a typo in a name is only found when the code runs, with `foo isn't defined`.

A word can call itself by name, since its name is looked up at run time.

## Locals instead of the return stack

Values that would go on the return stack go in locals:

    [ { a b } a a * b b * + ] :hyp2

`{ a b }` pops two values into locals, the last name taking the top. `->name` pops one value into a local, updating it if it is already live. Locals end with the quote that bound them.

Locals are dynamically scoped: a word called from a quote can see that quote's locals. In Forth terms, a local behaves like a variable that exists only while its quote runs and is visible to everything called during that time. A word that refers to a name it doesn't bind picks up its caller's local of that name if there is one ([names](names.md)).

## Transactions

Each line at the prompt is a transaction on the stack and the definitions. If any word fails, the whole line is undone, including definitions it made ([transactions](transactions.md)). An error ends the line and leaves the world as it was, so code needs no cleanup after one. `back` undoes up to 64 lines that succeeded, and `redo` takes a `back` back ([back](back.md)).

`try` catches a failure inside a line, like `CATCH`, but without restoring the stack: values pushed before the failure stay ([catching](catching.md)).

Memory written with `poke` and ports written with `out` are outside the transaction, as they are outside anything in Forth.

## The dictionary

Each tab has its own dictionary, layered over the shared [root](root.md):

- a definition at the prompt goes into the tab's dictionary;
- root words are built from units of source and shared by all tabs;
- a tab definition hides a root word or a built-in of the same name, in that tab only.

`words` lists the tab's own definitions, `see` prints one, and `'name forget` removes the newest definition of a name, uncovering whatever was underneath. `back` does the job of `MARKER`.

## Compilation

Forth compiles every definition. omega interprets a quote the first time it runs and compiles it to x86-64 the second time ([JIT](jit.md)). The compiler inlines the stack words, integer arithmetic with overflow checks, comparisons and the control words when their quotes are written in place, and calls everything else. `'name xray` shows the result with the built-in disassembler.

## Ring 0

omega runs in ring 0 without memory protection, like a Forth on bare hardware. `peek`, `poke`, `in`, `out`, `cpuid`, `cr` and `pci@` access the hardware directly ([omega for smarties](smarties.md)). A fault restarts only the tab that caused it ([tabs](tabs.md)).
