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

The pool is weighted: a place that has already caught someone out is three times
as likely to come back per miss, and the same place is never asked about twice
running. The point is not to cover the neck evenly. It is to find the four frets
somebody does not know and keep returning to them.

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
summary stays quiet rather than inventing a pattern out of four questions.
