# The quiz, and why it is in the engine

`FretboardQuiz` is the one stateful thing in the Core Engine. Everything else
here is a function; this holds a run of questions and what happened in it.

## Why not in the page

A quiz is a stream with a beginning and an end, and the alternative - the shell
posting back everything answered so far on every answer - puts the learner's
history in the UI. Which questions to ask next, and what a run of them means,
is pedagogy; pedagogy is theory; theory does not live in a page. Both shells get
the same quiz for free, and the whole of it is testable without a browser.

## The engine has no clock and is not getting one

Two things that look like they need one arrive as data instead:

- **The seed.** `QuizOptions::seed` comes from the shell. Same seed, same quiz -
  which is what makes the tests possible, and what would let two people take the
  same one.
- **How long an answer took.** `answer(given, elapsedMs)`. The engine reports it
  in the summary and never measured it. The page subtracts two `Date.now()`s,
  because the page is allowed to know what time it is.

An engine that called a clock would be an engine whose tests could not be
written, and the summary is the only thing that ever wanted one.

## Questions are not a uniform sample

The pool is weighted by two things that stack: what the learner got wrong in the
last ten minutes, and what they have been getting wrong for a fortnight. The
same place is never asked about twice running. The point is not to cover the
neck evenly - it is to find the four frets somebody does not know and keep
returning to them.

## The record between sessions

`Progress` is a learner's history: per place, per tuning, how often it was
asked, how often it was right, how long the current run of right answers is, and
the day it was last seen.

**The engine stores none of it.** It is handed the record at `start`, hands an
updated one back from `finish`, and forgets. Where the bytes live is the shell's
business - the page keeps the string in browser storage and passes it back next
time. That split is the same one as everywhere else here: deciding that the 7th
fret of the 5th string is due again is pedagogy, and writing a string to disk is
not.

It is **per tuning** because a fret is a different note in each. Knowing where
everything is in standard tells you nothing about DADGAD, and a record that
pooled them would be confidently wrong about both. The session count and the
lifetime totals are one learner's and span every tuning; the map and the
weighting are not.

Three numbers shape the weighting, and their *sizes* matter more than the shape:

```
weight = clamp(6 + 8 x misses - min(6, 2 x streak) + min(12, days since), 1, 60)
```

The confidence term is capped below the base deliberately. It was not, at first:
a run of five right answers subtracted more than the base was worth, the sum hit
the floor, and a mastered place then sat on the floor for ever - the decay term
underneath it could never lift it off. Capped, the same place comes back after a
fortnight, which is the entire reason for keeping a record at all.

A place never asked about weighs 6: worth showing, and never ahead of a fret
that has been missed three times.

`strengthFor` is the same record read the other way, as 0-100 for painting the
neck. Confidence is capped at "known" and the fade is applied *after* that
rather than into the same sum - otherwise a long streak banks enough credit to
swallow the fade whole, and a place last seen in March is still painted as solid
in June.

## What is not recorded

A quiz that was opened and walked away from. The session is counted on the first
answer, not on the start, because a count somebody reads as "days I practised"
should not include the days they opened the page and made tea. `finish` still
hands back the history unchanged in that case, so a shell can store the result
unconditionally without ever wiping one by opening a quiz.

## The four kinds

| Key | Asks | Answered with |
|---|---|---|
| `name-note` | a place on the neck | one of four note names |
| `find-note` | a note name and a string | a tap on the neck |
| `octaves` | a marked place | another place with the same name |
| `chord-tone` | a chord and a place | which degree of it that is |

`find-note` accepts **any** position that sounds the right note on the string
asked about - there is usually more than one, and a quiz that insisted on the
low one would be teaching a fret number rather than a note. `octaves` refuses a
unison: the same note in the same octave on another string is a real and useful
thing to know, and it is not what was asked for.

## Verdicts say something, not just yes

Every answer comes back with a landmark rather than a tick: *the same note as
the open 5th string*, *one fret above the 5th-fret dot*, *that is the open
string itself*. Nobody navigates a neck by fret numbers, and a quiz that only
confirms the number teaches the number.

Two near misses are told apart from wrong answers, because they mean something
different:

- the right note in a place that was not asked for - the octave shape half
  learnt;
- on a chord-tone question, a degree that is in the chord but not this one.

## The summary is words

Numbers go in it too, but "78%" is a fact about a quiz and "the fifth string is
where this falls down" is a fact about the player. Each observation has a floor
under it - a string asked about twice says nothing about that string - so the
summary stays quiet rather than inventing a pattern out of four questions. Four
observations is the cap, because four is as many as anyone reads.

Once there is a history, two more become possible and neither is worth saying
without one: how many of today's questions were places missed last time, and how
today compares with the accuracy across every session. Both are held back on a
first visit, where they would be statements about a single run dressed up as
history.
