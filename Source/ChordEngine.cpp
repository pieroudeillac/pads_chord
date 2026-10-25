#include "ChordEngine.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace pads
{
const std::array<ModeDef, 9> kModes {{
    { "ionian",     "Majeur (ionien)",         { 0, 2, 4, 5, 7, 9, 11 } },
    { "dorian",     "Dorien",                  { 0, 2, 3, 5, 7, 9, 10 } },
    { "phrygian",   "Phrygien",                { 0, 1, 3, 5, 7, 8, 10 } },
    { "lydian",     "Lydien",                  { 0, 2, 4, 6, 7, 9, 11 } },
    { "mixolydian", "Mixolydien",              { 0, 2, 4, 5, 7, 9, 10 } },
    { "aeolian",    "Mineur naturel (éolien)", { 0, 2, 3, 5, 7, 8, 10 } },
    { "locrian",    "Locrien",                 { 0, 1, 3, 5, 6, 8, 10 } },
    { "harmonic",   "Mineur harmonique",       { 0, 2, 3, 5, 7, 8, 11 } },
    { "melodic",    "Mineur mélodique",        { 0, 2, 3, 5, 7, 9, 11 } }
}};

const char* const kKeyLabels[12] = { "C", "C♯/D♭", "D", "D♯/E♭", "E", "F", "F♯/G♭", "G", "G♯/A♭", "A", "A♯/B♭", "B" };
const char* const kKindLabels[5] = { "Tonique", "Sous-dominante", "Dominante", "Emprunt", "Couleur" };

namespace
{
const char* const SHARP[12] = { "C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B" };
const char* const FLAT[12]  = { "C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B" };
const char* const LETTERS[7] = { "C", "D", "E", "F", "G", "A", "B" };
const int NAT[7] = { 0, 2, 4, 5, 7, 9, 11 };
const char* const RN_BASE[12] = { "I", "♭II", "II", "♭III", "III", "IV", "♯IV", "V", "♭VI", "VI", "♭VII", "VII" };
const Kind FUNC[7] = { Kind::Tonic, Kind::Sub, Kind::Tonic, Kind::Sub, Kind::Dom, Kind::Tonic, Kind::Dom };

struct Borrow { int off; char q; const char* rn; };
const Borrow BORROW[10] = {
    { 10, 'M', nullptr }, { 8, 'M', nullptr }, { 5, 'm', nullptr }, { 1, 'M', nullptr }, { 3, 'M', nullptr },
    { 7, 'M', nullptr },  { 7, 'D', "V7" },    { 2, 'D', "V7/V" }, { 5, 'M', nullptr }, { 0, 'M', nullptr }
};

std::string accStr (int a)
{
    std::string s;
    for (int i = 0; i < std::abs (a); ++i)
        s += a > 0 ? "♯" : "♭";
    return s;
}

int mod12 (int v) { return ((v % 12) + 12) % 12; }

struct Spelled { int acc; std::string name; };

std::array<Spelled, 7> spell (int li, int tonicPc, const std::array<int, 7>& iv)
{
    std::array<Spelled, 7> out;
    for (int k = 0; k < 7; ++k)
    {
        const int l = (li + k) % 7;
        const int pc = (tonicPc + iv[(size_t) k]) % 12;
        int d = mod12 (pc - NAT[l]);
        if (d > 6)
            d -= 12;
        out[(size_t) k] = { d, std::string (LETTERS[l]) + accStr (d) };
    }
    return out;
}

std::string suffix (const std::vector<int>& ints)
{
    const int t = ints[1], f = ints[2];
    const int s = ints.size() > 3 ? ints[3] : -1;
    const int n = ints.size() > 4 ? ints[4] : -1;
    const bool minor = t == 3, dim = f == 6, aug = f == 8;

    if (s < 0)
        return dim ? "°" : aug ? "+" : minor ? "m" : "";

    std::string base;
    if (dim)        base = s == 9 ? "°7" : s == 10 ? "m7♭5" : "m(maj7)♭5";
    else if (aug)   base = s == 11 ? "maj7♯5" : "7♯5";
    else if (minor) base = s == 11 ? "m(maj7)" : "m7";
    else            base = s == 11 ? "maj7" : "7";

    if (n < 0)
        return base;

    const int nine = n - 12;
    if (nine == 2)
    {
        if (base == "maj7") return "maj9";
        if (base == "7") return "9";
        if (base == "m7") return "m9";
        if (base == "m7♭5") return "m9♭5";
        if (base == "m(maj7)") return "m(maj9)";
        if (base == "°7") return "°7(9)";
        if (base == "maj7♯5") return "maj9♯5";
        if (base == "7♯5") return "9♯5";
        return base + "(9)";
    }
    if (nine == 1)
        return (base == "7" || base == "m7") ? base + "♭9" : base + "(♭9)";
    return base + "(♯9)";
}

void replaceAll (std::string& s, const std::string& from, const std::string& to)
{
    for (size_t pos = 0; (pos = s.find (from, pos)) != std::string::npos; pos += to.size())
        s.replace (pos, from.size(), to);
}

std::string roman (int off, const std::vector<int>& ints, int deg)
{
    std::string r = (off == 6 && deg == 5) ? "♭V" : RN_BASE[off];
    const bool minorish = ints[1] == 3;

    if (minorish)
    {
        // met en minuscules le premier groupe de I et de V
        size_t i = 0;
        while (i < r.size() && r[i] != 'I' && r[i] != 'V')
            ++i;
        while (i < r.size() && (r[i] == 'I' || r[i] == 'V'))
        {
            r[i] = (char) (r[i] + 32);
            ++i;
        }
    }

    std::string s = suffix (ints);
    if (minorish && s.rfind ("m", 0) == 0 && s.rfind ("maj", 0) != 0)
        s.erase (0, 1);
    replaceAll (s, "7♭5", "ø7");
    replaceAll (s, "9♭5", "ø9");
    return r + s;
}

std::vector<int> chordInts (char q, int type, bool always)
{
    std::vector<int> a { 0, q == 'm' ? 3 : 4, 7 };
    if (type >= 7 || always)
        a.push_back (q == 'M' ? 11 : 10);
    if (type >= 9)
        a.push_back (14);
    return a;
}

KeyInfo computeKey (const Settings& st)
{
    const auto& mode = kModes[(size_t) st.modeIndex];

    struct Cand { int li; int a; };
    std::vector<Cand> cands;
    for (int li = 0; li < 7; ++li)
        for (int a : { 0, 1, -1 })
            if (mod12 (NAT[li] + a) == st.tonicPc)
                cands.push_back ({ li, a });

    std::vector<Cand> pool;
    for (auto& c : cands)
        if (c.a == 0)
            pool.push_back (c);
    if (pool.empty())
        pool = cands;

    int bestCost = 1 << 30;
    std::array<Spelled, 7> best;
    for (auto& c : pool)
    {
        auto sp = spell (c.li, st.tonicPc, mode.intervals);
        int cost = 0;
        for (auto& x : sp)
            cost += std::abs (x.acc) <= 1 ? std::abs (x.acc) : 6;
        if (cost < bestCost)
        {
            bestCost = cost;
            best = sp;
        }
    }

    int flats = 0, sharps = 0;
    KeyInfo k;
    for (size_t i = 0; i < 7; ++i)
    {
        k.names[i] = best[i].name;
        if (best[i].acc < 0) ++flats;
        if (best[i].acc > 0) ++sharps;
    }
    k.flatPref = flats > sharps;
    return k;
}
} // namespace

int kindIndex (Kind k)
{
    switch (k)
    {
        case Kind::Tonic:  return 0;
        case Kind::Sub:    return 1;
        case Kind::Dom:    return 2;
        case Kind::Borrow: return 3;
        case Kind::Color:  return 4;
    }
    return 0;
}

Layout buildLayout (const Settings& st)
{
    Layout out;
    out.key = computeKey (st);
    const auto& mode = kModes[(size_t) st.modeIndex];
    const int n = st.chordType == 3 ? 3 : st.chordType == 7 ? 4 : 5;
    auto pcName = [&] (int pc) { return std::string ((out.key.flatPref ? FLAT : SHARP)[mod12 (pc)]); };

    std::vector<Pad> list;
    std::vector<Pad> diat;

    for (int k = 0; k < 7; ++k)
    {
        Pad p;
        p.degree = k + 1;
        p.kind = FUNC[k];
        p.offset = mode.intervals[(size_t) k];
        p.rootPc = (st.tonicPc + p.offset) % 12;
        for (int j = 0; j < n; ++j)
        {
            const int d = (k + 2 * j) % 7;
            const int pc = (st.tonicPc + mode.intervals[(size_t) d]) % 12;
            int raw = mod12 (pc - p.rootPc);
            if (j == 4)
                raw += 12;
            p.ints.push_back (raw);
            p.tones.push_back (out.key.names[(size_t) d]);
        }
        p.name = out.key.names[(size_t) k] + suffix (p.ints);
        p.roman = roman (p.offset, p.ints, k + 1);
        diat.push_back (p);
    }

    for (auto& p : diat)
        list.push_back (p);

    Pad up = diat[0];
    up.up = true;
    up.roman += " ↑";
    list.push_back (up);

    // 4 emprunts hors de la gamme
    std::set<std::string> taken;
    auto idOf = [] (int off, const std::vector<int>& ints)
    {
        std::string s = std::to_string (off) + ":";
        for (int i : ints) s += std::to_string (i) + ",";
        return s;
    };
    for (auto& p : diat)
        taken.insert (idOf (p.offset, p.ints));

    auto makeBorrow = [&] (const Borrow& b)
    {
        Pad p;
        p.kind = Kind::Borrow;
        p.offset = b.off;
        p.rootPc = (st.tonicPc + b.off) % 12;
        p.ints = chordInts (b.q, st.chordType, b.q == 'D');
        const std::string rn0 = RN_BASE[b.off];
        const char* const* tab = rn0.find ("♭") != std::string::npos ? FLAT
                               : rn0.find ("♯") != std::string::npos ? SHARP
                               : (out.key.flatPref ? FLAT : SHARP);
        for (int i : p.ints)
            p.tones.push_back (tab[mod12 (p.rootPc + i)]);
        p.name = std::string (tab[p.rootPc]) + suffix (p.ints);
        p.roman = b.rn ? std::string (b.rn) : roman (b.off, p.ints, 0);
        return p;
    };

    int added = 0;
    for (auto& b : BORROW)
    {
        if (added == 4)
            break;
        auto p = makeBorrow (b);
        const auto id = idOf (p.offset, p.ints);
        if (taken.count (id))
            continue;
        taken.insert (id);
        list.push_back (p);
        ++added;
    }
    for (size_t i = 0; added < 4; ++i, ++added) // cas de repli, jamais atteint avec les 9 modes
        list.push_back (makeBorrow (BORROW[i]));

    // 4 couleurs sur la tonique
    const auto& tonic = diat[0];
    const int third = tonic.ints[1];
    const bool lower = third == 3;
    const std::string tn = tonic.tones[0];
    const std::string I = lower ? "i" : "I";
    struct Col { std::string nm; std::vector<int> ints; std::string rn; };
    const std::vector<Col> colors {
        { tn + "sus2", { 0, 2, 7 }, I + "sus2" },
        { tn + "sus4", { 0, 5, 7 }, I + "sus4" },
        { tn + (lower ? "m(add9)" : "add9"), { 0, third, 7, 14 }, I + "add9" },
        { tn + (lower ? "m6" : "6"), { 0, third, 7, 9 }, I + "6" }
    };
    for (auto& c : colors)
    {
        Pad p;
        p.kind = Kind::Color;
        p.rootPc = tonic.rootPc;
        p.offset = 0;
        p.ints = c.ints;
        for (int i : c.ints)
            p.tones.push_back (pcName (tonic.rootPc + i));
        p.name = c.nm;
        p.roman = c.rn;
        list.push_back (p);
    }

    for (size_t i = 0; i < 16; ++i)
        out.pads[i] = list[i];
    return out;
}

std::vector<int> voicePad (const Pad& pad, const VoiceOptions& vo)
{
    const int rootMidi = 54 + ((pad.rootPc + 6) % 12);
    std::vector<int> notes;
    for (int i : pad.ints)
        notes.push_back (rootMidi + i);

    if (vo.voicing != 1)
    {
        std::vector<int> cur = notes;
        std::sort (cur.begin(), cur.end());
        double bestD = 1e9;
        std::vector<int> best;
        for (size_t inv = 0; inv < cur.size(); ++inv)
        {
            for (int sh : { -12, 0, 12 })
            {
                std::vector<int> cand;
                double sum = 0;
                for (int x : cur) { cand.push_back (x + sh); sum += x + sh; }
                const double d = std::abs (sum / (double) cand.size() - 62.0);
                if (best.empty() || d < bestD)
                {
                    bestD = d;
                    best = cand;
                }
            }
            const int first = cur.front();
            cur.erase (cur.begin());
            cur.push_back (first + 12);
            std::sort (cur.begin(), cur.end());
        }
        notes = best;
    }

    std::sort (notes.begin(), notes.end());
    if (vo.voicing == 2 && notes.size() >= 3)
    {
        notes[notes.size() - 2] -= 12;
        std::sort (notes.begin(), notes.end());
    }

    for (int& x : notes)
        x += 12 * vo.octave + (pad.up ? 12 : 0);

    if (vo.bass)
        notes.insert (notes.begin(), 36 + pad.rootPc);

    std::vector<int> out;
    for (int x : notes)
        if (x >= 0 && x <= 127 && std::find (out.begin(), out.end(), x) == out.end())
            out.push_back (x);
    std::sort (out.begin(), out.end());
    return out;
}

std::vector<Prog> progressionsFor (int modeIndex)
{
    switch (modeIndex)
    {
        case 0: return { { "Pop", { 1, 5, 6, 4 } }, { "Années 50", { 1, 6, 4, 5 } }, { "Jazz ii–V–I", { 2, 5, 1, 1 } },
                         { "Canon", { 1, 5, 6, 3, 4, 1, 4, 5 } }, { "Folk", { 1, 4, 5, 1 } }, { "Mélancolique", { 6, 4, 1, 5 } } };
        case 1: return { { "Vamp", { 1, 4, 1, 4 } }, { "Boucle", { 1, 7, 4, 1 } }, { "Montée", { 1, 2, 3, 4 } }, { "Retour", { 1, 3, 7, 4 } } };
        case 2: return { { "Vamp", { 1, 2, 1, 2 } }, { "Descente", { 1, 7, 6, 2 } }, { "Tension", { 1, 2, 3, 2 } } };
        case 3: return { { "Vamp", { 1, 2, 1, 2 } }, { "Cadence", { 1, 2, 5, 1 } }, { "Flottante", { 1, 5, 2, 1 } }, { "Lumineuse", { 1, 2, 6, 5 } } };
        case 4: return { { "Rock", { 1, 7, 4, 1 } }, { "Boucle", { 1, 5, 4, 1 } }, { "Balancier", { 1, 4, 7, 4 } } };
        case 5: return { { "Épique", { 1, 6, 3, 7 } }, { "Cadence", { 1, 4, 5, 1 } }, { "Descente", { 1, 7, 6, 7 } }, { "Sombre", { 1, 4, 7, 3 } } };
        case 6: return { { "Vamp", { 1, 2, 1, 2 } }, { "Instable", { 1, 4, 2, 1 } }, { "Chute", { 1, 6, 2, 1 } } };
        case 7: return { { "Cadence", { 1, 4, 5, 1 } }, { "Tension", { 1, 6, 5, 5 } }, { "ii°–V–i", { 2, 5, 1, 1 } }, { "Boucle", { 1, 6, 2, 5 } } };
        default: return { { "Cadence", { 1, 4, 5, 1 } }, { "ii–V–i", { 2, 5, 1, 1 } }, { "Montée", { 1, 3, 4, 5 } } };
    }
}

const std::vector<int>& nextDegrees (int degree)
{
    static const std::vector<int> table[8] = {
        {}, { 4, 5, 6, 2 }, { 5, 7 }, { 6, 4 }, { 5, 1, 2 }, { 1, 6 }, { 2, 4, 5 }, { 1, 3 }
    };
    return table[degree >= 1 && degree <= 7 ? degree : 0];
}

std::vector<int> randomProgression (std::mt19937& rng)
{
    std::vector<int> out { 1 };
    int cur = 1;
    for (int i = 1; i < 4; ++i)
    {
        const std::vector<int> last { 5, 5, 4, 7 };
        const auto& opts = i == 3 ? last : nextDegrees (cur);
        cur = opts[std::uniform_int_distribution<size_t> (0, opts.size() - 1) (rng)];
        out.push_back (cur);
    }
    return out;
}

std::string noteName (int midi, bool flatPref)
{
    return std::string ((flatPref ? FLAT : SHARP)[midi % 12]) + std::to_string (midi / 12 - 2);
}
} // namespace pads
