# Changelog

Notable changes, newest first. Versions are [semantic](https://semver.org), and
the format follows [Keep a Changelog](https://keepachangelog.com).

## Unreleased

### Added
- **Capo support.** A capo is a new nut: the strings it holds become the open
  strings, nothing below it can sound, and the shape that comes back is named
  for the grip you already know - with a capo at 3, E flat is your C shape.
- **Both names on every black-note root.** The twelve root buttons read `D♯/E♭`,
  and the chord is built with the name music actually uses for that quality.
- **An installable page.** `manifest.json`, two icons, and a theme colour, all
  cached by the offline worker.
- **A way through the neck without a mouse**: one tab stop, arrow keys inside
  it, `aria-label` on every fret, and a live region announcing what changed.
- **Redundancy for every colour that meant something on its own**: a ring on the
  root, four distinct treatments for the record's bands, and both facts in the
  title of every dot.
- **A real panel when the engine fails to load**, instead of an empty neck and a
  chip in the corner.
- `CONTRIBUTING.md`, this file, and `docs/SPELLING.md`.

### Fixed
- **Notes are spelled properly.** A B flat seven was coming back as A♯, D, F,
  G♯ - the right frets under a name no chart would print. Chords are now spelled
  from the root's letter, one letter per degree, which also means a diminished
  seventh is spelled with a double flat and A♯7 owns up to its double sharp.
- The muted-string cross was drawn at the nut with a capo on, inside the frets
  the capo had greyed out.

## 0.1.0

The first working version: the engine, the interface, the desktop shell and the
deployment.

### Added
- **Core engine**, pure C++17 with no JUCE: the fretboard and its tunings, chord
  parsing, the shape generator, shape reading, the quiz, and the record a
  learner builds up between sessions.
- **Chord practice**: a chord under your hand, every other way of playing it,
  degree or fingering on the dots, and a reading of whatever shape you tap in.
- **Fretboard practice**: note names, every place one note lives, four quizzes
  weighted by what you have already got wrong, and a summary in words.
- **A record that outlasts the visit**, kept by the shell and read by the
  engine, with a map of what you know painted onto the neck.
- **Eight tunings** across guitar, seven-string and bass, and left-handed
  drawing.
- **One interface** (`web/index.html`), served on the web and hosted by the
  desktop app in a webview against the native build of the same engine.
- CI on every push, and a smoke test that drives the built page in a real
  browser before anything deploys.
