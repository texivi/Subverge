# Limits

Subverge uses fixed sizes almost everywhere. Going past one fails with an error message, and the line is undone like any other failure ([transactions](transactions.md)). The messages are listed in [errors](errors.md).

## Values

| What | Limit | Error |
|---|---|---|
| integers | 64 bits, then [bignums](bignums.md) up to 2^65536 | `that number is too big, the limit is 2^65536` |
| `^` with plain integers | exponents of 0 or more | `negative powers aren't supported` |
| ordinal exponent | finite parts up to 64 when written out | `that exponent is too big to write out` |
| ordinal coefficients | 64 bits each | `that coefficient doesn't fit in 64 bits` |
| ordinal terms | 64, later terms are dropped | |
| `range` | 100000 items | `that range is too big` |
| `split` | 256 parts | `that splits into too many parts` |
| `pad` | 4096 characters, wider requests leave the string unchanged | |
| `alloc` | 1 byte to 16 MiB | `alloc takes 1 byte up to 16M` |
| `str` | 511 characters for anything but a bignum | |
| `join` | 64 KiB of text | `that string would be too long` |
| `replace` | 1 MiB of text | `that string would be too long` |
| `rand` | an upper bound of 1 or more | `rand needs a number above 0` |

## Code

| What | Limit | Error |
|---|---|---|
| items in one quote, as typed | 256 | `that quote is too long` |
| nested brackets | 32 levels | `the brackets are nested too deep` |
| names in one `{ }` | 32 | `too many local names` |
| live locals at once | 256 | `too many local names` |
| call depth | 400 nested quotes | `this nests too deep` |
| data stack | 1024 values | `the stack is full` |
| distinct names, ever | 8192 | the machine stops with `symbol table full` |

The limit on items in a quote applies to code as it is parsed. Lists built at run time with `range`, `map`, `append` or `cat` can be longer.

## Tabs and the prompt

| What | Limit |
|---|---|
| tabs | 4 |
| prompt line | 200 characters, less when the stack display is wide |
| prompt history | 16 lines per tab |
| [back](back.md) history | 64 lines per tab |
| key queue | 63 keys per tab, further keys are dropped |
| tab title | 23 characters |
| screen | 80 by 24 per tab, plus the shared tab bar |
| graphics screen | 640 by 480, 16 colours |

## Root and saving

| What | Limit |
|---|---|
| root units | 32 |
| unit name | 1 to 20 characters of `a-z`, `0-9` and `-` |
| names listed per unit | 96 |
| `'name edit` | 16 KiB of text |
| a saved slot | 1 MiB, for root and for each tab |
| an install target | 8 MiB |

A world or root that grows past its slot is not saved, without a message ([saving](saving.md)).

## The machine

| What | Limit |
|---|---|
| kernel heap | 64 MiB, or less on machines with less RAM |
| mapped memory | the first 4 GiB, identity mapped |
| stack per tab | 256 KiB |
| timer | 100 Hz |
| drives | 4, on the two IDE channels |
| PCI devices listed by `pci` | 64 |
| memory map entries | 32 |
| JIT code per quote | 64 KiB |
| help description | 52 characters |
