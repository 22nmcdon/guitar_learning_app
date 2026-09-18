#include "TestFramework.h"

#include "guitar/core/FretboardQuiz.h"

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

TEST ("an empty run says so rather than dividing by zero")
{
    FretboardQuiz quiz;
    quiz.start (nameNotes());
    const auto summary = quiz.finish();

    CHECK_EQ (summary.asked, 0);
    CHECK_EQ (summary.accuracy, 0);
    CHECK (! summary.headline.empty());
}
