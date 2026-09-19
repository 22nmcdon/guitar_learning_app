// Asking someone where they are on the neck.
//
// Two things here are less obvious than they look.
//
// The first is that the engine does not know what time it is and must not find
// out. How long an answer took is a fact about the player, it belongs in the
// summary, and it arrives as a number the shell passes in - exactly like the
// seed. An engine that called a clock would be an engine whose tests could not
// be written.
//
// The second is that the questions are not uniformly random. A place that has
// already caught someone out is more likely to come back, because the point is
// not to sample the fretboard evenly - it is to find the four frets they do not
// know and keep returning to them.

#include "guitar/core/FretboardQuiz.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <set>

namespace guitar::core
{

namespace
{
    /** Where the dots are on almost every guitar ever made. A learner navigates
        by these long before they navigate by note names, so an answer is worth
        explaining in terms of them. */
    const std::vector<int>& markers()
    {
        static const std::vector<int> dots { 3, 5, 7, 9, 12, 15 };
        return dots;
    }

    std::string trimmed (std::string text)
    {
        while (! text.empty() && std::isspace (static_cast<unsigned char> (text.front())))
            text.erase (text.begin());

        while (! text.empty() && std::isspace (static_cast<unsigned char> (text.back())))
            text.pop_back();

        return text;
    }

    bool sameWord (std::string a, std::string b)
    {
        a = trimmed (std::move (a));
        b = trimmed (std::move (b));

        if (a.size() != b.size())
            return false;

        for (std::size_t i = 0; i < a.size(); ++i)
            if (std::tolower (static_cast<unsigned char> (a[i])) != std::tolower (static_cast<unsigned char> (b[i])))
                return false;

        return true;
    }

    /** "3,5" - a string and a fret, which is how a shell reports a tap. */
    std::optional<Position> parsePosition (const std::string& text)
    {
        const auto comma = text.find (',');

        if (comma == std::string::npos)
            return std::nullopt;

        try
        {
            Position position;
            position.string = std::stoi (text.substr (0, comma));
            position.fret = std::stoi (text.substr (comma + 1));
            return position;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    /** Something to hang a fret on other than its number. */
    std::string landmarkFor (const Fretboard& board, const Position& position)
    {
        const auto note = board.noteAt (position);

        if (position.fret == 0)
            return "That is the open string itself.";

        for (auto string = 0; string < board.stringCount(); ++string)
            if (string != position.string && board.noteAt (string, 0) == note)
                return "The same note as the open " + board.stringName (string) + ".";

        if (std::find (markers().begin(), markers().end(), position.fret) != markers().end())
            return "It is on the " + std::to_string (position.fret) + "th-fret dot"
                 + (position.fret == 12 ? ", where the neck starts over." : ".");

        auto nearest = markers().front();

        for (auto dot : markers())
            if (std::abs (dot - position.fret) < std::abs (nearest - position.fret))
                nearest = dot;

        const auto distance = position.fret - nearest;
        return std::string (distance > 0 ? "One fret above" : "One fret below")
             + (std::abs (distance) > 1 ? " - " + std::to_string (std::abs (distance)) + " frets, in fact - "
                                        : " ")
             + "the " + std::to_string (nearest) + "th-fret dot.";
    }

    bool isNatural (PitchClass pitchClass)
    {
        return ! hasTwoNames (pitchClass);
    }
}

const std::vector<QuizKindInfo>& quizKinds()
{
    static const std::vector<QuizKindInfo> all
    {
        { "name-note", "Name the note", "A dot appears on the neck. Say what it is.", "note", false },
        { "find-note", "Find the note", "A note is named. Put your finger on it.", "position", false },
        { "octaves",   "Find the octave", "One note is marked. Find the same note somewhere else.", "position", false },
        { "chord-tone","Which note of the chord", "A chord and a place. Say which degree of it you are on.", "choice", true }
    };

    return all;
}

unsigned FretboardQuiz::nextRandom()
{
    // xorshift32: three lines, no library, and the same sequence on every
    // platform - which is what "the same seed gives the same quiz" needs.
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

int FretboardQuiz::randomBelow (int limit)
{
    return limit <= 0 ? 0 : static_cast<int> (nextRandom() % static_cast<unsigned> (limit));
}

QuizQuestion FretboardQuiz::start (const QuizOptions& optionsToUse)
{
    QuizQuestion question;
    options = optionsToUse;

    auto found = tuningFor (options.tuningKey);

    if (! found.has_value())
    {
        question.error = "no tuning called '" + options.tuningKey + "'";
        return question;
    }

    const auto* kind = static_cast<const QuizKindInfo*> (nullptr);

    for (const auto& candidate : quizKinds())
        if (candidate.key == options.kind)
            kind = &candidate;

    if (kind == nullptr)
    {
        question.error = "no quiz called '" + options.kind + "'";
        return question;
    }

    tuning = *found;

    // Everything written down is keyed by the tuning the engine resolved rather
    // than by whatever the shell typed: an empty key means standard, and a
    // history filed under "" is one nobody can find again.
    options.tuningKey = tuning.key;

    const Fretboard board { tuning };

    options.fromFret = std::max (0, options.fromFret);
    options.toFret = std::min (options.toFret, highestFret);

    if (options.toFret < options.fromFret)
        std::swap (options.fromFret, options.toFret);

    pool.clear();

    for (auto string = 0; string < board.stringCount(); ++string)
    {
        if (! options.strings.empty()
            && std::find (options.strings.begin(), options.strings.end(), string) == options.strings.end())
            continue;

        for (auto fret = options.fromFret; fret <= options.toFret; ++fret)
            pool.push_back ({ string, fret });
    }

    if (pool.size() < 2)
    {
        question.error = "that leaves fewer than two places to ask about";
        return question;
    }

    if (! options.chordSymbol.empty() && ! parseChord (options.chordSymbol).has_value())
    {
        question.error = "cannot read the chord '" + options.chordSymbol + "'";
        return question;
    }

    // The history the shell handed back, and a copy of it as it stands now: the
    // summary wants to say what this run did, which needs the before as well as
    // the after.
    progress = Progress::fromText (options.progressText);
    incoming = progress;

    wrongAt.assign (pool.size(), 0);
    lastAsked = -1;
    rng = options.seed == 0 ? 1u : options.seed;
    asked = 0;
    right = 0;
    times.clear();
    tallies.clear();
    naturalsAsked = naturalsRight = accidentalsAsked = accidentalsRight = 0;
    lowAsked = lowRight = highAsked = highRight = 0;
    returnedToMisses = 0;
    isRunning = true;

    return ask();
}

QuizQuestion FretboardQuiz::ask()
{
    const Fretboard board { tuning };

    // Weighted by what has already gone wrong, and never the same place twice
    // running: a quiz that asked the fifth fret of the A string four times in a
    // row is testing patience rather than the neck.
    std::vector<int> weights (pool.size(), 0);
    auto total = 0;

    for (std::size_t i = 0; i < pool.size(); ++i)
    {
        if (static_cast<int> (i) == lastAsked)
            continue;

        // Two things stack: what the learner got wrong in the last ten minutes,
        // and what they have been getting wrong for a fortnight. With no
        // history the second term is the same everywhere, so a first visit
        // behaves exactly as it did before any of this existed.
        weights[i] = progress.weightFor (options.tuningKey, pool[i], options.today)
                   + 4 * wrongAt[i];
        total += weights[i];
    }

    auto ticket = randomBelow (total);
    std::size_t chosen = 0;

    for (std::size_t i = 0; i < pool.size(); ++i)
    {
        ticket -= weights[i];

        if (weights[i] > 0 && ticket < 0)
        {
            chosen = i;
            break;
        }
    }

    lastAsked = static_cast<int> (chosen);
    const auto position = pool[chosen];
    const auto note = board.noteAt (position);

    QuizQuestion question;
    question.ok = true;
    question.number = asked + 1;
    question.kind = options.kind;

    if (options.kind == "name-note")
    {
        question.prompt = "What note is this?";
        question.detail = board.stringName (position.string) + ", "
                        + (position.fret == 0 ? "open" : "fret " + std::to_string (position.fret));
        question.answerKind = "note";
        question.position = position;
        question.highlight = { position };

        // Decoys that are nearly right: a semitone either side, and the note a
        // string away. Four random note names would be a quiz about reading
        // rather than about the fretboard.
        std::vector<PitchClass> choices { toPitchClass (note) };
        const std::vector<int> nearby { 1, -1, 2, -2, 5, 7 };

        for (auto offset : nearby)
        {
            if (choices.size() >= 4)
                break;

            const auto candidate = toPitchClass (note + offset);

            if (std::find (choices.begin(), choices.end(), candidate) == choices.end())
                choices.push_back (candidate);
        }

        // Shuffled, or the answer is always first and the quiz is about nothing.
        for (auto i = static_cast<int> (choices.size()) - 1; i > 0; --i)
            std::swap (choices[static_cast<std::size_t> (i)],
                       choices[static_cast<std::size_t> (randomBelow (i + 1))]);

        for (auto choice : choices)
            question.choices.push_back (enharmonicName (choice));
    }
    else if (options.kind == "find-note")
    {
        question.prompt = "Where is " + enharmonicName (toPitchClass (note)) + "?";
        question.detail = "On the " + board.stringName (position.string)
                        + ", between frets " + std::to_string (options.fromFret)
                        + " and " + std::to_string (options.toFret) + ".";
        question.answerKind = "position";
        question.position = position;
    }
    else if (options.kind == "octaves")
    {
        question.prompt = "Find this note somewhere else.";
        question.detail = "Same name, different octave - not the same fret on another string.";
        question.answerKind = "position";
        question.position = position;
        question.highlight = { position };
    }
    else   // chord-tone
    {
        auto chord = parseChord (options.chordSymbol);

        if (! chord.has_value())
        {
            // A new chord every few questions, so the drill is about hearing
            // where the third is rather than remembering one answer.
            static const std::vector<std::string> teaching
                { "C", "G", "D", "A", "E", "F", "Am", "Em", "Dm", "G7", "Cmaj7", "Am7" };

            chord = parseChord (teaching[static_cast<std::size_t> (randomBelow (static_cast<int> (teaching.size())))]);
        }

        currentChord = *chord;

        // The place has to be in the chord, or the only answer is "it isn't",
        // which is a different and much less useful question.
        const auto inChord = currentChord.pitchClasses();
        auto tries = 0;
        auto here = position;

        while (std::find (inChord.begin(), inChord.end(), toPitchClass (board.noteAt (here))) == inChord.end()
               && tries++ < 200)
            here = pool[static_cast<std::size_t> (randomBelow (static_cast<int> (pool.size())))];

        question.position = here;
        question.highlight = { here };
        question.prompt = "In " + currentChord.symbol + ", which note is this?";
        question.detail = board.stringName (here.string) + ", "
                        + (here.fret == 0 ? "open" : "fret " + std::to_string (here.fret));
        question.answerKind = "choice";

        for (auto interval : currentChord.quality.intervals)
            question.choices.push_back (intervalName (interval, currentChord.quality.minorThird));
    }

    awaitingAnswer = true;
    current = question;
    return question;
}

QuizVerdict FretboardQuiz::answer (const std::string& given, int elapsedMs)
{
    QuizVerdict verdict;

    if (! isRunning || ! awaitingAnswer)
    {
        verdict.error = "no question is waiting for an answer";
        return verdict;
    }

    const Fretboard board { tuning };
    const auto note = board.noteAt (current.position);
    const auto pitch = toPitchClass (note);

    verdict.ok = true;
    awaitingAnswer = false;

    if (current.answerKind == "note")
    {
        auto parsed = parsePitchClass (trimmed (given));

        // "A#/Bb" is what the shell shows on a button, so it is what comes back
        // when the button is pressed. Either half of it is the same note.
        if (! parsed.has_value())
        {
            const auto slash = given.find ('/');

            if (slash != std::string::npos)
                parsed = parsePitchClass (trimmed (given.substr (0, slash)));
        }

        verdict.correct = parsed.has_value() && *parsed == pitch;
        verdict.correctAnswer = enharmonicName (pitch);
        verdict.correctPositions = { current.position };
    }
    else if (current.answerKind == "position")
    {
        auto where = parsePosition (given);

        if (options.kind == "octaves")
        {
            verdict.correctPositions = board.octaveTwins (current.position, options.fromFret, options.toFret);
            verdict.correctAnswer = "any " + pitchClassName (pitch) + " in another octave";
        }
        else
        {
            for (const auto& candidate : board.positionsFor (pitch, options.fromFret, options.toFret))
                if (candidate.string == current.position.string)
                    verdict.correctPositions.push_back (candidate);

            verdict.correctAnswer = board.stringName (current.position.string) + ", fret "
                                  + std::to_string (current.position.fret);
        }

        verdict.correct = where.has_value()
                       && std::find (verdict.correctPositions.begin(), verdict.correctPositions.end(), *where)
                              != verdict.correctPositions.end();

        // Landing on the right note in the wrong octave is worth saying out
        // loud rather than marking wrong and moving on: it is the mistake that
        // means the shape is understood and the map is not.
        if (! verdict.correct && where.has_value() && toPitchClass (board.noteAt (*where)) == pitch)
        {
            verdict.detail = "Right note, and not one of the places asked for - "
                             "that is the octave shape half-learnt. ";
        }
    }
    else   // choice: a degree of the chord
    {
        const auto degree = currentChord.degreeOf (pitch);
        verdict.correctAnswer = degree.value_or ("not in the chord");
        verdict.correct = degree.has_value() && sameWord (given, *degree);
    }

    ++asked;

    // Counted on the first answer, not on `start`: a quiz that was opened and
    // walked away from is not a session, and would otherwise show up as one in
    // a count somebody is reading as "days I practised".
    if (asked == 1)
        progress.beginSession (options.today);

    // Whether this place was already a known weakness before today, asked of the
    // history as it was rather than as it is about to be.
    if (const auto* before = incoming.lookup (options.tuningKey, current.position))
        if (before->right < before->asked)
            ++returnedToMisses;

    progress.record (options.tuningKey, current.position, verdict.correct, options.today);

    if (verdict.correct)
        ++right;
    else if (lastAsked >= 0 && lastAsked < static_cast<int> (wrongAt.size()))
        ++wrongAt[static_cast<std::size_t> (lastAsked)];

    if (elapsedMs > 0)
        times.push_back (elapsedMs);

    const auto string = current.position.string;
    auto tally = std::find_if (tallies.begin(), tallies.end(),
                               [string] (const StringTally& t) { return t.string == string; });

    if (tally == tallies.end())
    {
        tallies.push_back ({ string, board.stringNameWithNote (string), 0, 0 });
        tally = std::prev (tallies.end());
    }

    ++tally->asked;
    tally->right += verdict.correct ? 1 : 0;

    if (isNatural (pitch))
    {
        ++naturalsAsked;
        naturalsRight += verdict.correct ? 1 : 0;
    }
    else
    {
        ++accidentalsAsked;
        accidentalsRight += verdict.correct ? 1 : 0;
    }

    if (current.position.fret <= 5)
    {
        ++lowAsked;
        lowRight += verdict.correct ? 1 : 0;
    }
    else
    {
        ++highAsked;
        highRight += verdict.correct ? 1 : 0;
    }

    verdict.asked = asked;
    verdict.right = right;

    verdict.verdict = verdict.correct ? "Yes" : "Not quite";

    // Whatever was already said about a near miss stands, and the rest is added
    // to it: being told the note and being told why you nearly had it are two
    // different things and a learner wants both.
    if (current.answerKind == "choice")
        verdict.detail += "That note is " + verdict.correctAnswer + " of " + currentChord.symbol + ".";
    else
        verdict.detail += (verdict.correct ? "" : "It is ") + midiNoteName (note) + ". "
                        + landmarkFor (board, current.position);

    return verdict;
}

QuizQuestion FretboardQuiz::nextQuestion()
{
    QuizQuestion question;

    if (! isRunning)
    {
        question.error = "no quiz is running";
        return question;
    }

    if (awaitingAnswer)
        return current;

    return ask();
}

QuizSummary FretboardQuiz::finish()
{
    QuizSummary summary;
    summary.ok = true;
    summary.asked = asked;
    summary.right = right;
    summary.accuracy = asked > 0 ? (right * 100 + asked / 2) / asked : 0;
    summary.perString = tallies;

    std::stable_sort (summary.perString.begin(), summary.perString.end(),
                      [] (const StringTally& a, const StringTally& b) { return a.string < b.string; });

    if (! times.empty())
    {
        auto sorted = times;
        std::sort (sorted.begin(), sorted.end());
        summary.medianMs = sorted[sorted.size() / 2];
    }

    isRunning = false;
    awaitingAnswer = false;

    // Carried whether or not anything was asked, so that a shell storing this
    // never has to decide: what comes back is the history, and an abandoned run
    // leaves it exactly as it found it.
    summary.progressText = progress.toText();
    summary.sessions = progress.sessions();
    summary.lifetimeAsked = progress.totalAsked();
    summary.lifetimeRight = progress.totalRight();
    summary.daysSinceLastSession = incoming.sessions() > 0
                                       ? std::max (0, options.today - incoming.lastDay())
                                       : 0;

    {
        const Fretboard board { tuning };

        for (const auto& record : progress.weakest (options.tuningKey, 4, options.today))
            summary.weakSpots.push_back ({ record.string, record.fret,
                                           board.stringName (record.string) + ", "
                                               + (record.fret == 0 ? std::string ("open")
                                                                   : "fret " + std::to_string (record.fret)),
                                           progress.strengthFor (options.tuningKey,
                                                                 { record.string, record.fret },
                                                                 options.today) });
    }

    if (asked == 0)
    {
        summary.headline = "Nothing asked yet.";
        return summary;
    }

    if (summary.accuracy == 100)
        summary.headline = "A clean run: " + std::to_string (right) + " out of " + std::to_string (asked) + ".";
    else
        summary.headline = std::to_string (right) + " out of " + std::to_string (asked) + ".";

    // The observations. Each one has to be worth reading, so each has a floor
    // under it: a string asked twice says nothing about that string, and a gap
    // of five per cent says nothing about anything.
    for (const auto& tally : summary.perString)
    {
        if (tally.asked < 3)
            continue;

        const auto accuracy = (tally.right * 100 + tally.asked / 2) / tally.asked;

        if (accuracy + 20 <= summary.accuracy)
            summary.observations.push_back ("The " + tally.name + " is where this falls down - "
                                            + std::to_string (tally.right) + " of "
                                            + std::to_string (tally.asked) + ". Worth a run on that string alone.");
    }

    if (naturalsAsked >= 3 && accidentalsAsked >= 3)
    {
        const auto naturals = (naturalsRight * 100 + naturalsAsked / 2) / naturalsAsked;
        const auto accidentals = (accidentalsRight * 100 + accidentalsAsked / 2) / accidentalsAsked;

        if (naturals >= accidentals + 25)
            summary.observations.push_back ("The natural notes are solid and the sharps are not, which is the "
                                            "usual order of things: they are learnt as \"one above F\" long "
                                            "before they are learnt as F#.");
        else if (accidentals > naturals)
            summary.observations.push_back ("Unusually, the sharps and flats went better than the plain notes. "
                                            "Suspect the naturals are being counted up from the open string.");
    }

    if (lowAsked >= 3 && highAsked >= 3)
    {
        const auto low = (lowRight * 100 + lowAsked / 2) / lowAsked;
        const auto high = (highRight * 100 + highAsked / 2) / highAsked;

        if (low >= high + 25)
            summary.observations.push_back ("Below the fifth fret is known ground; above it is not. "
                                            "The twelfth fret is the same as the open string - working "
                                            "down from there is quicker than counting up.");
    }

    if (summary.medianMs > 0)
    {
        const auto seconds = (summary.medianMs + 500) / 1000;

        if (seconds >= 4)
            summary.observations.push_back ("About " + std::to_string (seconds) + " seconds a question. "
                                            "Right but slow is still counting rather than knowing - the speed "
                                            "is the thing to push next.");
        else if (summary.accuracy >= 80 && seconds <= 2)
            summary.observations.push_back ("Quick as well as right. Move the fret range up and do it again.");
    }

    // What this run was against everything before it. Only worth saying once
    // there is a before: on a first visit these would all be statements about
    // a single run, dressed up as history.
    if (summary.daysSinceLastSession >= 1 && returnedToMisses > 0)
        summary.observations.push_back (std::to_string (returnedToMisses) + " of today's "
            + std::to_string (asked) + " were places you missed last time. That is the point of "
            "keeping a record - and getting one of those right is worth more than getting a "
            "fresh one right.");

    const auto lifetime = summary.lifetimeAsked > 0
                              ? (summary.lifetimeRight * 100 + summary.lifetimeAsked / 2) / summary.lifetimeAsked
                              : 0;

    if (incoming.totalAsked() >= 20 && std::abs (summary.accuracy - lifetime) >= 10)
        summary.observations.push_back (summary.accuracy > lifetime
            ? "Today's " + std::to_string (summary.accuracy) + "% is up on the "
              + std::to_string (lifetime) + "% you are running across every session."
            : "Below the " + std::to_string (lifetime) + "% you usually run. A wider fret range "
              "will do that, and it is the right kind of worse.");

    if (summary.observations.empty())
        summary.observations.push_back (summary.accuracy >= 80
            ? "Nothing stands out as weak. Widen the fret range, or take the string filter off."
            : "Too early to see a pattern. Another dozen questions will show one.");

    // Four is as many as anyone reads. The ones above are ordered weakest-first
    // by what they say about the player, so the tail is the right end to drop.
    if (summary.observations.size() > 4)
        summary.observations.resize (4);

    return summary;
}

} // namespace guitar::core
