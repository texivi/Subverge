# Task records

A task is the record the kernel keeps for something that runs. There are four, one for each shell. The idle task has a record of its own ([idle](idle.md)).

## The record

| Field | Holds |
|---|---|
| `id` | the slot, from 0 to 3, and -1 for the idle task |
| `entry` | the function the task runs, empty for a slot that is free |
| `stack` | the 256 KiB kernel stack, taken from the heap the first time the slot is started |
| `wait_key`, `wake_at` | what the task is waiting for: a key, a tick, or neither ([scheduler](scheduler.md)) |
| key queue | the keys waiting for the task ([queues](queues.md)) |
| `brkflag` | set by Ctrl-C and checked by the interpreter between steps |
| `term` | the screen of the task, 80 by 24 cells with their colours and the cursor |
| `title` | the name shown in the tab bar |
| `fault` | the message of the last fault |
| `om` | the omega world of the task |

## Starting and ending

A task starts with a stack filled with a known byte, so that `top` can tell how much of it was used. A task never returns from its entry.

See [scheduler](scheduler.md) for how the next task is chosen.
