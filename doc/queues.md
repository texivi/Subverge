# Queues and rings

The kernel has a few queues that an interrupt handler fills and a task empties. Each is a ring with a head where entries are added and a tail where they are taken, so neither side needs a lock. A ring holds one entry fewer than it has slots.

| Ring | Filled by | Emptied by |
|---|---|---|
| key queue, one for each tab | the keyboard handler | the program, with `key`, `key?`, `waitkey` and the prompt |
| mouse events | the mouse handler | the program or viewer that reads the mouse |

## The key queue

The keyboard handler puts a key into the queue of the tab in front, and the program takes it out. When the queue is empty the program waits and the scheduler doesn't run it until a key arrives ([scheduler](scheduler)). Keys pressed while the queue is full, after 63 keys, are dropped. When a line fails, the prompt throws away the keys that arrived during it, so keys typed while a broken loop was running don't reach the next prompt. Printable keys are their ASCII code, and the arrow and paging keys are 256 and up ([keyboard](keyboard)).

## Mouse events

The mouse handler records a button press or a wheel step, and not every movement, so a drag doesn't fill the ring. The ring holds 15 events and drops new ones when it is full. The pointer position is kept apart and is read with `mouse` ([mouse](mouse)).
