#pragma once

// Moteur de théorie musicale : aucune dépendance à JUCE, donc testable seul.
// Port fidèle du prototype web (pads-accords.html).

#include <array>
#include <random>
#include <string>
#include <vector>

namespace pads
{
enum class Kind : char
{
    Tonic = 'T',
    Sub = 'S',
    Dom = 'D',
    Borrow = 'B',
    Color = 'C'
};

struct ModeDef
{
    const char* id;
    const char* name;
    std::array<int, 7> intervals;
};

extern const std::array<ModeDef, 9> kModes;
extern const char* const kKeyLabels[12];
extern const char* const kKindLabels[5]; // même ordre que Kind : T S D B C

struct Pad
{
    int degree = 0; // 1..7 pour les accords de la gamme, 0 sinon
    Kind kind = Kind::Tonic;
    int rootPc = 0;
    int offset = 0; // demi-tons depuis la tonique
    std::vector<int> ints; // intervalles depuis la fondamentale
    std::vector<std::string> tones;
    std::string name;  // ex. "Dm7"
    std::string roman; // ex. "ii7"
    bool up = false;   // tonique jouée une octave plus haut
};

struct Settings
{
    int tonicPc = 0;    // 0 = do
    int modeIndex = 0;  // index dans kModes
    int chordType = 3;  // 3 = triades, 7 = septièmes, 9 = neuvièmes
};

struct KeyInfo
{
    std::array<std::string, 7> names;
    bool flatPref = false;
};

struct Layout
{
    KeyInfo key;
    std::array<Pad, 16> pads; // index 0 = pad 1 (en bas à gauche)
};

struct VoiceOptions
{
    int voicing = 0; // 0 = lissé, 1 = serré, 2 = étalé (drop 2)
    int octave = 0;  // -2..2
    bool bass = false;
};

struct Prog
{
    std::string name;
    std::vector<int> degs; // numéros de pads (1..16)
};

Layout buildLayout (const Settings&);
std::vector<int> voicePad (const Pad&, const VoiceOptions&);
std::vector<Prog> progressionsFor (int modeIndex);
std::vector<int> randomProgression (std::mt19937&);
const std::vector<int>& nextDegrees (int degree);
std::string noteName (int midi, bool flatPref);
int kindIndex (Kind);
} // namespace pads
