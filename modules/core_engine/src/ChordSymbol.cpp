#include "guitar/core/ChordSymbol.h"

#include <algorithm>
#include <cctype>
#include <map>

namespace guitar::core
{

namespace
{
    /** Spellings that mean the same quality as one in the catalogue.

        A learner types what they see on a chart, and charts are not consistent:
        "Am", "Amin", "A-" and "AMI" are one chord written four ways. These are
        folded onto a catalogue suffix before anything else looks at them, so
        the catalogue stays the menu rather than becoming the dictionary.
    */
    const std::map<std::string, std::string>& aliases()
    {
        static const std::map<std::string, std::string> all
        {
            { "M",      ""      }, { "maj",   ""      }, { "major", ""     },
            { "min",    "m"     }, { "-",     "m"     }, { "mi",    "m"    }, { "minor", "m" },
            { "M7",     "maj7"  }, { "ma7",   "maj7"  }, { "j7",    "maj7" },
            { "min7",   "m7"    }, { "-7",    "m7"    }, { "mi7",   "m7"   },
            { "dom7",   "7"     },
            { "m7-5",   "m7b5"  }, { "min7b5","m7b5"  }, { "-7b5",  "m7b5" }, { "halfdim", "m7b5" },
            { "o",      "dim"   }, { "°",     "dim"   },
            { "o7",     "dim7"  }, { "°7",    "dim7"  }, { "dim/7", "dim7" },
            { "+",      "aug"   }, { "#5",    "aug"   }, { "+5",    "aug"  },
            { "sus",    "sus4"  }, { "sus11", "sus4"  },
            { "min9",   "m9"    }, { "-9",    "m9"    },
            { "M9",     "maj9"  }, { "ma9",   "maj9"  },
            { "min6",   "m6"    }, { "-6",    "m6"    },
            { "add2",   "add9"  },
            { "7sus",   "7sus4" }
        };

        return all;
    }

    /** One altered note, and what it does to a dominant chord. */
    struct Alteration
    {
        const char* text;
        int adds;      ///< interval to add, or -1
        int replaces;  ///< interval it displaces, or -1
    };

    const std::vector<Alteration>& alterations()
    {
        static const std::vector<Alteration> all
        {
            { "b9",  1,  -1 },
            { "#9",  3,  -1 },
            { "#11", 6,  -1 },
            { "b5",  6,   7 },
            { "#5",  8,   7 },
            { "b13", 8,  -1 },
            { "b6",  8,  -1 }
        };

        return all;
    }
}

const std::vector<ChordQuality>& chordQualities()
{
    // Ordered the way it should be learnt and the way a menu should read:
    // triads first, then the sevenths that most music is made of, then the
    // ones that are colour rather than function.
    static const std::vector<ChordQuality> all
    {
        { "",      "major",              "Triads",   "The first chord anyone plays. Three notes, no argument.",
          { 0, 4, 7 },        { 0, 4 },        false },
        { "m",     "minor",              "Triads",   "The major triad with its third flattened, and the whole mood with it.",
          { 0, 3, 7 },        { 0, 3 },        true  },
        { "5",     "power chord",        "Triads",   "Root and fifth, no third. Neither major nor minor, which is why it takes distortion.",
          { 0, 7 },           { 0, 7 },        false },
        { "sus2",  "suspended second",   "Triads",   "The third replaced by the note below it. Open and unresolved.",
          { 0, 2, 7 },        { 0, 2, 7 },     false },
        { "sus4",  "suspended fourth",   "Triads",   "The third replaced by the note above it. Wants to fall back into the major.",
          { 0, 5, 7 },        { 0, 5, 7 },     false },
        { "dim",   "diminished",         "Triads",   "A minor triad with the fifth flattened too. Tense, and rarely stays long.",
          { 0, 3, 6 },        { 0, 3, 6 },     true  },
        { "aug",   "augmented",          "Triads",   "A major triad with the fifth raised. Every note is the root of another one.",
          { 0, 4, 8 },        { 0, 4, 8 },     false },

        { "7",     "dominant seventh",   "Sevenths", "The blues chord, and the one that pulls hardest to the chord a fourth up.",
          { 0, 4, 7, 10 },    { 0, 4, 10 },    false },
        { "maj7",  "major seventh",      "Sevenths", "Major, with the note a semitone below the root on top. Soft, settled.",
          { 0, 4, 7, 11 },    { 0, 4, 11 },    false },
        { "m7",    "minor seventh",      "Sevenths", "Minor and unhurried. Half of every soul record is this chord.",
          { 0, 3, 7, 10 },    { 0, 3, 10 },    true  },
        { "m7b5",  "half-diminished",    "Sevenths", "The chord that starts a minor two-five. Also written with a slashed circle.",
          { 0, 3, 6, 10 },    { 0, 3, 6, 10 }, true  },
        { "dim7",  "diminished seventh", "Sevenths", "Four notes a minor third apart. The same shape every three frets.",
          { 0, 3, 6, 9 },     { 0, 3, 6, 9 },  true  },
        { "7sus4", "dominant seventh sus4", "Sevenths", "A dominant with the fourth where the third was. Hangs before it lands.",
          { 0, 5, 7, 10 },    { 0, 5, 10 },    false },

        { "6",     "major sixth",        "Colour",   "Major with the sixth added. Where a maj7 is dreamy, this is cheerful.",
          { 0, 4, 7, 9 },     { 0, 4, 9 },     false },
        { "m6",    "minor sixth",        "Colour",   "Minor with a major sixth on top. Bittersweet rather than sad.",
          { 0, 3, 7, 9 },     { 0, 3, 9 },     true  },
        { "add9",  "added ninth",        "Colour",   "A major triad with the ninth added and no seventh. Bright and open.",
          { 0, 2, 4, 7 },     { 0, 4, 2 },     false },
        { "9",     "dominant ninth",     "Colour",   "A dominant seventh with the ninth on top. Funk lives here.",
          { 0, 2, 4, 7, 10 }, { 0, 4, 10, 2 }, false },
        { "maj9",  "major ninth",        "Colour",   "Major seventh with the ninth added. Lush, and easier on guitar than it looks.",
          { 0, 2, 4, 7, 11 }, { 0, 4, 11, 2 }, false },
        { "m9",    "minor ninth",        "Colour",   "Minor seventh with the ninth. Warm rather than dark.",
          { 0, 2, 3, 7, 10 }, { 0, 3, 10, 2 }, true  },
        { "13",    "dominant thirteenth","Colour",   "A dominant with the thirteenth on top; the fifth goes to make room.",
          { 0, 4, 9, 10 },    { 0, 4, 10, 9 }, false }
    };

    return all;
}

std::vector<PitchClass> Chord::pitchClasses() const
{
    std::vector<PitchClass> classes;

    for (auto interval : quality.intervals)
        classes.push_back (toPitchClass (root + interval));

    return classes;
}

std::optional<std::string> Chord::degreeOf (PitchClass pitchClass) const
{
    const auto distance = ascendingInterval (root, pitchClass);

    for (auto interval : quality.intervals)
        if (toPitchClass (interval) == distance)
            return intervalName (interval, quality.minorThird);

    return std::nullopt;
}

std::optional<std::string> Chord::degreeDescription (PitchClass pitchClass) const
{
    const auto distance = ascendingInterval (root, pitchClass);

    for (auto interval : quality.intervals)
        if (toPitchClass (interval) == distance)
            return intervalDescription (interval, quality.minorThird);

    return std::nullopt;
}

std::optional<Chord> parseChord (std::string_view text)
{
    // Leading and trailing space is what a paste brings with it, not an error.
    while (! text.empty() && std::isspace (static_cast<unsigned char> (text.front())))
        text.remove_prefix (1);

    while (! text.empty() && std::isspace (static_cast<unsigned char> (text.back())))
        text.remove_suffix (1);

    auto root = parseRoot (text);

    if (! root.has_value())
        return std::nullopt;

    Chord chord;
    chord.root = root->pitchClass;

    auto rest = std::string (text.substr (root->charactersConsumed));

    // A slash chord names its bass separately, and the bass is not a chord
    // tone: D/F# is a D with the third underneath, not a D with an F# added.
    const auto slash = rest.find ('/');

    if (slash != std::string::npos)
    {
        auto bassText = rest.substr (slash + 1);
        auto bass = parsePitchClass (bassText);

        if (! bass.has_value())
            return std::nullopt;

        if (*bass != chord.root)
            chord.bass = *bass;

        rest = rest.substr (0, slash);
    }

    const auto resolve = [] (std::string suffix) -> const ChordQuality*
    {
        auto alias = aliases().find (suffix);

        if (alias != aliases().end())
            suffix = alias->second;

        for (const auto& quality : chordQualities())
            if (quality.suffix == suffix)
                return &quality;

        return nullptr;
    };

    // The whole suffix first, and only then the alterations. Both orders read
    // "m7b5" - as half-diminished, or as a minor seventh with its fifth pulled
    // down - and they are the same four notes either way. What differs is what
    // the chord is called, and calling it "minor seventh b5" on a diagram
    // teaches a name nobody uses.
    std::vector<Alteration> asked;
    auto found = resolve (rest);

    if (found == nullptr)
    {
        auto changed = true;

        while (changed)
        {
            changed = false;

            for (const auto& alteration : alterations())
            {
                const std::string suffix = alteration.text;

                if (rest.size() > suffix.size()
                    && rest.compare (rest.size() - suffix.size(), suffix.size(), suffix) == 0)
                {
                    asked.push_back (alteration);
                    rest.resize (rest.size() - suffix.size());
                    changed = true;
                    break;
                }
            }

            if (changed && (found = resolve (rest)) != nullptr)
                break;
        }
    }

    if (found == nullptr)
        return std::nullopt;

    chord.quality = *found;

    // Applied in the order they were written, which for a chord like 7#5b9 is
    // the order they matter: the one that displaces the fifth has to find it
    // still there.
    for (auto it = asked.rbegin(); it != asked.rend(); ++it)
    {
        auto& intervals = chord.quality.intervals;

        if (it->replaces >= 0)
            intervals.erase (std::remove (intervals.begin(), intervals.end(), it->replaces), intervals.end());

        if (it->adds >= 0 && std::find (intervals.begin(), intervals.end(), it->adds) == intervals.end())
        {
            intervals.push_back (it->adds);

            // An altered note is the reason the chord was written that way, so
            // it is not the one to leave out when there are not enough fingers.
            chord.quality.essential.push_back (it->adds);
        }

        std::sort (intervals.begin(), intervals.end());
        chord.quality.suffix += it->text;
        chord.quality.name += std::string (" ") + it->text;
    }

    chord.symbol = pitchClassName (chord.root,
                                   // Keep the spelling that was typed: someone
                                   // asking for Bb wants to read Bb back, not A#.
                                   text.size() > 1 && text[1] == 'b' ? Accidental::flats
                                                                     : Accidental::sharps)
                 + chord.quality.suffix;

    if (chord.bass.has_value())
        chord.symbol += "/" + pitchClassName (*chord.bass);

    return chord;
}

} // namespace guitar::core
