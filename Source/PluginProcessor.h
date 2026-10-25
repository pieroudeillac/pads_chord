#pragma once

#include "ChordEngine.h"
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>

class PadsProcessor final : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::AsyncUpdater
{
public:
    PadsProcessor();
    ~PadsProcessor() override;

    // ----- AudioProcessor
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Pads Accords"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ----- Utilisé par l'interface
    juce::AudioProcessorValueTreeState apvts;

    void setPadHeld (int pad, bool down);
    uint32_t getSoundingMask() const { return soundingMask.load(); }
    int getSeqPad() const { return seqPad.load(); }
    int getLastPad() const { return lastPad.load(); }

    std::vector<int> getDegrees() const;
    void setDegrees (const std::vector<int>& pads); // numéros de pads, 1 à 16, 8 maximum

    pads::Settings getSettings() const;
    pads::VoiceOptions getVoiceOptions() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;
    void rebuildSnapshot();

    struct Snapshot
    {
        uint8_t notes[16][8] {};
        uint8_t count[16] {};
        uint32_t version = 0;
    };

    juce::SpinLock snapLock;
    Snapshot shared;
    Snapshot local; // copie propre au thread audio
    uint32_t sharedVersion = 0;

    std::atomic<uint32_t> uiMask { 0 };
    std::atomic<uint32_t> soundingMask { 0 };
    std::atomic<int> seqPad { -1 };
    std::atomic<int> lastPad { 0 };
    std::atomic<uint64_t> packedDegs { 0 }; // 4 bits de longueur, puis (pad-1) sur 4 bits par pas

    std::atomic<float>* pVelocity = nullptr;
    std::atomic<float>* pChannel = nullptr;
    std::atomic<float>* pFollow = nullptr;
    std::atomic<float>* pBeats = nullptr;

    // état du thread audio
    uint32_t activeMask = 0;
    uint32_t midiMask = 0;
    uint8_t held[16][8] {};
    uint8_t heldCount[16] {};
    int noteRefs[128] {};
    int noteChannel[128] {};
    int lastModeIndex = -1;

    juce::StringArray paramIds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadsProcessor)
};
