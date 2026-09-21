// The engine's answers, as JSON.
//
// Both shells ask the engine the same questions and both want the answer as
// text: the browser because JavaScript cannot see C++ types, and the JUCE app
// because its UI is that same page in a webview. Encoding lives here rather
// than in either shell, because two copies of a wire format is how two shells
// start disagreeing about what a chord looks like.
//
// This holds no theory. It asks the Core Engine and writes down what it said.

#include "guitar/api/EngineApi.h"

#include "guitar/core/ChordShapes.h"
#include "guitar/core/ChordSymbol.h"
#include "guitar/core/Fretboard.h"
#include "guitar/core/FretboardQuiz.h"
#include "guitar/core/Progress.h"
#include "guitar/core/ShapeIdentifier.h"
#include "guitar/core/Tuning.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

using namespace guitar::core;
namespace core = guitar::core;

namespace
{
    //==========================================================================
    // Minimal JSON writing. The engine has no serialisation of its own and
    // should not grow any: how results are encoded is the shell's business.

    std::string quoted (const std::string& text)
    {
        std::string out = "\"";

        for (auto c : text)
        {
            switch (c)
            {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    if (static_cast<unsigned char> (c) < 0x20)
                        out += ' ';
                    else
                        out += c;
            }
        }

        return out + "\"";
    }

    template <typename Item, typename Fn>
    std::string jsonArray (const std::vector<Item>& items, Fn&& toJson)
    {
        std::string out = "[";

        for (std::size_t i = 0; i < items.size(); ++i)
        {
            if (i > 0)
                out += ",";

            out += toJson (items[i]);
        }

        return out + "]";
    }

    std::string jsonNumbers (const std::vector<int>& values)
    {
        return jsonArray (values, [] (int value) { return std::to_string (value); });
    }

    std::string jsonStrings (const std::vector<std::string>& values)
    {
        return jsonArray (values, [] (const std::string& value) { return quoted (value); });
    }

    std::string jsonError (const std::string& message)
    {
        return "{\"ok\":false,\"error\":" + quoted (message) + "}";
    }

    std::string jsonPosition (const Position& position)
    {
        return "{\"string\":" + std::to_string (position.string)
             + ",\"fret\":" + std::to_string (position.fret) + "}";
    }

    std::string jsonPositions (const std::vector<Position>& positions)
    {
        return jsonArray (positions, jsonPosition);
    }

    /** "x,3,2,0,1,0" - one entry per string, lowest first.

        Accepts "x", "X", "-" and "-1" for a muted string, because three
        different shells will spell it three different ways and the one that
        matters is that a muted string is spelled at all.
    */
    std::vector<int> parseFrets (const std::string& csv)
    {
        std::vector<int> frets;
        std::stringstream stream (csv);
        std::string field;

        while (std::getline (stream, field, ','))
        {
            while (! field.empty() && std::isspace (static_cast<unsigned char> (field.front())))
                field.erase (field.begin());

            while (! field.empty() && std::isspace (static_cast<unsigned char> (field.back())))
                field.pop_back();

            if (field.empty() || field == "x" || field == "X" || field == "-")
            {
                frets.push_back (muted);
                continue;
            }

            try
            {
                frets.push_back (std::stoi (field));
            }
            catch (...)
            {
                frets.push_back (muted);
            }
        }

        return frets;
    }

    std::vector<int> parseNumbers (const std::string& csv)
    {
        std::vector<int> numbers;
        std::stringstream stream (csv);
        std::string field;

        while (std::getline (stream, field, ','))
        {
            try
            {
                numbers.push_back (std::stoi (field));
            }
            catch (...) { /* a field that is not a number is not a string index */ }
        }

        return numbers;
    }

    std::string text (const char* value)
    {
        return value != nullptr ? std::string (value) : std::string();
    }

    /** The one quiz this process has. See the header for why it is here and not
        in the shell. */
    core::FretboardQuiz& quiz()
    {
        static core::FretboardQuiz theQuiz;
        return theQuiz;
    }

    std::string jsonQuestion (const QuizQuestion& question)
    {
        if (! question.ok)
            return jsonError (question.error.empty() ? "the quiz could not set a question" : question.error);

        return "{\"ok\":true,\"number\":" + std::to_string (question.number)
             + ",\"kind\":" + quoted (question.kind)
             + ",\"prompt\":" + quoted (question.prompt)
             + ",\"detail\":" + quoted (question.detail)
             + ",\"answerKind\":" + quoted (question.answerKind)
             + ",\"position\":" + jsonPosition (question.position)
             + ",\"choices\":" + jsonStrings (question.choices)
             + ",\"highlight\":" + jsonPositions (question.highlight)
             + "}";
    }
}

namespace guitar::api
{

//==============================================================================
std::string tunings()
{
    return "{\"ok\":true,\"tunings\":"
         + jsonArray (core::tunings(), [] (const Tuning& tuning)
           {
               const Fretboard board { tuning };
               std::vector<std::string> names;

               for (auto string = 0; string < board.stringCount(); ++string)
                   names.push_back (pitchClassName (tuning.openNotes[static_cast<std::size_t> (string)],
                                                    tuning.spelling.find ('b') != std::string::npos
                                                        ? Accidental::flats : Accidental::sharps));

               return "{\"key\":" + quoted (tuning.key)
                    + ",\"name\":" + quoted (tuning.name)
                    + ",\"spelling\":" + quoted (tuning.spelling)
                    + ",\"summary\":" + quoted (tuning.summary)
                    + ",\"instrument\":" + quoted (tuning.instrument)
                    + ",\"strings\":" + std::to_string (tuning.openNotes.size())
                    + ",\"openNotes\":" + jsonNumbers (tuning.openNotes)
                    + ",\"openNames\":" + jsonStrings (names) + "}";
           })
         + "}";
}

std::string chordQualities()
{
    return "{\"ok\":true,\"qualities\":"
         + jsonArray (core::chordQualities(), [] (const ChordQuality& quality)
           {
               return "{\"suffix\":" + quoted (quality.suffix)
                    + ",\"name\":" + quoted (quality.name)
                    + ",\"family\":" + quoted (quality.family)
                    + ",\"summary\":" + quoted (quality.summary)
                    + ",\"intervals\":" + jsonNumbers (quality.intervals) + "}";
           })
         + "}";
}

std::string rootNames()
{
    std::string roots = "[";

    for (PitchClass pitchClass = 0; pitchClass < semitonesPerOctave; ++pitchClass)
    {
        if (pitchClass > 0)
            roots += ",";

        roots += "{\"pitchClass\":" + std::to_string (pitchClass)
               + ",\"sharp\":" + quoted (pitchClassName (pitchClass, Accidental::sharps))
               + ",\"flat\":" + quoted (pitchClassName (pitchClass, Accidental::flats))
               + ",\"major\":" + quoted (preferredRootName (pitchClass, false))
               + ",\"minor\":" + quoted (preferredRootName (pitchClass, true)) + "}";
    }

    return "{\"ok\":true,\"roots\":" + roots + "]}";
}

std::string quizKinds()
{
    return "{\"ok\":true,\"kinds\":"
         + jsonArray (core::quizKinds(), [] (const QuizKindInfo& kind)
           {
               return "{\"key\":" + quoted (kind.key)
                    + ",\"name\":" + quoted (kind.name)
                    + ",\"summary\":" + quoted (kind.summary)
                    + ",\"answerKind\":" + quoted (kind.answerKind)
                    + ",\"needsChord\":" + (kind.needsChord ? "true" : "false") + "}";
           })
         + "}";
}

//==============================================================================
std::string chordShapes (const char* symbol, const char* tuningKey,
                         int fromFret, int toFret, int maxShapes, int simpleOnly, int capo)
{
    auto chord = parseChord (text (symbol));

    if (! chord.has_value())
        return jsonError ("cannot read the chord '" + text (symbol) + "'");

    auto tuning = tuningFor (text (tuningKey));

    if (! tuning.has_value())
        return jsonError ("no tuning called '" + text (tuningKey) + "'");

    ShapeSearch search;
    search.fromFret = fromFret;
    search.toFret = toFret > 0 ? toFret : 12;
    search.maxShapes = maxShapes > 0 ? maxShapes : 8;
    search.simpleOnly = simpleOnly != 0;
    search.capo = capo;

    const auto shapes = core::chordShapes (*chord, *tuning, search);
    const Fretboard board { *tuning };

    std::vector<std::string> chordNotes;

    for (auto interval : chord->quality.intervals)
        chordNotes.push_back (chord->spelledNote (interval) + " ("
                              + intervalName (interval, chord->quality.minorThird) + ")");

    return "{\"ok\":true,\"symbol\":" + quoted (chord->symbol)
         + ",\"quality\":" + quoted (chord->quality.name)
         + ",\"summary\":" + quoted (chord->quality.summary)
         + ",\"notes\":" + jsonStrings (chordNotes)
         + ",\"shapes\":" + jsonArray (shapes, [&board, &chord] (const ChordShape& shape)
           {
               std::vector<std::string> noteNames;

               for (std::size_t string = 0; string < shape.frets.size(); ++string)
                   noteNames.push_back (shape.frets[string] == muted
                                            ? std::string()
                                            : chord->spelledMidiNote (board.noteAt (static_cast<int> (string),
                                                                                    shape.frets[string])));

               return "{\"frets\":" + jsonNumbers (shape.frets)
                    + ",\"fingers\":" + jsonNumbers (shape.fingers)
                    + ",\"degrees\":" + jsonStrings (shape.degrees)
                    + ",\"noteNames\":" + jsonStrings (noteNames)
                    + ",\"name\":" + quoted (shape.name)
                    + ",\"difficulty\":" + quoted (shape.difficulty)
                    + ",\"note\":" + quoted (shape.note)
                    + ",\"bassNote\":" + quoted (shape.bassNote)
                    + ",\"omitted\":" + jsonStrings (shape.omitted)
                    + ",\"capo\":" + std::to_string (shape.capo)
                    + ",\"baseFret\":" + std::to_string (shape.baseFret)
                    + ",\"barreFret\":" + std::to_string (shape.barreFret)
                    + ",\"barreFrom\":" + std::to_string (shape.barreFromString)
                    + ",\"barreTo\":" + std::to_string (shape.barreToString)
                    + ",\"span\":" + std::to_string (shape.span)
                    + ",\"fingersUsed\":" + std::to_string (shape.fingersUsed)
                    + ",\"strings\":" + std::to_string (shape.soundedStrings)
                    + ",\"rootInBass\":" + (shape.rootInBass ? "true" : "false")
                    + ",\"open\":" + (shape.hasOpenStrings ? "true" : "false") + "}";
           })
         + "}";
}

std::string identifyShape (const char* tuningKey, const char* fretsCsv)
{
    auto tuning = tuningFor (text (tuningKey));

    if (! tuning.has_value())
        return jsonError ("no tuning called '" + text (tuningKey) + "'");

    const auto reading = core::identifyShape (*tuning, parseFrets (text (fretsCsv)));

    return "{\"ok\":true,\"playing\":" + std::string (reading.anythingPlayed ? "true" : "false")
         + ",\"notes\":" + jsonNumbers (reading.notes)
         + ",\"noteNames\":" + jsonStrings (reading.noteNames)
         + ",\"summary\":" + quoted (reading.summary)
         + ",\"namings\":" + jsonArray (reading.namings, [] (const ChordNaming& naming)
           {
               return "{\"symbol\":" + quoted (naming.symbol)
                    + ",\"quality\":" + quoted (naming.qualityName)
                    + ",\"confidence\":" + std::to_string (naming.confidence)
                    + ",\"verdict\":" + quoted (naming.verdict)
                    + ",\"missing\":" + jsonStrings (naming.missing)
                    + ",\"rootInBass\":" + (naming.rootInBass ? "true" : "false") + "}";
           })
         + "}";
}

std::string checkShape (const char* symbol, const char* tuningKey, const char* fretsCsv)
{
    auto chord = parseChord (text (symbol));

    if (! chord.has_value())
        return jsonError ("cannot read the chord '" + text (symbol) + "'");

    auto tuning = tuningFor (text (tuningKey));

    if (! tuning.has_value())
        return jsonError ("no tuning called '" + text (tuningKey) + "'");

    const auto verdict = core::checkShape (*chord, *tuning, parseFrets (text (fretsCsv)));

    return "{\"ok\":true,\"correct\":" + std::string (verdict.correct ? "true" : "false")
         + ",\"verdict\":" + quoted (verdict.verdict)
         + ",\"detail\":" + quoted (verdict.detail)
         + ",\"missing\":" + jsonStrings (verdict.missing)
         + ",\"outside\":" + jsonStrings (verdict.outside)
         + ",\"degrees\":" + jsonStrings (verdict.degrees)
         + ",\"symbol\":" + quoted (chord->symbol) + "}";
}

//==============================================================================
std::string fretboardNotes (const char* tuningKey, int fromFret, int toFret)
{
    auto tuning = tuningFor (text (tuningKey));

    if (! tuning.has_value())
        return jsonError ("no tuning called '" + text (tuningKey) + "'");

    // A neck has no chord and no key, so nothing decides between A sharp and B
    // flat except the tuning it is in: somebody tuned to E flat reads their
    // strings in flats, and the rest of us read sharps.
    const auto accidental = tuning->spelling.find ('b') != std::string::npos
                                ? Accidental::flats : Accidental::sharps;

    const Fretboard board { *tuning };
    const auto low = std::max (0, fromFret);
    const auto high = std::min (toFret > 0 ? toFret : 12, highestFret);

    std::string strings = "[";

    for (auto string = 0; string < board.stringCount(); ++string)
    {
        if (string > 0)
            strings += ",";

        std::vector<std::string> names;
        std::vector<int> notes;

        for (auto fret = low; fret <= high; ++fret)
        {
            notes.push_back (board.noteAt (string, fret));
            names.push_back (pitchClassName (board.noteAt (string, fret), accidental));
        }

        strings += "{\"string\":" + std::to_string (string)
                 + ",\"name\":" + quoted (board.stringName (string))
                 + ",\"open\":" + quoted (midiNoteName (board.noteAt (string, 0), accidental))
                 + ",\"notes\":" + jsonNumbers (notes)
                 + ",\"names\":" + jsonStrings (names) + "}";
    }

    strings += "]";

    return "{\"ok\":true,\"tuning\":" + quoted (tuning->name)
         + ",\"fromFret\":" + std::to_string (low)
         + ",\"toFret\":" + std::to_string (high)
         + ",\"strings\":" + strings + "}";
}

std::string positionsFor (const char* noteName, const char* tuningKey, int fromFret, int toFret)
{
    auto tuning = tuningFor (text (tuningKey));

    if (! tuning.has_value())
        return jsonError ("no tuning called '" + text (tuningKey) + "'");

    auto pitchClass = parsePitchClass (text (noteName));

    if (! pitchClass.has_value())
        return jsonError ("'" + text (noteName) + "' is not a note name");

    const Fretboard board { *tuning };
    const auto found = board.positionsFor (*pitchClass, std::max (0, fromFret),
                                           std::min (toFret > 0 ? toFret : 12, highestFret));

    return "{\"ok\":true,\"note\":" + quoted (enharmonicName (*pitchClass))
         + ",\"positions\":" + jsonPositions (found) + "}";
}

//==============================================================================
std::string quizStart (const char* kind, const char* tuningKey, int fromFret, int toFret,
                       const char* stringsCsv, const char* chordSymbol, int seed,
                       const char* progressText, int today)
{
    QuizOptions options;
    options.kind = text (kind).empty() ? "name-note" : text (kind);
    options.tuningKey = text (tuningKey);
    options.fromFret = fromFret;
    options.toFret = toFret > 0 ? toFret : 12;
    options.strings = parseNumbers (text (stringsCsv));
    options.chordSymbol = text (chordSymbol);
    options.seed = static_cast<unsigned> (seed);
    options.progressText = text (progressText);
    options.today = today;

    return jsonQuestion (quiz().start (options));
}

std::string quizAnswer (const char* given, int elapsedMs)
{
    const auto verdict = quiz().answer (text (given), elapsedMs);

    if (! verdict.ok)
        return jsonError (verdict.error);

    return "{\"ok\":true,\"correct\":" + std::string (verdict.correct ? "true" : "false")
         + ",\"verdict\":" + quoted (verdict.verdict)
         + ",\"detail\":" + quoted (verdict.detail)
         + ",\"answer\":" + quoted (verdict.correctAnswer)
         + ",\"positions\":" + jsonPositions (verdict.correctPositions)
         + ",\"asked\":" + std::to_string (verdict.asked)
         + ",\"right\":" + std::to_string (verdict.right) + "}";
}

std::string quizNext()
{
    return jsonQuestion (quiz().nextQuestion());
}

std::string quizEnd()
{
    const auto summary = quiz().finish();

    return "{\"ok\":true,\"asked\":" + std::to_string (summary.asked)
         + ",\"right\":" + std::to_string (summary.right)
         + ",\"accuracy\":" + std::to_string (summary.accuracy)
         + ",\"headline\":" + quoted (summary.headline)
         + ",\"medianMs\":" + std::to_string (summary.medianMs)
         + ",\"observations\":" + jsonStrings (summary.observations)
         // What the shell stores. It is opaque to the shell on purpose: the
         // format is the engine's, and a page that parsed it would be a second
         // reader to keep in step with the first.
         + ",\"progress\":" + quoted (summary.progressText)
         + ",\"sessions\":" + std::to_string (summary.sessions)
         + ",\"lifetimeAsked\":" + std::to_string (summary.lifetimeAsked)
         + ",\"lifetimeRight\":" + std::to_string (summary.lifetimeRight)
         + ",\"daysSince\":" + std::to_string (summary.daysSinceLastSession)
         + ",\"weakSpots\":" + jsonArray (summary.weakSpots, [] (const WeakSpot& spot)
           {
               return "{\"string\":" + std::to_string (spot.string)
                    + ",\"fret\":" + std::to_string (spot.fret)
                    + ",\"name\":" + quoted (spot.name)
                    + ",\"strength\":" + std::to_string (spot.strength) + "}";
           })
         + ",\"perString\":" + jsonArray (summary.perString, [] (const StringTally& tally)
           {
               return "{\"string\":" + std::to_string (tally.string)
                    + ",\"name\":" + quoted (tally.name)
                    + ",\"asked\":" + std::to_string (tally.asked)
                    + ",\"right\":" + std::to_string (tally.right) + "}";
           })
         + "}";
}

//==============================================================================
std::string progressMap (const char* progressText, const char* tuningKey,
                         int fromFret, int toFret, int today)
{
    auto tuning = tuningFor (text (tuningKey));

    if (! tuning.has_value())
        return jsonError ("no tuning called '" + text (tuningKey) + "'");

    const auto progress = Progress::fromText (text (progressText));
    const Fretboard board { *tuning };
    const auto low = std::max (0, fromFret);
    const auto high = std::min (toFret > 0 ? toFret : 12, highestFret);

    std::string places = "[";

    for (auto string = 0; string < board.stringCount(); ++string)
    {
        for (auto fret = low; fret <= high; ++fret)
        {
            // Keyed by the tuning the engine resolved, not by whatever the
            // shell typed: an empty key means standard, and a record written
            // under "" would be a second history nobody could find again.
            const auto strength = progress.strengthFor (tuning->key, { string, fret }, today);

            // Places never asked about are left out rather than sent as -1:
            // the map is what is known, and on a first visit that is nothing.
            if (strength < 0)
                continue;

            if (places.size() > 1)
                places += ",";

            places += "{\"string\":" + std::to_string (string)
                    + ",\"fret\":" + std::to_string (fret)
                    + ",\"strength\":" + std::to_string (strength) + "}";
        }
    }

    places += "]";

    std::vector<std::string> weakest;

    for (const auto& record : progress.weakest (tuning->key, 4, today))
        weakest.push_back (board.stringName (record.string) + ", "
                           + (record.fret == 0 ? std::string ("open") : "fret " + std::to_string (record.fret)));

    return "{\"ok\":true,\"sessions\":" + std::to_string (progress.sessions())
         + ",\"asked\":" + std::to_string (progress.totalAsked())
         + ",\"right\":" + std::to_string (progress.totalRight())
         + ",\"placesSeen\":" + std::to_string (progress.placesSeen (tuning->key))
         + ",\"placesOnNeck\":" + std::to_string (board.stringCount() * (high - low + 1))
         + ",\"daysSince\":" + std::to_string (progress.sessions() > 0
                                                    ? std::max (0, today - progress.lastDay()) : 0)
         + ",\"weakest\":" + jsonStrings (weakest)
         + ",\"places\":" + places + "}";
}

} // namespace guitar::api
