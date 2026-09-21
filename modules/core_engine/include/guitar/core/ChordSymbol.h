#pragma once

#include "guitar/core/Pitch.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace guitar::core
{

/** One chord quality the engine can read and build: the suffix, what it is
    made of, and what a learner should be told it is. */
struct ChordQuality
{
    std::string suffix;        ///< what follows the root: "m7", "maj7", "" for a plain triad
    std::string name;          ///< "minor seventh"
    std::string family;        ///< "Triads", "Sevenths", "Colour" - how a menu groups them
    std::string summary;       ///< one line: where this chord is used

    /** Semitones above the root, ascending, always starting at 0. */
    std::vector<int> intervals;

    /** The intervals without which this is a different chord.

        The fifth is almost never in here, and that is the point: a guitar has
        six strings and four fingers, and the note that gets dropped first is
        the fifth. A voicing that leaves out the third of a minor seventh is
        not a voicing of it, and the shape generator needs to know which is
        which rather than guessing.
    */
    std::vector<int> essential;

    /** True when three semitones above the root means a minor third rather
        than a sharp ninth, which is the only thing interval spelling needs. */
    bool minorThird {};
};

/** Every quality the engine knows, in the order a menu should show them.

    The shells build their chord picker from this. Which chords exist, and what
    is in them, is theory - a second copy of the list in a page is a second copy
    that goes stale the first time this one grows.
*/
const std::vector<ChordQuality>& chordQualities();

/** A chord as the engine understands it: a root, a quality, and what is in it. */
struct Chord
{
    std::string symbol;     ///< normalised: the root as written plus the quality's suffix
    PitchClass root {};
    ChordQuality quality;

    /** How the root was written, which decides how everything else is.

        A B flat chord is spelled B flat, D, F, A flat - not A sharp, D, F, G
        sharp - and the difference is not cosmetic to anyone reading a chart.
        Carrying the root's letter is what lets every other note be written on
        the letter its degree demands.
    */
    NoteSpelling rootSpelling;

    /** A slash chord's bass note, when one was asked for and it is not the
        root. The shape generator treats this as a requirement on the lowest
        sounding string rather than as another chord tone. */
    std::optional<PitchClass> bass;

    /** Pitch classes in the chord, root first. */
    std::vector<PitchClass> pitchClasses() const;

    /** Which degree of this chord a pitch class is, or nullopt if it is not in
        it. The answer is a spelling - "b7", not 10 - because that is what goes
        on a diagram. */
    std::optional<std::string> degreeOf (PitchClass pitchClass) const;

    /** The same thing in words, for a learner: "the flat seventh". */
    std::optional<std::string> degreeDescription (PitchClass pitchClass) const;

    /** Which letter, counting in letters rather than semitones, the note
        @p semitones above the root is written on.

        The chord decides this and nothing else can: three semitones is a minor
        third in a minor chord and a sharp ninth in a dominant one, and a
        tritone is a flat fifth in a diminished chord and a sharp eleventh in a
        chord that still has its fifth.
    */
    int degreeStepFor (int semitones) const;

    /** The note @p semitones above the root, spelled the way this chord writes
        it: "Ab" for the seventh of Bb7, "G#" for the seventh of A#7. */
    std::string spelledNote (int semitones) const;

    /** A pitch class in this chord, spelled. Falls back to naming it against
        the root's own accidental when it is not a chord tone at all. */
    std::string spelledPitchClass (PitchClass pitchClass) const;

    /** "Bb2", "Ab3" - the same spelling with the octave the note sounds in. */
    std::string spelledMidiNote (int midiNote) const;
};

/** Reads "C", "Am", "G7", "F#m7b5", "Bb13", "D/F#".

    Returns nullopt for anything it cannot read, never a guess: a chord the
    engine half-understood would draw a shape for a chord nobody asked for.
*/
std::optional<Chord> parseChord (std::string_view text);

} // namespace guitar::core
