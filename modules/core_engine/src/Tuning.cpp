#include "guitar/core/Tuning.h"

namespace guitar::core
{

const std::vector<Tuning>& tunings()
{
    // Written as MIDI numbers rather than parsed from the spelling beside them,
    // because the spelling is for a reader and the numbers are the tuning. They
    // are checked against each other in the tests instead, which is the only
    // place a typo in either could be caught.
    static const std::vector<Tuning> all
    {
        { "standard", "Standard", "E A D G B E",
          "What almost everything is written for. Start here.",
          { 40, 45, 50, 55, 59, 64 } },                     // E2 A2 D3 G3 B3 E4

        { "drop-d", "Drop D", "D A D G B E",
          "The sixth string down a tone: power chords on one finger, and a low D.",
          { 38, 45, 50, 55, 59, 64 } },

        { "half-step-down", "Half step down", "Eb Ab Db Gb Bb Eb",
          "Standard, a semitone flat. Every shape is where it was; every name moved.",
          { 39, 44, 49, 54, 58, 63 } },

        { "open-g", "Open G", "D G D G B D",
          "The open strings are a G chord. Slide blues and a lot of Keith Richards.",
          { 38, 43, 50, 55, 59, 62 } },

        { "dadgad", "DADGAD", "D A D G A D",
          "Suspended and open. Folk, Celtic, and anything that wants to ring.",
          { 38, 45, 50, 55, 57, 62 } }
    };

    return all;
}

std::optional<Tuning> tuningFor (std::string_view key)
{
    if (key.empty())
        return tunings().front();

    for (const auto& tuning : tunings())
        if (tuning.key == key)
            return tuning;

    return std::nullopt;
}

} // namespace guitar::core
