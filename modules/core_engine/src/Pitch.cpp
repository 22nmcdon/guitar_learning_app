#include "guitar/core/Pitch.h"

#include <array>
#include <cctype>

namespace guitar::core
{

namespace
{
    // Sharps first, because that is what a guitarist meets first: the notes
    // between the open strings are named upwards from them, and every standard
    // tuning string but one climbs through a sharp before it reaches a flat.
    constexpr std::array<const char*, semitonesPerOctave> sharpNames
        { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    constexpr std::array<const char*, semitonesPerOctave> flatNames
        { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };

    /** Letter names as pitch classes, C = 0. */
    std::optional<int> letterPitchClass (char letter)
    {
        switch (std::toupper (static_cast<unsigned char> (letter)))
        {
            case 'C': return 0;
            case 'D': return 2;
            case 'E': return 4;
            case 'F': return 5;
            case 'G': return 7;
            case 'A': return 9;
            case 'B': return 11;
            default:  return std::nullopt;
        }
    }
}

int toPitchClass (int value) noexcept
{
    return ((value % semitonesPerOctave) + semitonesPerOctave) % semitonesPerOctave;
}

int ascendingInterval (PitchClass from, PitchClass to) noexcept
{
    return toPitchClass (to - from);
}

int octaveOf (int midiNote) noexcept
{
    // Integer division rounds towards zero, which would put every note below
    // MIDI 0 in the wrong octave. Nothing on a guitar is down there, but the
    // quiz generator does arithmetic on notes before it checks they are.
    auto floored = midiNote >= 0 ? midiNote / semitonesPerOctave
                                 : (midiNote - (semitonesPerOctave - 1)) / semitonesPerOctave;
    return floored - 1;
}

std::string pitchClassName (PitchClass pitchClass, Accidental accidental)
{
    const auto folded = toPitchClass (pitchClass);
    return accidental == Accidental::sharps ? sharpNames[static_cast<std::size_t> (folded)]
                                            : flatNames[static_cast<std::size_t> (folded)];
}

std::string midiNoteName (int midiNote, Accidental accidental)
{
    return pitchClassName (midiNote, accidental) + std::to_string (octaveOf (midiNote));
}

bool hasTwoNames (PitchClass pitchClass) noexcept
{
    const auto folded = toPitchClass (pitchClass);
    return std::string (sharpNames[static_cast<std::size_t> (folded)])
        != std::string (flatNames[static_cast<std::size_t> (folded)]);
}

std::string enharmonicName (PitchClass pitchClass)
{
    if (! hasTwoNames (pitchClass))
        return pitchClassName (pitchClass);

    return pitchClassName (pitchClass, Accidental::sharps) + "/"
         + pitchClassName (pitchClass, Accidental::flats);
}

std::optional<ParsedRoot> parseRoot (std::string_view text)
{
    if (text.empty())
        return std::nullopt;

    auto letter = letterPitchClass (text[0]);

    if (! letter.has_value())
        return std::nullopt;

    ParsedRoot parsed;
    parsed.pitchClass = *letter;
    parsed.charactersConsumed = 1;

    // One accidental only. "C##" is real notation and not something anyone
    // writes on a chord chart or asks a fretboard quiz about, so reading the
    // second '#' as part of the chord quality is the useful failure.
    if (text.size() > 1)
    {
        if (text[1] == '#')
        {
            parsed.pitchClass += 1;
            parsed.charactersConsumed = 2;
        }
        else if (text[1] == 'b')
        {
            // Lower case only. An upper-case B after a letter is a second note
            // name, not an accidental, and reading "AB" as A-flat would make
            // nonsense of any caller that spells two notes without a separator.
            parsed.pitchClass -= 1;
            parsed.charactersConsumed = 2;
        }
    }

    parsed.pitchClass = toPitchClass (parsed.pitchClass);
    return parsed;
}

std::optional<PitchClass> parsePitchClass (std::string_view text)
{
    auto parsed = parseRoot (text);

    if (! parsed.has_value() || parsed->charactersConsumed != text.size())
        return std::nullopt;

    return parsed->pitchClass;
}

std::optional<int> parseMidiNote (std::string_view text)
{
    auto parsed = parseRoot (text);

    if (! parsed.has_value() || parsed->charactersConsumed >= text.size())
        return std::nullopt;

    auto rest = text.substr (parsed->charactersConsumed);
    auto negative = false;

    if (! rest.empty() && rest[0] == '-')
    {
        negative = true;
        rest = rest.substr (1);
    }

    if (rest.empty())
        return std::nullopt;

    auto octave = 0;

    for (auto c : rest)
    {
        if (! std::isdigit (static_cast<unsigned char> (c)))
            return std::nullopt;

        octave = octave * 10 + (c - '0');
    }

    if (negative)
        octave = -octave;

    return (octave + 1) * semitonesPerOctave + parsed->pitchClass;
}

namespace
{
    /** Where each letter sits in the octave. */
    constexpr std::array<int, 7> letterPitches { 0, 2, 4, 5, 7, 9, 11 };
    constexpr std::array<char, 7> letterNames { 'C', 'D', 'E', 'F', 'G', 'A', 'B' };
}

PitchClass NoteSpelling::pitchClass() const noexcept
{
    return toPitchClass (letterPitches[static_cast<std::size_t> (((letter % 7) + 7) % 7)] + alteration);
}

std::string NoteSpelling::name() const
{
    std::string out (1, letterNames[static_cast<std::size_t> (((letter % 7) + 7) % 7)]);

    for (auto i = 0; i < alteration; ++i)
        out += '#';

    for (auto i = 0; i > alteration; --i)
        out += 'b';

    return out;
}

std::optional<NoteSpelling> parseSpelling (std::string_view text)
{
    if (text.empty())
        return std::nullopt;

    NoteSpelling spelling;
    auto found = false;

    for (std::size_t i = 0; i < letterNames.size(); ++i)
        if (letterNames[i] == std::toupper (static_cast<unsigned char> (text[0])))
        {
            spelling.letter = static_cast<int> (i);
            found = true;
        }

    if (! found)
        return std::nullopt;

    for (std::size_t i = 1; i < text.size(); ++i)
    {
        if (text[i] == '#')
            ++spelling.alteration;
        else if (text[i] == 'b')
            --spelling.alteration;
        else
            return std::nullopt;
    }

    return spelling;
}

NoteSpelling spellAbove (const NoteSpelling& root, int semitones, int degreeStep)
{
    NoteSpelling spelled;
    spelled.letter = ((root.letter + degreeStep) % 7 + 7) % 7;

    // The letter is decided; the accidental is then whatever it takes to reach
    // the note that was actually asked for.
    const auto wanted = toPitchClass (root.pitchClass() + semitones);
    const auto natural = letterPitches[static_cast<std::size_t> (spelled.letter)];

    auto alteration = wanted - natural;

    // Fold into the nearest way of writing it: a note is at most a double sharp
    // or a double flat from its letter, never ten semitones above it.
    while (alteration > 6) alteration -= semitonesPerOctave;
    while (alteration < -6) alteration += semitonesPerOctave;

    spelled.alteration = alteration;
    return spelled;
}

std::string preferredRootName (PitchClass pitchClass, bool minorKey)
{
    // Read off what music actually writes. The three that differ are the ones
    // worth knowing: C# minor rather than Db minor, Eb major rather than D#
    // major, and G# minor rather than Ab minor.
    static const std::array<const char*, semitonesPerOctave> major
        { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

    static const std::array<const char*, semitonesPerOctave> minor
        { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };

    const auto folded = static_cast<std::size_t> (toPitchClass (pitchClass));
    return minorKey ? minor[folded] : major[folded];
}

std::string intervalName (int semitones, bool spellThirdAsMinor)
{
    switch (toPitchClass (semitones))
    {
        case 0:  return "R";
        case 1:  return "b9";
        case 2:  return "9";
        case 3:  return spellThirdAsMinor ? "b3" : "#9";
        case 4:  return "3";
        case 5:  return "11";
        case 6:  return "#11";
        case 7:  return "5";
        case 8:  return "b13";
        case 9:  return "13";
        case 10: return "b7";
        case 11: return "7";
        default: return "?";
    }
}

std::string intervalDescription (int semitones, bool spellThirdAsMinor)
{
    switch (toPitchClass (semitones))
    {
        case 0:  return "the root";
        case 1:  return "the flat ninth";
        case 2:  return "the ninth";
        case 3:  return spellThirdAsMinor ? "the minor third" : "the sharp ninth";
        case 4:  return "the major third";
        case 5:  return "the eleventh";
        case 6:  return "the sharp eleventh";
        case 7:  return "the fifth";
        case 8:  return "the flat thirteenth";
        case 9:  return "the thirteenth";
        case 10: return "the flat seventh";
        case 11: return "the major seventh";
        default: return "outside the chord";
    }
}

} // namespace guitar::core
