# Quick Start

This is the first five minutes. Nothing here can damage the system.

## Look around

Type `docs` and press Enter for the help pages. This page is one of them. Press Tab to select the next link and Enter to follow it, Backspace to go back and `q` to leave. A click with the mouse follows a link as well.

## Type

The `»` is the prompt. Type

    2 3 +

and press Enter. The result stays on the stack, which the prompt shows in grey in front of the `»`. In a help page, Tab selects a line of code and Enter runs it at the prompt.

## Make something

    [ dup * ] :sq
    7 sq .

The first line makes a word called `sq` and the second uses it. If a line fails it is undone, and `back` undoes a line that worked.

## Move

Press F2 for the second tab. It has its own stack and doesn't know `sq`. Press F1 for the first. Alt-1 to Alt-4 do the same. Ctrl-C stops a line that is still running.

## Go on

The [tour](tour) is twelve short parts that type what they show. [First steps](first-steps) is the same session in prose. [Tips](tips) lists things worth knowing early and [key map](keys) lists every key. The [demo index](demo-index) lists every program.
