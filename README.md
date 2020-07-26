# BlitLife
### an implementation of the Life cellular automaton using the Amiga's hardware blitter

What we have here is an implementation of John Horton Conway's "Life" game,
the famous cellular automaton, in which the growth algorithm is implemented on an Amiga
using the "blitter", controlled by direct hardware pokes.

At bottom, the blitter is does logical operations on large fields of bits,
such as AND or XOR, barrel-shifting them as necessary to align with each other.
By decomposing the numeric operations of the life algorithm (such as counting neighbors)
into a basic logical form, the blitter can implement the Life growth algorithm as a series of steps,
each one acting on the entire map in a single asynchronous operation.
This is probably not perfectly optimized in terms of the sequence of steps necessary to
implement the life algorithm with blitter operations, but it should be reasonably close.

As a user-friendly tool for exploring the life game, this is rudimentary —
it's just an exercise in using the chipset in a role somewhat beyond its intended purpose.

No license, use this however you like.
