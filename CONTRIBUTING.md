# Contributing

Anything here is worth changing. The rules below are not house style - each one
is something that broke, or would break silently, and most of them are cheap to
violate by accident.

## Before you start

Read `CLAUDE.md`. It is written for an AI assistant and is just as useful to a
person: it holds the architecture, the invariants and the settled decisions, so
reading it is how you avoid re-litigating something that is already decided or
rebuilding something that already exists.

For anything touching the engine, read the doc for that corner first:

| If you are changing | Read |
|---|---|
| `ChordShapes.cpp` | `docs/CHORD_SHAPES.md` |
| `FretboardQuiz.cpp`, `Progress.cpp` | `docs/QUIZZES.md` |
| anything that counts strings or frets | `docs/FRETBOARD.md` |
| anything that names a note | `docs/SPELLING.md` |

## Running everything

```bash
# engine and tests, no JUCE and no GUI libraries needed
cmake -S . -B build-core -DGUITAR_BUILD_APP=OFF && cmake --build build-core
./build-core/tests/guitar_core_tests

# the page, against the WebAssembly build, in a real browser
./web/build.sh out && cp web/index.html web/sw.js out
node web/smoke-test.mjs out

# the desktop app
cmake -S . -B build && cmake --build build
```

## The checklist

A change is ready when all of these are true. They are in the order things
actually go wrong.

- [ ] **The engine still links no JUCE.** `-DGUITAR_BUILD_APP=OFF` builds and
      tests clean. A failure there is a layering mistake, not a missing package.
- [ ] **New theory went into the engine, not the page.** If a shell had to be
      taught a rule - which chords exist, which tunings, which quizzes, how to
      spell a note - the rule is in the wrong place and will drift.
- [ ] **The engine still has no clock and stores nothing.** Time and the
      calendar arrive as data; the learner's record crosses the boundary as
      text the shell keeps.
- [ ] **Tests were written for what changed**, and a failing one was treated as
      a question. Several tests here encoded an assumption rather than correct
      behaviour and had to be corrected - check which side is wrong before
      changing either.
- [ ] **The test count quoted in `README.md` and in the page's colophon matches
      the suite.** CI checks this; it has gone stale before.
- [ ] **Every engine call the page makes exists in both shells** -
      `app/src/WebUi.cpp` and the export list in `web/build.sh`. CI checks this
      too: a missing one is silent in the desktop app and fine in the browser.
- [ ] **The smoke test passes against a built page**, and you added a check for
      whatever you added.
- [ ] **You looked at it.** Screenshot the page. The neck once rendered as
      stripes, and its fret numbers once stacked in two columns down the side,
      with every smoke check passing - nothing was asserting on what it looked
      like. Check 430px as well as a desktop width.
- [ ] **Colour is not the only thing saying something.** Root, third and fifth
      are red, green and gold; the record's four bands run green to red. Both
      carry a label, a ring or an outline as well, because that pair of hues is
      exactly the one a good number of people cannot separate.
- [ ] **The neck is still reachable without a mouse**: one tab stop, arrow keys
      inside it, and an `aria-label` on every fret.

## Commits

Say what changed and why it was worth changing. The repository's history is
meant to be readable a year later by somebody deciding whether they are allowed
to undo something - a commit that says "fix bug" gives them nothing to go on.

## Versions

Semantic versioning, tagged `v0.1.0`. The version lives in the top-level
`CMakeLists.txt`; `CHANGELOG.md` records what each one contained. The build
stamp in the page (`__GUITAR_BUILD__`, replaced by the Pages workflow) is a
different thing and is not a version: it is a commit and a timestamp, so that
"is the site serving what I pushed?" is answered by looking rather than by
digging through deploy logs.
