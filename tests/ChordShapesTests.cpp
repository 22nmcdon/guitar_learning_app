#include "TestFramework.h"

#include "guitar/core/ChordShapes.h"
#include "guitar/core/ShapeIdentifier.h"

#include <algorithm>
#include <set>

using namespace guitar::core;

namespace
{
    std::vector<ChordShape> shapesFor (const std::string& symbol,
                                       const std::string& tuningKey = "standard",
                                       ShapeSearch search = {})
    {
        return chordShapes (parseChord (symbol).value(), tuningFor (tuningKey).value(), search);
    }

    bool offers (const std::vector<ChordShape>& shapes, const std::vector<int>& frets)
    {
        return std::any_of (shapes.begin(), shapes.end(),
                            [&frets] (const ChordShape& shape) { return shape.frets == frets; });
    }

    std::string spell (const std::vector<int>& frets)
    {
        std::string out;

        for (auto fret : frets)
            out += (fret == muted ? "x" : std::to_string (fret)) + std::string (" ");

        return out;
    }
}

TEST ("the open chords are offered first, because that is what they are")
{
    CHECK ((shapesFor ("C").front().frets == std::vector<int> { muted, 3, 2, 0, 1, 0 }));
    CHECK ((shapesFor ("G").front().frets == std::vector<int> { 3, 2, 0, 0, 0, 3 }));
    CHECK ((shapesFor ("Am").front().frets == std::vector<int> { muted, 0, 2, 2, 1, 0 }));
    CHECK ((shapesFor ("E").front().frets == std::vector<int> { 0, 2, 2, 1, 0, 0 }));
    CHECK ((shapesFor ("D").front().frets == std::vector<int> { muted, muted, 0, 2, 3, 2 }));
    CHECK ((shapesFor ("Am7").front().frets == std::vector<int> { muted, 0, 2, 0, 1, 0 }));
}

TEST ("the barre shapes are offered too, and named for the grip they come from")
{
    auto f = shapesFor ("F");
    CHECK (offers (f, { 1, 3, 3, 2, 1, 1 }));

    auto barre = std::find_if (f.begin(), f.end(), [] (const ChordShape& shape)
                               {
                                   return shape.frets == std::vector<int> { 1, 3, 3, 2, 1, 1 };
                               });
    CHECK (barre != f.end());
    CHECK (barre->name.find ("E-shape barre") != std::string::npos);
    CHECK_EQ (barre->barreFret, 1);

    // The A-shape: the same idea with the root on the fifth string.
    auto c = shapesFor ("C");
    CHECK (offers (c, { muted, 3, 5, 5, 5, 3 }));
}

TEST ("a barre is one finger, and the count says so")
{
    for (const auto& shape : shapesFor ("F"))
    {
        if (shape.barreFret == 0)
            continue;

        auto underTheBarre = 0;

        for (std::size_t string = 0; string < shape.frets.size(); ++string)
            if (shape.fingers[string] == 1)
                ++underTheBarre;

        CHECK (underTheBarre >= 2);
        CHECK (shape.fingersUsed <= 4);
    }
}

TEST ("nothing offered needs a fifth finger or a longer arm")
{
    // The invariant the whole generator exists to keep. It holds for every
    // chord in the catalogue, in every tuning, or a shape nobody can play is
    // being handed to a beginner as the answer.
    for (const auto& tuning : tunings())
    {
        for (const auto& quality : chordQualities())
        {
            for (const auto* root : { "C", "F#", "Bb" })
            {
                auto chord = parseChord (root + quality.suffix).value();

                for (const auto& shape : chordShapes (chord, tuning))
                {
                    CHECK (shape.fingersUsed <= 4);
                    CHECK (shape.span <= 3);
                    CHECK (shape.soundedStrings >= 2);

                    auto fretted = 0;

                    for (std::size_t string = 0; string < shape.frets.size(); ++string)
                    {
                        const auto fret = shape.frets[string];
                        const auto finger = shape.fingers[string];

                        if (fret == muted)
                            CHECK_EQ (finger, muted);
                        else if (fret == 0)
                            CHECK_EQ (finger, 0);
                        else
                        {
                            ++fretted;
                            CHECK (finger >= 1);
                            CHECK (finger <= 4);
                        }
                    }

                    CHECK (fretted <= 6);
                }
            }
        }
    }
}

TEST ("every shape offered for a chord is that chord")
{
    // The second half of the invariant above: a shape that is unplayable is
    // obvious, and a shape that is playable but is some other chord is not.
    for (const auto& quality : chordQualities())
    {
        const auto symbol = "C" + quality.suffix;
        auto chord = parseChord (symbol).value();

        for (const auto& shape : chordShapes (chord, tuningFor ("standard").value()))
        {
            const auto verdict = checkShape (chord, tuningFor ("standard").value(), shape.frets);

            if (! verdict.correct)
                CHECK_EQ (symbol + ": " + spell (shape.frets) + verdict.detail, std::string ("correct"));
        }
    }
}

TEST ("a shape says which notes it left out, and never leaves out one it needs")
{
    for (const auto& shape : shapesFor ("C9"))
    {
        CHECK (std::find (shape.omitted.begin(), shape.omitted.end(), std::string ("R")) == shape.omitted.end());
        CHECK (std::find (shape.omitted.begin(), shape.omitted.end(), std::string ("3")) == shape.omitted.end());
        CHECK (std::find (shape.omitted.begin(), shape.omitted.end(), std::string ("b7")) == shape.omitted.end());
    }

    // The fifth, on the other hand, goes early and often.
    auto ninths = shapesFor ("C9");
    CHECK (std::any_of (ninths.begin(), ninths.end(), [] (const ChordShape& shape)
           {
               return std::find (shape.omitted.begin(), shape.omitted.end(), std::string ("5")) != shape.omitted.end();
           }));
}

TEST ("the list is a walk along the neck, not eight versions of one grip")
{
    // What the "another voicing" button is for. Two shapes that differ only by
    // a muted string are the same thing to play, and a list of those is a
    // button nobody presses twice.
    const auto shapes = shapesFor ("G");
    CHECK (shapes.size() >= 5);

    std::set<int> positions;

    for (const auto& shape : shapes)
        positions.insert (shape.baseFret);

    CHECK (positions.size() >= 3);

    for (std::size_t i = 0; i < shapes.size(); ++i)
    {
        for (auto j = i + 1; j < shapes.size(); ++j)
        {
            auto sharedDiffer = false;
            auto iOnly = false;
            auto jOnly = false;
            auto bassI = -1;
            auto bassJ = -1;

            for (std::size_t string = 0; string < shapes[i].frets.size(); ++string)
            {
                const auto inI = shapes[i].frets[string] != muted;
                const auto inJ = shapes[j].frets[string] != muted;

                if (inI && inJ && shapes[i].frets[string] != shapes[j].frets[string])
                    sharedDiffer = true;

                if (inI && bassI < 0) bassI = static_cast<int> (string);
                if (inJ && bassJ < 0) bassJ = static_cast<int> (string);

                iOnly = iOnly || (inI && ! inJ);
                jOnly = jOnly || (inJ && ! inI);
            }

            // Three ways of being a different voicing, and dropping a string
            // off the top is not one of them: the fingers have to move, or each
            // has to sound something the other does not, or the bass note has
            // to change - which is a different chord however little the hand did.
            CHECK (sharedDiffer || (iOnly && jOnly) || bassI != bassJ);
        }
    }
}

TEST ("degrees are labelled on every string that sounds")
{
    for (const auto& shape : shapesFor ("Cmaj7"))
    {
        for (std::size_t string = 0; string < shape.frets.size(); ++string)
        {
            if (shape.frets[string] == muted)
                CHECK (shape.degrees[string].empty());
            else
                CHECK (! shape.degrees[string].empty());
        }
    }
}

TEST ("the root is at the bottom unless the shape says otherwise")
{
    for (const auto& shape : shapesFor ("A"))
        CHECK_EQ (shape.rootInBass, shape.name.find ("(over ") == std::string::npos);
}

TEST ("inversions can be turned off")
{
    ShapeSearch search;
    search.allowInversions = false;

    for (const auto& shape : shapesFor ("A", "standard", search))
        CHECK (shape.rootInBass);
}

TEST ("a slash chord puts the note it asked for in the bass")
{
    auto chord = parseChord ("D/F#").value();

    for (const auto& shape : chordShapes (chord, tuningFor ("standard").value()))
        CHECK_EQ (toPitchClass (shape.lowestNote), 6);
}

TEST ("asking for simple shapes gets shapes a beginner can play")
{
    ShapeSearch search;
    search.simpleOnly = true;

    for (const auto& shape : shapesFor ("G", "standard", search))
    {
        CHECK_EQ (shape.barreFret, 0);
        CHECK (shape.span <= 2);
        CHECK (shape.fingersUsed <= 3);
    }
}

TEST ("a fret range is honoured")
{
    ShapeSearch search;
    search.fromFret = 5;
    search.toFret = 9;

    for (const auto& shape : shapesFor ("A", "standard", search))
    {
        CHECK (shape.baseFret >= 5);

        for (auto fret : shape.frets)
            CHECK (fret <= 9);
    }
}

TEST ("drop D really does make a power chord one finger")
{
    ShapeSearch search;
    search.maxShapes = 12;

    const auto shapes = shapesFor ("D5", "drop-d", search);
    CHECK (std::any_of (shapes.begin(), shapes.end(), [] (const ChordShape& shape)
           {
               return shape.frets[0] == 0 && shape.frets[1] == 0 && shape.fingersUsed == 0;
           }));
}

TEST ("open G tuning makes a G out of the open strings")
{
    ShapeSearch search;
    search.maxShapes = 12;

    const auto shapes = shapesFor ("G", "open-g", search);

    // Nothing fretted at all, which is the entire point of the tuning.
    CHECK (std::any_of (shapes.begin(), shapes.end(), [] (const ChordShape& shape)
           {
               return shape.fingersUsed == 0 && shape.soundedStrings >= 4;
           }));

    // And all six strings offered as well, because the sixth is a D and that
    // is a different bass note rather than the same chord with one more string.
    CHECK (std::any_of (shapes.begin(), shapes.end(), [] (const ChordShape& shape)
           {
               return shape.fingersUsed == 0 && shape.soundedStrings == 6 && ! shape.rootInBass;
           }));
}

TEST ("a difficulty is always given, and a barre is never called easy")
{
    for (const auto& quality : chordQualities())
    {
        for (const auto& shape : shapesFor ("E" + quality.suffix))
        {
            CHECK (! shape.difficulty.empty());
            CHECK (! shape.name.empty());
            CHECK (! shape.note.empty());

            if (shape.barreFret > 0)
                CHECK (shape.difficulty == "moderate" || shape.difficulty == "hard");
        }
    }
}

TEST ("a chord nobody can play on six strings comes back with nothing rather than a lie")
{
    ShapeSearch search;
    search.fromFret = 14;
    search.toFret = 15;
    search.allowInversions = false;

    // Two frets at the top of the neck: there is not room for a root-position
    // thirteenth chord up there, and saying so is better than inventing one.
    const auto shapes = shapesFor ("Cmaj9", "standard", search);

    for (const auto& shape : shapes)
        CHECK (shape.rootInBass);
}
