#include "TestFramework.h"

#include "guitar/core/ChordSymbol.h"

#include <algorithm>

using namespace guitar::core;

namespace
{
    bool has (const std::vector<int>& intervals, int semitones)
    {
        return std::find (intervals.begin(), intervals.end(), semitones) != intervals.end();
    }
}

TEST ("a bare letter is a major triad")
{
    auto chord = parseChord ("C").value();
    CHECK_EQ (chord.root, 0);
    CHECK_EQ (chord.quality.name, std::string ("major"));
    CHECK_EQ (static_cast<int> (chord.quality.intervals.size()), 3);
    CHECK (has (chord.quality.intervals, 4));
    CHECK (has (chord.quality.intervals, 7));
}

TEST ("the four chords every guitarist learns first parse")
{
    for (const auto* symbol : { "G", "Am", "D", "Em" })
        CHECK (parseChord (symbol).has_value());
}

TEST ("minor is written four ways and means one thing")
{
    for (const auto* symbol : { "Am", "Amin", "A-", "Am2" })
    {
        auto chord = parseChord (symbol);

        if (std::string (symbol) == "Am2")
        {
            CHECK (! chord.has_value());   // not a spelling of anything
            continue;
        }

        CHECK_EQ (chord.value().quality.name, std::string ("minor"));
        CHECK (has (chord.value().quality.intervals, 3));
    }
}

TEST ("sevenths keep their thirds apart")
{
    CHECK (has (parseChord ("G7").value().quality.intervals, 10));
    CHECK (has (parseChord ("Gmaj7").value().quality.intervals, 11));
    CHECK (has (parseChord ("Gm7").value().quality.intervals, 3));
    CHECK (has (parseChord ("Gm7").value().quality.intervals, 10));
}

TEST ("half-diminished is read as itself, not as a minor seventh with a note moved")
{
    // Both readings give the same four notes. Only one of them is what anybody
    // calls the chord, and the name is what goes on the diagram.
    auto chord = parseChord ("Bm7b5").value();
    CHECK_EQ (chord.quality.name, std::string ("half-diminished"));
    CHECK_EQ (chord.symbol, std::string ("Bm7b5"));
}

TEST ("an alteration that has no quality of its own is applied to one that has")
{
    auto chord = parseChord ("G7b9").value();
    CHECK (has (chord.quality.intervals, 1));
    CHECK (has (chord.quality.intervals, 10));
    CHECK (has (chord.quality.intervals, 4));
}

TEST ("a raised fifth replaces the fifth rather than joining it")
{
    auto chord = parseChord ("C7#5").value();
    CHECK (has (chord.quality.intervals, 8));
    CHECK (! has (chord.quality.intervals, 7));
}

TEST ("a power chord has no third, which is the whole point of it")
{
    auto chord = parseChord ("A5").value();
    CHECK_EQ (static_cast<int> (chord.quality.intervals.size()), 2);
    CHECK (! has (chord.quality.intervals, 4));
    CHECK (! has (chord.quality.intervals, 3));
}

TEST ("a suspended chord has a fourth or a second where the third was")
{
    CHECK (has (parseChord ("Dsus4").value().quality.intervals, 5));
    CHECK (has (parseChord ("Dsus2").value().quality.intervals, 2));
    CHECK (! has (parseChord ("Dsus4").value().quality.intervals, 4));
    CHECK_EQ (parseChord ("Dsus").value().quality.suffix, std::string ("sus4"));
}

TEST ("a slash chord names a bass note that is not a chord tone")
{
    auto chord = parseChord ("D/F#").value();
    CHECK_EQ (chord.root, 2);
    CHECK_EQ (chord.bass.value(), 6);
    CHECK_EQ (chord.symbol, std::string ("D/F#"));

    // F# is in a D chord, so it is not added to it; the slash says where it goes.
    CHECK_EQ (static_cast<int> (chord.quality.intervals.size()), 3);
}

TEST ("a slash that names the root is not an inversion")
{
    CHECK (! parseChord ("C/C").value().bass.has_value());
}

TEST ("the spelling that was typed is the spelling that comes back")
{
    CHECK_EQ (parseChord ("Bb7").value().symbol, std::string ("Bb7"));
    CHECK_EQ (parseChord ("A#7").value().symbol, std::string ("A#7"));
}

TEST ("a chord knows which degree a note of it is")
{
    auto chord = parseChord ("Am7").value();
    CHECK_EQ (chord.degreeOf (9).value(), std::string ("R"));
    CHECK_EQ (chord.degreeOf (0).value(), std::string ("b3"));
    CHECK_EQ (chord.degreeOf (7).value(), std::string ("b7"));
    CHECK (! chord.degreeOf (10).has_value());
    CHECK_EQ (chord.degreeDescription (0).value(), std::string ("the minor third"));
}

TEST ("nonsense is refused rather than guessed at")
{
    CHECK (! parseChord ("").has_value());
    CHECK (! parseChord ("H7").has_value());
    CHECK (! parseChord ("Cwhatever").has_value());
    CHECK (! parseChord ("C/H").has_value());
}

TEST ("whitespace around a chord is not an error")
{
    CHECK_EQ (parseChord ("  Am7  ").value().symbol, std::string ("Am7"));
}

TEST ("every quality in the catalogue parses back to itself")
{
    for (const auto& quality : chordQualities())
    {
        auto chord = parseChord ("C" + quality.suffix);
        CHECK (chord.has_value());
        CHECK_EQ (chord.value().quality.suffix, quality.suffix);
        CHECK_EQ (chord.value().quality.name, quality.name);
    }
}

TEST ("every quality starts at the root and says what it cannot do without")
{
    for (const auto& quality : chordQualities())
    {
        CHECK_EQ (quality.intervals.front(), 0);
        CHECK (! quality.essential.empty());
        CHECK (! quality.family.empty());
        CHECK (! quality.summary.empty());

        for (auto interval : quality.essential)
            CHECK (has (quality.intervals, interval));

        // The fifth is the first note a guitarist drops, and a four-note chord
        // that calls it essential is one whose shapes will not fit on four
        // fingers. Triads are exempt: a major chord without its fifth is two
        // notes, which is a different thing entirely.
        CHECK (quality.intervals.size() < 4 || ! has (quality.essential, 7));
    }
}
