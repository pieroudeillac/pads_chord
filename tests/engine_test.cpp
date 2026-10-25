// Test du moteur de théorie. Les valeurs attendues viennent du prototype web.
#include "../Source/ChordEngine.h"

#include <cstdio>
#include <string>

static int failures = 0;

static void expect (const std::string& got, const std::string& want, const std::string& what)
{
    if (got != want)
    {
        std::printf ("ECHEC  %s : obtenu \"%s\", attendu \"%s\"\n", what.c_str(), got.c_str(), want.c_str());
        ++failures;
    }
}

static std::string join (const std::vector<int>& v)
{
    std::string s;
    for (size_t i = 0; i < v.size(); ++i)
        s += (i ? "," : "") + std::to_string (v[i]);
    return s;
}

int main()
{
    using namespace pads;

    // do majeur, triades
    {
        auto L = buildLayout ({ 0, 0, 3 });
        const char* names[16] = { "C", "Dm", "Em", "F", "G", "Am", "B°", "C", "B♭", "A♭", "Fm", "D♭", "Csus2", "Csus4", "Cadd9", "C6" };
        const char* rn[16]    = { "I", "ii", "iii", "IV", "V", "vi", "vii°", "I ↑", "♭VII", "♭VI", "iv", "♭II", "Isus2", "Isus4", "Iadd9", "I6" };
        for (int i = 0; i < 16; ++i)
        {
            expect (L.pads[(size_t) i].name, names[i], "do maj pad " + std::to_string (i + 1) + " nom");
            expect (L.pads[(size_t) i].roman, rn[i], "do maj pad " + std::to_string (i + 1) + " chiffrage");
        }
        expect (join (voicePad (L.pads[0], {})), "60,64,67", "voicing C");
        expect (noteName (60, false), "C3", "nom de note (do central = C3)");
    }

    // do majeur, septièmes
    {
        auto L = buildLayout ({ 0, 0, 7 });
        expect (L.pads[0].name, "Cmaj7", "C maj7");
        expect (L.pads[1].name, "Dm7", "Dm7");
        expect (L.pads[4].name, "G7", "G7");
        expect (L.pads[6].name, "Bm7♭5", "Bm7b5");
        expect (L.pads[6].roman, "viiø7", "vii demi-diminué");
        expect (join (voicePad (L.pads[0], {})), "59,60,64,67", "voicing Cmaj7 lissé");
    }

    // la mineur
    {
        auto L = buildLayout ({ 9, 5, 3 });
        expect (L.pads[0].name, "Am", "Am");
        expect (L.pads[1].name, "B°", "B dim");
        expect (L.pads[1].roman, "ii°", "ii dim");
        expect (L.pads[3].name, "Dm", "Dm");
        expect (L.pads[9].name, "E", "E (V majeur emprunté)");
        expect (L.pads[11].name, "B7", "B7 (V7/V)");
        expect (join (voicePad (L.pads[0], {})), "57,60,64", "voicing Am");
    }

    // ré dorien, septièmes
    {
        auto L = buildLayout ({ 2, 1, 7 });
        expect (L.pads[3].name, "G7", "G7 (IV7 dorien)");
        expect (L.pads[3].roman, "IV7", "IV7");
        expect (L.pads[5].name, "Bm7♭5", "Bm7b5 dorien");
        expect (join (voicePad (L.pads[0], {})), "57,60,62,65", "voicing Dm7");
    }

    // ré bémol majeur : orthographe en bémols
    {
        auto L = buildLayout ({ 1, 0, 3 });
        expect (L.key.names[0], "D♭", "tonique D♭");
        expect (L.key.names[3], "G♭", "quarte G♭");
        expect (L.pads[3].name, "G♭", "pad G♭");
        expect (L.pads[6].name, "C°", "pad C dim");
        expect (join (voicePad (L.pads[0], {})), "56,61,65", "voicing D♭");
    }

    // do locrien : le degré V s'écrit ♭V
    {
        auto L = buildLayout ({ 0, 6, 7 });
        expect (L.pads[4].roman, "♭Vmaj7", "locrien V");
        expect (L.pads[0].roman, "iø7", "locrien i");
    }

    // sol mineur harmonique, neuvièmes
    {
        auto L = buildLayout ({ 7, 7, 9 });
        expect (L.pads[4].name, "D7♭9", "V7b9");
        expect (L.pads[4].roman, "V7♭9", "V7b9 chiffrage");
        expect (L.pads[7].name, "Gm(maj9)", "tonique m(maj9)");
        expect (join (voicePad (L.pads[0], {})), "55,58,62,66,69", "voicing Gm(maj9)");
    }

    // options de voicing : valeurs de référence produites par le prototype web
    {
        auto L = buildLayout ({ 0, 0, 3 });
        expect (join (voicePad (L.pads[4], { 0, 0, false })), "59,62,67", "G lissé");
        expect (join (voicePad (L.pads[4], { 1, 0, false })), "55,59,62", "G serré");
        expect (join (voicePad (L.pads[4], { 2, 0, false })), "50,59,67", "G étalé");
        expect (join (voicePad (L.pads[0], { 0, 1, true })), "36,72,76,79", "C octave +1 avec basse");
        expect (join (voicePad (L.pads[7], {})), "72,76,79", "tonique à l'octave");
        auto L7 = buildLayout ({ 0, 0, 7 });
        expect (join (voicePad (L7.pads[1], { 1, 0, false })), "62,65,69,72", "Dm7 serré");
        expect (join (voicePad (L7.pads[2], { 2, 0, false })), "52,59,62,67", "Em7 étalé");
        auto L9 = buildLayout ({ 0, 0, 9 });
        expect (join (voicePad (L9.pads[0], { 2, 0, false })), "52,59,60,62,67", "Cmaj9 étalé");
    }

    // progressions
    {
        auto p = progressionsFor (0);
        expect (std::to_string (p.size()), "6", "6 progressions en majeur");
        expect (join (p[0].degs), "1,5,6,4", "pop");
        std::mt19937 rng (42);
        for (int i = 0; i < 200; ++i)
        {
            auto g = randomProgression (rng);
            expect (std::to_string (g.size()), "4", "progression générée à 4 accords");
            expect (std::to_string (g[0]), "1", "démarre sur la tonique");
        }
    }

    // tous les modes et toutes les tonalités : 16 pads nommés, aucune exception
    for (int m = 0; m < 9; ++m)
        for (int t = 0; t < 12; ++t)
            for (int type : { 3, 7, 9 })
            {
                auto L = buildLayout ({ t, m, type });
                for (auto& p : L.pads)
                {
                    if (p.name.empty() || p.roman.empty() || voicePad (p, {}).empty())
                    {
                        std::printf ("ECHEC  pad vide : mode %d tonalité %d type %d\n", m, t, type);
                        ++failures;
                    }
                }
            }

    if (failures == 0)
        std::printf ("OK : tous les tests passent\n");
    return failures == 0 ? 0 : 1;
}
