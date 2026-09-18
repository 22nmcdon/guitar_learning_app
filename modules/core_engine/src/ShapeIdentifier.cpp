// Reading a shape back: the other direction, and the harder one.
//
// Going from a chord to frets has one right answer per voicing. Coming back has
// several, most of them wrong in a way that is hard to see - a set of notes will
// happily be named as some rootless extension of a chord nobody was playing, and
// that name will be *correct*, and useless. The scoring below is therefore
// biased towards the small, ordinary chord: every name in the catalogue competes
// with every other, and the ones with more notes in them have to earn the extra.

#include "guitar/core/ShapeIdentifier.h"

#include <algorithm>
#include <set>

namespace guitar::core
{

namespace
{
    std::string joinNames (const std::vector<std::string>& names, const std::string& separator = ", ")
    {
        std::string out;

        for (std::size_t i = 0; i < names.size(); ++i)
        {
            if (i > 0)
                out += (i + 1 == names.size() && names.size() > 1 ? std::string (" and ") : separator);

            out += names[i];
        }

        return out;
    }

    /** The name of the interval between two pitch classes, for the two-note case. */
    std::string intervalBetween (int semitones)
    {
        switch (toPitchClass (semitones))
        {
            case 0:  return "a unison";
            case 1:  return "a minor second";
            case 2:  return "a major second";
            case 3:  return "a minor third";
            case 4:  return "a major third";
            case 5:  return "a fourth";
            case 6:  return "a tritone";
            case 7:  return "a fifth";
            case 8:  return "a minor sixth";
            case 9:  return "a major sixth";
            case 10: return "a minor seventh";
            case 11: return "a major seventh";
            default: return "an interval";
        }
    }
}

ShapeReading identifyShape (const Tuning& tuning, const std::vector<int>& frets)
{
    const Fretboard board { tuning };
    ShapeReading reading;

    for (std::size_t string = 0; string < frets.size() && string < static_cast<std::size_t> (board.stringCount()); ++string)
    {
        if (frets[string] == muted)
            continue;

        const auto note = board.noteAt (static_cast<int> (string), frets[string]);

        if (note < 0)
            continue;

        reading.notes.push_back (note);
        reading.noteNames.push_back (midiNoteName (note));
    }

    if (reading.notes.empty())
    {
        reading.summary = "Nothing is sounding. Put a finger down, or let a string ring open.";
        return reading;
    }

    reading.anythingPlayed = true;

    std::set<PitchClass> played;

    for (auto note : reading.notes)
        played.insert (toPitchClass (note));

    const auto bass = toPitchClass (reading.notes.front());

    if (played.size() == 1)
    {
        reading.summary = "One note, " + pitchClassName (bass)
                        + (reading.notes.size() > 1 ? ", in octaves." : ".");
        return reading;
    }

    if (played.size() == 2)
    {
        // Measured up from the note actually at the bottom, not from whichever
        // of the two happens to be lower in the alphabet. A and E played in
        // that order is a fifth; calling it a fourth describes a chord nobody
        // played.
        const auto other = *std::find_if (played.begin(), played.end(),
                                          [bass] (PitchClass note) { return note != bass; });

        reading.summary = pitchClassName (bass) + " and " + pitchClassName (other) + " - "
                        + intervalBetween (ascendingInterval (bass, other))
                        + ". Two notes name a chord only once you decide which is the root.";
        // A power chord is two notes and does have a name, so it is still
        // offered below; the line above is what to say when it is not one.
    }

    struct Scored
    {
        ChordNaming naming;
        int score {};
    };

    std::vector<Scored> candidates;

    for (PitchClass root = 0; root < semitonesPerOctave; ++root)
    {
        for (const auto& quality : chordQualities())
        {
            Chord chord;
            chord.root = root;
            chord.quality = quality;
            chord.symbol = pitchClassName (root) + quality.suffix;

            const auto chordNotes = chord.pitchClasses();
            std::set<PitchClass> inChord (chordNotes.begin(), chordNotes.end());

            // A chord that cannot account for a note being played is not what
            // is being played, whatever else it has going for it.
            std::vector<std::string> extra;

            for (auto note : played)
                if (inChord.count (note) == 0)
                    extra.push_back (pitchClassName (note));

            if (! extra.empty())
                continue;

            auto essentialMissing = false;

            for (auto interval : quality.essential)
                if (played.count (toPitchClass (root + interval)) == 0)
                    essentialMissing = true;

            if (essentialMissing)
                continue;

            ChordNaming naming;
            naming.symbol = chord.symbol;
            naming.qualityName = quality.name;
            naming.rootInBass = bass == root;

            for (auto interval : quality.intervals)
                if (played.count (toPitchClass (root + interval)) == 0)
                    naming.missing.push_back (intervalName (interval, quality.minorThird));

            auto score = 100;

            // Every note the chord claims and nobody played is a claim it has
            // not earned: the more of those, the more this is a name for some
            // other chord that happens to contain these notes.
            score -= 14 * static_cast<int> (naming.missing.size());

            // A bigger chord is a bigger claim. Without this, every open C is
            // also an Am7 with no root and an F6 with no root, and both of
            // those are true and neither is worth saying first.
            score -= 3 * static_cast<int> (quality.intervals.size());

            if (! naming.rootInBass)
                score -= 18;

            if (naming.missing.empty() && naming.rootInBass)
                naming.verdict = "exactly";
            else if (! naming.rootInBass)
                naming.verdict = "over its " + (chord.degreeDescription (bass).value_or ("bass note"));
            else
                naming.verdict = "with " + joinNames (naming.missing) + " left out";

            naming.confidence = std::max (5, std::min (100, score));
            candidates.push_back ({ std::move (naming), score });
        }
    }

    std::stable_sort (candidates.begin(), candidates.end(),
                      [] (const Scored& a, const Scored& b) { return a.score > b.score; });

    for (std::size_t i = 0; i < candidates.size() && i < 3; ++i)
        reading.namings.push_back (candidates[i].naming);

    if (reading.namings.empty())
    {
        if (reading.summary.empty())
            reading.summary = "No chord in the book is made of "
                            + std::to_string (played.size())
                            + " notes including those. Which is not the same as it sounding bad.";

        return reading;
    }

    const auto& best = reading.namings.front();
    reading.summary = best.symbol + " - " + best.qualityName
                    + (best.verdict == "exactly" ? "." : ", " + best.verdict + ".");

    if (reading.namings.size() > 1)
        reading.summary += " It would also answer to " + reading.namings[1].symbol + ".";

    return reading;
}

ShapeVerdict checkShape (const Chord& chord, const Tuning& tuning, const std::vector<int>& frets)
{
    const Fretboard board { tuning };
    ShapeVerdict verdict;

    const auto chordNotes = chord.pitchClasses();
    std::set<PitchClass> inChord (chordNotes.begin(), chordNotes.end());
    std::set<PitchClass> played;
    auto lowest = -1;

    for (std::size_t string = 0; string < frets.size() && string < static_cast<std::size_t> (board.stringCount()); ++string)
    {
        if (frets[string] == muted)
        {
            verdict.degrees.push_back ({});
            continue;
        }

        const auto note = board.noteAt (static_cast<int> (string), frets[string]);
        played.insert (toPitchClass (note));

        if (lowest < 0)
            lowest = note;

        auto degree = chord.degreeOf (toPitchClass (note));
        verdict.degrees.push_back (degree.value_or ("!"));

        if (! degree.has_value())
            verdict.outside.push_back (midiNoteName (note));
    }

    if (played.empty())
    {
        verdict.verdict = "Nothing yet";
        verdict.detail = "Fret the shape on the neck above, or tap a string to let it ring open.";
        return verdict;
    }

    for (auto interval : chord.quality.intervals)
        if (played.count (toPitchClass (chord.root + interval)) == 0)
            verdict.missing.push_back (intervalName (interval, chord.quality.minorThird));

    std::vector<std::string> essentialMissing;

    for (auto interval : chord.quality.essential)
        if (played.count (toPitchClass (chord.root + interval)) == 0)
            essentialMissing.push_back (intervalName (interval, chord.quality.minorThird));

    const auto wantedBass = chord.bass.value_or (chord.root);
    const auto bassRight = lowest >= 0 && toPitchClass (lowest) == wantedBass;

    if (! verdict.outside.empty())
    {
        verdict.verdict = "Not this one";
        verdict.detail = joinNames (verdict.outside) + " "
                       + (verdict.outside.size() > 1 ? "are" : "is")
                       + " not in " + chord.symbol + ". Check which string that is on.";
        return verdict;
    }

    if (! essentialMissing.empty())
    {
        verdict.verdict = "Nearly";
        verdict.detail = "Every note is in the chord, but " + joinNames (essentialMissing)
                       + " is missing - and that is the note that makes it "
                       + chord.quality.name + ".";
        return verdict;
    }

    if (! bassRight)
    {
        verdict.correct = true;
        verdict.verdict = "That's it, inverted";
        verdict.detail = "All the right notes, with "
                       + pitchClassName (toPitchClass (lowest)) + " at the bottom rather than "
                       + pitchClassName (wantedBass) + ". A real voicing - just a different one.";
        return verdict;
    }

    verdict.correct = true;
    verdict.verdict = "That's it";

    if (verdict.missing.empty())
        verdict.detail = chord.symbol + ", every note of it.";
    else
        verdict.detail = chord.symbol + ", with " + joinNames (verdict.missing)
                       + " left out - which is what six strings and four fingers usually means.";

    return verdict;
}

} // namespace guitar::core
