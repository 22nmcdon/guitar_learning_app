# Where the shapes come from

Read this before changing `ChordShapes.cpp`. The generator looks like a search
and is really a filter, and the interesting decisions are all in what it throws
away.

## It is not a table of shapes

There is no dictionary of chord diagrams in this repository, deliberately. A
table knows the shapes somebody typed in; it cannot offer the voicing halfway up
the neck that nobody wrote down, and it silently offers nothing at all for
`F#m7b5` in DADGAD.

Instead: for each four-fret window of the neck, every string gets a list of the
frets in that window whose note is in the chord, plus the open string when it
qualifies, plus not playing it at all. Then every combination of those is
generated - a few thousand per chord, which takes about a tenth of a second for
the whole catalogue - and nearly all of them are thrown away.

## What gets thrown away, and why

- **Anything a hand cannot make.** Four fingers, a four-fret span, and a barre
  only where the index finger could actually lie: at the lowest fret in the
  shape, starting at the bass string, with nothing underneath it that needs to
  ring open. `examine()` works the fingering out rather than assuming one, which
  is also what makes the finger numbers on the diagram real.
- **Anything missing a note the chord cannot do without.** `ChordQuality::essential`
  names those. The fifth is almost never in that list and that is the point: on
  six strings with four fingers, the fifth is the first note to go. The third is
  not.
- **Anything sounding a note that is not in the chord.** No "close enough".
- **The same grip with a string muted.** `sameGrip` treats two fret vectors as
  one shape when the fingers do not move - *unless* the bass note changes, which
  is a different chord however little the hand did.

## Ranking is about what to show first, not what is good

There is no such thing as the best voicing of a chord. The score exists to
decide what a learner sees first, and it prefers the familiar: root in the bass,
low on the neck, open strings while the hand is still in open position, fewer
fingers, no unisons.

Two of those terms are worth knowing about:

- **Open strings are worth +22 in open position and -14 above it.** Both halves
  are the same fact - an open string is a note that cannot move, so a shape
  built on one only works in this key. Down at the nut that is the whole point
  and it is why open chords sound the way they do. At the seventh fret it is
  usually an accident of which key was asked for.
- **A barre costs almost nothing.** It costs something to play, but the reason
  to learn one is that it moves, and a list that ranked barre chords below every
  four-string grab would hide exactly the shapes worth learning. This was -6 and
  the standard F barre chord fell off the end of the list.

## The list is a walk along the neck

The last step is not ranking. Taking the top eight by score gives eight
near-identical grips in the same two frets, and a button called *try another
voicing* that hands back the same chord with one note moved is a button nobody
presses twice. So the selection takes the best shape from each part of the neck
first, and only then fills up from what is left.

`ChordShapesTests` holds the two invariants that must survive any change here:

1. **Every shape offered for a chord is that chord** - checked by feeding each
   generated shape back through `checkShape`, in every tuning, for every quality
   in the catalogue.
2. **Nothing offered needs a fifth finger or a longer arm.**

Both are cheap to break and expensive to notice: an unplayable shape is obvious
to a guitarist and invisible in a diff, and a shape that is playable but is some
other chord is invisible to both.

## Naming is derived, not stored

"E-shape barre, 5th fret" is read off the shape: there is a barre, the bass note
is the root, and the root is on the sixth string. That is the whole of what the
CAGED letters mean, so the name comes out of the geometry rather than out of a
lookup that could disagree with it.
