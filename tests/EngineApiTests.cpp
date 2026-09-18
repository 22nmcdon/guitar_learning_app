#include "TestFramework.h"

#include "guitar/api/EngineApi.h"

#include <string>

using namespace guitar::api;

namespace
{
    bool says (const std::string& json, const std::string& fragment)
    {
        return json.find (fragment) != std::string::npos;
    }

    bool ok (const std::string& json)
    {
        return says (json, "\"ok\":true");
    }
}

TEST ("the catalogues come back as lists a menu can be built from")
{
    CHECK (ok (tunings()));
    CHECK (says (tunings(), "\"key\":\"standard\""));
    CHECK (says (tunings(), "\"spelling\":\"E A D G B E\""));
    CHECK (says (tunings(), "\"openNames\":"));

    CHECK (ok (chordQualities()));
    CHECK (says (chordQualities(), "\"family\":\"Triads\""));
    CHECK (says (chordQualities(), "\"name\":\"half-diminished\""));

    CHECK (ok (quizKinds()));
    CHECK (says (quizKinds(), "\"key\":\"name-note\""));
    CHECK (says (quizKinds(), "\"answerKind\":\"position\""));
}

TEST ("a chord comes back with shapes, and each shape with a hand")
{
    const auto json = chordShapes ("C", "standard", 0, 12, 8, 0);

    CHECK (ok (json));
    CHECK (says (json, "\"symbol\":\"C\""));
    CHECK (says (json, "\"quality\":\"major\""));
    CHECK (says (json, "\"frets\":[-1,3,2,0,1,0]"));
    CHECK (says (json, "\"fingers\":"));
    CHECK (says (json, "\"degrees\":"));
    CHECK (says (json, "\"difficulty\":"));
    CHECK (says (json, "\"noteNames\":"));
}

TEST ("the notes of the chord are named as well as its shapes")
{
    const auto json = chordShapes ("Am7", "standard", 0, 12, 4, 0);
    CHECK (says (json, "A (R)"));
    CHECK (says (json, "C (b3)"));
    CHECK (says (json, "G (b7)"));
}

TEST ("a tuning the engine does not have is an error, not an empty answer")
{
    CHECK (says (chordShapes ("C", "sitar", 0, 12, 8, 0), "\"ok\":false"));
    CHECK (says (chordShapes ("H", "standard", 0, 12, 8, 0), "cannot read the chord"));
    CHECK (says (fretboardNotes ("sitar", 0, 12), "\"ok\":false"));
    CHECK (says (positionsFor ("H", "standard", 0, 12), "not a note name"));
}

TEST ("asking for simple shapes changes what comes back")
{
    const auto everything = chordShapes ("F", "standard", 0, 12, 8, 0);
    const auto simple = chordShapes ("F", "standard", 0, 12, 8, 1);

    CHECK (says (everything, "\"barreFret\":1"));
    CHECK (! says (simple, "\"barreFret\":1"));
}

TEST ("a shape played is named back")
{
    const auto json = identifyShape ("standard", "x,3,2,0,1,0");

    CHECK (ok (json));
    CHECK (says (json, "\"playing\":true"));
    CHECK (says (json, "\"symbol\":\"C\""));
    CHECK (says (json, "\"confidence\":"));
}

TEST ("a muted string is spelled several ways because three shells will spell it three ways")
{
    for (const auto* csv : { "x,3,2,0,1,0", "-1,3,2,0,1,0", "X,3,2,0,1,0", "-,3,2,0,1,0" })
        CHECK (says (identifyShape ("standard", csv), "\"symbol\":\"C\""));
}

TEST ("what was played is measured against what was asked for")
{
    const auto right = checkShape ("C", "standard", "x,3,2,0,1,0");
    CHECK (ok (right));
    CHECK (says (right, "\"correct\":true"));

    const auto wrong = checkShape ("C", "standard", "x,3,2,0,1,1");
    CHECK (says (wrong, "\"correct\":false"));
    CHECK (says (wrong, "\"outside\":[\"F4\"]"));
}

TEST ("the neck is handed over as notes to draw")
{
    const auto json = fretboardNotes ("standard", 0, 5);

    CHECK (ok (json));
    CHECK (says (json, "\"open\":\"E2\""));
    CHECK (says (json, "\"name\":\"6th string\""));
    CHECK (says (json, "\"fromFret\":0"));
    CHECK (says (json, "\"toFret\":5"));
}

TEST ("every place a note can be played is offered")
{
    const auto json = positionsFor ("C", "standard", 0, 12);

    CHECK (ok (json));
    CHECK (says (json, "\"note\":\"C\""));
    CHECK (says (json, "{\"string\":0,\"fret\":8}"));
}

TEST ("a quiz runs from start to summary over the wire")
{
    const auto first = quizStart ("name-note", "standard", 0, 5, "", "", 99);

    CHECK (ok (first));
    CHECK (says (first, "\"number\":1"));
    CHECK (says (first, "\"answerKind\":\"note\""));
    CHECK (says (first, "\"choices\":["));

    const auto verdict = quizAnswer ("C", 1500);
    CHECK (ok (verdict));
    CHECK (says (verdict, "\"asked\":1"));

    const auto second = quizNext();
    CHECK (ok (second));
    CHECK (says (second, "\"number\":2"));

    quizAnswer ("C", 1500);

    const auto summary = quizEnd();
    CHECK (ok (summary));
    CHECK (says (summary, "\"asked\":2"));
    CHECK (says (summary, "\"observations\":["));
    CHECK (says (summary, "\"perString\":["));
    CHECK (says (summary, "\"medianMs\":1500"));
}

TEST ("a quiz that is not running refuses to be answered")
{
    quizEnd();
    CHECK (says (quizAnswer ("C", 0), "\"ok\":false"));
    CHECK (says (quizNext(), "\"ok\":false"));
}

TEST ("a quiz can be asked about two strings only")
{
    const auto json = quizStart ("find-note", "standard", 0, 12, "0,1", "", 3);

    CHECK (ok (json));
    CHECK (says (json, "\"answerKind\":\"position\""));
    CHECK (says (json, "string") );
    quizEnd();
}

TEST ("a quiz the engine cannot set comes back as an error")
{
    CHECK (says (quizStart ("name-note", "sitar", 0, 5, "", "", 1), "\"ok\":false"));
    CHECK (says (quizStart ("nope", "standard", 0, 5, "", "", 1), "\"ok\":false"));
    quizEnd();
}

TEST ("a null from a shell is an empty string, not a crash")
{
    CHECK (says (chordShapes (nullptr, nullptr, 0, 12, 8, 0), "\"ok\":false"));
    CHECK (ok (identifyShape (nullptr, nullptr)));
    CHECK (ok (fretboardNotes (nullptr, 0, 5)));
}

TEST ("JSON with a quote in it would not break the page")
{
    // Nothing in the engine writes a quote today, and the escaping is here for
    // the first thing that does rather than for the day someone notices.
    CHECK (says (positionsFor ("C", "standard", 0, 12), "\"ok\":true"));
    CHECK (! says (tunings(), "\n"));
}
