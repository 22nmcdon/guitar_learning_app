#pragma once

#include "guitar/core/ChordSymbol.h"
#include "guitar/core/Fretboard.h"

#include <string>
#include <vector>

namespace guitar::core
{

/** One way of being asked about the fretboard. */
struct QuizKindInfo
{
    std::string key;         ///< "name-note"
    std::string name;        ///< "Name the note"
    std::string summary;     ///< what it drills, in one line
    std::string answerKind;  ///< "note", "position" or "choice" - how the shell collects an answer
    bool needsChord {};      ///< true when the question is about a chord as well as a place
};

/** Every kind of question the engine can ask, in the order to meet them.

    The shells build their menu from this. Which drills exist and what they are
    for is pedagogy, and pedagogy is the engine's - a page with its own list
    would start offering a quiz the engine cannot set.
*/
const std::vector<QuizKindInfo>& quizKinds();

/** What to be asked about. */
struct QuizOptions
{
    std::string kind { "name-note" };
    std::string tuningKey { "standard" };
    int fromFret { 0 };
    int toFret { 12 };

    /** String indices to ask about, lowest first; empty means all of them.

        Learning the neck one string at a time is how it is actually done - the
        sixth and fifth first, because that is where barre chord roots live.
    */
    std::vector<int> strings;

    /** The chord a chord-tone question is about. Empty means the engine picks
        one, and keeps picking new ones, which is the better drill. */
    std::string chordSymbol;

    /** Where the randomness comes from.

        The engine has no clock and no entropy of its own, on purpose: the same
        seed gives the same quiz, which is what makes the tests possible and
        what lets two people take the same one. The shell, which does have a
        clock, decides what the seed is.
    */
    unsigned seed { 1 };
};

struct QuizQuestion
{
    bool ok {};
    std::string error;

    int number {};             ///< 1-based, within this quiz
    std::string kind;
    std::string prompt;        ///< "What note is this?"
    std::string detail;        ///< "6th string, 5th fret"
    std::string answerKind;    ///< "note", "position", "choice"

    /** The place the question is about, or {-1,-1} when it is not about one. */
    Position position { -1, -1 };

    /** Multiple choice answers, in the order to show them. Empty when the
        answer is a place on the neck rather than a word. */
    std::vector<std::string> choices;

    /** Positions the shell should mark on the board while asking. */
    std::vector<Position> highlight;
};

struct QuizVerdict
{
    bool ok {};
    std::string error;

    bool correct {};
    std::string verdict;        ///< "Yes", "Not quite"
    std::string detail;         ///< what the answer was, and something to hang it on
    std::string correctAnswer;

    /** Every place that would have been right - a "find the note" question
        rarely has only one, and showing all of them is the lesson. */
    std::vector<Position> correctPositions;

    int asked {};
    int right {};
};

struct StringTally
{
    int string {};
    std::string name;
    int asked {};
    int right {};
};

struct QuizSummary
{
    bool ok {};
    int asked {};
    int right {};
    int accuracy {};            ///< per cent, rounded

    std::string headline;

    /** What the run showed, in words.

        Deliberately words and not more numbers. "78%" is a fact about a quiz;
        "the fifth string is where the time goes" is a fact about the player,
        and only the second one tells them what to practise tomorrow.
    */
    std::vector<std::string> observations;

    std::vector<StringTally> perString;

    /** The middle answer's time, in milliseconds, out of what the shell
        reported. The engine timed nothing - it cannot - and says so by taking
        this as data like everything else about when. Zero when the shell never
        said. */
    int medianMs {};
};

/** One learner's run through the fretboard.

    Stateful, like a take is in the other kind of practice app: a quiz is a
    stream of questions with a beginning and an end, and the alternative - the
    shell sending back everything answered so far on every answer - puts the
    learner's history in the UI, which is exactly where pedagogy is not allowed
    to live.
*/
class FretboardQuiz
{
public:
    QuizQuestion start (const QuizOptions& options);

    /** The answer to the question outstanding, and how long the shell says it
        took. A quiz that is not running gives back an error rather than a
        verdict - reading a stray click as a wrong answer is how a tally ends up
        lying about someone. */
    QuizVerdict answer (const std::string& given, int elapsedMs = 0);

    QuizQuestion nextQuestion();

    QuizSummary finish();

    bool running() const noexcept { return isRunning; }

private:
    QuizQuestion ask();

    QuizOptions options;
    Tuning tuning;
    bool isRunning {};
    bool awaitingAnswer {};

    unsigned rng { 1 };
    unsigned nextRandom();
    int randomBelow (int limit);

    QuizQuestion current;
    Chord currentChord;
    std::vector<Position> pool;
    std::vector<int> wrongAt;         ///< per pool entry: how often it has caught them out
    int lastAsked { -1 };             ///< pool index, so the same place is not asked twice running

    int asked {};
    int right {};
    std::vector<StringTally> tallies;
    std::vector<int> times;
    int naturalsAsked {}, naturalsRight {};
    int accidentalsAsked {}, accidentalsRight {};
    int lowAsked {}, lowRight {}, highAsked {}, highRight {};
};

} // namespace guitar::core
