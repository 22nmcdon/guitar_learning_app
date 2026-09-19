#include "TestFramework.h"

#include "guitar/core/Progress.h"

using namespace guitar::core;

namespace
{
    Progress after (int right, int wrong, int day = 100)
    {
        Progress progress;

        for (auto i = 0; i < right; ++i)
            progress.record ("standard", { 0, 5 }, true, day);

        for (auto i = 0; i < wrong; ++i)
            progress.record ("standard", { 0, 5 }, false, day);

        return progress;
    }
}

TEST ("an empty record is what a first visit looks like")
{
    Progress progress;
    CHECK_EQ (progress.sessions(), 0);
    CHECK_EQ (progress.totalAsked(), 0);
    CHECK (progress.lookup ("standard", { 0, 5 }) == nullptr);
    CHECK_EQ (progress.strengthFor ("standard", { 0, 5 }, 100), -1);
}

TEST ("what was written down comes back")
{
    Progress progress;
    progress.beginSession (100);
    progress.record ("standard", { 0, 5 }, true, 100);
    progress.record ("standard", { 0, 5 }, false, 100);
    progress.record ("drop-d", { 3, 7 }, true, 100);

    const auto read = Progress::fromText (progress.toText());

    CHECK_EQ (read.sessions(), 1);
    CHECK_EQ (read.totalAsked(), 3);
    CHECK_EQ (read.totalRight(), 2);
    CHECK_EQ (read.lookup ("standard", { 0, 5 })->asked, 2);
    CHECK_EQ (read.lookup ("drop-d", { 3, 7 })->right, 1);
}

TEST ("a fret is a different note in a different tuning, so it is a different record")
{
    Progress progress;
    progress.record ("standard", { 0, 5 }, true, 100);

    CHECK (progress.lookup ("drop-d", { 0, 5 }) == nullptr);
    CHECK_EQ (progress.strengthFor ("drop-d", { 0, 5 }, 100), -1);
    CHECK_EQ (progress.placesSeen ("standard"), 1);
    CHECK_EQ (progress.placesSeen ("drop-d"), 0);
}

TEST ("rubbish in browser storage starts a learner over rather than stopping them")
{
    // This arrives from wherever the shell put it, which is a place a previous
    // version of this app - or a passing extension - can also write.
    for (const auto* text : { "", "nonsense", "guitar-progress 99\nstandard 0 5 2 1 1 100\n",
                              "guitar-progress 1\nstandard 0\n",
                              "guitar-progress 1\nstandard 0 5 2 9 1 100\n" })
    {
        const auto progress = Progress::fromText (text);
        CHECK_EQ (progress.totalAsked(), 0);
    }
}

TEST ("a half-written line is dropped and the rest of the record survives")
{
    const auto progress = Progress::fromText (
        "guitar-progress 1\nsessions 3 220\nstandard 0 5 4 3 1 220\nstandard 1\nstandard 2 7 2 0 0 219\n");

    CHECK_EQ (progress.sessions(), 3);
    CHECK_EQ (progress.totalAsked(), 6);
    CHECK (progress.lookup ("standard", { 1, 0 }) == nullptr);
}

TEST ("a session is a day, not a run")
{
    Progress progress;
    progress.beginSession (10);
    progress.beginSession (10);
    progress.beginSession (10);
    CHECK_EQ (progress.sessions(), 1);

    progress.beginSession (11);
    CHECK_EQ (progress.sessions(), 2);
    CHECK_EQ (progress.lastDay(), 11);
}

TEST ("a miss makes a place more likely to come back than a place never seen")
{
    Progress progress;
    progress.record ("standard", { 0, 3 }, false, 100);

    const auto missed = progress.weightFor ("standard", { 0, 3 }, 100);
    const auto fresh = progress.weightFor ("standard", { 0, 4 }, 100);

    CHECK (missed > fresh);
    CHECK (fresh > 1);   // new ground is worth showing, just not first
}

TEST ("a place answered right several times running drops away")
{
    const auto known = after (5, 0).weightFor ("standard", { 0, 5 }, 100);
    const auto shaky = after (2, 2).weightFor ("standard", { 0, 5 }, 100);

    CHECK (known < shaky);
    CHECK (known >= 1);
}

TEST ("everything drifts back up as the days pass, and stops drifting")
{
    const auto progress = after (5, 0);
    const auto sameDay = progress.weightFor ("standard", { 0, 5 }, 100);
    const auto aWeek = progress.weightFor ("standard", { 0, 5 }, 107);
    const auto aYear = progress.weightFor ("standard", { 0, 5 }, 465);

    CHECK (aWeek > sameDay);
    CHECK (aYear >= aWeek);

    // Capped: a neck left alone for a year would otherwise come back uniformly
    // urgent, and a weighting that says everything is urgent says nothing.
    CHECK (aYear - aWeek <= 6);
}

TEST ("strength is what the neck is painted from")
{
    CHECK_EQ (after (4, 0).strengthFor ("standard", { 0, 5 }, 100), 100);
    CHECK (after (1, 3).strengthFor ("standard", { 0, 5 }, 100) < 40);
    CHECK_EQ (after (0, 0).strengthFor ("standard", { 0, 5 }, 100), -1);
}

TEST ("what is known fades, and never below nothing")
{
    const auto progress = after (4, 0);
    CHECK (progress.strengthFor ("standard", { 0, 5 }, 110) < progress.strengthFor ("standard", { 0, 5 }, 100));
    CHECK (progress.strengthFor ("standard", { 0, 5 }, 5000) >= 0);
}

TEST ("the weakest places come back weakest first, and only the ones that were missed")
{
    Progress progress;

    for (auto i = 0; i < 4; ++i)
        progress.record ("standard", { 0, 1 }, true, 100);   // known

    progress.record ("standard", { 1, 2 }, false, 100);       // missed once
    progress.record ("standard", { 2, 3 }, false, 100);       // missed twice
    progress.record ("standard", { 2, 3 }, false, 100);

    const auto weak = progress.weakest ("standard", 4, 100);

    CHECK_EQ (static_cast<int> (weak.size()), 2);
    CHECK_EQ (weak.front().string, 2);
    CHECK_EQ (weak.front().fret, 3);
}

TEST ("a limit is a limit")
{
    Progress progress;

    for (auto fret = 0; fret < 8; ++fret)
        progress.record ("standard", { 0, fret }, false, 100);

    CHECK_EQ (static_cast<int> (progress.weakest ("standard", 3, 100).size()), 3);
}
