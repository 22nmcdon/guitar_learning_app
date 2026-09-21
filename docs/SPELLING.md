# How notes are spelled

A B flat and an A sharp are one fret. They are not one note, and a page that
calls a chord Bb7 and then names its notes A#, D, F, G# is telling a reader
something false about music they are trying to learn to read. This is how the
engine avoids that.

## A chord is spelled from its root's letter

`Chord` carries a `NoteSpelling` - a letter and an alteration - taken from the
root as it was written. Every other note of the chord is then spelled by
`spellAbove`, which works in two steps:

1. **The letter is decided by the degree.** A third is two letters above the
   root, a fifth is four, a seventh is six. Counting happens in letters, not in
   semitones.
2. **The accidental is whatever it takes to reach the note.** Once the letter is
   fixed, the alteration is arithmetic: the pitch that was asked for, minus the
   natural pitch of that letter.

So the seventh of Bb7 is written on the seventh letter above B - which is A -
and needs a flat to be ten semitones above B flat. Hence A flat, never G sharp.

This is also why `A#7` comes back as **A#, C##, E#, G#**. That is genuinely how
the chord is spelled, and it is the best argument in music for writing B flat
instead.

## Which letter a degree wants depends on the chord

Three intervals are ambiguous on their own, and the chord resolves all three:

| Semitones | Written as | When |
|---|---|---|
| 3 | minor third | the chord's third is minor |
| 3 | sharp ninth | it is not - a dominant with a raised ninth |
| 6 | flat fifth | the chord has no perfect fifth (diminished, half-diminished) |
| 6 | sharp eleventh | it still has one |
| 8 | sharp fifth | no perfect fifth (augmented) |
| 8 | flat thirteenth | there is one |
| 9 | diminished seventh | a stack of minor thirds, i.e. a diminished seventh chord |
| 9 | sixth or thirteenth | everywhere else |

`Chord::degreeStepFor` is that table. The dim7 case is the one people forget: a
diminished seventh is spelled C Eb Gb **Bbb**, with a seventh on the seventh
letter. The double flat looks alarming and is what the chord is.

`SpellingTests` asserts that no chord in the catalogue ever writes two of its
notes on the same letter, in any of four roots. That invariant is what makes the
table above right rather than merely plausible.

## Which of two names a root gets

Both names of a black note are correct and only one is used: nobody writes D
sharp major, because it contains an F double sharp, and nobody writes D flat
minor, because it contains a B double flat. `preferredRootName` is a table of
what music actually writes, and it differs between major and minor:

```
major   C  Db  D  Eb  E  F  F#  G  Ab  A  Bb  B
minor   C  C#  D  Eb  E  F  F#  G  G#  A  Bb  B
```

It is a table rather than a calculation because it is a convention rather than a
derivation. The page's twelve root buttons show **both** names - `D♯/E♭` - and
send the one that belongs to the quality being built, so picking that button and
`major` asks for E flat while picking it and `m` asks for E flat minor, and
neither produces a double sharp.

## Where there is no chord to ask

A neck has no key. In fretboard practice the names come from the **tuning**:
anybody tuned to E flat reads their strings in flats, and everybody else reads
sharps. The quiz sidesteps the question entirely by showing and accepting both
(`A#/Bb`), because a quiz about the fretboard should not be a quiz about
spelling.

`identifyShape` also has no key - it is handed notes, not a chord - so it names
what it finds in sharps and spells each candidate consistently from there.
