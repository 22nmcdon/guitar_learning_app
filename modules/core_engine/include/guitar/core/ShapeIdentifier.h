#pragma once

#include "guitar/core/ChordSymbol.h"
#include "guitar/core/Fretboard.h"

#include <string>
#include <vector>

namespace guitar::core
{

/** One name for a set of notes, and how well it fits them. */
struct ChordNaming
{
    std::string symbol;       ///< "Cmaj7"
    std::string qualityName;  ///< "major seventh"
    int confidence {};        ///< 0-100, and only ever a comparison between these
    std::string verdict;      ///< "exactly", "with the fifth left out", "over its third"
    std::vector<std::string> missing;  ///< chord tones not being played, spelled
    std::vector<std::string> extra;    ///< notes played that the chord has no room for
    bool rootInBass {};
};

/** What a set of fretted strings adds up to. */
struct ShapeReading
{
    bool anythingPlayed {};
    std::vector<int> notes;              ///< MIDI notes, low to high
    std::vector<std::string> noteNames;  ///< the same, named
    std::vector<ChordNaming> namings;    ///< best first, at most three
    std::string summary;                 ///< one line in words, always filled in
};

/** Names what is being held down.

    @p frets is one entry per string, lowest first, `muted` for a string not
    played - the same vector a `ChordShape` carries, because the shape the
    engine suggested and the shape the player actually fretted have to be the
    same kind of thing for either to be comparable to the other.

    More names is not better. Every quality in the catalogue is scored against
    the notes, so adding a name to that catalogue means a new competitor for
    every chord already in it - which is how an app ends up calling an open C
    an Am7add4 with great confidence.
*/
ShapeReading identifyShape (const Tuning& tuning, const std::vector<int>& frets);

/** How what was played measures against the chord that was asked for.

    This is the feedback half of chord practice: not "here is a C", but "that
    is a C without its third, and the note on the second string is a D".
*/
struct ShapeVerdict
{
    bool correct {};
    std::string verdict;   ///< short: "That's it", "Nearly", "Not this one"
    std::string detail;    ///< one sentence saying what is wrong, or right
    std::vector<std::string> missing;   ///< chord tones not sounding
    std::vector<std::string> outside;   ///< notes that are not in the chord, named
    std::vector<std::string> degrees;   ///< degree sounding on each string, "" where muted
};

ShapeVerdict checkShape (const Chord& chord, const Tuning& tuning, const std::vector<int>& frets);

} // namespace guitar::core
