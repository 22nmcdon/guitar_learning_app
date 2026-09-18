#include "TestFramework.h"

#include "guitar/core/Fretboard.h"
#include "guitar/core/Tuning.h"

using namespace guitar::core;

TEST ("standard tuning is what everything defaults to")
{
    CHECK_EQ (tunings().front().key, std::string ("standard"));
    CHECK_EQ (tuningFor ("").value().key, std::string ("standard"));
    CHECK_EQ (tuningFor ("drop-d").value().name, std::string ("Drop D"));
    CHECK (! tuningFor ("open-z").has_value());
}

TEST ("every tuning has six strings, written low to high")
{
    for (const auto& tuning : tunings())
    {
        CHECK_EQ (static_cast<int> (tuning.openNotes.size()), 6);

        for (std::size_t i = 1; i < tuning.openNotes.size(); ++i)
            CHECK (tuning.openNotes[i] >= tuning.openNotes[i - 1]);
    }
}

TEST ("the spelling written beside a tuning is the tuning")
{
    // The numbers are what the engine uses and the spelling is what a reader
    // sees, so nothing but this test can catch the two drifting apart.
    for (const auto& tuning : tunings())
    {
        const Fretboard board { tuning };
        std::string spelled;

        for (auto string = 0; string < board.stringCount(); ++string)
            spelled += (string > 0 ? " " : "") + pitchClassName (board.noteAt (string, 0),
                                                                 tuning.spelling.find ('b') != std::string::npos
                                                                     ? Accidental::flats
                                                                     : Accidental::sharps);

        CHECK_EQ (spelled, tuning.spelling);
    }
}

TEST ("standard tuning is the notes everyone tunes to")
{
    const auto standard = tuningFor ("standard").value();
    CHECK_EQ (standard.openNotes[0], 40);   // E2
    CHECK_EQ (standard.openNotes[1], 45);   // A2
    CHECK_EQ (standard.openNotes[2], 50);   // D3
    CHECK_EQ (standard.openNotes[3], 55);   // G3
    CHECK_EQ (standard.openNotes[4], 59);   // B3
    CHECK_EQ (standard.openNotes[5], 64);   // E4
}

TEST ("drop D moves one string and leaves the rest alone")
{
    const auto standard = tuningFor ("standard").value();
    const auto dropped = tuningFor ("drop-d").value();

    CHECK_EQ (dropped.openNotes[0], standard.openNotes[0] - 2);

    for (std::size_t i = 1; i < standard.openNotes.size(); ++i)
        CHECK_EQ (dropped.openNotes[i], standard.openNotes[i]);
}

TEST ("half step down is standard, a semitone flat, all the way across")
{
    const auto standard = tuningFor ("standard").value();
    const auto flat = tuningFor ("half-step-down").value();

    for (std::size_t i = 0; i < standard.openNotes.size(); ++i)
        CHECK_EQ (flat.openNotes[i], standard.openNotes[i] - 1);
}

TEST ("open G is a G chord before anyone touches it")
{
    const auto openG = tuningFor ("open-g").value();
    const Fretboard board { openG };

    for (auto string = 0; string < board.stringCount(); ++string)
    {
        const auto pitch = toPitchClass (board.noteAt (string, 0));
        CHECK (pitch == 7 || pitch == 11 || pitch == 2);   // G, B, D
    }
}
