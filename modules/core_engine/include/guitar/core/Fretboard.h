#pragma once

#include "guitar/core/Pitch.h"
#include "guitar/core/Tuning.h"

#include <string>
#include <vector>

namespace guitar::core
{

/** The highest fret the engine will look at.

    Most guitars have more, and nothing here breaks if a shell draws them. It
    is a bound on searching, not a claim about the instrument: shapes above the
    fifteenth are the same shapes further up, and a quiz that asked about the
    twenty-second fret of the top string would be asking about a place most
    players cannot comfortably reach.
*/
inline constexpr int highestFret = 15;

/** A place to put a finger.

    @p string is an index into the tuning's open notes: 0 is the lowest-pitched
    string. @p fret is 0 for an open string and @p muted for a string that is
    not sounded - which is a real part of a guitar chord, not an absence, and so
    it is spelled rather than left out.
*/
struct Position
{
    int string {};
    int fret {};

    bool operator== (const Position& other) const noexcept
    {
        return string == other.string && fret == other.fret;
    }
};

/** The fret value that means "this string is not played". */
inline constexpr int muted = -1;

/** A tuning, and the arithmetic every other part of the engine does on it.

    There is no state here beyond the tuning: a fretboard is a function from a
    string and a fret to a note, and treating it as anything more is how a
    "current position" ends up somewhere only the UI should have one.
*/
class Fretboard
{
public:
    explicit Fretboard (Tuning tuningToUse);

    const Tuning& tuning() const noexcept { return boardTuning; }

    int stringCount() const noexcept { return static_cast<int> (boardTuning.openNotes.size()); }

    /** MIDI note sounded at a position. A muted or out-of-range position has no
        note, and returns -1 rather than an arbitrary one. */
    int noteAt (int string, int fret) const noexcept;
    int noteAt (const Position& position) const noexcept { return noteAt (position.string, position.fret); }

    bool isOnBoard (int string, int fret) const noexcept;

    /** "6th string", counting the guitarist's way: the lowest-pitched string is
        the sixth. The engine counts from zero upwards and translates here, once,
        so that the two conventions never have to meet anywhere else. */
    std::string stringName (int string) const;

    /** "6th string (E)" - the name with the note it is tuned to. */
    std::string stringNameWithNote (int string) const;

    /** Every place a pitch class can be played, low string to high, low fret to
        high. This is the answer to "where are all the C's", which is both a
        lesson and a quiz question. */
    std::vector<Position> positionsFor (PitchClass pitchClass, int fromFret = 0, int toFret = highestFret) const;

    /** Every place one exact note - not just its pitch class - can be played.
        Two of these on a guitar are not a redundancy: the same note on a lower
        string sounds rounder, which is half of why voicings are chosen. */
    std::vector<Position> positionsForNote (int midiNote, int fromFret = 0, int toFret = highestFret) const;

    /** The same note name in other octaves, anywhere on the board.

        The octave shapes are how the fretboard stops being memorisation: find
        one C and the rest follow by shape. The quiz asks for them for that
        reason, and the chord diagrams label them for the same one.
    */
    std::vector<Position> octaveTwins (const Position& from, int fromFret = 0, int toFret = highestFret) const;

private:
    Tuning boardTuning;
};

} // namespace guitar::core
