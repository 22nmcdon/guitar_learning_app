#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace guitar::core
{

/** Number of semitones in an octave. Used everywhere pitch classes are folded. */
inline constexpr int semitonesPerOctave = 12;

/** MIDI note number of middle C (C4 in the convention this project uses). */
inline constexpr int middleC = 60;

/** A pitch class is a note name folded into a single octave: 0 = C ... 11 = B. */
using PitchClass = int;

/** Folds any integer (MIDI note, negative interval, ...) into 0..11. */
int toPitchClass (int value) noexcept;

/** Ascending distance in semitones from one pitch class to another, 0..11. */
int ascendingInterval (PitchClass from, PitchClass to) noexcept;

/** Octave number of a MIDI note, with middle C (60) in octave 4. */
int octaveOf (int midiNote) noexcept;

/** Spelling preference for a pitch class that has no single correct name.

    A guitarist meets both spellings in the first week - the second fret of the
    low E string is F#, and the same fret on the D string is usually called E
    when it is the dominant of A. Neither is more true, so the caller says
    which it wants and the engine never picks for it.
*/
enum class Accidental
{
    sharps,
    flats
};

/** "C", "Bb", "F#" ... Enharmonic spelling follows @p accidental. */
std::string pitchClassName (PitchClass pitchClass, Accidental accidental = Accidental::sharps);

/** "Bb3", "C4" ... */
std::string midiNoteName (int midiNote, Accidental accidental = Accidental::sharps);

/** Both readings of a pitch class that has two, e.g. 6 -> "F#/Gb".

    A quiz that asked for the note at the sixth fret of the low E and accepted
    only "A#" would be teaching a spelling rather than a fretboard. Anything
    the shell shows a learner uses this; anything it compares uses the pitch
    class itself.
*/
std::string enharmonicName (PitchClass pitchClass);

/** True when the pitch class is one of the five that has a second spelling. */
bool hasTwoNames (PitchClass pitchClass) noexcept;

/** Parses "C", "c", "Bb", "F#", "Eb", "Fb", "B#" into a pitch class. */
std::optional<PitchClass> parsePitchClass (std::string_view text);

/** Parses "E2", "A#3", "Bb3" - a note name with an octave - into a MIDI note. */
std::optional<int> parseMidiNote (std::string_view text);

/** Reads a root note from the front of @p text, returning the pitch class and
    how many characters it consumed. Returns nullopt if @p text does not start
    with a note name.
*/
struct ParsedRoot
{
    PitchClass pitchClass {};
    std::size_t charactersConsumed {};
};

std::optional<ParsedRoot> parseRoot (std::string_view text);

/** Interval spelling relative to a chord root, e.g. 4 -> "3", 10 -> "b7".

    @param semitones          distance above the root, any range, folded to an octave
    @param spellThirdAsMinor  true when the chord's third is minor, so 3 semitones
                              reads as "b3" rather than "#9"
*/
std::string intervalName (int semitones, bool spellThirdAsMinor = false);

/** The long name of an interval, for a learner who has not met "b7" yet. */
std::string intervalDescription (int semitones, bool spellThirdAsMinor = false);

} // namespace guitar::core
