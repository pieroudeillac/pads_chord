#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
constexpr int kFirstPadNote = 36; // do1 sur un MPC : les notes 36 à 51 jouent les pads 1 à 16
}

juce::AudioProcessorValueTreeState::ParameterLayout PadsProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    StringArray keys, modes;
    for (auto* k : pads::kKeyLabels) keys.add (u8 (k));
    for (auto& m : pads::kModes) modes.add (u8 (m.name));

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, u8 ("Tonalité"), keys, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode", modes, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "type", 1 }, "Accords", StringArray { "Triades", "7es", "9es" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "voicing", 1 }, "Voicing",
                                                   StringArray { u8 ("Registre lissé"), u8 ("Position serrée"), u8 ("Étalé (drop 2)") }, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "octave", 1 }, "Octave", -2, 2, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "velocity", 1 }, u8 ("Vélocité"), 1, 127, 96));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "channel", 1 }, "Canal MIDI", 1, 16, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bass", 1 }, "Basse", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "follow", 1 }, u8 ("Suivre l'hôte"), false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "beats", 1 }, u8 ("Durée d'un accord"),
                                                   StringArray { "2 temps", "1 mesure", "2 mesures" }, 1));
    return l;
}

PadsProcessor::PadsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pVelocity = apvts.getRawParameterValue ("velocity");
    pChannel = apvts.getRawParameterValue ("channel");
    pFollow = apvts.getRawParameterValue ("follow");
    pBeats = apvts.getRawParameterValue ("beats");

    paramIds = { "key", "mode", "type", "voicing", "octave", "bass" };
    for (auto& id : paramIds)
        apvts.addParameterListener (id, this);

    setDegrees (pads::progressionsFor (0)[0].degs);
    lastModeIndex = 0;
    rebuildSnapshot();
}

PadsProcessor::~PadsProcessor()
{
    cancelPendingUpdate();
    for (auto& id : paramIds)
        apvts.removeParameterListener (id, this);
}

pads::Settings PadsProcessor::getSettings() const
{
    pads::Settings s;
    s.tonicPc = (int) apvts.getRawParameterValue ("key")->load();
    s.modeIndex = (int) apvts.getRawParameterValue ("mode")->load();
    const int t = (int) apvts.getRawParameterValue ("type")->load();
    s.chordType = t == 0 ? 3 : t == 1 ? 7 : 9;
    return s;
}

pads::VoiceOptions PadsProcessor::getVoiceOptions() const
{
    pads::VoiceOptions v;
    v.voicing = (int) apvts.getRawParameterValue ("voicing")->load();
    v.octave = (int) apvts.getRawParameterValue ("octave")->load();
    v.bass = apvts.getRawParameterValue ("bass")->load() > 0.5f;
    return v;
}

std::vector<int> PadsProcessor::getDegrees() const
{
    const auto v = packedDegs.load();
    const int len = (int) (v & 0xF);
    std::vector<int> out;
    for (int i = 0; i < len; ++i)
        out.push_back ((int) ((v >> (4 + 4 * i)) & 0xF) + 1);
    return out;
}

void PadsProcessor::setDegrees (const std::vector<int>& degs)
{
    uint64_t v = 0;
    int len = 0;
    for (int d : degs)
    {
        if (len == 8 || d < 1 || d > 16)
            continue;
        v |= (uint64_t) (d - 1) << (4 + 4 * len);
        ++len;
    }
    packedDegs.store (v | (uint64_t) len); // len = 0 autorisé (progression vide, mode composer)

    juce::String s;
    for (int d : getDegrees())
        s << (s.isEmpty() ? "" : ",") << d;
    apvts.state.setProperty ("degs", s, nullptr);
}

std::vector<PadsProcessor::UserProg> PadsProcessor::getUserProgs() const
{
    std::vector<UserProg> out;
    for (auto c : apvts.state)
        if (c.hasType ("UserProg"))
        {
            UserProg p;
            p.name = c.getProperty ("name").toString();
            for (auto& t : juce::StringArray::fromTokens (c.getProperty ("degs").toString(), ",", ""))
                p.degs.push_back (t.getIntValue());
            out.push_back (std::move (p));
        }
    return out;
}

void PadsProcessor::saveUserProg (const juce::String& name, const std::vector<int>& degs)
{
    juce::String s;
    for (int d : degs)
        s << (s.isEmpty() ? "" : ",") << d;

    for (auto c : apvts.state)
        if (c.hasType ("UserProg") && c.getProperty ("name").toString() == name)
        {
            c.setProperty ("degs", s, nullptr); // une progression du même nom est remplacée
            return;
        }

    auto c = juce::ValueTree ("UserProg");
    c.setProperty ("name", name, nullptr);
    c.setProperty ("degs", s, nullptr);
    apvts.state.appendChild (c, nullptr);
}

void PadsProcessor::deleteUserProg (const juce::String& name)
{
    for (int i = 0; i < apvts.state.getNumChildren(); ++i)
    {
        auto c = apvts.state.getChild (i);
        if (c.hasType ("UserProg") && c.getProperty ("name").toString() == name)
        {
            apvts.state.removeChild (i, nullptr);
            return;
        }
    }
}

void PadsProcessor::resetMidi()
{
    uiMask.store (0);      // plus aucun pad tenu depuis l'interface
    panicPending.store (true); // le thread audio enverra les note-off et les All Notes Off
}

void PadsProcessor::setPadHeld (int pad, bool down)
{
    if (pad < 0 || pad > 15)
        return;
    if (down)
        uiMask.fetch_or (1u << pad);
    else
        uiMask.fetch_and (~(1u << pad));
}

void PadsProcessor::rebuildSnapshot()
{
    const auto layout = pads::buildLayout (getSettings());
    const auto vo = getVoiceOptions();

    Snapshot snap;
    for (size_t i = 0; i < 16; ++i)
    {
        const auto notes = pads::voicePad (layout.pads[i], vo);
        const size_t n = std::min<size_t> (notes.size(), 8);
        for (size_t j = 0; j < n; ++j)
            snap.notes[i][j] = (uint8_t) notes[j];
        snap.count[i] = (uint8_t) n;
    }

    const juce::SpinLock::ScopedLockType sl (snapLock);
    shared = snap;
    shared.version = ++sharedVersion;
}

void PadsProcessor::handleAsyncUpdate()
{
    const int mode = (int) apvts.getRawParameterValue ("mode")->load();
    if (mode != lastModeIndex)
    {
        lastModeIndex = mode;
        setDegrees (pads::progressionsFor (mode)[0].degs); // un nouveau mode repart de sa première progression
    }
    rebuildSnapshot();
}

void PadsProcessor::prepareToPlay (double, int)
{
    activeMask = 0;
    midiMask = 0;
    std::fill (std::begin (noteRefs), std::end (noteRefs), 0);
    std::fill (std::begin (heldCount), std::end (heldCount), (uint8_t) 0);
    soundingMask.store (0);
}

bool PadsProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono() || out.isDisabled();
}

void PadsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    {
        const juce::SpinLock::ScopedTryLockType sl (snapLock);
        if (sl.isLocked() && shared.version != local.version)
            local = shared;
    }

    // « RàZ MIDI » : note-off des notes encore tenues, puis All Notes Off sur chaque canal
    if (panicPending.exchange (false))
    {
        for (int n = 0; n < 128; ++n)
            if (noteRefs[n] > 0)
            {
                midi.addEvent (juce::MidiMessage::noteOff (noteChannel[n], n), 0);
                noteRefs[n] = 0;
            }
        std::fill (std::begin (heldCount), std::end (heldCount), (uint8_t) 0);
        activeMask = 0;
        midiMask = 0;
        for (int ch = 1; ch <= 16; ++ch)
            midi.addEvent (juce::MidiMessage::allNotesOff (ch), 0);
        soundingMask.store (0);
    }

    // Notes reçues : 36 à 51 jouent les pads 1 à 16, comme sur un MPC.
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn() && m.getNoteNumber() >= kFirstPadNote && m.getNoteNumber() < kFirstPadNote + 16)
            midiMask |= 1u << (m.getNoteNumber() - kFirstPadNote);
        else if (m.isNoteOff() && m.getNoteNumber() >= kFirstPadNote && m.getNoteNumber() < kFirstPadNote + 16)
            midiMask &= ~(1u << (m.getNoteNumber() - kFirstPadNote));
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            midiMask = 0;
    }
    midi.clear();

    // Progression qui suit le transport de l'hôte
    uint32_t seqBits = 0;
    int seqIdx = -1;
    if (pFollow->load() > 0.5f)
    {
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (pos->getIsPlaying())
                    if (auto ppq = pos->getPpqPosition())
                    {
                        static const double beatsTable[3] = { 2.0, 4.0, 8.0 };
                        const double len = beatsTable[juce::jlimit (0, 2, (int) pBeats->load())];
                        const auto degs = packedDegs.load();
                        const int n = (int) (degs & 0xF);
                        if (n > 0 && *ppq >= 0.0)
                        {
                            const int step = (int) std::floor (*ppq / len) % n;
                            seqIdx = (int) ((degs >> (4 + 4 * step)) & 0xF);
                            seqBits = 1u << seqIdx;
                        }
                    }
    }
    seqPad.store (seqIdx);

    const uint32_t want = uiMask.load() | midiMask | seqBits;
    const uint32_t changed = want ^ activeMask;
    const int vel = juce::jlimit (1, 127, (int) pVelocity->load());
    const int ch = juce::jlimit (1, 16, (int) pChannel->load());

    // d'abord les relâchements, puis les attaques : un changement d'accord rejoue les notes communes
    for (int pad = 0; pad < 16; ++pad)
    {
        if (! (changed & (1u << pad)) || (want & (1u << pad)))
            continue;
        for (int i = 0; i < heldCount[pad]; ++i)
        {
            const int n = held[pad][i];
            if (noteRefs[n] > 0 && --noteRefs[n] == 0)
                midi.addEvent (juce::MidiMessage::noteOff (noteChannel[n], n), 0);
        }
        heldCount[pad] = 0;
    }
    for (int pad = 0; pad < 16; ++pad)
    {
        if (! (changed & (1u << pad)) || ! (want & (1u << pad)))
            continue;
        heldCount[pad] = local.count[pad];
        for (int i = 0; i < local.count[pad]; ++i)
        {
            const int n = local.notes[pad][i];
            held[pad][i] = (uint8_t) n;
            if (noteRefs[n]++ == 0)
            {
                noteChannel[n] = ch;
                midi.addEvent (juce::MidiMessage::noteOn (ch, n, (juce::uint8) vel), 0);
            }
        }
        lastPad.store (pad + 1);
    }

    activeMask = want;
    soundingMask.store (want);
}

juce::AudioProcessorEditor* PadsProcessor::createEditor()
{
    return new PadsEditor (*this);
}

void PadsProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void PadsProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            lastModeIndex = (int) apvts.getRawParameterValue ("mode")->load();

            std::vector<int> degs;
            for (auto& t : juce::StringArray::fromTokens (apvts.state.getProperty ("degs").toString(), ",", ""))
                degs.push_back (t.getIntValue());
            if (! degs.empty())
                setDegrees (degs);

            rebuildSnapshot();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PadsProcessor();
}
