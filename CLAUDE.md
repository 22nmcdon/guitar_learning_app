# CLAUDE.md

Guidance for Claude Code (or any Claude instance) working in this repository.

For **what the app does and how each feature behaves**, see `README.md`. For the
coordinate system every part of the engine reads the neck through, see
`docs/FRETBOARD.md`; for how voicings are found and ranked, `docs/CHORD_SHAPES.md`;
for the quiz, `docs/QUIZZES.md`. Read the relevant one before touching
`ChordShapes.cpp`, `FretboardQuiz.cpp` or anything that counts strings. This file
is only for what you need to not break the repo or redo settled work.

## Project Overview

A standalone, cross-platform guitar education app built on **JUCE**, targeting
desktop (macOS/Windows) and mobile (iOS/Android) from one shared codebase. It has
two modes over one neck: **chord practice** (a chord under your hand, and every
other way of playing it) and **fretboard practice** (what the notes are, and
quizzes on them, remembered between visits). There is no keyboard anywhere in it
- the fretboard is the instrument and the input device.

## Architecture - Read Before Adding Code

Three layers, as separate modules. Do not blur this boundary.

| Layer | Responsibility | May depend on |
|---|---|---|
| **Core Engine** | The fretboard, chord parsing, shape generation, shape reading, the quiz and what it remembers. Pure C++, no JUCE GUI. | Nothing platform-specific |
| **Engine API** (`modules/engine_api`) | JSON wire format for every shell. Pure C++, no JUCE, no Emscripten. | Core Engine |
| **User Interface** (`web/`) | The one interface (neck, panels, quiz), served on the web and hosted by the app. | Engine API |
| **Platform Shell** (`app/`) | Webview hosting `web/`, and app lifecycle. | JUCE platform APIs |

- **There is one UI: `web/index.html`.** A visual change belongs there. Do not
  add a second implementation of a screen in JUCE components.
- **Core Engine never includes JUCE headers.** A reference to `juce::Component`
  in that layer means the logic belongs one layer up.
- **UI reads from Core Engine, never the reverse.**
- **The shell is deliberately tiny.** It owns a window and a webview and nothing
  else: no audio device, no MIDI, no file formats. The page plucks its own
  strings with WebAudio, which a webview has exactly as a browser does. Before
  adding anything to `app/`, check whether the page can already do it - if it
  can, doing it here means writing it twice.

## UI Conventions

One page (`web/index.html`) serves both desktop and mobile - "responsive" means
CSS, not per-platform builds.

- **One breakpoint, 760px.** Add a second only when something actually collides
  (check with `GUITAR_UI_SIZE=430x860` or a 430px viewport).
- **The neck scrolls sideways rather than shrinking.** A twelve-fret diagram
  squeezed into a phone is a diagram nobody can tap. This is why `.neck-scroll`
  exists and why `--fret-width` lives on it rather than on `.neck` - the fret
  numbers underneath are a second grid that needs the same measurement, and when
  it only had one the `repeat()` beneath them was silently invalid.
- **Anything mode-specific is `data-mode` + a CSS/JS branch - never a second
  component.** The neck is one component painted differently; `paintNeck()` is
  the only place that branches.
- **The accent colour moves with the mode** (`--accent`, blush in chord
  practice, teal in fretboard practice) and the note colours do not: root, third
  and fifth mean the same thing in both.
- **Left-handed is `data-hand` on the body and reordered cells** - never a CSS
  transform. A mirrored neck mirrors the note names written on it.
- **Nothing in the page hard-codes six strings.** Fret vectors come from
  `silentNeck()`, which sizes itself from the board the engine handed over.

## Input Scope (Current Phase)

In scope: tapping and clicking the neck. Out of scope: audio input and pitch
detection (a much larger DSP undertaking - deferred deliberately), and MIDI,
which almost no guitarist has. Do not add either without an explicit decision to
expand scope.

## Build System

CMake, not the Projucer - one `CMakeLists.txt` per module, JUCE pulled in by
`cmake/GetJUCE.cmake`. Commands are in `README.md`.

Two rules the build enforces:
- **Core Engine links no JUCE.** `cmake -DGUITAR_BUILD_APP=OFF` must build the
  engine and its tests with no JUCE/GUI libraries present. If a change breaks
  this, the layering has been broken.
- **Tests live with the engine** (`tests/`, against `modules/core_engine` and
  `modules/engine_api` only, using the harness in `tests/TestFramework.h` - no
  third-party test dependency).

`web/` is both the UI and a second transport to the engine (Emscripten →
WebAssembly via `web/build.sh`). If `web/build.sh` fails, something
platform-specific has leaked into Core Engine.

CI (`.github/workflows/ci.yml`): one job builds with `-DGUITAR_BUILD_APP=OFF` on a
GUI-less runner (a failure there means broken layering, not a missing package -
check that before reaching for `apt-get`); another builds the full app. CI also
checks the test count against what `README.md` and the page's colophon quote.

`.github/workflows/pages.yml` builds the wasm, deploys to GitHub Pages, and runs
`web/smoke-test.mjs` against the built page before deploy. Run it yourself after
touching the page:

```bash
./web/build.sh out && cp web/index.html web/sw.js out && node web/smoke-test.mjs out
```

A build that compiles has not been the thing that goes wrong here.

Android is the exception to CMake-everywhere (JUCE's CMake support doesn't cover
it): uses the Projucer/Gradle exporter over the same source tree.

## Feature Status

Full behaviour is in `README.md`. Status only, so a session doesn't re-plan
something finished or assume something unfinished is done:

- **Chord practice** (shape generation, the walk along the neck, degree labels,
  reading back a shape you fretted, checking it against the chord asked for) -
  done.
- **Fretboard practice** (note names, lighting up every place a note lives, four
  quizzes, weighted question selection, the summary) - done.
- **A record kept between sessions** (`Progress`), the weighting that reads it,
  the "what you know" map and the forget button - done.
- **Eight tunings** across guitar, seven-string and bass, and **left-handed
  drawing** - done. Nothing anywhere assumes standard, or six strings.
- **Not built, deliberately open**: scales and modes on the neck, arpeggio
  shapes, a chord progression to play along with, a metronome, strumming
  patterns, a record that follows a learner between devices (there is no account
  and no server here, and adding either is a much larger decision than it
  looks), audio input.

## Critical Invariants

These aren't stylistic preferences - violating them breaks something in a way
that's easy to miss in review:

- **The engine has no clock and must never get one.** How long an answer took,
  what to seed the shuffle with, and *what day it is* all arrive as data the
  shell computed. The engine only ever subtracts two day numbers, which is the
  most a thing with no clock can honestly do with a date. This is what keeps
  `FretboardQuiz` testable.
- **The engine stores nothing, and the shell reads nothing.** `Progress` is
  handed in as text and handed back as text; the page keeps it in browser
  storage and never parses it. A page that parsed it would be a second reader to
  keep in step with the first. The format is versioned and anything unreadable
  starts empty - it comes from storage a previous version of this app also wrote
  to.
- **A record is per tuning.** A fret is a different note in each, so knowing
  standard says nothing about DADGAD. The session count and lifetime totals are
  one learner's and span all of them; the map and the weighting are not.
- **An abandoned quiz changes nothing.** The session is counted on the first
  answer, not on `start`, and `finish` hands the history back unchanged - so a
  shell can store the result unconditionally without wiping a record by opening
  a quiz. `saveProgress` also refuses an empty string for the same reason.
- **Strings are indexed from the lowest pitch, and only `Fretboard::stringName`
  knows that players count the other way.** See `docs/FRETBOARD.md`.
- **`muted` is a value, not an absence.** Every `frets` vector has one entry per
  string. A shape that "leaves out" the sixth string is saying something.
- **Single source of truth for anything on the wire.** `tunings()`,
  `chordQualities()` and `quizKinds()` come from the engine via `EngineApi`;
  never hardcode a copy in a shell - it will drift the first time the engine's
  catalogue changes.
- **`ChordShapesTests` holds two invariants that must survive any change to the
  generator**: every shape it offers must identify as the chord it was offered
  for, and none may need a fifth finger or a span no hand has. Both are cheap to
  break and expensive to notice, and both run over every tuning - including the
  bass and the seven-string.
- **The CAGED letters come from the tuning's interval pattern, not the string
  number.** Index 0 is not "the E shape": on a seven-string that is the low B.
  See `docs/CHORD_SHAPES.md`.
- **A wider chord vocabulary is not automatically better.** `identifyShape`
  scores every quality against the notes played, so a new name competes with all
  the rest - an open C is also a rootless Am7 and a rootless F6, and both of
  those are true and useless. Check what a new name costs the chords that
  already resolve correctly before adding it.
- **Engine/page version mismatch fails silently, not loudly.** `web/index.html`
  requests `guitar-engine.js?v=<build stamp>`; without it a cached engine paired
  with a fresh page makes a feature simply *vanish* (an empty menu, the engine
  chip still saying ready) rather than erroring.
- **The app's UI must never block on the network** - a render-blocking resource
  that never arrives leaves the whole webview invisible. Webfonts are requested
  by script, only when served over the web.
- **The service worker (`web/sw.js`) never sees the visit that registers it** -
  registration happens on `load`, after the page and engine have been fetched, so
  it fetches them itself on install. Test offline behaviour by asserting on the
  cache's contents, not just a reload.
- **Load the page from a file, not JUCE's resource provider** - the provider
  intermittently fails to deliver a document this size on Linux (page loads,
  script never runs). `WebUi` writes the page to the temp dir and points the
  webview there instead.
- **Engine calls are async in the app, sync on the web** - every call site
  `await`s `call()`. An engine call made around that funnel works in the browser
  and fails silently in the app.

## Open Questions (Don't Assume - Ask)

- Should scales and arpeggios get their own mode, or extend chord practice? They
  are the obvious next thing and the two answers lead to different UIs.
- The record is per browser. Following a learner between devices needs an
  account and a server, which this project has never had - and the first one of
  those is a much larger decision than the feature that wants it.
- Should the record do anything for chord practice? It only informs the quiz
  today, and "the shapes you keep coming back to" is a different and less
  obviously useful thing to measure than "the frets you cannot name".

If work touches one of these, flag the ambiguity rather than silently picking a
direction.

## Working Conventions

- Expand Core Engine (unit-testable, UI-agnostic) first; wire into the UI second.
- **Which chords, tunings and quizzes exist is the engine's to say**, never a
  shell-side copy (see Critical Invariants).
- **Check the narrow window before calling a layout done** - things that overlap
  at 430px look fine at 1200px, and the neck is the widest thing here.
- **A failing test is a question, not a chore.** Several tests here encoded my
  assumption rather than correct behaviour and had to be corrected - check which
  side is wrong before changing either.
- **Look at the page, don't just run the smoke test.** The neck rendered as
  stripes and its fret numbers stacked in two columns while all 41 checks passed,
  because nothing was asserting on what it looked like. The inlay dots sat a
  string low on every neck for the same reason. Screenshot it.
