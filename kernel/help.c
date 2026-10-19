#ifdef HOST
#include <stdio.h>
#endif
#include "omega.h"
#include "sv.h"
#include "task.h"
#include "vga.h"

struct doc { const char *group, *name, *effect, *text, *example; };
struct grp { const char *name, *text; };

static const struct grp groups[] = {
    { "syntax", "how code is written" },
    { "stack", "move values around" },
    { "math", "arithmetic" },
    { "logic", "comparisons and bit operations" },
    { "flow", "choices and loops" },
    { "lists", "lists, which are also quotes" },
    { "text", "strings" },
    { "io", "printing and keys" },
    { "names", "definitions" },
    { "world", "history, saving and root" },
    { "infinity", "ordinals and cardinals" },
    { "memory", "raw memory" },
    { "ports", "ports, cpu and pci" },
    { "time", "clocks and counters" },
    { "screen", "drawing on the screen" },
    { "machine", "hardware and drives" },
    { "dev", "the jit and the disassembler" },
    { "tools", "ready-made utilities" },
    { "demos", "math demos" },
    { "games", "games and toys" },
    { "util", "small helpers" },
    { "helpers", "words the utilities use inside" },
};

static const struct doc docs[] = {
    { "syntax", "[ ]", "-- q", "A quote: code or a list, not run yet.", "[ 1 2 + ]" },
    { "syntax", ":name", "q --", "Defines a word from the top value.", "[ dup * ] :sq" },
    { "syntax", "->name", "v --", "Sets a local name inside a quote.", "5 ->x   x x +" },
    { "syntax", "{ a b }", "a b --", "Takes values off the stack as locals.", "[ { a b } a b - ] :diff" },
    { "syntax", "'name", "-- 'name", "A symbol: a name used as a value.", "'sq see" },
    { "syntax", "\"text\"", "-- s", "A string. Escapes: \\n \\t \\e \\w \\A.", "\"hi\\n\" write" },
    { "syntax", "42 0x2a", "-- n", "Whole numbers of any size, decimal or hex.", "2 100 ^" },
    { "syntax", "#", "", "A comment to the end of the line.", "1 2 + # three" },

    { "stack", "dup", "a -- a a", "Copies the top value.", "3 dup *   gives 9" },
    { "stack", "drop", "a --", "Throws the top value away.", "1 2 drop   leaves 1" },
    { "stack", "swap", "a b -- b a", "Swaps the top two values.", "1 2 swap   leaves 2 1" },
    { "stack", "over", "a b -- a b a", "Copies the second value to the top.", "1 2 over   leaves 1 2 1" },
    { "stack", "rot", "a b c -- b c a", "Moves the third value to the top.", "1 2 3 rot   leaves 2 3 1" },
    { "stack", "nip", "a b -- b", "Drops the second value.", "1 2 nip   leaves 2" },
    { "stack", "depth", "-- n", "How many values are on the stack.", "1 2 depth   leaves 1 2 2" },
    { "stack", "empty", "...  --", "Clears the whole stack.", "1 2 3 empty" },

    { "math", "+", "a b -- c", "Adds. Also works on ordinals and cardinals.", "2 3 +   gives 5" },
    { "math", "-", "a b -- c", "Subtracts. Whole numbers only.", "5 2 -   gives 3" },
    { "math", "*", "a b -- c", "Multiplies.", "6 7 *   gives 42" },
    { "math", "/", "a b -- c", "Divides, rounding toward zero.", "7 2 /   gives 3" },
    { "math", "mod", "a b -- r", "The remainder after dividing.", "7 3 mod   gives 1" },
    { "math", "^", "a b -- c", "Raises a to the power b, exactly.", "2 100 ^   all 31 digits" },
    { "math", "neg", "a -- -a", "Flips the sign.", "5 neg   gives -5" },
    { "math", "abs", "a -- b", "Drops the sign.", "-5 abs   gives 5" },
    { "math", "min", "a b -- c", "The smaller of two numbers.", "3 9 min   gives 3" },
    { "math", "max", "a b -- c", "The bigger of two numbers.", "3 9 max   gives 9" },

    { "logic", "=", "a b -- 1|0", "1 if equal, otherwise 0. Works on strings.", "3 3 =   gives 1" },
    { "logic", "<", "a b -- 1|0", "1 if a is less than b.", "1 2 <   gives 1" },
    { "logic", ">", "a b -- 1|0", "1 if a is greater than b.", "1 2 >   gives 0" },
    { "logic", "<=", "a b -- 1|0", "1 if a is less than or equal to b.", "2 2 <=   gives 1" },
    { "logic", ">=", "a b -- 1|0", "1 if a is greater than or equal to b.", "2 3 >=   gives 0" },
    { "logic", "and", "a b -- 1|0", "1 if both are non-zero.", "1 0 and   gives 0" },
    { "logic", "or", "a b -- 1|0", "1 if either is non-zero.", "1 0 or   gives 1" },
    { "logic", "not", "a -- 1|0", "1 if a is zero, otherwise 0.", "0 not   gives 1" },
    { "logic", "&", "a b -- c", "Bitwise and.", "12 10 &   gives 8" },
    { "logic", "|", "a b -- c", "Bitwise or.", "12 10 |   gives 14" },
    { "logic", "xor", "a b -- c", "Bitwise exclusive or.", "12 10 xor   gives 6" },
    { "logic", "~", "a -- b", "Flips every bit.", "0 ~   gives -1" },
    { "logic", "<<", "a b -- c", "Shifts a left by b bits.", "1 4 <<   gives 16" },
    { "logic", ">>", "a b -- c", "Shifts a right by b bits.", "256 4 >>   gives 16" },

    { "flow", "do", "q --", "Runs a quote.", "[ 1 2 + ] do   gives 3" },
    { "flow", "if", "c t f --", "Runs t if c is non-zero, otherwise f.", "1 [ \"yes\" ] [ \"no\" ] if" },
    { "flow", "when", "c q --", "Runs q if c is non-zero.", "5 0 > [ \"positive\" say ] when" },
    { "flow", "unless", "c q --", "Runs q if c is zero.", "0 [ \"zero\" say ] unless" },
    { "flow", "times", "n q --", "Runs q n times.", "3 [ \"hi\" say ] times" },
    { "flow", "for", "a b q --", "Runs q for i from a to b-1, i on the stack.", "0 3 [ . ] for   prints 0 1 2" },
    { "flow", "while", "c q --", "Runs c, and while it gives non-zero, q.", "0 [ dup 3 < ] [ 1 + ] while   leaves 3" },
    { "flow", "each", "l q --", "Runs q on each item of a list or string.", "[ 1 2 3 ] [ . ] each" },
    { "flow", "map", "l q -- list", "A new list: q run on each item.", "[ 1 2 3 ] [ 2 * ] map   gives [ 2 4 6 ]" },
    { "flow", "filter", "l q -- list", "Keeps the items where q gives non-zero.", "[ 1 2 3 4 ] [ 2 mod 0 = ] filter   gives [ 2 4 ]" },
    { "flow", "eval", "s --", "Runs a string as code.", "\"1 2 +\" eval   gives 3" },
    { "flow", "trace", "q --", "Runs q a word at a time, showing the stack.", "[ 1 2 + ] trace" },

    { "lists", "list", "... n -- l", "Packs the top n values into a list.", "1 2 3 3 list   gives [ 1 2 3 ]" },
    { "lists", "range", "a b -- l", "The numbers from a up to b-1.", "0 4 range   gives [ 0 1 2 3 ]" },
    { "lists", "append", "l x -- list", "A new list with x on the end.", "[ 1 2 ] 3 append   gives [ 1 2 3 ]" },
    { "lists", "cat", "a b -- c", "Joins two lists or two strings.", "[ 1 ] [ 2 ] cat   gives [ 1 2 ]" },
    { "lists", "len", "l -- n", "How long a list or string is.", "\"hello\" len   gives 5" },
    { "lists", "at", "l i -- x", "Item i of a list or string, from 0.", "[ 5 6 7 ] 1 at   gives 6" },
    { "lists", "first", "l -- x", "The first item.", "[ 5 6 7 ] first   gives 5" },
    { "lists", "last", "l -- x", "The last item.", "[ 5 6 7 ] last   gives 7" },
    { "lists", "rest", "l -- list", "Everything except the first item.", "[ 5 6 7 ] rest   gives [ 6 7 ]" },
    { "lists", "has", "l x -- 1|0", "1 if the list contains x.", "[ 5 6 ] 6 has   gives 1" },
    { "lists", "spread", "l -- ...", "Puts every item of the list on the stack.", "[ 1 2 3 ] spread   leaves 1 2 3" },

    { "text", "str", "v -- s", "Turns any value into a string.", "42 str   gives \"42\"" },
    { "text", "hex", "n -- s", "A number in hex, without the 0x.", "255 hex   gives \"ff\"" },
    { "text", "chr", "n -- s", "The character with that code.", "65 chr   gives \"A\"" },
    { "text", "slice", "s a n -- str", "n characters of s, starting at a.", "\"hello\" 1 3 slice   gives \"ell\"" },
    { "text", "pad", "s n -- str", "Pads with spaces to n wide. Negative pads left.", "\"7\" -3 pad   gives \"  7\"" },
    { "text", "find", "s sub -- i", "Where sub first appears in s, or -1.", "\"hello\" \"ll\" find   gives 2" },
    { "text", "split", "s sep -- l", "Cuts s into a list at each sep.", "\"a,b\" \",\" split   gives [ \"a\" \"b\" ]" },
    { "text", ">num", "s -- n", "Reads a number from a string.", "\"42\" >num   gives 42" },
    { "text", "upper", "s -- str", "Turns letters into capitals.", "\"hi\" upper   gives \"HI\"" },

    { "io", ".", "v --", "Prints a value as code, then a new line.", "\"hi\" .   prints \"hi\"" },
    { "io", "say", "v --", "Prints a value, then a new line.", "\"hi\" say   prints hi" },
    { "io", "write", "v --", "Prints a value with no new line.", "\"a\" write \"b\" say   prints ab" },
    { "io", "emit", "n --", "Prints the character with that code.", "65 emit   prints A" },
    { "io", "nl", "--", "Starts a new line.", "nl" },
    { "io", "color", "n --", "Sets the text colour, 0 to 15.", "14 color \"yellow\" say" },
    { "io", "key", "-- n", "Waits for a key and gives its code.", "key   then press a key" },
    { "io", "key?", "-- n", "The key pressed, or -1. Doesn't wait.", "key?" },
    { "io", "waitkey", "ticks -- n", "Waits up to that many ticks for a key.", "50 waitkey   half a second, -1 if none" },
    { "io", "ask", "s -- str", "Prints a prompt and reads a line.", "\"name? \" ask" },

    { "names", "get", "'x -- v", "Pushes what a name is defined as.", "'sq get   gives [ dup * ]" },
    { "names", "forget", "'x --", "Removes a definition from this tab.", "'sq forget" },
    { "names", ">sym", "s -- 'x", "Makes a symbol from a string.", "\"sq\" >sym get" },
    { "names", "defined?", "'x -- 1|0", "1 if the name is defined or built in.", "'dup defined?   gives 1" },
    { "names", "see", "'x --", "Shows how a name is defined.", "'sq see" },
    { "names", "words", "--", "Lists everything defined in this tab.", "words" },
    { "names", "edit", "'x --", "Opens a name in the editor. Saving redefines it.", "'sq edit" },

    { "world", "help", "'x --", "Explains a word or a group. Alone, lists all.", "'dup help   or just   help" },
    { "world", "back", "--", "Undoes the last line, up to 64 lines.", "back" },
    { "world", "stats", "-- a b c d e", "Objects, collections, names, stack, history.", "stats" },
    { "world", "save", "--", "Saves root and every tab to the drive now.", "save" },
    { "world", "home", "-- n", "The drive Subverge lives on, or -1.", "home" },
    { "world", "root", "--", "Lists the root units and the words in each.", "root" },
    { "world", "open", "'x --", "Edits the root unit that defines x.", "'top open" },
    { "world", "unit", "s --", "Makes a new root unit and opens it.", "\"mine\" unit" },
    { "flow", "try", "q -- 1|0", "Runs q. Leaves 0 and prints the error if it fails.", "[ 1 0 / ] try" },
    { "world", "src", "'x -- s", "The source of the root unit that defines x.", "'top src" },

    { "infinity", "omega", "-- w", "The first infinite ordinal.", "omega 1 +   gives w+1" },
    { "infinity", "epsilon0", "-- e", "The first ordinal where omega^x = x.", "omega epsilon0 ^ epsilon0 =   gives 1" },
    { "infinity", "omega1", "-- w1", "The first uncountable ordinal.", "omega1 card   gives aleph1" },
    { "infinity", "aleph0", "-- c", "The size of the natural numbers.", "aleph0 aleph0 +   gives aleph0" },
    { "infinity", "aleph1", "-- c", "The next size of infinity up.", "aleph0 aleph1 <   gives 1" },
    { "infinity", "aleph", "n -- c", "The nth aleph.", "2 aleph" },
    { "infinity", "continuum", "-- c", "The size of the real numbers, 2^aleph0.", "aleph1 continuum <=   gives 1" },
    { "infinity", "card", "o -- c", "The size of an ordinal.", "omega 2 * card   gives aleph0" },
    { "infinity", "ordinal?", "v -- 1|0", "1 if the value is an ordinal.", "omega ordinal?   gives 1" },

    { "memory", "peek", "addr -- n", "Reads one byte of memory.", "0xB8000 peek" },
    { "memory", "poke", "n addr --", "Writes one byte of memory.", "65 0xB8000 poke" },
    { "memory", "peekd", "addr -- n", "Reads 4 bytes of memory.", "0xB8000 peekd" },
    { "memory", "poked", "n addr --", "Writes 4 bytes of memory.", "0 0xB8000 poked" },
    { "memory", "peekq", "addr -- n", "Reads 8 bytes of memory.", "0xB8000 peekq" },
    { "memory", "pokeq", "n addr --", "Writes 8 bytes of memory.", "0 0xB8000 pokeq" },
    { "memory", "alloc", "n -- addr", "Gets n bytes of zeroed memory, up to 16M.", "64 alloc" },
    { "memory", "free", "addr --", "Gives alloc memory back.", "64 alloc free" },
    { "memory", "heap", "-- used total", "Bytes of heap in use, and in total.", "heap" },

    { "ports", "in", "port -- n", "Reads a byte from an I/O port.", "0x64 in" },
    { "ports", "out", "n port --", "Writes a byte to an I/O port.", "0 0x80 out" },
    { "ports", "ind", "port -- n", "Reads 4 bytes from an I/O port.", "0xCFC ind" },
    { "ports", "outd", "n port --", "Writes 4 bytes to an I/O port.", "0 0x80 outd" },
    { "ports", "pci", "-- l", "PCI devices: [ bus dev fn vendor dev class sub ]", "pci len" },
    { "ports", "pci@", "b d f off -- n", "Reads 4 bytes of a device's PCI config.", "0 0 0 0 pci@" },
    { "ports", "cpuid", "leaf -- a b c d", "Runs cpuid and gives eax ebx ecx edx.", "0 cpuid" },
    { "ports", "cpu", "-- s", "The CPU's name.", "cpu say" },
    { "ports", "cr", "n -- v", "Reads control register 0, 2, 3 or 4.", "0 cr" },
    { "ports", "rdtsc", "-- n", "The CPU's cycle counter.", "rdtsc" },
    { "ports", "irq", "n -- count", "How many times interrupt n has fired.", "1 irq   the keyboard" },

    { "time", "ticks", "-- n", "Timer ticks since boot, 100 a second.", "ticks" },
    { "time", "hz", "-- n", "Ticks per second.", "hz" },
    { "time", "time", "-- y mo d h m s", "The clock: year month day hour minute second.", "time" },
    { "time", "idle", "-- n", "Ticks the CPU spent idle.", "idle" },
    { "time", "sleep", "ticks --", "Waits that long. Ctrl-C stops it.", "100 sleep   waits one second" },
    { "time", "rand", "n -- r", "A random number from 0 to n-1.", "6 rand" },

    { "screen", "put", "ch x y color --", "Puts one character on the screen.", "42 10 5 14 put" },
    { "screen", "text", "s x y color --", "Writes a string at a position.", "\"hi\" 10 5 14 text" },
    { "screen", "screen@", "x y -- ch color", "Reads the character and colour there.", "10 5 screen@" },
    { "screen", "wipe", "--", "Clears the screen.", "wipe" },
    { "screen", "cursor", "x y --", "Moves the cursor.", "0 0 cursor" },
    { "screen", "nocursor", "--", "Hides the cursor.", "nocursor" },
    { "screen", "where", "-- x y", "Where the cursor is.", "where" },
    { "screen", "size", "-- w h", "The screen size: 80 by 24.", "size" },
    { "screen", "title", "s --", "Names this tab in the tab bar.", "\"build\" title" },
    { "screen", "tab", "n -- l", "About tab n: [ title state active ].", "2 tab" },
    { "screen", "mytab", "-- n", "Which tab you are in, 1 to 4.", "mytab" },

    { "machine", "memmap", "-- l", "The memory map: [ base length type ].", "memmap len" },
    { "machine", "kernel", "-- k m", "Kernel image bytes, and bytes of RAM.", "kernel" },
    { "machine", "sys-c", "--", "The system info screen written in C.", "sys-c" },
    { "machine", "reboot", "--", "Restarts the machine.", "reboot" },
    { "machine", "halt", "--", "Stops the CPU. Only a reset gets out.", "halt" },
    { "machine", "drives", "-- l", "Every disk: [ n model megabytes home ].", "drives len" },
    { "machine", "disks", "--", "Lists the disks.", "disks" },
    { "machine", "install", "--", "Installs Subverge on a drive you pick.", "install   erases that drive" },
    { "machine", "install-to", "n --", "Installs on drive n without asking.", "0 install-to   erases drive 0" },

    { "dev", "jit", "--", "Shows if the jit is on and what it compiled.", "jit" },
    { "dev", "jit-on", "--", "Turns the jit on.", "jit-on" },
    { "dev", "jit-off", "--", "Turns the jit off, so only the interpreter runs.", "jit-off" },
    { "dev", "compile", "'x -- addr len", "Compiles a word now. Gives its code address, size.", "'sq compile" },
    { "dev", "addr", "'x -- addr", "The address of a word's compiled code.", "'sq addr" },
    { "dev", "dis", "addr n --", "Disassembles n instructions at an address.", "'sq addr 10 dis" },
    { "dev", "disr", "addr bytes --", "Disassembles a range of bytes.", "'sq compile disr" },
    { "dev", "jitdis", "'x --", "Compiles a word and disassembles all of it.", "'sq jitdis" },
    { "dev", "xray", "'x --", "A word's source, then its machine code.", "'sq xray" },
    { "dev", "bench", "q --", "Times q interpreted, then compiled.", "[ 1000000 [ 1 + ] times ] bench" },

    { "world", "welcome", "--", "The intro screen shown at startup.", "welcome" },
    { "tools", "learn", "--", "A guided tour of the language. Space goes on.", "learn" },
    { "tools", "lesson", "n --", "Starts learn at lesson n, counting from 0.", "28 lesson" },
    { "tools", "apps", "--", "Lists the programs you can run.", "apps" },
    { "tools", "sys", "--", "The logo and system info.", "sys" },
    { "tools", "top", "--", "Live tabs, memory and interrupts. q leaves.", "top" },
    { "tools", "watch", "q --", "Runs q twice a second until a key.", "[ ticks . ] watch" },
    { "tools", "timeit", "q --", "Runs q and says how long it took.", "[ 1000 [ ] times ] timeit" },
    { "tools", "hexview", "addr --", "Hex editor on live memory. q leaves.", "0xB8000 hexview" },
    { "tools", "pcils", "--", "Lists the PCI devices.", "pcils" },
    { "tools", "memls", "--", "Lists the memory map.", "memls" },
    { "tools", "cpuinfo", "--", "What the CPU says about itself.", "cpuinfo" },
    { "tools", "irqmon", "--", "Live interrupt counts. q leaves.", "irqmon" },
    { "tools", "calc", "--", "A calculator. An empty line leaves.", "calc" },

    { "demos", "count", "--", "Counts from 0 past every finite number.", "count" },
    { "demos", "ord-demo", "--", "Ordinal and cardinal arithmetic, live.", "ord-demo" },
    { "demos", "powerset", "l -- list", "Every subset of a list.", "[ 1 2 ] powerset" },
    { "demos", "pairing", "--", "The naturals and the evens match up.", "pairing" },
    { "demos", "diagonal", "--", "Cantor's proof the reals can't be listed.", "diagonal" },
    { "demos", "supertask", "--", "Halve a line forever, in finite time.", "supertask" },
    { "demos", "mandel", "--", "The Mandelbrot set, zooming in. Any key goes on.", "mandel" },
    { "demos", "mandel-at", "ox oy sc mx --", "Draws one view of the set. 65536 means 1.0.", "-32768 0 2816 40 mandel-at" },

    { "games", "snake", "--", "Arrow keys steer, q quits.", "snake" },
    { "games", "life", "--", "Conway's Game of Life on raw memory.", "life" },
    { "games", "rain", "--", "A screensaver. Any key leaves.", "rain" },
    { "games", "fortune", "--", "Prints one short fact.", "fortune" },
    { "games", "sieve", "--", "The sieve of Eratosthenes, drawn live.", "sieve" },
    { "games", "bounce", "--", "Balls bouncing and leaving trails.", "bounce" },
    { "games", "sorting", "--", "Bubble sort drawn as bars.", "sorting" },
    { "games", "stars", "--", "Flying through a starfield.", "stars" },
    { "tools", "clock", "--", "A live clock. q leaves.", "clock" },
    { "tools", "stopwatch", "--", "Counts up. Space marks a lap, q leaves.", "stopwatch" },
    { "tools", "timer", "seconds --", "Counts down, then says the time is up.", "90 timer" },
    { "tools", "todo", "--", "Shows your to-do list.", "todo" },
    { "tools", "todo-add", "s --", "Adds an item to the to-do list.", "\"buy milk\" todo-add" },
    { "tools", "todo-done", "n --", "Removes item n from the to-do list.", "1 todo-done" },
    { "tools", "bases", "n --", "Shows n in decimal, hex, octal and binary.", "255 bases" },
    { "tools", "ascii", "--", "A table of the printable characters.", "ascii" },
    { "tools", "colors", "--", "The 16 colours and their numbers.", "colors" },
    { "tools", "convert", "--", "Lists the unit conversions.", "convert" },
    { "tools", "password", "--", "Prints five random passwords.", "password" },
    { "tools", "km>mi", "n -- s", "Kilometres to miles, as text.", "100 km>mi   gives \"62.13\"" },
    { "tools", "mi>km", "n -- s", "Miles to kilometres, as text.", "100 mi>km   gives \"160.93\"" },
    { "tools", "kg>lb", "n -- s", "Kilograms to pounds, as text.", "10 kg>lb   gives \"22.04\"" },
    { "tools", "lb>kg", "n -- s", "Pounds to kilograms, as text.", "10 lb>kg   gives \"4.53\"" },
    { "tools", "c>f", "n -- s", "Celsius to Fahrenheit, as text.", "37 c>f   gives \"98.60\"" },
    { "tools", "f>c", "n -- s", "Fahrenheit to Celsius, as text.", "98 f>c   gives \"36.66\"" },
    { "games", "pong", "--", "Left and right move the paddle, q quits.", "pong" },

    { "util", "sq", "n -- n2", "Squares a number.", "7 sq   gives 49" },
    { "util", "inc", "n -- n+1", "Adds one.", "7 inc   gives 8" },
    { "util", "dec", "n -- n-1", "Subtracts one.", "7 dec   gives 6" },
    { "util", "kb", "n -- s", "Bytes as a string of K.", "2048 kb   gives \"2K\"" },
    { "util", "mb", "n -- s", "Bytes as a string of M.", "2097152 mb   gives \"2M\"" },
    { "util", "uptime", "-- s", "How long since boot.", "uptime say" },
    { "util", ">hex", "n -- s", "A number as a string like 0xff.", "255 >hex   gives \"0xff\"" },
    { "util", "fmt", "v n -- s", "A value padded to n columns.", "42 6 fmt   gives \"42    \"" },
    { "util", "attr", "fg bg -- n", "A colour byte for put and text.", "14 1 attr" },
    { "util", "fill", "ch x y n c --", "Draws a row of one character.", "42 0 5 20 14 fill" },
    { "util", "bar", "x y w pct c --", "Draws a meter that is pct percent full.", "2 5 20 60 10 bar" },
    { "util", "header", "s --", "Draws a title bar across the top.", "\"hi\" header" },
    { "util", "quit-key?", "k -- 1|0", "1 if k is q, Esc or Ctrl-C.", "113 quit-key?   gives 1" },

    { "helpers", "lessons", "-- list", "The lessons that learn walks through.", "lessons len" },
    { "helpers", "app-list", "-- list", "The programs that apps lists.", "app-list len" },
    { "helpers", "app-cell", "e w --", "Prints one entry of apps.", "" },
    { "helpers", "mb-iter", "cx cy mx -- n", "Steps a point until it escapes, up to mx steps.", "0 0 40 mb-iter   gives 40" },
    { "helpers", "two", "n -- s", "A number as at least two digits.", "7 two   gives \"07\"" },
    { "helpers", "hms", "time -- s", "The time part of time as hh:mm:ss.", "time hms" },
    { "helpers", "mmss", "ticks -- s", "Ticks as mm:ss.cc.", "6150 mmss   gives \"01:01.50\"" },
    { "helpers", "todo-get", "-- list", "The to-do list.", "todo-get" },
    { "helpers", "to-base", "n b -- s", "n written in base b.", "255 16 to-base   gives \"ff\"" },
    { "helpers", "base-row", "label v --", "Prints one row of bases.", "" },
    { "helpers", "dec2", "n -- s", "Hundredths as text with a decimal point.", "6213 dec2   gives \"62.13\"" },
    { "helpers", "alphabet", "-- s", "The letters and digits passwords use.", "alphabet" },
    { "helpers", "password-of", "n -- s", "A random password n characters long.", "12 password-of" },
    { "helpers", "bounce-step", "ball -- ball", "Moves one ball a step and draws it.", "" },
    { "helpers", "sort-col", "mem i --", "Redraws one bar of sorting.", "" },
    { "helpers", "star-step", "star -- star", "Moves one star and draws it.", "" },
    { "helpers", "startup", "--", "Runs on tab 1 at boot: sys, then welcome.", "startup" },
    { "helpers", "show", "o --", "Prints an ordinal in colour.", "omega show" },
    { "helpers", "fact", "s q --", "Prints a label and what q gives.", "\"1+w\" [ 1 omega + ] fact" },
    { "helpers", "fortunes", "-- l", "The facts fortune picks from.", "fortunes len" },
    { "helpers", "food-spot", "-- n", "A random square for snake food.", "food-spot" },
    { "helpers", "draw-food", "n --", "Draws snake food on a square.", "food-spot draw-food" },
    { "helpers", "vendor", "id -- s", "The name of a PCI vendor.", "0x8086 vendor   gives \"intel\"" },
    { "helpers", "classes", "-- l", "The PCI class names.", "classes len" },
    { "helpers", "class-name", "n -- s", "The name of a PCI class.", "1 class-name   gives \"storage\"" },
    { "helpers", "mem-type", "n -- s", "The name of a memory map type.", "1 mem-type   gives \"usable\"" },
    { "helpers", "reg>str", "n -- s", "Four characters packed in a cpuid register.", "0 cpuid drop reg>str" },
    { "helpers", "edx-flags", "-- l", "CPU feature names and bits for edx.", "edx-flags len" },
    { "helpers", "ecx-flags", "-- l", "CPU feature names and bits for ecx.", "ecx-flags len" },
    { "helpers", "show-flags", "reg flags --", "Prints which feature bits are set.", "1 cpuid nip nip nip edx-flags show-flags" },
    { "helpers", "memline", "-- s", "Used and total memory as text.", "memline say" },
    { "helpers", "stat", "label val --", "Prints one line of the sys screen.", "\"os\" \"x\" stat" },
    { "helpers", "logo-moon", "-- l", "The moon half of the logo.", "logo-moon len" },
    { "helpers", "logo-cat", "-- l", "The cat half of the logo.", "logo-cat len" },
    { "helpers", "user", "-- s", "The user name on the sys screen.", "user say" },
    { "helpers", "os", "-- s", "The OS name on the sys screen.", "os say" },
    { "helpers", "host", "-- s", "The host name on the sys screen.", "host say" },
    { "helpers", "kernel-name", "-- s", "The kernel name and version.", "kernel-name say" },
    { "helpers", "row", "label val y --", "Prints one row of the top screen.", "\"hi\" \"x\" 3 row" },
    { "stack", "pick", "... n -- ... x", "Copies the value n below the top.", "1 2 3 2 pick   gives 1" },
    { "stack", "tuck", "a b -- b a b", "Copies the top under the second.", "1 2 tuck   leaves 2 1 2" },
    { "stack", "2dup", "a b -- a b a b", "Copies the top two values.", "1 2 2dup   leaves 1 2 1 2" },
    { "lists", "index", "l x -- i", "Where x is in the list, or -1.", "[ 5 6 7 ] 6 index   gives 1" },
    { "screen", "theme", "'x --", "Switches to a colour theme, and remembers it.", "'raw theme" },
    { "screen", "themes", "--", "Lists the themes and shows the colours.", "themes" },
    { "screen", "theme@", "-- s", "The name of the current theme.", "theme@ say" },
    { "screen", "rgb", "n r g b --", "Sets colour n to red, green, blue 0 to 255.", "14 255 0 0 rgb" },
    { "text", "lower", "s -- str", "Turns capitals into small letters.", "\"Hi\" lower   gives \"hi\"" },
    { "text", "trim", "s -- str", "Drops spaces and newlines at both ends.", "\" hi \" trim   gives \"hi\"" },
    { "text", "starts?", "s pre -- 1|0", "1 if s begins with pre.", "\"hello\" \"he\" starts?   gives 1" },
    { "text", "ends?", "s suf -- 1|0", "1 if s ends with suf.", "\"hello\" \"lo\" ends?   gives 1" },
    { "text", "replace", "s a b -- str", "Replaces every a in s with b.", "\"a-b\" \"-\" \"+\" replace" },
    { "text", "join", "l sep -- s", "Joins a list into one string with sep between.", "[ 1 2 ] \", \" join   gives \"1, 2\"" },
    { "lists", "reverse", "l -- list", "A list or string back to front.", "[ 1 2 3 ] reverse" },
    { "lists", "sort", "l -- list", "Sorts numbers or strings, smallest first.", "[ 3 1 2 ] sort   gives [ 1 2 3 ]" },
    { "lists", "sum", "l -- n", "Adds up a list of numbers.", "[ 1 2 3 ] sum   gives 6" },
    { "lists", "fold", "l x q -- y", "Runs q on x and each item in turn.", "[ 1 2 3 ] 1 [ * ] fold   gives 6" },
    { "math", "sqrt", "n -- r", "The square root, rounded down.", "99 sqrt   gives 9" },
    { "math", "gcd", "a b -- c", "The greatest common divisor.", "12 18 gcd   gives 6" },
    { "world", "redo", "--", "Takes back a back.", "back redo" },
    { "world", "apropos", "s --", "Lists every word whose help mentions s.", "\"string\" apropos" },
    { "tools", "cal", "--", "This month, with today marked.", "cal" },
    { "tools", "month", "y m --", "The calendar for any month.", "2026 12 month" },
    { "tools", "date", "-- s", "Today as year-month-day.", "date say" },
    { "games", "tiles", "--", "Slide and merge numbered tiles.", "tiles" },
    { "helpers", "leap?", "y -- 1|0", "1 if y is a leap year.", "2024 leap?   gives 1" },
    { "helpers", "month-days", "y m -- n", "How many days a month has.", "2024 2 month-days   gives 29" },
    { "helpers", "first-weekday", "y m -- n", "Weekday of the 1st, 0 is Monday.", "2026 10 first-weekday" },
    { "helpers", "month-names", "-- l", "The names of the months.", "month-names first" },
    { "helpers", "month-of", "y m d --", "A month with day d marked.", "2026 10 19 month-of" },
    { "helpers", "set-at", "l i x -- list", "A copy of l with item i replaced.", "[ 1 2 ] 0 9 set-at" },
    { "helpers", "slide-row", "l -- l n", "Slides one row of tiles left.", "[ 2 2 4 0 ] slide-row" },
    { "helpers", "move-board", "b lines -- b n", "Slides the whole tiles board.", "b left-lines move-board" },
    { "helpers", "left-lines", "-- l", "Board rows in the order a left slide uses.", "left-lines" },
    { "helpers", "right-lines", "-- l", "Board rows for a right slide.", "right-lines" },
    { "helpers", "up-lines", "-- l", "Board columns for an up slide.", "up-lines" },
    { "helpers", "down-lines", "-- l", "Board columns for a down slide.", "down-lines" },
    { "helpers", "spawn", "b -- b", "Adds a 2 or a 4 on an empty square.", "b spawn" },
    { "helpers", "moves?", "b lines -- 1|0", "1 if that slide changes the board.", "b up-lines moves?" },
    { "helpers", "can-move?", "b -- 1|0", "1 if any slide changes the board.", "b can-move?" },
    { "helpers", "log2", "n -- k", "How many times n halves to 1.", "1024 log2   gives 10" },
    { "helpers", "tile-color", "n -- c", "The colour of a tile.", "8 tile-color" },
    { "helpers", "draw-tiles", "b score --", "Draws the tiles board.", "b 0 draw-tiles" },
};

#define NDOCS ((int)(sizeof docs / sizeof *docs))
#define NGROUPS ((int)(sizeof groups / sizeof *groups))

static void wrap(const char *s, int indent, u8 color) {
    int x = indent;
    vga_color(color);
    for (int i = 0; i < indent; i++) kputc(' ');
    while (*s) {
        int n = 0;
        while (s[n] && s[n] != ' ') n++;
        if (x + n > 78) { kputc('\n'); for (int i = 0; i < indent; i++) kputc(' '); x = indent; }
        for (int i = 0; i < n; i++) kputc(s[i]);
        x += n;
        s += n;
        if (*s == ' ') { kputc(' '); x++; s++; }
    }
    kputc('\n');
    vga_color(C_TEXT);
}

static void overview(void) {
    kputc('\n');
    for (int g = 0; g < NGROUPS; g++) {
        if (!strcmp(groups[g].name, "helpers") || !strcmp(groups[g].name, "syntax")) continue;
        if (!strcmp(groups[g].name, "tools") || !strcmp(groups[g].name, "demos") || !strcmp(groups[g].name, "games")) continue;
        vga_color(C_SKY);
        kprintf("  %-9s", groups[g].name);
        vga_color(C_TEXT);
        int x = 11;
        for (int i = 0; i < NDOCS; i++) {
            if (strcmp(docs[i].group, groups[g].name)) continue;
            int l = (int)strlen(docs[i].name);
            if (x + l + 1 > 79) { kputs("\n           "); x = 11; }
            kprintf("%s ", docs[i].name);
            x += l + 1;
        }
        kputc('\n');
    }
    vga_color(C_SKY);
    kputs("  programs ");
    vga_color(C_TEXT);
    kputs("learn apps  ");
    vga_color(C_DIM);
    kputs("see 'tools help, 'demos help, 'games help\n");
    vga_color(C_DIM);
    kputs("  'word help explains a word, 'group help a group (also 'syntax, 'helpers)\n");
    vga_color(C_TEXT);
}

static void group_page(const struct grp *g) {
    kputc('\n');
    vga_color(C_SKY);
    kprintf("  %s", g->name);
    vga_color(C_DIM);
    kprintf("  %s\n", g->text);
    for (int i = 0; i < NDOCS; i++) {
        if (strcmp(docs[i].group, g->name)) continue;
        vga_color(C_MOON);
        kprintf("  %-11s", docs[i].name);
        vga_color(C_DIM);
        kprintf("%-15s", docs[i].effect);
        vga_color(C_TEXT);
        kprintf("%s\n", docs[i].text);
    }
}

static void plain_effect(const char *e) {
    char l[16], r[16];
    const char *p = strstr(e, "--");
    int n = p ? (int)(p - e) : 0;
    if (n > 0 && e[n - 1] == ' ') n--;
    memcpy(l, e, (u64)n);
    l[n] = 0;
    scopy(r, p ? p + 2 + (p[2] == ' ') : "", sizeof r);
    vga_color(C_DIM);
    kprintf("  takes %s, leaves %s\n", l[0] ? l : "nothing", r[0] ? r : "nothing");
    kputs("  n is a number, s a string, l a list, q code, 'x a name\n");
}

static void word_page(const struct doc *d) {
    kputc('\n');
    vga_color(C_MOON);
    kprintf("  %s\n", d->name);
    wrap(d->text, 2, C_TEXT);
    plain_effect(d->effect);
    vga_color(C_DIM);
    kputs("  Try:  ");
    vga_color(C_SKY);
    kprintf("%s\n", d->example);
    vga_color(C_DIM);
    kprintf("  Group: %s. Type '%s help to see the rest of it.\n", d->group, d->group);
    vga_color(C_TEXT);
}

static void b_help(void) {
    if (W->sp > 0 && W->stk[W->sp - 1].t == T_SYM) {
        str *s = (str *)pop().p;
        for (int g = 0; g < NGROUPS; g++)
            if (!strcmp(groups[g].name, s->s)) { group_page(&groups[g]); return; }
        for (int i = 0; i < NDOCS; i++)
            if (!strcmp(docs[i].name, s->s)) { word_page(&docs[i]); return; }
        if (lookup(s)) { vga_color(C_DIM); kprintf("  %s is yours. Type '%s see to read it.\n", s->s, s->s); vga_color(C_TEXT); }
        else fail("there is no help for %s", s->s);
        return;
    }
    overview();
}

static int has_ci(const char *h, const char *n) {
    for (; *h; h++) {
        int i = 0;
        while (n[i] && h[i] && (h[i] | 32) == (n[i] | 32)) i++;
        if (!n[i]) return 1;
    }
    return !*n;
}

static void b_apropos(void) {
    str *s;
    if (!pop_str("apropos", &s)) return;
    int found = 0;
    for (int i = 0; i < NDOCS; i++) {
        if (!has_ci(docs[i].name, s->s) && !has_ci(docs[i].text, s->s)) continue;
        if (!found++) kputc('\n');
        vga_color(C_MOON);
        kprintf("  %-11s", docs[i].name);
        vga_color(C_DIM);
        kprintf("%-9s", docs[i].group);
        vga_color(C_TEXT);
        kprintf("%s\n", docs[i].text);
    }
    vga_color(C_DIM);
    if (!found) kprintf("  nothing mentions %s\n", s->s);
    vga_color(C_TEXT);
}

const struct builtin help_words[] = { { "help", b_help }, { "apropos", b_apropos }, { NULL, NULL } };

#ifdef HOST
int help_check(void) {
    extern const struct builtin core_words[], hw_words[], ord_words[], root_words[], persist_words[], jit_words[], dis_words[];
    const struct builtin *sets[] = { core_words, hw_words, ord_words, root_words, persist_words, jit_words, dis_words, help_words };
    int bad = 0;
    for (unsigned k = 0; k < sizeof sets / sizeof *sets; k++)
        for (const struct builtin *b = sets[k]; b->name; b++) {
            int found = 0;
            for (int i = 0; i < NDOCS; i++) if (!strcmp(docs[i].name, b->name)) found = 1;
            if (!found) { printf("undocumented builtin: %s\n", b->name); bad++; }
        }
    for (bind *b = root_dict; b; b = b->next) {
        if (b->v.t == T_NONE) continue;
        int found = 0;
        for (int i = 0; i < NDOCS; i++) if (!strcmp(docs[i].name, b->name->s)) found = 1;
        if (!found) { printf("undocumented root word: %s\n", b->name->s); bad++; }
    }
    for (int i = 0; i < NDOCS; i++) {
        if (!strcmp(docs[i].group, "syntax")) continue;
        str *n = intern(docs[i].name, (u32)strlen(docs[i].name));
        if (!n->fn && !lookup(n)) { printf("doc for a word that doesn't exist: %s\n", docs[i].name); bad++; }
        int g, ok = 0;
        for (g = 0; g < NGROUPS; g++) if (!strcmp(groups[g].name, docs[i].group)) ok = 1;
        if (!ok) { printf("bad group on %s\n", docs[i].name); bad++; }
        if (strlen(docs[i].text) > 52) { printf("text too long: %s\n", docs[i].name); bad++; }
        if (strlen(docs[i].effect) > 15) { printf("effect too long: %s\n", docs[i].name); bad++; }
    }
    return bad;
}
#endif
