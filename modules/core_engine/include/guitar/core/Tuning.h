#pragma once

#include "guitar/core/Pitch.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace guitar::core
{

/** How the six strings are tuned, lowest first.

    Everything else in the engine reads the fretboard through one of these, and
    nothing anywhere holds a hard-coded E-A-D-G-B-E. A chord shape in drop D is
    a different set of frets for the same chord, and a quiz in DADGAD asks about
    a different note at the same place - so a tuning is an argument, never an
    assumption.
*/
struct Tuning
{
    std::string key;        ///< stable identifier, the shells use this
    std::string name;       ///< "Standard", "Drop D" ...
    std::string spelling;   ///< "E A D G B E", for showing next to the name
    std::string summary;    ///< one line on what it is for

    /** "guitar" or "bass". The engine does nothing differently for either - a
        bass is six strings minus two, as far as any of this is concerned - and
        it is here so a menu can group them and a shell can say which instrument
        it is drawing. */
    std::string instrument { "guitar" };

    /** MIDI note of each open string, lowest-pitched first.

        Index 0 is the lowest string - the thick one nearest your chin. This is
        the one convention the whole engine shares; `docs/FRETBOARD.md` says why
        it is this way round and not the player's numbering.

        The length of this is the number of strings, and nothing anywhere
        assumes it is six: a four-string bass and a seven-string guitar are the
        same code path with a different list.
    */
    std::vector<int> openNotes;
};

/** Every tuning the engine knows, in menu order.

    The shells build their picker from this rather than holding a list of their
    own: which tunings exist, and what the strings are in each, is the engine's
    to say.
*/
const std::vector<Tuning>& tunings();

/** The tuning with this key, or nullopt. An empty key means standard tuning,
    so a shell that has not asked the question yet still gets an answer. */
std::optional<Tuning> tuningFor (std::string_view key);

} // namespace guitar::core
