// Turning a chord into something a left hand can do.
//
// The search is deliberately dumb and the filter is where the knowledge is: it
// enumerates every combination of frets in a four-fret window whose notes are in
// the chord, and then throws away the ones a hand cannot make. That order
// matters. A generator that only knew the shapes it was taught would offer the
// five it was taught; this one finds the voicing halfway up the neck that nobody
// wrote down, and rejects the one that needs a sixth finger, for the same
// reason and in the same pass.

#include "guitar/core/ChordShapes.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <string>

namespace guitar::core
{

namespace
{
    /** Four frets, because that is how many a hand covers without moving.

        Not a rule of music - a rule of arms. Everything above the fifth fret
        could stretch further and everything below it rather less, and one
        number for the whole neck is the simplification this makes.
    */
    constexpr int handSpan = 3;   // highest fret minus lowest, so four frets inclusive

    std::string ordinal (int number)
    {
        const auto lastTwo = number % 100;

        if (lastTwo >= 11 && lastTwo <= 13)
            return std::to_string (number) + "th";

        switch (number % 10)
        {
            case 1:  return std::to_string (number) + "st";
            case 2:  return std::to_string (number) + "nd";
            case 3:  return std::to_string (number) + "rd";
            default: return std::to_string (number) + "th";
        }
    }

    bool contains (const std::vector<int>& values, int value)
    {
        return std::find (values.begin(), values.end(), value) != values.end();
    }

    /** Everything about a fret vector that deciding on it needs.

        Filled in one pass, because half of these questions answer each other:
        whether there is a barre decides how many fingers are needed, which
        decides whether the shape is playable at all.
    */
    struct Examined
    {
        bool playable {};
        std::vector<int> fingers;
        int baseFret {};
        int barreFret {};
        int barreFrom {};
        int barreTo {};
        int span {};
        int fingersUsed {};
        int sounded {};
        int innerMutes {};
        bool hasOpen {};
    };

    Examined examine (const std::vector<int>& frets)
    {
        Examined result;
        result.fingers.assign (frets.size(), muted);

        std::vector<int> soundedStrings;
        std::vector<int> fretted;   // string indices with fret > 0

        for (std::size_t string = 0; string < frets.size(); ++string)
        {
            if (frets[string] == muted)
                continue;

            soundedStrings.push_back (static_cast<int> (string));

            if (frets[string] == 0)
            {
                result.hasOpen = true;
                result.fingers[string] = 0;
            }
            else
            {
                fretted.push_back (static_cast<int> (string));
            }
        }

        result.sounded = static_cast<int> (soundedStrings.size());

        if (result.sounded == 0)
            return result;

        // A muted string with sounded strings either side of it has to be
        // silenced by the fretting hand touching it, which is a real technique
        // and a real cost. More than one is somebody's transcription error.
        for (auto string = soundedStrings.front(); string <= soundedStrings.back(); ++string)
            if (frets[static_cast<std::size_t> (string)] == muted)
                ++result.innerMutes;

        if (fretted.empty())
        {
            result.playable = true;   // everything open: nothing to finger
            return result;
        }

        auto low = highestFret + 1;
        auto high = 0;

        for (auto string : fretted)
        {
            low = std::min (low, frets[static_cast<std::size_t> (string)]);
            high = std::max (high, frets[static_cast<std::size_t> (string)]);
        }

        result.baseFret = low;
        result.span = high - low;

        if (result.span > handSpan)
            return result;

        // A barre is the index finger lying across the lowest fret in the shape,
        // and it only works if it starts at the bass: a finger cannot press the
        // top strings and leave the ones underneath it ringing open.
        std::vector<int> atLow;

        for (auto string : fretted)
            if (frets[static_cast<std::size_t> (string)] == low)
                atLow.push_back (string);

        auto barre = false;

        if (atLow.size() >= 2 && atLow.front() == soundedStrings.front())
        {
            barre = true;

            // Nothing under the barre may need to sound open or be fretted
            // below it - the finger is already there.
            for (auto string = atLow.front(); string <= atLow.back(); ++string)
            {
                const auto fret = frets[static_cast<std::size_t> (string)];

                if (fret != muted && fret < low)
                    barre = false;
            }
        }

        std::size_t nextFinger = 1;

        if (barre)
        {
            result.barreFret = low;
            result.barreFrom = atLow.front();
            result.barreTo = atLow.back();

            for (auto string : atLow)
                result.fingers[static_cast<std::size_t> (string)] = 1;

            nextFinger = 2;
        }

        // Everything the barre did not cover, lowest fret first: a hand that
        // put its little finger on the second fret and its index on the fifth
        // would have to be on backwards.
        std::vector<int> remaining;

        for (auto string : fretted)
            if (result.fingers[static_cast<std::size_t> (string)] != 1)
                remaining.push_back (string);

        std::stable_sort (remaining.begin(), remaining.end(),
                          [&frets] (int a, int b)
                          {
                              return frets[static_cast<std::size_t> (a)] < frets[static_cast<std::size_t> (b)];
                          });

        for (auto string : remaining)
        {
            if (nextFinger > 4)
                return result;   // more notes than fingers

            result.fingers[static_cast<std::size_t> (string)] = static_cast<int> (nextFinger++);
        }

        result.fingersUsed = static_cast<int> (nextFinger) - 1;
        result.playable = true;
        return result;
    }

    /** Which of the CAGED grips this is, read off the tuning rather than off the
        string number.

        The letters are not names for "the sixth string" and so on - they are
        names for the open chord the grip came from, and that only exists where
        the strings above the root are tuned the way a standard guitar's are.
        So the test is the interval pattern above the bass string: five strings
        at 5-5-5-4-5 is the E shape wherever that string happens to be, which is
        why it is still the E shape on a seven-string (where it is the second
        string up) and still the E shape tuned down a semitone (where the string
        is an Eb).

        Everything else gets no letter, and correctly: the low string in drop D
        is a D, but the grip rooted there is not the D shape, because the string
        above it is a fifth away instead of a fourth. A bass fails the test too,
        for the good reason that CAGED is guitar vocabulary about six strings.
    */
    std::string barreShapeName (const Fretboard& board, int bassString)
    {
        std::vector<int> above;

        for (auto string = bassString; string + 1 < board.stringCount(); ++string)
            above.push_back (board.noteAt (string + 1, 0) - board.noteAt (string, 0));

        if (above == std::vector<int> { 5, 5, 5, 4, 5 }) return "E-shape barre";
        if (above == std::vector<int> { 5, 5, 4, 5 })    return "A-shape barre";
        if (above == std::vector<int> { 5, 4, 5 })       return "D-shape barre";

        return "Barre";
    }

    /** True when two fret vectors are the same grip with different strings let
        go of - "x32010" and "x3201x" are one shape, and offering both as
        alternatives is how a list of eight voicings turns out to contain two.

        The test is deliberately geometric rather than harmonic: the fingers do
        not move, so it is the same thing to play, whatever the muting does to
        what it says.
    */
    bool sameGrip (const std::vector<int>& a, const std::vector<int>& b)
    {
        auto aOnly = false;
        auto bOnly = false;

        // Letting go of the lowest string is not a muting decision, it is a
        // different chord: the bass note is what a voicing is named after and
        // half of what it sounds like. An open G in open-G tuning strummed
        // across all six strings has a D underneath it, and offering that
        // beside the five-string version is offering two real answers.
        const auto lowestOf = [] (const std::vector<int>& frets)
        {
            for (std::size_t string = 0; string < frets.size(); ++string)
                if (frets[string] != muted)
                    return static_cast<int> (string);

            return -1;
        };

        if (lowestOf (a) != lowestOf (b))
            return false;

        for (std::size_t string = 0; string < a.size(); ++string)
        {
            const auto inA = a[string] != muted;
            const auto inB = b[string] != muted;

            if (inA && inB && a[string] != b[string])
                return false;   // a finger in a different place: a different shape

            aOnly = aOnly || (inA && ! inB);
            bOnly = bOnly || (inB && ! inA);
        }

        // Each sounds a string the other does not: neither is the other with
        // something muted, so they are two shapes that happen to overlap.
        return ! (aOnly && bOnly);
    }

    std::string describeStringSet (const Fretboard& board, const std::vector<int>& frets)
    {
        auto lowest = -1;
        auto highest = -1;

        for (std::size_t string = 0; string < frets.size(); ++string)
        {
            if (frets[string] == muted)
                continue;

            if (lowest < 0)
                lowest = static_cast<int> (string);

            highest = static_cast<int> (string);
        }

        if (lowest < 0)
            return {};

        return board.stringName (highest) + " to the " + board.stringName (lowest);
    }
}

std::vector<ChordShape> chordShapes (const Chord& chord, const Tuning& tuning, const ShapeSearch& search)
{
    const Fretboard board { tuning };
    const auto chordNotes = chord.pitchClasses();
    const auto wantedBass = chord.bass.value_or (chord.root);

    const auto lowestFret = std::max (0, search.fromFret);
    const auto topFret = std::min (search.toFret, highestFret);

    // A power chord is two notes and is allowed to be; everything else needs at
    // least three sounding, or it is an interval rather than a chord.
    const auto minimumStrings = chord.quality.intervals.size() <= 2 ? 2 : 3;

    struct Found
    {
        ChordShape shape;
        int score {};
    };

    std::vector<Found> found;
    std::set<std::vector<int>> seen;

    for (auto window = lowestFret; window + handSpan <= topFret || window == lowestFret; ++window)
    {
        if (window > topFret)
            break;

        // Per string: every fret in this window that sounds a note of the
        // chord, plus the open string when it does, plus not playing it at all.
        std::vector<std::vector<int>> choices (static_cast<std::size_t> (board.stringCount()));

        for (auto string = 0; string < board.stringCount(); ++string)
        {
            auto& perString = choices[static_cast<std::size_t> (string)];
            perString.push_back (muted);

            const auto openNote = board.noteAt (string, 0);

            if (window <= 4 && lowestFret == 0 && contains (chordNotes, toPitchClass (openNote)))
                perString.push_back (0);

            for (auto fret = std::max (1, window); fret <= std::min (window + handSpan, topFret); ++fret)
                if (contains (chordNotes, toPitchClass (board.noteAt (string, fret))))
                    perString.push_back (fret);
        }

        std::vector<int> frets (static_cast<std::size_t> (board.stringCount()), muted);

        // Depth-first over the strings. The combinations are few because the
        // candidate list above is already only chord tones - four or five per
        // string at most, and usually two.
        std::function<void (std::size_t)> walk = [&] (std::size_t string)
        {
            if (string == frets.size())
            {
                std::vector<PitchClass> sounding;
                auto soundedCount = 0;
                auto lowestNote = 0;
                auto highestNote = 0;
                auto haveLowest = false;

                for (std::size_t s = 0; s < frets.size(); ++s)
                {
                    if (frets[s] == muted)
                        continue;

                    const auto note = board.noteAt (static_cast<int> (s), frets[s]);
                    ++soundedCount;
                    sounding.push_back (toPitchClass (note));

                    if (! haveLowest)
                    {
                        lowestNote = note;
                        haveLowest = true;
                    }

                    highestNote = note;
                }

                if (soundedCount < static_cast<int> (minimumStrings))
                    return;

                for (auto interval : chord.quality.essential)
                    if (std::find (sounding.begin(), sounding.end(), toPitchClass (chord.root + interval)) == sounding.end())
                        return;

                const auto bassIsWanted = toPitchClass (lowestNote) == wantedBass;

                if (! bassIsWanted && (! search.allowInversions || chord.bass.has_value()))
                    return;

                auto examined = examine (frets);

                if (! examined.playable || examined.innerMutes > 1)
                    return;

                if (examined.barreFret > 0 && ! search.allowBarre)
                    return;

                if (search.simpleOnly && (examined.barreFret > 0 || examined.span > 2 || examined.fingersUsed > 3))
                    return;

                if (! seen.insert (frets).second)
                    return;

                ChordShape shape;
                shape.frets = frets;
                shape.fingers = examined.fingers;
                shape.baseFret = examined.baseFret;
                shape.barreFret = examined.barreFret;
                shape.barreFromString = examined.barreFrom;
                shape.barreToString = examined.barreTo;
                shape.span = examined.span;
                shape.fingersUsed = examined.fingersUsed;
                shape.soundedStrings = soundedCount;
                shape.lowestNote = lowestNote;
                shape.highestNote = highestNote;
                shape.rootInBass = toPitchClass (lowestNote) == chord.root;
                shape.hasOpenStrings = examined.hasOpen;
                shape.bassNote = midiNoteName (lowestNote);

                for (std::size_t s = 0; s < frets.size(); ++s)
                {
                    if (frets[s] == muted)
                    {
                        shape.degrees.push_back ({});
                        continue;
                    }

                    const auto note = board.noteAt (static_cast<int> (s), frets[s]);
                    auto degree = chord.degreeOf (toPitchClass (note));
                    shape.degrees.push_back (degree.value_or (""));
                }

                std::set<PitchClass> distinct (sounding.begin(), sounding.end());

                for (auto interval : chord.quality.intervals)
                    if (distinct.count (toPitchClass (chord.root + interval)) == 0)
                        shape.omitted.push_back (intervalName (interval, chord.quality.minorThird));

                // --- naming, and what it is for ------------------------------
                const auto bassString = static_cast<int> (std::distance (frets.begin(),
                    std::find_if (frets.begin(), frets.end(), [] (int f) { return f != muted; })));

                const auto movedUp = pitchClassName (chord.root + 2) + chord.quality.suffix;

                if (examined.barreFret > 0)
                {
                    shape.name = (shape.rootInBass ? barreShapeName (board, bassString) : std::string ("Barre"))
                               + ", " + ordinal (examined.barreFret) + " fret";
                    shape.note = "One grip, and it moves: the same shape two frets up is "
                               + movedUp + ".";
                }
                else if (examined.hasOpen && examined.baseFret <= 3)
                {
                    shape.name = "Open position";
                    shape.note = "Open strings ring longer than fretted ones. This is the sound "
                                 "the instrument was designed around - and it does not move.";
                }
                else if (soundedCount <= 3)
                {
                    shape.name = "Triad, " + ordinal (examined.baseFret) + " fret";
                    shape.note = "Three notes, on the " + describeStringSet (board, frets)
                               + ". Small enough to leave room for a bass player.";
                }
                else if (shape.omitted.size() == 1 && shape.omitted.front() == "5" && soundedCount <= 4)
                {
                    shape.name = "Shell voicing, " + ordinal (examined.baseFret) + " fret";
                    shape.note = "Root, third and seventh - the fifth says nothing about which "
                                 "chord this is, so it goes first.";
                }
                else if (examined.hasOpen)
                {
                    // Open strings against a shape further up the neck. Real,
                    // and the reason people play in DADGAD - but it is not a
                    // movable shape, and saying so would be teaching a lie:
                    // move this one and the open strings stay where they are.
                    shape.name = "Open strings, " + ordinal (examined.baseFret) + " fret";
                    shape.note = "Fretted notes high up against open strings below them. "
                                 "It rings like nothing else and it does not transpose.";
                }
                else
                {
                    shape.name = "Movable shape, " + ordinal (examined.baseFret) + " fret";
                    shape.note = "Nothing open, so it moves: the same grip two frets up is "
                               + movedUp + ".";
                }

                if (! shape.rootInBass)
                {
                    shape.name += " (over " + pitchClassName (toPitchClass (lowestNote)) + ")";
                    shape.note = "An inversion: " + pitchClassName (toPitchClass (lowestNote))
                               + " is at the bottom rather than the root. Useful when the bass "
                                 "is walking and you are not.";
                }

                if (examined.barreFret > 0 && examined.fingersUsed >= 4)
                    shape.difficulty = "hard";
                else if (examined.barreFret > 0 || examined.fingersUsed >= 4 || examined.span >= 3)
                    shape.difficulty = "moderate";
                else if (examined.hasOpen)
                    shape.difficulty = "open";
                else
                    shape.difficulty = "easy";

                // --- ranking -------------------------------------------------
                // Not a measure of how good a voicing is - there is no such
                // number - but of which one to show a learner first. Familiar,
                // low on the neck, root at the bottom, and playable without
                // heroics all count for more than completeness.
                auto score = 0;
                score += shape.rootInBass ? 30 : 0;

                /* Open strings are worth a great deal in the open position and
                   are a liability once the hand has left it. Both halves of
                   that are the same fact: an open string is a note you cannot
                   move, so a shape built on one is a shape for this key only.
                   Down here that is the point - it is why open chords sound
                   the way they do. At the seventh fret it is usually an
                   accident of which key was asked for, and the same voicing
                   with the string fretted is the one worth learning. */
                score += 4 * soundedCount;
                score += 5 * static_cast<int> (distinct.size());
                score -= 2 * examined.baseFret;
                score -= 2 * examined.fingersUsed;
                score -= 2 * examined.span;

                // A barre costs something to play and almost nothing here: the
                // whole reason to learn one is that it moves, and a list that
                // ranked barre chords below every four-string grab would hide
                // exactly the shapes worth learning.
                score -= examined.barreFret > 0 ? 2 : 0;
                score -= 25 * examined.innerMutes;

                if (examined.hasOpen)
                    score += examined.baseFret <= 2 ? 22 : (examined.baseFret <= 4 ? -14 : -22);

                // The same note twice at the same pitch is a real guitar sound
                // and a poor first answer: it spends a string saying something
                // already said. Worth having in the list, not at the top of it.
                std::set<int> distinctNotes;
                auto unisons = 0;

                for (std::size_t s2 = 0; s2 < frets.size(); ++s2)
                    if (frets[s2] != muted && ! distinctNotes.insert (board.noteAt (static_cast<int> (s2), frets[s2])).second)
                        ++unisons;

                score -= 18 * unisons;

                found.push_back ({ std::move (shape), score });
                return;
            }

            for (auto fret : choices[string])
            {
                frets[string] = fret;
                walk (string + 1);
            }

            frets[string] = muted;
        };

        walk (0);
    }

    std::stable_sort (found.begin(), found.end(),
                      [] (const Found& a, const Found& b) { return a.score > b.score; });

    // Spread the answers out. Left alone, the top of that list is eight
    // near-identical grips in the same two frets, and a button called "try
    // another voicing" that hands back the same chord with one note moved is a
    // button nobody presses twice. One shape per position per string set, and
    // the next press goes somewhere else on the neck.
    std::vector<ChordShape> chosen;
    std::map<int, int> usedRegions;

    for (auto pass = 0; pass < 2 && static_cast<int> (chosen.size()) < search.maxShapes; ++pass)
    {
        for (const auto& candidate : found)
        {
            if (static_cast<int> (chosen.size()) >= search.maxShapes)
                break;

            // First pass takes the best shape in each part of the neck; a
            // second pass fills up from what is left, so a chord with few
            // options still comes back with a list rather than two entries.
            const auto region = candidate.shape.baseFret / 3;

            if (pass == 0 && usedRegions[region] > 0)
                continue;

            if (std::any_of (chosen.begin(), chosen.end(),
                             [&candidate] (const ChordShape& already)
                             {
                                 return sameGrip (already.frets, candidate.shape.frets);
                             }))
                continue;

            ++usedRegions[region];
            chosen.push_back (candidate.shape);
        }
    }

    return chosen;
}

} // namespace guitar::core
