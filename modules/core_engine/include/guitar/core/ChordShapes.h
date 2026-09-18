#pragma once

#include "guitar/core/ChordSymbol.h"
#include "guitar/core/Fretboard.h"

#include <string>
#include <vector>

namespace guitar::core
{

/** A chord as a guitarist has to play it: one fret per string, and a hand.

    A chord on a piano is a set of notes. On a guitar it is a set of notes *and*
    a decision - which of the several places each note could be played it is
    played in, what is left out, and what the left hand does about it. That
    second half is what this type carries and what makes "try another voicing"
    worth a button.
*/
struct ChordShape
{
    /** Fret per string, lowest string first. `muted` for a string not played. */
    std::vector<int> frets;

    /** Finger per string: 0 open, `muted` unplayed, 1-4 index to little.

        Worked out here rather than left to the player, because a shape whose
        fingering nobody can find is not a shape they can use - and because the
        count is what decides whether it is playable at all.
    */
    std::vector<int> fingers;

    /** Degree of the chord sounding on each string ("R", "b3", "5"), empty for
        a muted one. This is the label that turns a diagram into a lesson. */
    std::vector<std::string> degrees;

    int baseFret {};        ///< lowest fretted fret; 0 when nothing is fretted
    int barreFret {};       ///< 0 when there is no barre
    int barreFromString {}; ///< first string under the barre, when there is one
    int barreToString {};
    int span {};            ///< frets between the lowest and highest fretted note
    int fingersUsed {};
    int soundedStrings {};
    int lowestNote {};
    int highestNote {};
    bool rootInBass {};
    bool hasOpenStrings {};

    std::string name;        ///< "Open position", "E-shape barre, 5th fret"
    std::string difficulty;  ///< "open", "easy", "moderate", "hard"
    std::string note;        ///< one line on what this voicing is good for
    std::string bassNote;    ///< the note actually sounding lowest, named

    /** Which chord tones are not in this shape, spelled ("5", "9").

        Never empty by accident: leaving the fifth out of a ninth chord is the
        normal thing to do on six strings, and a learner who sees it written
        down learns that rather than wondering what went wrong.
    */
    std::vector<std::string> omitted;
};

/** What to look for. Defaults are what a learner should see first. */
struct ShapeSearch
{
    int fromFret { 0 };
    int toFret { 12 };
    int maxShapes { 8 };

    /** Allow shapes whose lowest sounding note is not the root.

        On by default, and ranked below the ones that do: an inversion is a real
        voicing, but the first thing anyone should be shown for "G" is a G with
        a G at the bottom.
    */
    bool allowInversions { true };

    bool allowBarre { true };

    /** Only shapes that need no barre and no stretch. What the "easy shapes
        only" switch in a shell asks for. */
    bool simpleOnly { false };
};

/** Every way of playing @p chord in @p tuning that is worth playing, best first.

    "Worth playing" is doing real work: the search enumerates far more
    combinations than this returns, and throws away the ones that are not
    fingerable by a hand with four fingers, that leave out a note the chord
    cannot do without, or that are the same shape as one already offered two
    frets along. What comes back is a walk through genuinely different
    voicings, which is what the button in the UI is for.
*/
std::vector<ChordShape> chordShapes (const Chord& chord, const Tuning& tuning,
                                     const ShapeSearch& search = {});

} // namespace guitar::core
