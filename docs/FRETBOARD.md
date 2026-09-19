# The fretboard, as the engine sees it

One coordinate system, shared by everything: the shape generator, the quiz, the
JSON layer and the page. Three parts of this project count strings, and if they
counted them differently the bug would look like a chord diagram that is nearly
right.

## Strings are numbered from the lowest pitch, starting at zero

`Position { string, fret }` and every `frets` vector is ordered **lowest-pitched
string first**. In standard tuning that is the low E: index 0.

Players count the other way. The thick string nearest your chin is the *sixth*,
and the thin one nearest the floor is the *first*. Both conventions are
unavoidable - the engine wants an index it can loop over, and a learner wants to
be told "sixth string" - so the translation happens in exactly one place,
`Fretboard::stringName`, and nowhere else. Anything that prints a string name
asks for it; nothing else needs to know the two systems exist.

The page draws the high string at the top, which is how tablature is written and
how the neck looks when the instrument is in your lap. That is a drawing
decision and it lives in the drawing: `renderNeck` iterates downwards.

## A fret is a semitone, and 0 is the open string

`noteAt(string, fret)` is the open string's MIDI note plus the fret number.
There is nothing cleverer than that, and the whole of the neck falls out of it:
the twelfth fret is the open string an octave up, and the fifth fret of one
string is the open string above it - except between the G and B strings, which
are a major third apart, where it is the fourth fret. That exception is the one
irregularity on the instrument and the reason the neck is harder to learn than
a keyboard. `FretboardTests` pins both halves of it.

## Muted is a value, not an absence

`muted` (-1) is a real fret value. On a guitar, *not* playing a string is a
decision made with the fretting hand, it changes the bass note and therefore the
chord, and a shape that leaves it out is missing information rather than saying
nothing. Every `frets` vector has one entry per string, always.

Over the wire it is spelled `-1`, and read back from `"x"`, `"X"`, `"-"` or
`"-1"`, because three shells will spell it three ways.

## The neck stops at the fifteenth fret

`highestFret` bounds the search, not the instrument. Shapes above the fifteenth
are the same shapes further up, most players cannot reach the top of the neck
comfortably, and every extra fret multiplies the shape search. Draw more if you
like - the page draws twelve by default - but do not expect the engine to find a
voicing up there.

## Nothing assumes six strings

A `Tuning` is a list of open strings of any length. Four of them is a bass,
seven is a seven-string, and neither is a special case anywhere: the shape
generator loops over `stringCount()`, the quiz builds its pool from it, and the
page sizes its fret vectors from the board the engine handed it. The one place
that ever knew the number was a test that asserted six, and it was wrong.

Two things do read the count, and both read it rather than assuming it:
`Fretboard::stringName` (the sixth string of a four-string bass is its fourth)
and the CAGED naming in `ChordShapes.cpp`, which is about the *intervals* above
the root rather than about which string it is. See `docs/CHORD_SHAPES.md`.

## Which way round is a drawing decision

Left-handed is not in the engine and must not be. The notes at a position do not
change with the hand holding the instrument; what changes is which side of the
picture the nut is on. The page draws the same cells in the other order - it does
not mirror them with a transform, because a mirrored neck mirrors the note names
written on it too.

## Tunings are arguments, never assumptions

Nothing in the engine contains E-A-D-G-B-E. A `Fretboard` is built from a
`Tuning`, a `Tuning` comes from the catalogue in `Tuning.cpp`, and the catalogue
is what the shells build their menu from. A chord in drop D is a different set
of frets for the same notes, and a quiz in DADGAD asks about a different note at
the same place; neither is a special case anywhere.

The spelling written next to each tuning ("E A D G B E") is for a reader, and the
MIDI numbers beside it are the tuning. `TuningTests` checks the two against each
other, which is the only thing that could.
