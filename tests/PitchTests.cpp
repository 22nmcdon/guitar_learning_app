#include "TestFramework.h"

#include "guitar/core/Pitch.h"

using namespace guitar::core;

TEST ("pitch classes fold in both directions")
{
    CHECK_EQ (toPitchClass (60), 0);
    CHECK_EQ (toPitchClass (61), 1);
    CHECK_EQ (toPitchClass (-1), 11);
    CHECK_EQ (toPitchClass (-13), 11);
}

TEST ("an ascending interval is always upwards")
{
    CHECK_EQ (ascendingInterval (0, 4), 4);
    CHECK_EQ (ascendingInterval (4, 0), 8);
    CHECK_EQ (ascendingInterval (7, 7), 0);
}

TEST ("middle C is C4 and the octave below it is C3")
{
    CHECK_EQ (octaveOf (60), 4);
    CHECK_EQ (octaveOf (48), 3);
    CHECK_EQ (midiNoteName (60), std::string ("C4"));
    CHECK_EQ (midiNoteName (40), std::string ("E2"));
}

TEST ("the low E string is E2, which is where every guitar diagram starts")
{
    CHECK_EQ (midiNoteName (40), std::string ("E2"));
    CHECK_EQ (midiNoteName (64), std::string ("E4"));
}

TEST ("a pitch class is spelled the way it is asked for")
{
    CHECK_EQ (pitchClassName (6, Accidental::sharps), std::string ("F#"));
    CHECK_EQ (pitchClassName (6, Accidental::flats), std::string ("Gb"));
    CHECK_EQ (pitchClassName (0, Accidental::flats), std::string ("C"));
}

TEST ("the five notes with two names are told apart from the seven with one")
{
    CHECK (hasTwoNames (1));
    CHECK (hasTwoNames (6));
    CHECK (! hasTwoNames (0));
    CHECK (! hasTwoNames (4));
    CHECK_EQ (enharmonicName (6), std::string ("F#/Gb"));
    CHECK_EQ (enharmonicName (5), std::string ("F"));
}

TEST ("note names are read in either case and with either accidental")
{
    CHECK_EQ (parsePitchClass ("C").value(), 0);
    CHECK_EQ (parsePitchClass ("c").value(), 0);
    CHECK_EQ (parsePitchClass ("F#").value(), 6);
    CHECK_EQ (parsePitchClass ("Gb").value(), 6);
    CHECK_EQ (parsePitchClass ("Bb").value(), 10);
    CHECK (! parsePitchClass ("H").has_value());
    CHECK (! parsePitchClass ("C7").has_value());
}

TEST ("an upper-case B after a note name is a note, not a flat")
{
    // Two notes written without a separator. Reading "AB" as A-flat would make
    // a nonsense of any caller that spells a pair of notes that way.
    auto parsed = parseRoot ("AB");
    CHECK_EQ (parsed.value().pitchClass, 9);
    CHECK_EQ (static_cast<int> (parsed.value().charactersConsumed), 1);
}

TEST ("a note with an octave reads back as a MIDI number")
{
    CHECK_EQ (parseMidiNote ("C4").value(), 60);
    CHECK_EQ (parseMidiNote ("E2").value(), 40);
    CHECK_EQ (parseMidiNote ("Bb3").value(), 58);
    CHECK (! parseMidiNote ("C").has_value());
    CHECK (! parseMidiNote ("Cx").has_value());
}

TEST ("three semitones is a minor third in a minor chord and a sharp nine elsewhere")
{
    CHECK_EQ (intervalName (3, true), std::string ("b3"));
    CHECK_EQ (intervalName (3, false), std::string ("#9"));
    CHECK_EQ (intervalName (0), std::string ("R"));
    CHECK_EQ (intervalName (10), std::string ("b7"));
    CHECK_EQ (intervalName (11), std::string ("7"));
}

TEST ("intervals above an octave are named as the extensions they are")
{
    CHECK_EQ (intervalName (14), std::string ("9"));
    CHECK_EQ (intervalName (21), std::string ("13"));
}

TEST ("an interval has a long name for someone who has not met the short one")
{
    CHECK_EQ (intervalDescription (0), std::string ("the root"));
    CHECK_EQ (intervalDescription (3, true), std::string ("the minor third"));
    CHECK_EQ (intervalDescription (10), std::string ("the flat seventh"));
}
