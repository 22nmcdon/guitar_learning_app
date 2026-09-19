#include "TestFramework.h"

#include "guitar/core/FretboardQuiz.h"
#include "guitar/core/Progress.h"

#include <algorithm>

using namespace guitar::core;

namespace
{
    QuizOptions nameNotes (unsigned seed = 7)
    {
        QuizOptions options;
        options.kind = "name-note";
        options.fromFret = 0;
        options.toFret = 5;
        options.seed = seed;
        return options;
    }

    /** The answer a question is looking for, worked out from the board rather
        than from the quiz - so a test that passes proves the quiz agrees with
        the fretboard rather than with itself. */
    std::string rightAnswer (const QuizQuestion& question, const std::string& tuningKey = "standard")
    {
        const Fretboard board { tuningFor (tuningKey).value() };

        if (question.answerKind == "position")
        {
            if (question.kind == "octaves")
            {
                const auto twins = board.octaveTwins (question.position, 0, 12);
                return std::to_string (twins.front().string) + "," + std::to_string (twins.front().fret);
            }

            return std::to_string (question.position.string) + "," + std::to_string (question.position.fret);
        }

        return pitchClassName (board.noteAt (question.position));
    }
}

TEST ("the quiz catalogue is what a menu is built from")
{
    CHECK (quizKinds().size() >= 4);

    for (const auto& kind : quizKinds())
    {
        CHECK (! kind.key.empty());
        CHECK (! kind.name.empty());
        CHECK (! kind.summary.empty());
        CHECK (kind.answerKind == "note" || kind.answerKind == "position" || kind.answerKind == "choice");
    }
}

TEST ("a quiz starts with a question")
{
    FretboardQuiz quiz;
    const auto question = quiz.start (nameNotes());

    CHECK (question.ok);
    CHECK_EQ (question.number, 1);
    CHECK (! question.prompt.empty());
    CHECK_EQ (static_cast<int> (question.choices.size()), 4);
    CHECK (quiz.running());
}

TEST ("the same seed asks the same questions")
{
    FretboardQuiz first, second;
    const auto a = first.start (nameNotes (42));
    const auto b = second.start (nameNotes (42));

    CHECK (a.position == b.position);
    CHECK (a.choices == b.choices);

    // And a different one does not, or the seed is doing nothing.
    FretboardQuiz third;
    auto differed = false;

    for (auto seed = 1u; seed <= 8u && ! differed; ++seed)
    {
        FretboardQuiz other;
        differed = ! (other.start (nameNotes (seed)).position == a.position);
    }

    CHECK (differed);
}

TEST ("the right answer is marked right and counted")
{
    FretboardQuiz quiz;
    const auto question = quiz.start (nameNotes());
    const auto verdict = quiz.answer (rightAnswer (question));

    CHECK (verdict.ok);
    CHECK (verdict.correct);
    CHECK_EQ (verdict.asked, 1);
    CHECK_EQ (verdict.right, 1);
    CHECK (! verdict.detail.empty());
}

TEST ("a wrong answer says what the right one was, and where it is")
{
    FretboardQuiz quiz;
    const auto question = quiz.start (nameNotes());
    const auto verdict = quiz.answer ("H");

    CHECK (! verdict.correct);
    CHECK_EQ (verdict.right, 0);
    CHECK (! verdict.correctAnswer.empty());
    CHECK (! verdict.detail.empty());
}

TEST ("either spelling of a black note is accepted")
{
    QuizOptions options = nameNotes();
    options.strings = { 0 };
    options.fromFret = 1;
    options.toFret = 1;
    options.toFret = 2;

    FretboardQuiz quiz;
    auto question = quiz.start (options);

    // Whichever of the two frets came up, the answer is given both ways.
    const Fretboard board { tuningFor ("standard").value() };
    const auto pitch = toPitchClass (board.noteAt (question.position));

    CHECK (quiz.answer (pitchClassName (pitch, Accidental::sharps)).correct);

    question = quiz.nextQuestion();
    const auto second = toPitchClass (board.noteAt (question.position));
    CHECK (quiz.answer (pitchClassName (second, Accidental::flats)).correct);
}

TEST ("the answer to a multiple choice is one of the choices offered")
{
    FretboardQuiz quiz;
    const auto question = quiz.start (nameNotes (11));
    const auto answer = rightAnswer (question);

    auto offered = false;

    for (const auto& choice : question.choices)
        if (choice.find (answer) != std::string::npos)
            offered = true;

    CHECK (offered);
}

TEST ("the choices are four different notes")
{
    FretboardQuiz quiz;
    auto question = quiz.start (nameNotes (3));

    for (auto i = 0; i < 6; ++i)
    {
        auto sorted = question.choices;
        std::sort (sorted.begin(), sorted.end());
        CHECK (std::unique (sorted.begin(), sorted.end()) == sorted.end());

        quiz.answer (rightAnswer (question));
        question = quiz.nextQuestion();
    }
}

TEST ("a find-the-note question accepts any place that note is")
{
    QuizOptions options;
    options.kind = "find-note";
    options.strings = { 0 };
    options.fromFret = 0;
    options.toFret = 12;
    options.seed = 5;

    FretboardQuiz quiz;
    const auto question = quiz.start (options);
    CHECK_EQ (question.answerKind, std::string ("position"));

    const Fretboard board { tuningFor ("standard").value() };
    const auto pitch = toPitchClass (board.noteAt (question.position));

    // The same note an octave up the same string is a different fret and an
    // equally correct answer, and a quiz that said otherwise would be wrong.
    auto elsewhere = question.position;
    elsewhere.fret += 12;

    if (elsewhere.fret <= 12)
    {
        const auto verdict = quiz.answer (std::to_string (elsewhere.string) + "," + std::to_string (elsewhere.fret));
        CHECK (verdict.correct);
        CHECK_EQ (toPitchClass (board.noteAt (elsewhere)), pitch);
    }
}

TEST ("landing on the right note in the wrong place is told apart from a wrong note")
{
    QuizOptions options;
    options.kind = "find-note";
    options.strings = { 0 };
    options.fromFret = 0;
    options.toFret = 5;
    options.seed = 9;

    FretboardQuiz quiz;
    const auto question = quiz.start (options);

    const Fretboard board { tuningFor ("standard").value() };
    const auto pitch = toPitchClass (board.noteAt (question.position));
    const auto elsewhere = board.positionsFor (pitch, 6, 12);

    if (! elsewhere.empty())
    {
        const auto verdict = quiz.answer (std::to_string (elsewhere.front().string) + ","
                                        + std::to_string (elsewhere.front().fret));
        CHECK (! verdict.correct);
        CHECK (verdict.detail.find ("Right note") != std::string::npos);
    }
}

TEST ("an octave question wants a different octave, not another string")
{
    QuizOptions options;
    options.kind = "octaves";
    options.fromFret = 0;
    options.toFret = 12;
    options.seed = 4;

    FretboardQuiz quiz;
    const auto question = quiz.start (options);
    const Fretboard board { tuningFor ("standard").value() };

    const auto verdict = quiz.answer (rightAnswer (question));
    CHECK (verdict.correct);

    for (const auto& position : verdict.correctPositions)
    {
        CHECK_EQ (toPitchClass (board.noteAt (position)), toPitchClass (board.noteAt (question.position)));
        CHECK (board.noteAt (position) != board.noteAt (question.position));
    }
}

TEST ("a chord-tone question asks about a note that is in the chord")
{
    QuizOptions options;
    options.kind = "chord-tone";
    options.chordSymbol = "Am7";
    options.fromFret = 0;
    options.toFret = 12;
    options.seed = 6;

    FretboardQuiz quiz;
    auto question = quiz.start (options);
    const Fretboard board { tuningFor ("standard").value() };
    const auto chord = parseChord ("Am7").value();

    for (auto i = 0; i < 5; ++i)
    {
        CHECK (question.ok);
        CHECK (chord.degreeOf (toPitchClass (board.noteAt (question.position))).has_value());
        CHECK_EQ (static_cast<int> (question.choices.size()), 4);

        const auto degree = chord.degreeOf (toPitchClass (board.noteAt (question.position))).value();
        CHECK (quiz.answer (degree).correct);
        question = quiz.nextQuestion();
    }
}

TEST ("an answer with no question is an error, not a wrong answer")
{
    FretboardQuiz quiz;
    const auto verdict = quiz.answer ("C");

    CHECK (! verdict.ok);
    CHECK (! verdict.correct);
    CHECK (! verdict.error.empty());
}

TEST ("a quiz that cannot be set says so")
{
    QuizOptions options = nameNotes();
    options.tuningKey = "sitar";
    CHECK (! FretboardQuiz().start (options).ok);

    QuizOptions unknown = nameNotes();
    unknown.kind = "guess-the-guitarist";
    CHECK (! FretboardQuiz().start (unknown).ok);

    QuizOptions tiny = nameNotes();
    tiny.strings = { 0 };
    tiny.fromFret = 3;
    tiny.toFret = 3;
    CHECK (! FretboardQuiz().start (tiny).ok);

    QuizOptions badChord = nameNotes();
    badChord.kind = "chord-tone";
    badChord.chordSymbol = "H7";
    CHECK (! FretboardQuiz().start (badChord).ok);
}

TEST ("the same place is never asked about twice running")
{
    FretboardQuiz quiz;
    auto question = quiz.start (nameNotes (21));
    auto previous = question.position;

    for (auto i = 0; i < 20; ++i)
    {
        quiz.answer (rightAnswer (question));
        question = quiz.nextQuestion();
        CHECK (! (question.position == previous));
        previous = question.position;
    }
}

TEST ("asking for the next question before answering gives back the one outstanding")
{
    FretboardQuiz quiz;
    const auto first = quiz.start (nameNotes (2));
    const auto again = quiz.nextQuestion();

    CHECK (again.position == first.position);
    CHECK_EQ (again.number, first.number);
}

TEST ("a run is summed up in words as well as numbers")
{
    FretboardQuiz quiz;
    auto question = quiz.start (nameNotes (13));

    for (auto i = 0; i < 10; ++i)
    {
        quiz.answer (rightAnswer (question), 1200);
        question = quiz.nextQuestion();
    }

    const auto summary = quiz.finish();

    CHECK_EQ (summary.asked, 10);
    CHECK_EQ (summary.right, 10);
    CHECK_EQ (summary.accuracy, 100);
    CHECK (summary.headline.find ("clean") != std::string::npos);
    CHECK (! summary.observations.empty());
    CHECK (! quiz.running());
}

TEST ("what the shell said about time is reported; the engine measured nothing")
{
    FretboardQuiz quiz;
    auto question = quiz.start (nameNotes (14));

    for (auto i = 0; i < 5; ++i)
    {
        quiz.answer (rightAnswer (question), 6000);
        question = quiz.nextQuestion();
    }

    const auto summary = quiz.finish();
    CHECK_EQ (summary.medianMs, 6000);

    auto mentioned = false;

    for (const auto& observation : summary.observations)
        if (observation.find ("seconds") != std::string::npos)
            mentioned = true;

    CHECK (mentioned);
}

TEST ("a shell that says nothing about time gets a summary without one")
{
    FretboardQuiz quiz;
    auto question = quiz.start (nameNotes (15));

    for (auto i = 0; i < 4; ++i)
    {
        quiz.answer (rightAnswer (question));
        question = quiz.nextQuestion();
    }

    CHECK_EQ (quiz.finish().medianMs, 0);
}

TEST ("a weak string is named in the summary")
{
    QuizOptions options = nameNotes (17);
    options.strings = { 0, 1 };
    options.toFret = 7;

    FretboardQuiz quiz;
    auto question = quiz.start (options);

    // Everything on the sixth string wrong, everything on the fifth right.
    for (auto i = 0; i < 16; ++i)
    {
        quiz.answer (question.position.string == 0 ? std::string ("H") : rightAnswer (question));
        question = quiz.nextQuestion();
    }

    const auto summary = quiz.finish();
    auto named = false;

    for (const auto& observation : summary.observations)
        if (observation.find ("6th string") != std::string::npos)
            named = true;

    CHECK (named);
    CHECK_EQ (static_cast<int> (summary.perString.size()), 2);
}

TEST ("a place that caught someone out comes back")
{
    // The quiz is not a uniform sample of the neck: the point is to find the
    // four frets somebody does not know and keep returning to them.
    QuizOptions options = nameNotes (23);
    options.strings = { 2 };
    options.toFret = 6;

    FretboardQuiz quiz;
    auto question = quiz.start (options);
    const auto missed = question.position;

    quiz.answer ("H");   // wrong

    auto seenAgain = 0;

    for (auto i = 0; i < 12; ++i)
    {
        question = quiz.nextQuestion();

        if (question.position == missed)
            ++seenAgain;

        quiz.answer (rightAnswer (question));
    }

    CHECK (seenAgain >= 1);
}

//==============================================================================
// What one run knows about the ones before it.

namespace
{
    /** Runs a quiz of @p count questions, getting everything right, and hands
        back what the shell would store. */
    std::string runQuiz (const std::string& progressText, int day, unsigned seed, int count,
                         bool answerRight = true)
    {
        QuizOptions options = nameNotes (seed);
        options.progressText = progressText;
        options.today = day;

        FretboardQuiz quiz;
        auto question = quiz.start (options);

        for (auto i = 0; i < count; ++i)
        {
            quiz.answer (answerRight ? rightAnswer (question) : std::string ("H"));
            question = quiz.nextQuestion();
        }

        return quiz.finish().progressText;
    }
}

TEST ("what one run wrote down, the next one reads")
{
    const auto first = runQuiz ({}, 500, 31, 6);
    CHECK (first.find ("guitar-progress") == 0);

    QuizOptions options = nameNotes (32);
    options.progressText = first;
    options.today = 501;

    FretboardQuiz quiz;
    auto question = quiz.start (options);
    quiz.answer (rightAnswer (question));

    const auto summary = quiz.finish();

    CHECK_EQ (summary.sessions, 2);
    CHECK_EQ (summary.lifetimeAsked, 7);
    CHECK_EQ (summary.daysSinceLastSession, 1);
}

TEST ("a quiz that was opened and walked away from is not a session")
{
    const auto first = runQuiz ({}, 500, 33, 4);

    QuizOptions options = nameNotes (34);
    options.progressText = first;
    options.today = 501;

    FretboardQuiz quiz;
    quiz.start (options);
    const auto summary = quiz.finish();   // no answers at all

    CHECK_EQ (summary.asked, 0);
    CHECK_EQ (summary.sessions, 1);
    CHECK_EQ (summary.lifetimeAsked, 4);

    // And what comes back is still the history, so a shell that stores it
    // unconditionally does not wipe one by opening a quiz.
    CHECK (summary.progressText.find ("guitar-progress") == 0);
}

TEST ("a place missed a fortnight ago comes back")
{
    // The whole reason for keeping a record between sessions. Built by hand
    // rather than by running a quiz, so the test says what it means.
    Progress history;

    for (auto i = 0; i < 3; ++i)
        history.record ("standard", { 3, 4 }, false, 480);

    for (auto string = 0; string < 6; ++string)
        for (auto fret = 0; fret <= 5; ++fret)
            if (! (string == 3 && fret == 4))
                for (auto i = 0; i < 3; ++i)
                    history.record ("standard", { string, fret }, true, 480);

    QuizOptions options = nameNotes (41);
    options.progressText = history.toText();
    options.today = 494;

    FretboardQuiz quiz;
    auto question = quiz.start (options);
    auto sawTheWeakOne = 0;

    for (auto i = 0; i < 24; ++i)
    {
        if (question.position == Position { 3, 4 })
            ++sawTheWeakOne;

        quiz.answer (rightAnswer (question));
        question = quiz.nextQuestion();
    }

    // One place out of 36, so chance would give it to us well under twice.
    CHECK (sawTheWeakOne >= 2);
}

TEST ("a history of another tuning says nothing about this one")
{
    Progress history;

    for (auto i = 0; i < 5; ++i)
        history.record ("drop-d", { 0, 3 }, false, 100);

    QuizOptions options = nameNotes (43);
    options.progressText = history.toText();
    options.today = 100;

    FretboardQuiz quiz;
    auto question = quiz.start (options);
    quiz.answer (rightAnswer (question));

    const auto summary = quiz.finish();

    // The drop-D record is still carried - it is one learner's history, not one
    // tuning's - but nothing in it is a weakness of the neck being played.
    CHECK_EQ (summary.lifetimeAsked, 6);

    for (const auto& spot : summary.weakSpots)
        CHECK (! (spot.string == 0 && spot.fret == 3 && spot.strength == 0));
}

TEST ("the summary says what today was against every day before it")
{
    auto history = runQuiz ({}, 400, 51, 12, false);   // a bad first session
    history = runQuiz (history, 401, 52, 12, false);
    history = runQuiz (history, 402, 53, 12, false);

    QuizOptions options = nameNotes (54);
    options.progressText = history;
    options.today = 403;

    FretboardQuiz quiz;
    auto question = quiz.start (options);

    for (auto i = 0; i < 10; ++i)
    {
        quiz.answer (rightAnswer (question));   // a good one, for once
        question = quiz.nextQuestion();
    }

    const auto summary = quiz.finish();
    auto compared = false;
    auto revisited = false;

    for (const auto& observation : summary.observations)
    {
        if (observation.find ("up on the") != std::string::npos)
            compared = true;

        if (observation.find ("missed last time") != std::string::npos)
            revisited = true;
    }

    CHECK (compared);
    CHECK (revisited);
    CHECK (summary.observations.size() <= 4);
}

TEST ("the places still worth going back to come back named")
{
    const auto history = runQuiz ({}, 600, 61, 8, false);

    QuizOptions options = nameNotes (62);
    options.progressText = history;
    options.today = 600;

    FretboardQuiz quiz;
    auto question = quiz.start (options);
    quiz.answer (rightAnswer (question));

    const auto summary = quiz.finish();

    CHECK (! summary.weakSpots.empty());

    for (const auto& spot : summary.weakSpots)
    {
        CHECK (spot.name.find ("string") != std::string::npos);
        CHECK (spot.strength >= 0);
        CHECK (spot.strength <= 100);
    }
}

TEST ("an empty run says so rather than dividing by zero")
{
    FretboardQuiz quiz;
    quiz.start (nameNotes());
    const auto summary = quiz.finish();

    CHECK_EQ (summary.asked, 0);
    CHECK_EQ (summary.accuracy, 0);
    CHECK (! summary.headline.empty());
}
