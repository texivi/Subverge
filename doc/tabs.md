# Tabs

Subverge has four tabs. Each is a complete omega session with its own stack, definitions and screen, and all four run at the same time. The tabs are switched with keys or with the mouse. Tabs are the only form of concurrency in Subverge, and there are always exactly four of them.

## Switching

F1 to F4 and Alt-1 to Alt-4 show tab 1 to 4. A click on a tab title in the top bar does the same when a PS/2 mouse is present ([ps2](ps2.md)). The keys are handled inside the keyboard interrupt, so they work even while every tab is busy, and they are the way out of a program that doesn't respond to Ctrl-C.

Only one tab is shown at a time. Switching copies the chosen tab's screen to the display, so you see it exactly as it was left, including output it printed while it was hidden.

## What each tab has

Each tab has its own:

- **[world](world.md)**: its stack and its definitions;
- **[back](back.md) history** of 64 worlds;
- **[prompt](prompt.md) history** of 16 lines;
- **key queue**. Keys go to the tab in front only;
- **screen** of 80 by 24 characters, with its own cursor and colour;
- **title**, shown in the tab bar;
- **editor state**, so [scrawl](scrawl.md) can be open in two tabs at once;
- **stack for the kernel**, 256 KiB, used by the interpreter and anything it calls.

The tabs share [root](root.md), the kernel heap, the hardware and the disk. A root edit in one tab is visible in all four as soon as it builds. A definition made at a tab's prompt is visible only in that tab, and overrides a root word of the same name there ([names](names.md)).

## The tab bar

The top row belongs to the tab bar. It shows each tab's number and title, highlights the visible one, and shows the time on the right. It is redrawn once a second and on every switch. A `*` after a tab's title means the tab has written to its screen since you last looked at it. When a long job runs in tab 2 while you work in tab 1, the `*` appearing on tab 2 means it finished, failed or printed progress.

## Working with tabs from omega

    mytab .              this tab's number, 1 to 4
    "build" title        renames this tab in the tab bar
    2 tab .              [ "omega" "waiting for keys" 0 ]

`tab` returns a list of the tab's title, its state and whether it is visible. The state is `running` for the tab asking, `waiting for keys`, `sleeping` or `ready`.

Tabs are numbered from 1 in these words, to match the keys that select them. Everything else in omega counts from 0.

Titles are not saved. Every tab starts as `omega` after a reboot, and programs like `top` set their own title while they run and put `omega` back when they exit.

## Running things in parallel

Start a long job in one tab and keep working in another. Press F2 and type:

    0 100000000 [ 1 + ] times .

Then press F1 and run `top`. `top` in tab 1 shows tab 2 as `ready` while it computes, and the idle percentage drops. Tab 2's title gets a `*` when the result is printed.

The tabs share one CPU. The [scheduler](scheduler.md) switches between ready tabs 100 times a second, so two busy tabs each get about half. A tab waiting for a key, or sleeping with `sleep` or `waitkey`, takes no CPU time at all.

Tabs keep their data to themselves, and the only place all four read from is root. Values that every tab should see go in a [root unit](units.md).

## A way to use them

Nothing ties a tab to a job, but a layout like this works well:

| Tab | Use |
|---|---|
| 1 | the prompt you work at |
| 2 | long computations, such as `mandel` or a big bignum |
| 3 | [scrawl](scrawl.md) on the root unit you are editing |
| 4 | `top` or `clock`, left running |

Because every tab has its own definitions, tab 1 can try a new version of a word while tab 3 still has the old one. Once the version in tab 1 works, put it in root with `'name open`, and all four tabs use it.

## Titles in programs

A full-screen program usually sets the title when it starts and puts `omega` back when it leaves, so the tab bar shows what each tab is doing:

    [ "busy" title
      3 [ "working" say 100 sleep ] times
      "omega" title
    ] :job

If the program fails halfway, the title stays as it was, since titles aren't part of the [world](world.md) and aren't undone. `clock` and `top` follow this pattern, and `'clock see` shows it.

## Stopping a tab

Ctrl-C stops the running line in the tab in front with the error `interrupted`, which undoes the line. Every built-in loop and the interpreter check for it between steps.

A line can still ignore Ctrl-C, for example one stuck inside a single long built-in. In that case, switch to another tab and leave it. The stuck tab doesn't stop the others from running. One tab can't stop another, but `reboot` from any tab restarts the machine, which is safe on an installed disk since every completed line has already been [saved](saving.md).

## Faults

If code in a tab causes a CPU exception, for example by reading an address above the 4 GiB that the kernel maps, only that tab is affected. It starts over at its prompt, prints the fault and `your last line was dropped`, and its world is the same as before the line. The other tabs don't notice.

## Startup

At boot, each tab loads its saved world if Subverge is installed. Tab 1 then runs `startup`, which by default shows the system information and asks the first-boot questions ([startup](startup.md)). Tabs 2 to 4 start at an empty prompt.
