#pragma once

#include "guitar/core/Fretboard.h"

#include <string>
#include <vector>

namespace guitar::core
{

/** What a learner has shown they know about one place on one neck. */
struct PositionRecord
{
    std::string tuning;     ///< the tuning it was asked in; a fret is a different note in each
    int string {};
    int fret {};
    int asked {};
    int right {};
    int streak {};          ///< consecutive right answers, reset by one miss
    int lastSeenDay {};     ///< the day the shell said it was, when this was last asked
};

/** One learner's history, carried between sessions.

    The engine stores nothing and has nowhere to store it. What it owns is what
    a history *is* and what it is for - which places to ask about next, and what
    a run of them means against everything before it. Where the bytes live is
    the shell's business: the page keeps this in its browser storage, and the
    only thing crossing the boundary is the string `toText` returns.

    That split is the same one as everywhere else here. Deciding that the 7th
    fret of the 5th string is due again is pedagogy, and pedagogy does not live
    in a page. Writing a string to disk is not pedagogy, and does not belong in
    the engine.

    As with the seed and the stopwatch, the calendar arrives as data: `today` is
    a day number the shell worked out, and the engine only ever subtracts two of
    them. It still has no clock.
*/
class Progress
{
public:
    /** Reads a record back. Anything unreadable comes back empty rather than
        throwing: this arrives from browser storage, which can hold whatever a
        previous version of this app wrote, and losing a history is a bad day
        where refusing to start is a bug report. */
    static Progress fromText (const std::string& text);

    /** The record as one line-based block, version-stamped so that a future
        format can recognise this one rather than guess at it. */
    std::string toText() const;

    /** Notes one answer. @p day is the shell's day number. */
    void record (const std::string& tuning, const Position& position, bool wasRight, int day);

    /** Starts counting a new session, if @p day is not the day of the last one. */
    void beginSession (int day);

    const PositionRecord* lookup (const std::string& tuning, const Position& position) const;

    /** How much this place deserves to come up, before anything that has
        happened in the current run is taken into account.

        Higher is more likely. The shape of it: somewhere never asked about is
        worth showing, somewhere missed is worth a great deal, somewhere
        answered right several times running is worth very little - and
        everywhere drifts back up as the days pass, because that is what
        forgetting is.
    */
    int weightFor (const std::string& tuning, const Position& position, int today) const;

    /** 0-100, or -1 for a place that has never been asked about. What the neck
        is painted from when a learner asks what they know. */
    int strengthFor (const std::string& tuning, const Position& position, int today) const;

    /** The places in this tuning worth going back to, weakest first. */
    std::vector<PositionRecord> weakest (const std::string& tuning, int limit, int today) const;

    int sessions() const noexcept  { return sessionCount; }
    int lastDay() const noexcept   { return lastSessionDay; }
    int totalAsked() const;
    int totalRight() const;

    /** How many places in this tuning have been asked about at all. */
    int placesSeen (const std::string& tuning) const;

    const std::vector<PositionRecord>& records() const noexcept { return entries; }

private:
    PositionRecord& entryFor (const std::string& tuning, const Position& position);

    std::vector<PositionRecord> entries;
    int sessionCount {};
    int lastSessionDay {};
};

} // namespace guitar::core
