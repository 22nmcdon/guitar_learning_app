#pragma once

#include <string>

namespace guitar::api
{

/** The engine's answers, as JSON - the one wire format both shells read.

    Every call takes plain strings and returns a JSON object as a std::string.
    On failure the object is {"ok":false,"error":"..."} rather than an exception
    or an empty string, so a caller in either shell can report what went wrong
    without knowing any C++ types.

    Nothing here decides any theory. It asks the Core Engine and writes down
    what it said. A shell adds a transport for these - Emscripten exports for
    the browser, JUCE native functions for the app - never a rule.
*/

//==============================================================================
// Catalogues. Each of these is a menu somewhere in the UI, and the list lives
// here so that there is one of it: a page holding its own copy of the tunings
// is a page that disagrees with the engine the first time one is added.

/** Every tuning, with the notes of its open strings. */
std::string tunings();

/** Every chord quality the engine can read and build, grouped for a menu. */
std::string chordQualities();

/** Every kind of fretboard question the quiz can set. */
std::string quizKinds();

//==============================================================================
// Chords.

/** Ways of playing a chord, best first.

    @param symbol      "C", "Am7", "F#m7b5", "D/F#"
    @param tuningKey   a key from tunings(); empty means standard
    @param fromFret    lowest fret to search
    @param toFret      highest fret to search
    @param maxShapes   how many to return
    @param simpleOnly  non-zero for shapes with no barre and no stretch

    The list is the whole point of the "another voicing" button: it comes back
    already spread across the neck, so pressing it walks somewhere new rather
    than nudging one finger.
*/
std::string chordShapes (const char* symbol, const char* tuningKey,
                         int fromFret, int toFret, int maxShapes, int simpleOnly);

/** What a set of fretted strings is called.

    @param fretsCsv  one entry per string, lowest first; -1 or "x" for muted,
                     e.g. "x,3,2,0,1,0"
*/
std::string identifyShape (const char* tuningKey, const char* fretsCsv);

/** How what was fretted measures against the chord that was asked for. */
std::string checkShape (const char* symbol, const char* tuningKey, const char* fretsCsv);

//==============================================================================
// The fretboard itself.

/** The note at every position in a range: what the neck diagram is drawn from. */
std::string fretboardNotes (const char* tuningKey, int fromFret, int toFret);

/** Every place one note can be played - "show me all the C's", which is the
    first thing that makes the neck look like a map rather than a grid. */
std::string positionsFor (const char* noteName, const char* tuningKey, int fromFret, int toFret);

//==============================================================================
/** The quiz: the one stateful corner of this API, and deliberately so.

    A quiz is a stream of questions with a beginning and an end. The alternative
    - the shell sending back everything answered so far on every answer - puts
    the learner's history in the UI, which is where pedagogy is not allowed to
    live. So the engine holds it: one quiz for the one learner this process has.

      quizStart    set the quiz up and ask the first question
      quizAnswer   one answer, and how long the shell says it took. The engine
                   has no clock; it is told, like it is told the seed
      quizNext     the next question. Called when the learner has read the
                   verdict and wants to go on
      quizEnd      stop, and hand back the run to read
*/
std::string quizStart (const char* kind, const char* tuningKey, int fromFret, int toFret,
                       const char* stringsCsv, const char* chordSymbol, int seed);
std::string quizAnswer (const char* given, int elapsedMs);
std::string quizNext();
std::string quizEnd();

} // namespace guitar::api
