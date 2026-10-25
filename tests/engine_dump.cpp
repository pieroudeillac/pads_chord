// Pendant C++ de tests/engine_dump.js : même format de sortie, pour comparer les deux moteurs ligne à ligne.
#include "../Source/ChordEngine.h"

#include <cstdio>
#include <string>

int main()
{
    using namespace pads;
    for (int pc = 0; pc < 12; ++pc)
        for (size_t m = 0; m < kModes.size(); ++m)
            for (int type : { 3, 7, 9 })
            {
                const auto L = buildLayout ({ pc, (int) m, type });
                std::printf ("## %d %s %d | %s%s\n", pc, kModes[m].id, type, L.key.names[0].c_str(), L.key.flatPref ? " flat" : "");
                for (size_t i = 0; i < 16; ++i)
                {
                    const auto& p = L.pads[i];
                    std::printf ("%zu %d %s [%s] |", i, kindIndex (p.kind), p.name.c_str(), p.roman.c_str());
                    const VoiceOptions opts[3] = { { 1, 0, false }, { 0, 0, false }, { 2, 1, true } };
                    for (auto& o : opts)
                    {
                        std::string s;
                        for (int n : voicePad (p, o))
                            s += (s.empty() ? "" : " ") + std::to_string (n);
                        std::printf (" %s |", s.c_str());
                    }
                    std::printf ("\n");
                }
            }
}
