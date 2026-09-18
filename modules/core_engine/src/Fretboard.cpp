#include "guitar/core/Fretboard.h"

#include <algorithm>
#include <utility>

namespace guitar::core
{

Fretboard::Fretboard (Tuning tuningToUse)
    : boardTuning (std::move (tuningToUse))
{
}

bool Fretboard::isOnBoard (int string, int fret) const noexcept
{
    return string >= 0 && string < stringCount() && fret >= 0 && fret <= highestFret;
}

int Fretboard::noteAt (int string, int fret) const noexcept
{
    if (! isOnBoard (string, fret))
        return -1;

    return boardTuning.openNotes[static_cast<std::size_t> (string)] + fret;
}

std::string Fretboard::stringName (int string) const
{
    // The engine counts strings upwards from the lowest; players count them
    // downwards from the highest. Both are "the sixth string" when they mean
    // the thick one, and this is the only place that has to know it.
    const auto players = stringCount() - string;

    switch (players)
    {
        case 1:  return "1st string";
        case 2:  return "2nd string";
        case 3:  return "3rd string";
        default: return std::to_string (players) + "th string";
    }
}

std::string Fretboard::stringNameWithNote (int string) const
{
    if (string < 0 || string >= stringCount())
        return {};

    return stringName (string) + " ("
         + pitchClassName (boardTuning.openNotes[static_cast<std::size_t> (string)]) + ")";
}

std::vector<Position> Fretboard::positionsFor (PitchClass pitchClass, int fromFret, int toFret) const
{
    std::vector<Position> found;
    const auto wanted = toPitchClass (pitchClass);

    for (auto string = 0; string < stringCount(); ++string)
        for (auto fret = std::max (0, fromFret); fret <= std::min (toFret, highestFret); ++fret)
            if (toPitchClass (noteAt (string, fret)) == wanted)
                found.push_back ({ string, fret });

    return found;
}

std::vector<Position> Fretboard::positionsForNote (int midiNote, int fromFret, int toFret) const
{
    std::vector<Position> found;

    for (auto string = 0; string < stringCount(); ++string)
        for (auto fret = std::max (0, fromFret); fret <= std::min (toFret, highestFret); ++fret)
            if (noteAt (string, fret) == midiNote)
                found.push_back ({ string, fret });

    return found;
}

std::vector<Position> Fretboard::octaveTwins (const Position& from, int fromFret, int toFret) const
{
    const auto note = noteAt (from);

    if (note < 0)
        return {};

    std::vector<Position> twins;

    for (const auto& position : positionsFor (toPitchClass (note), fromFret, toFret))
    {
        // The same note in the same octave on another string is a unison, not
        // an octave - a real and useful thing to know, and not what was asked
        // for. The position it was asked about is not its own twin either.
        if (noteAt (position) != note)
            twins.push_back (position);
    }

    return twins;
}

} // namespace guitar::core
