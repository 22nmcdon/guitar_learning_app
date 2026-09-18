#include "TestFramework.h"

#include "guitar/core/ShapeIdentifier.h"

#include <algorithm>

using namespace guitar::core;

namespace
{
    ShapeReading read (const std::vector<int>& frets, const std::string& tuningKey = "standard")
    {
        return identifyShape (tuningFor (tuningKey).value(), frets);
    }

    ShapeVerdict check (const std::string& symbol, const std::vector<int>& frets)
    {
        return checkShape (parseChord (symbol).value(), tuningFor ("standard").value(), frets);
    }

    bool named (const ShapeReading& reading, const std::string& symbol)
    {
        return std::any_of (reading.namings.begin(), reading.namings.end(),
                            [&symbol] (const ChordNaming& naming) { return naming.symbol == symbol; });
    }
}

TEST ("the open chords are named as themselves")
{
    CHECK_EQ (read ({ muted, 3, 2, 0, 1, 0 }).namings.front().symbol, std::string ("C"));
    CHECK_EQ (read ({ 3, 2, 0, 0, 0, 3 }).namings.front().symbol, std::string ("G"));
    CHECK_EQ (read ({ muted, 0, 2, 2, 1, 0 }).namings.front().symbol, std::string ("Am"));
    CHECK_EQ (read ({ 0, 2, 2, 1, 0, 0 }).namings.front().symbol, std::string ("E"));
}

TEST ("a barre chord is named wherever it is")
{
    CHECK_EQ (read ({ 1, 3, 3, 2, 1, 1 }).namings.front().symbol, std::string ("F"));
    CHECK_EQ (read ({ 5, 7, 7, 6, 5, 5 }).namings.front().symbol, std::string ("A"));
    CHECK_EQ (read ({ 8, 10, 10, 9, 8, 8 }).namings.front().symbol, std::string ("C"));
}

TEST ("a seventh is not confused with the triad inside it")
{
    CHECK_EQ (read ({ 3, 2, 0, 0, 0, 1 }).namings.front().symbol, std::string ("G7"));
    CHECK_EQ (read ({ muted, 3, 2, 0, 0, 0 }).namings.front().symbol, std::string ("Cmaj7"));
}

TEST ("the simple name wins over the clever one")
{
    // The notes of an open C are also in Am7 with no root and F6 with no root.
    // Both are true. Neither is what anyone is playing.
    const auto reading = read ({ muted, 3, 2, 0, 1, 0 });
    CHECK_EQ (reading.namings.front().symbol, std::string ("C"));
    CHECK (reading.namings.front().confidence > 80);
}

TEST ("an inversion is named as the chord it is, over the note it is over")
{
    const auto reading = read ({ 2, 0, 0, 2, 3, 2 });   // D over F#
    CHECK (named (reading, "D"));

    const auto naming = std::find_if (reading.namings.begin(), reading.namings.end(),
                                      [] (const ChordNaming& n) { return n.symbol == "D"; });
    CHECK (! naming->rootInBass);
    CHECK (naming->verdict.find ("over") != std::string::npos);
}

TEST ("nothing fretted is said plainly rather than named")
{
    const auto reading = read ({ muted, muted, muted, muted, muted, muted });
    CHECK (! reading.anythingPlayed);
    CHECK (reading.namings.empty());
    CHECK (! reading.summary.empty());
}

TEST ("one note is one note")
{
    const auto reading = read ({ muted, muted, muted, muted, muted, 0 });
    CHECK (reading.anythingPlayed);
    CHECK (reading.summary.find ("One note") != std::string::npos);
}

TEST ("two notes that make a chord are named as that chord")
{
    const auto reading = read ({ 5, 7, muted, muted, muted, muted });   // A and E
    CHECK (named (reading, "A5"));
    CHECK_EQ (reading.namings.front().symbol, std::string ("A5"));
}

TEST ("two notes that make no chord are named as the interval they are")
{
    const auto reading = read ({ 5, 6, muted, muted, muted, muted });   // A and Eb
    CHECK (reading.namings.empty());
    CHECK (reading.summary.find ("tritone") != std::string::npos);

    // Measured up from the bass, not from whichever note sorts first: the same
    // two notes the other way up are a different interval and a different sound.
    CHECK (reading.summary.find ("A and") != std::string::npos);
}

TEST ("a shape with a note outside every chord gets no name at all")
{
    // C#, E and C: a chord would have to hold two notes a semitone apart and
    // nothing in the catalogue does, so no name is the honest answer.
    const auto reading = read ({ muted, 4, 2, 5, muted, muted });
    CHECK (reading.namings.empty());
    CHECK (! reading.summary.empty());
}

TEST ("the notes are reported whether or not a name was found")
{
    const auto reading = read ({ muted, 4, 2, 5, muted, muted });
    CHECK_EQ (static_cast<int> (reading.notes.size()), 3);
    CHECK_EQ (reading.noteNames.front(), std::string ("C#3"));
}

TEST ("playing the chord that was asked for is confirmed")
{
    const auto verdict = check ("C", { muted, 3, 2, 0, 1, 0 });
    CHECK (verdict.correct);
    CHECK_EQ (verdict.verdict, std::string ("That's it"));
    CHECK (verdict.outside.empty());
}

TEST ("a wrong note is pointed at rather than scored")
{
    const auto verdict = check ("C", { muted, 3, 2, 0, 1, 1 });
    CHECK (! verdict.correct);
    CHECK_EQ (verdict.verdict, std::string ("Not this one"));
    CHECK_EQ (static_cast<int> (verdict.outside.size()), 1);
    CHECK_EQ (verdict.outside.front(), std::string ("F4"));
}

TEST ("a missing third is the difference between nearly and not")
{
    const auto verdict = check ("C", { muted, 3, 5, 5, muted, muted });   // C and G, no E
    CHECK (! verdict.correct);
    CHECK_EQ (verdict.verdict, std::string ("Nearly"));
    CHECK (verdict.detail.find ("major") != std::string::npos);
}

TEST ("a missing fifth is not a mistake")
{
    const auto verdict = check ("Cmaj7", { muted, 3, muted, muted, 0, 0 });
    CHECK (verdict.correct);
    CHECK (std::find (verdict.missing.begin(), verdict.missing.end(), std::string ("5")) != verdict.missing.end());
}

TEST ("the right notes in the wrong order are still the right notes")
{
    const auto verdict = check ("C", { muted, muted, 2, 0, 1, 0 });   // E in the bass
    CHECK (verdict.correct);
    CHECK (verdict.verdict.find ("inverted") != std::string::npos);
}

TEST ("every string that sounds is given a degree, and the wrong ones are marked")
{
    const auto verdict = check ("C", { muted, 3, 2, 0, 1, 1 });
    CHECK_EQ (verdict.degrees[0], std::string (""));
    CHECK_EQ (verdict.degrees[1], std::string ("R"));
    CHECK_EQ (verdict.degrees[5], std::string ("!"));
}

TEST ("nothing played is not a wrong answer")
{
    const auto verdict = check ("C", { muted, muted, muted, muted, muted, muted });
    CHECK (! verdict.correct);
    CHECK_EQ (verdict.verdict, std::string ("Nothing yet"));
}

TEST ("a shape is read against the tuning it was played in")
{
    // The same frets, a tone lower on the sixth string.
    CHECK_EQ (read ({ 0, 0, 0, 2, 3, 2 }, "drop-d").namings.front().symbol, std::string ("D"));
}
