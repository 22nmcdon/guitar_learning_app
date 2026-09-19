#include "guitar/core/Progress.h"

#include <algorithm>
#include <sstream>

namespace guitar::core
{

namespace
{
    // Bumped only when the format changes in a way an older reader would get
    // wrong. A reader that meets a version it does not know starts empty, which
    // is the same thing that happens on a first visit and is survivable.
    constexpr int formatVersion = 1;

    int clamp (int value, int low, int high)
    {
        return std::max (low, std::min (high, value));
    }
}

Progress Progress::fromText (const std::string& text)
{
    Progress progress;

    if (text.empty())
        return progress;

    std::istringstream stream (text);
    std::string line;

    if (! std::getline (stream, line))
        return progress;

    {
        std::istringstream header (line);
        std::string magic;
        auto version = 0;

        if (! (header >> magic >> version) || magic != "guitar-progress" || version != formatVersion)
            return progress;
    }

    while (std::getline (stream, line))
    {
        std::istringstream fields (line);
        std::string what;

        if (! (fields >> what))
            continue;

        if (what == "sessions")
        {
            fields >> progress.sessionCount >> progress.lastSessionDay;
            continue;
        }

        // Anything else is a position: tuning, string, fret, then its numbers.
        PositionRecord record;
        record.tuning = what;

        if (! (fields >> record.string >> record.fret >> record.asked
                      >> record.right >> record.streak >> record.lastSeenDay))
            continue;   // a half-written line is dropped, not guessed at

        if (record.string < 0 || record.fret < 0 || record.asked <= 0 || record.right < 0
            || record.right > record.asked)
            continue;

        progress.entries.push_back (record);
    }

    return progress;
}

std::string Progress::toText() const
{
    std::ostringstream out;
    out << "guitar-progress " << formatVersion << "\n";
    out << "sessions " << sessionCount << " " << lastSessionDay << "\n";

    for (const auto& record : entries)
        out << record.tuning << " " << record.string << " " << record.fret << " "
            << record.asked << " " << record.right << " " << record.streak << " "
            << record.lastSeenDay << "\n";

    return out.str();
}

PositionRecord& Progress::entryFor (const std::string& tuning, const Position& position)
{
    for (auto& record : entries)
        if (record.tuning == tuning && record.string == position.string && record.fret == position.fret)
            return record;

    PositionRecord fresh;
    fresh.tuning = tuning;
    fresh.string = position.string;
    fresh.fret = position.fret;
    entries.push_back (fresh);
    return entries.back();
}

const PositionRecord* Progress::lookup (const std::string& tuning, const Position& position) const
{
    for (const auto& record : entries)
        if (record.tuning == tuning && record.string == position.string && record.fret == position.fret)
            return &record;

    return nullptr;
}

void Progress::record (const std::string& tuning, const Position& position, bool wasRight, int day)
{
    auto& entry = entryFor (tuning, position);
    ++entry.asked;
    entry.right += wasRight ? 1 : 0;
    entry.streak = wasRight ? entry.streak + 1 : 0;
    entry.lastSeenDay = day;
}

void Progress::beginSession (int day)
{
    // A session is a day, not a run: somebody who does three quizzes over lunch
    // has practised once, and telling them otherwise makes the number worthless.
    if (sessionCount == 0 || day != lastSessionDay)
        ++sessionCount;

    lastSessionDay = day;
}

int Progress::weightFor (const std::string& tuning, const Position& position, int today) const
{
    const auto* entry = lookup (tuning, position);

    // Never asked about. Worth more than a place that is known and less than one
    // that is known to be wrong: new ground should come up, but not ahead of the
    // fret that has been missed three times.
    if (entry == nullptr)
        return 6;

    const auto missed = entry->asked - entry->right;
    const auto daysSince = std::max (0, today - entry->lastSeenDay);

    // Three terms, and the sizes of them matter more than the shape.
    //
    // Confidence is capped below the base, which it was not to begin with: a
    // run of five right answers subtracted more than the base was worth, the
    // sum hit the floor, and a mastered place then sat at the floor for ever -
    // the decay term underneath it could never lift it off. Capped, the same
    // place comes back after a fortnight, which is the entire reason for
    // keeping a record between sessions.
    const auto base = 6 + 8 * missed;
    const auto confidence = std::min (6, 2 * entry->streak);
    const auto forgetting = std::min (12, daysSince);

    return clamp (base - confidence + forgetting, 1, 60);
}

int Progress::strengthFor (const std::string& tuning, const Position& position, int today) const
{
    const auto* entry = lookup (tuning, position);

    if (entry == nullptr || entry->asked == 0)
        return -1;

    const auto accuracy = (entry->right * 100 + entry->asked / 2) / entry->asked;
    const auto confidence = std::min (20, entry->streak * 7);
    const auto faded = std::min (40, std::max (0, today - entry->lastSeenDay) * 2);

    // Confidence is capped at knowing it, and the fade is applied after that
    // rather than into the same sum. Otherwise a long streak banks enough
    // credit to swallow the fade whole, and a place last seen in March is still
    // painted as solid in June.
    return clamp (std::min (100, accuracy + confidence) - faded, 0, 100);
}

std::vector<PositionRecord> Progress::weakest (const std::string& tuning, int limit, int today) const
{
    std::vector<PositionRecord> found;

    for (const auto& entry : entries)
        if (entry.tuning == tuning && entry.asked > 0 && entry.right < entry.asked)
            found.push_back (entry);

    std::stable_sort (found.begin(), found.end(),
                      [this, &tuning, today] (const PositionRecord& a, const PositionRecord& b)
                      {
                          const auto strengthA = strengthFor (tuning, { a.string, a.fret }, today);
                          const auto strengthB = strengthFor (tuning, { b.string, b.fret }, today);

                          if (strengthA != strengthB)
                              return strengthA < strengthB;

                          // Strength bottoms out at nothing, and two places at
                          // nothing are not equally in need: the one missed
                          // three times is the one to go back to.
                          return (a.asked - a.right) > (b.asked - b.right);
                      });

    if (static_cast<int> (found.size()) > limit)
        found.resize (static_cast<std::size_t> (limit));

    return found;
}

int Progress::totalAsked() const
{
    auto total = 0;

    for (const auto& entry : entries)
        total += entry.asked;

    return total;
}

int Progress::totalRight() const
{
    auto total = 0;

    for (const auto& entry : entries)
        total += entry.right;

    return total;
}

int Progress::placesSeen (const std::string& tuning) const
{
    auto seen = 0;

    for (const auto& entry : entries)
        if (entry.tuning == tuning && entry.asked > 0)
            ++seen;

    return seen;
}

} // namespace guitar::core
