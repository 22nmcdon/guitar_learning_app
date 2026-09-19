// WebAssembly transport for the engine's JSON API.
//
// This is a platform shell, exactly like app/ is: it owns none of the theory and
// none of the encoding either - guitar::api does that, so the browser and the
// JUCE app hand the same page the same answers. All that is left here is the
// part that is genuinely Emscripten's: C linkage, and a buffer that outlives the
// call so JavaScript can copy the string out.

#include "guitar/api/EngineApi.h"

#include <string>
#include <utility>

#ifdef __EMSCRIPTEN__
 #include <emscripten/emscripten.h>
 #define GUITAR_EXPORT extern "C" EMSCRIPTEN_KEEPALIVE
#else
 #define GUITAR_EXPORT extern "C"
#endif

namespace
{
    /** Each entry point owns one buffer; JavaScript copies the string out before
        the next call, which is how Emscripten's UTF8ToString works anyway. */
    const char* hold (std::string&& json)
    {
        static std::string buffer;
        buffer = std::move (json);
        return buffer.c_str();
    }

    /** The engine's calls take strings; a null from JavaScript is an empty one. */
    const char* orEmpty (const char* text)
    {
        return text != nullptr ? text : "";
    }
}

GUITAR_EXPORT const char* guitarTunings()
{
    return hold (guitar::api::tunings());
}

GUITAR_EXPORT const char* guitarChordQualities()
{
    return hold (guitar::api::chordQualities());
}

GUITAR_EXPORT const char* guitarQuizKinds()
{
    return hold (guitar::api::quizKinds());
}

GUITAR_EXPORT const char* guitarChordShapes (const char* symbol, const char* tuningKey,
                                             int fromFret, int toFret, int maxShapes, int simpleOnly)
{
    return hold (guitar::api::chordShapes (orEmpty (symbol), orEmpty (tuningKey),
                                           fromFret, toFret, maxShapes, simpleOnly));
}

GUITAR_EXPORT const char* guitarIdentifyShape (const char* tuningKey, const char* fretsCsv)
{
    return hold (guitar::api::identifyShape (orEmpty (tuningKey), orEmpty (fretsCsv)));
}

GUITAR_EXPORT const char* guitarCheckShape (const char* symbol, const char* tuningKey, const char* fretsCsv)
{
    return hold (guitar::api::checkShape (orEmpty (symbol), orEmpty (tuningKey), orEmpty (fretsCsv)));
}

GUITAR_EXPORT const char* guitarFretboardNotes (const char* tuningKey, int fromFret, int toFret)
{
    return hold (guitar::api::fretboardNotes (orEmpty (tuningKey), fromFret, toFret));
}

GUITAR_EXPORT const char* guitarPositionsFor (const char* noteName, const char* tuningKey,
                                              int fromFret, int toFret)
{
    return hold (guitar::api::positionsFor (orEmpty (noteName), orEmpty (tuningKey), fromFret, toFret));
}

GUITAR_EXPORT const char* guitarQuizStart (const char* kind, const char* tuningKey,
                                           int fromFret, int toFret, const char* stringsCsv,
                                           const char* chordSymbol, int seed,
                                           const char* progressText, int today)
{
    return hold (guitar::api::quizStart (orEmpty (kind), orEmpty (tuningKey), fromFret, toFret,
                                         orEmpty (stringsCsv), orEmpty (chordSymbol), seed,
                                         orEmpty (progressText), today));
}

GUITAR_EXPORT const char* guitarProgressMap (const char* progressText, const char* tuningKey,
                                             int fromFret, int toFret, int today)
{
    return hold (guitar::api::progressMap (orEmpty (progressText), orEmpty (tuningKey),
                                           fromFret, toFret, today));
}

GUITAR_EXPORT const char* guitarQuizAnswer (const char* given, int elapsedMs)
{
    return hold (guitar::api::quizAnswer (orEmpty (given), elapsedMs));
}

GUITAR_EXPORT const char* guitarQuizNext()
{
    return hold (guitar::api::quizNext());
}

GUITAR_EXPORT const char* guitarQuizEnd()
{
    return hold (guitar::api::quizEnd());
}
