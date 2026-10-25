#pragma once

#include "PluginProcessor.h"
#include <random>

class PadsEditor final : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit PadsEditor (PadsProcessor&);
    ~PadsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool) override;

private:
    class PadButton;
    class SaveDialog;

    void timerCallback() override;
    void relayout();          // recalcule les 16 pads quand tonalité, mode ou type changent
    void refreshProgBox();
    void refreshSteps();
    void updateReadout();
    void appendStep (int pad);  // mode composer : ajoute un pas à la progression
    void updateButtons();
    void openSaveDialog();
    void finishSaveDialog (bool ok);

    PadsProcessor& proc;
    pads::Layout layout;
    pads::Settings cachedSettings { -1, -1, -1 };
    bool flatPref = false;

    std::vector<std::unique_ptr<PadButton>> padButtons;

    juce::Label lKey, lMode, lType, lVoicing, lOctave, lVelocity, lChannel, lBeats, lProg;
    juce::ComboBox cKey, cMode, cType, cVoicing, cBeats, cProg;
    juce::Slider sOctave, sVelocity, sChannel;
    juce::ToggleButton tBass, tFollow, tCompose;
    juce::TextButton bGenerate { juce::String::fromUTF8 ("Générer") },
                     bUndo { "Annuler" }, bClear { "Effacer" },
                     bSave { "Enregistrer" }, bDelete { "Supprimer" },
                     bPanic { juce::String::fromUTF8 ("RàZ MIDI") };

    juce::Label readName, readMeta, readNotes;
    juce::Rectangle<int> padsArea;

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<CA> aKey, aMode, aType, aVoicing, aBeats;
    std::unique_ptr<SA> aOctave, aVelocity, aChannel;
    std::unique_ptr<BA> aBass, aFollow;

    std::vector<pads::Prog> progs;
    std::vector<int> generated;
    std::vector<int> composed;                      // progression en cours de composition
    std::vector<PadsProcessor::UserProg> userProgs; // progressions enregistrées par l'utilisateur
    bool composing = false;
    std::unique_ptr<SaveDialog> saveBox;
    std::mt19937 rng { std::random_device {}() };
    int shownPad = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadsEditor)
};
