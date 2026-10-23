# GC

Each tab has its own mark-sweep garbage collector. It runs between words, marks every object the tab can still reach, and frees the rest. Tabs never share objects, so one tab's collection never has to stop or look at another tab.

## What's collected

Strings, quotes, bignums, ordinals and dictionary bindings are heap objects. Each tab keeps every object it allocated on one linked list. Integers that fit in 64 bits, cardinals and symbols aren't objects: the first two live inside the value itself, and names are interned once and never freed.

Raw memory from `alloc` isn't an object either, and the collector doesn't know it exists. It's yours to free (see [memory](memory.md)).

## When it runs

At safe points, which are the gaps between words, and only once the tab has allocated past a threshold. After every collection the threshold is set to twice the number of objects that survived, with a minimum of 8192, so a tab that keeps a lot of live data collects less often. Compiled code from the [JIT](jit.md) calls the same safe point on every loop iteration, so long-running compiled loops collect too.

It never runs in the middle of a word, so a built-in word in C can hold pointers to objects in local variables without registering them, as long as it doesn't call back into omega in the meantime.

## The roots

Everything reachable from these survives:

- the current world, the stack snapshot and the dictionary
- all 64 worlds in the [back](back.md) history
- the working stack of the line that's running
- every bound local
- the working dictionary
- the parsed line itself
- the keep stack

Because the old worlds are roots, nothing they point at is freed until they fall off the end of the 64, and `back` can always return to them.

## The keep stack

`do`, `times`, `for`, `while`, `each`, `map` and `filter`, and the `call` function in C that all of them use, pop their quotes off the data stack and then run them, which means for the length of the loop, the quote they're running isn't on the stack anymore. So they push it onto a separate keep stack first, and pop it when they're done, and the keep stack is a root. If you write a built-in that pops something and then calls back into omega, it has to do the same, or the collector will free the thing you're running while you're running it.

If the keep stack is deeper than 256, collection is skipped until it isn't, rather than risk missing a root.

## Freeing quotes

When a quote is freed, any machine code the [JIT](jit.md) compiled for it is retired too, through the same dead list as invalidation, because another tab might be executing it right now.
