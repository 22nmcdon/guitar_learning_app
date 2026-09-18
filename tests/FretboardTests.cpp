#include "TestFramework.h"

#include "guitar/core/Fretboard.h"

#include <algorithm>

using namespace guitar::core;

namespace
{
    Fretboard standard()
    {
        return Fretboard { tuningFor ("standard").value() };
    }
}

TEST ("a fret is a semitone, counted up from the open string")
{
    const auto board = standard();
    CHECK_EQ (board.noteAt (0, 0), 40);    // open low E
    CHECK_EQ (board.noteAt (0, 1), 41);    // F
    CHECK_EQ (board.noteAt (0, 5), 45);    // A, the open 5th string
    CHECK_EQ (board.noteAt (0, 12), 52);   // E again, an octave up
}

TEST ("the fifth fret of one string is the string below it, except at the G")
{
    const auto board = standard();

    CHECK_EQ (board.noteAt (0, 5), board.noteAt (1, 0));
    CHECK_EQ (board.noteAt (1, 5), board.noteAt (2, 0));
    CHECK_EQ (board.noteAt (2, 5), board.noteAt (3, 0));

    // The one place the pattern breaks, and the reason so many people learn
    // the neck wrong: the B string is tuned a major third above the G.
    CHECK_EQ (board.noteAt (3, 4), board.noteAt (4, 0));
    CHECK_EQ (board.noteAt (4, 5), board.noteAt (5, 0));
}

TEST ("nothing off the end of the neck has a note")
{
    const auto board = standard();
    CHECK_EQ (board.noteAt (6, 0), -1);
    CHECK_EQ (board.noteAt (-1, 3), -1);
    CHECK_EQ (board.noteAt (0, highestFret + 1), -1);
    CHECK_EQ (board.noteAt (0, muted), -1);
}

TEST ("strings are named the way a player counts them")
{
    const auto board = standard();
    CHECK_EQ (board.stringName (0), std::string ("6th string"));
    CHECK_EQ (board.stringName (5), std::string ("1st string"));
    CHECK_EQ (board.stringName (4), std::string ("2nd string"));
    CHECK_EQ (board.stringName (3), std::string ("3rd string"));
    CHECK_EQ (board.stringNameWithNote (0), std::string ("6th string (E)"));
}

TEST ("every C on the neck is found, in order")
{
    const auto board = standard();
    const auto positions = board.positionsFor (0, 0, 12);

    CHECK (positions.size() >= 6);

    for (const auto& position : positions)
        CHECK_EQ (toPitchClass (board.noteAt (position)), 0);

    // Low string first, low fret first - the order a diagram is drawn in.
    CHECK_EQ (positions.front().string, 0);
    CHECK_EQ (positions.front().fret, 8);
}

TEST ("one exact note is not the same question as its pitch class")
{
    const auto board = standard();
    const auto anyC = board.positionsFor (0, 0, 12);
    const auto middleC = board.positionsForNote (60, 0, 12);

    CHECK (middleC.size() < anyC.size());

    for (const auto& position : middleC)
        CHECK_EQ (board.noteAt (position), 60);
}

TEST ("the same note on two strings is a unison, not an octave")
{
    const auto board = standard();

    // The open B string and the fourth fret of the G string are both B3.
    const auto twins = board.octaveTwins ({ 4, 0 }, 0, 12);

    for (const auto& twin : twins)
        CHECK (board.noteAt (twin) != board.noteAt ({ 4, 0 }));

    CHECK (std::find (twins.begin(), twins.end(), Position { 3, 4 }) == twins.end());
}

TEST ("an octave twin is the same name in a different octave")
{
    const auto board = standard();
    const auto twins = board.octaveTwins ({ 0, 5 }, 0, 12);   // A on the low E string

    CHECK (! twins.empty());

    for (const auto& twin : twins)
    {
        CHECK_EQ (toPitchClass (board.noteAt (twin)), 9);
        CHECK (board.noteAt (twin) != 45);
    }
}

TEST ("a fret range is respected, and clamped to the neck")
{
    const auto board = standard();

    for (const auto& position : board.positionsFor (0, 3, 7))
    {
        CHECK (position.fret >= 3);
        CHECK (position.fret <= 7);
    }

    CHECK (board.positionsFor (0, 0, 500).size() <= board.positionsFor (0, 0, highestFret).size());
}

TEST ("the fretboard is whatever the tuning says it is")
{
    const Fretboard dropped { tuningFor ("drop-d").value() };

    CHECK_EQ (dropped.noteAt (0, 0), 38);            // D2
    CHECK_EQ (dropped.noteAt (0, 7), 45);            // A, where the fifth fret used to be
    CHECK_EQ (dropped.stringNameWithNote (0), std::string ("6th string (D)"));
}
