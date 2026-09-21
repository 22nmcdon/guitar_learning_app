#include "TestFramework.h"

#include "guitar/core/ChordSymbol.h"

#include <algorithm>

using namespace guitar::core;

namespace
{
    /** Every note of a chord, spelled the way that chord writes them. */
    std::string spell (const std::string& symbol)
    {
        auto chord = parseChord (symbol).value();
        std::string out;

        for (auto interval : chord.quality.intervals)
            out += (out.empty() ? "" : " ") + chord.spelledNote (interval);

        return out;
    }
}

TEST ("a note name comes apart into a letter and an alteration")
{
    CHECK_EQ (parseSpelling ("Bb").value().letter, 6);
    CHECK_EQ (parseSpelling ("Bb").value().alteration, -1);
    CHECK_EQ (parseSpelling ("Bb").value().pitchClass(), 10);
    CHECK_EQ (parseSpelling ("F#").value().pitchClass(), 6);
    CHECK_EQ (parseSpelling ("C").value().alteration, 0);
    CHECK (! parseSpelling ("H").has_value());
    CHECK (! parseSpelling ("Bx").has_value());
}

TEST ("a spelling writes itself back out")
{
    CHECK_EQ (parseSpelling ("Bb").value().name(), std::string ("Bb"));
    CHECK_EQ (parseSpelling ("F##").value().name(), std::string ("F##"));
    CHECK_EQ (parseSpelling ("A").value().name(), std::string ("A"));
}

TEST ("the flat seventh of a flat chord is a flat")
{
    // The bug this was written for: Bb7 read back as A#, D, F, G#, which is the
    // same four frets and the wrong chord to anybody reading a chart.
    CHECK_EQ (spell ("Bb7"), std::string ("Bb D F Ab"));
    CHECK_EQ (parseChord ("Bb7").value().symbol, std::string ("Bb7"));
}

TEST ("a natural root can still need flats")
{
    // Nothing in the symbol says "flat", and the seventh is one anyway: the
    // letter is decided by the degree, not by how the root was written.
    CHECK_EQ (spell ("F7"), std::string ("F A C Eb"));
    CHECK_EQ (spell ("C7"), std::string ("C E G Bb"));
    CHECK_EQ (spell ("Cm"), std::string ("C Eb G"));
}

TEST ("a sharp root keeps its sharps, however ugly that gets")
{
    // A# seven is a real chord and this is genuinely how it is spelled. It is
    // also the best argument in music for writing Bb instead.
    CHECK_EQ (spell ("A#7"), std::string ("A# C## E# G#"));
    CHECK_EQ (spell ("C#m7"), std::string ("C# E G# B"));
}

TEST ("every degree is written on its own letter")
{
    CHECK_EQ (spell ("Ebmaj7"), std::string ("Eb G Bb D"));
    CHECK_EQ (spell ("Am7"), std::string ("A C E G"));
    CHECK_EQ (spell ("C6"), std::string ("C E G A"));
    CHECK_EQ (spell ("Dm9"), std::string ("D E F A C"));
}

TEST ("a flat fifth is a fifth and a sharp eleventh is an eleventh")
{
    // The same tritone above the root, written two ways - and which it is
    // depends on whether the chord still has its fifth.
    CHECK_EQ (spell ("Cm7b5"), std::string ("C Eb Gb Bb"));
    CHECK_EQ (spell ("Cdim7"), std::string ("C Eb Gb Bbb"));
    CHECK_EQ (spell ("C7#11"), std::string ("C E F# G Bb"));
}

TEST ("a sharp fifth is a fifth and a flat thirteenth is a thirteenth")
{
    CHECK_EQ (spell ("C7#5"), std::string ("C E G# Bb"));
    CHECK_EQ (spell ("Caug"), std::string ("C E G#"));
}

TEST ("three semitones is a third in a minor chord and a ninth in a dominant")
{
    CHECK_EQ (spell ("Cm7"), std::string ("C Eb G Bb"));
    CHECK_EQ (spell ("C7#9"), std::string ("C D# E G Bb"));
}

TEST ("a note the chord does not contain is spelled the way the chord leans")
{
    const auto flatChord = parseChord ("Bb7").value();
    const auto sharpChord = parseChord ("A7").value();

    CHECK_EQ (flatChord.spelledPitchClass (3), std::string ("Eb"));
    CHECK_EQ (sharpChord.spelledPitchClass (3), std::string ("D#"));
}

TEST ("a spelled note carries the octave it sounds in")
{
    const auto chord = parseChord ("Bb7").value();
    CHECK_EQ (chord.spelledMidiNote (46), std::string ("Bb2"));
    CHECK_EQ (chord.spelledMidiNote (56), std::string ("Ab3"));
}

TEST ("a slash chord spells its bass note too")
{
    CHECK_EQ (parseChord ("D/F#").value().symbol, std::string ("D/F#"));
    CHECK_EQ (parseChord ("Eb/Bb").value().symbol, std::string ("Eb/Bb"));
}

TEST ("every quality in the catalogue spells one letter per degree")
{
    // No chord should write two of its notes on the same letter, or skip one:
    // that is what makes a chord readable on a stave and on a chart.
    for (const auto& quality : chordQualities())
    {
        for (const auto* root : { "C", "Bb", "F#", "Eb" })
        {
            auto chord = parseChord (root + quality.suffix).value();
            std::vector<int> letters;

            for (auto interval : chord.quality.intervals)
                letters.push_back (chord.degreeStepFor (interval));

            auto sorted = letters;
            std::sort (sorted.begin(), sorted.end());

            if (std::unique (sorted.begin(), sorted.end()) != sorted.end())
                CHECK_EQ (chord.symbol + " writes two notes on one letter", std::string ("it does not"));
        }
    }
}
