#include "PluginEditor.h"

namespace
{
juce::String u8 (const std::string& s) { return juce::String::fromUTF8 (s.c_str()); }

// Clavier de l'ordinateur : 16 touches = 16 pads, du bas vers le haut.
// Les caractères correspondent aux touches telles qu'étiquetées (AZERTY ou QWERTY) :
// Z X C V -> pads 1–4, A S D F -> 5–8, Q W E R -> 9–12, rangée des chiffres -> 13–16.
// Sur AZERTY la rangée des chiffres donne &, é, ", ' sans Maj : on accepte aussi ces caractères.
int padForCharacter (int c)
{
    if (c >= 'A' && c <= 'Z')
        c = c - 'A' + 'a';
    switch (c)
    {
        case 'z': return 0;
        case 'x': return 1;
        case 'c': return 2;
        case 'v': return 3;
        case 'a': return 4;
        case 's': return 5;
        case 'd': return 6;
        case 'f': return 7;
        case 'q': return 8;
        case 'w': return 9;
        case 'e': return 10;
        case 'r': return 11;
        case '1': case '&': return 12;
        case '2': case 0x00E9: return 13;
        case '3': case '"': return 14;
        case '4': case '\'': return 15;
        default: return -1;
    }
}

int padForKeyCode (int code)
{
    switch (code)
    {
        case 'z': case 'Z': return 0;
        case 'x': case 'X': return 1;
        case 'c': case 'C': return 2;
        case 'v': case 'V': return 3;
        case 'a': case 'A': return 4;
        case 's': case 'S': return 5;
        case 'd': case 'D': return 6;
        case 'f': case 'F': return 7;
        case 'q': case 'Q': return 8;
        case 'w': case 'W': return 9;
        case 'e': case 'E': return 10;
        case 'r': case 'R': return 11;
        case '1': return 12;
        case '2': return 13;
        case '3': return 14;
        case '4': return 15;
        default: return -1;
    }
}

const char* const padKeys[16]      = { "z", "x", "c", "v", "a", "s", "d", "f",
                                       "q", "w", "e", "r", "1", "2", "3", "4" };
const char* const padKeysAlt[4]    = { "&", "é", "\"", "'" };

// La touche équivalente est-elle actuellement enfoncée ?
bool isPadKeyCurrentlyDown (int pad)
{
    auto downChar = [] (juce::StringRef desc)
    {
        const auto kp = juce::KeyPress::createFromDescription (desc);
        return kp.isValid() && juce::KeyPress::isKeyCurrentlyDown (kp.getKeyCode());
    };

    if (downChar (padKeys[pad]))
        return true;
    if (pad >= 12)
    {
        // '1' à '4' (le code ASCII est le code de touche sur Windows) ou leur caractère AZERTY
        if (juce::KeyPress::isKeyCurrentlyDown ((int) (unsigned char) padKeys[pad][0])
            || downChar (juce::String::fromUTF8 (padKeysAlt[pad - 12])))
            return true;
    }
    return false;
}

const juce::Colour cBg      { 0xff101316 };
const juce::Colour cPanel   { 0xff171b1f };
const juce::Colour cPad     { 0xff262c32 };
const juce::Colour cLine    { 0xff2f373e };
const juce::Colour cInk     { 0xffe9edf0 };
const juce::Colour cInk2    { 0xff9aa6af };

juce::Colour kindColour (pads::Kind k)
{
    switch (k)
    {
        case pads::Kind::Tonic:  return juce::Colour (0xff7aa2ff);
        case pads::Kind::Sub:    return juce::Colour (0xfff2b552);
        case pads::Kind::Dom:    return juce::Colour (0xffff7f9a);
        case pads::Kind::Borrow: return juce::Colour (0xffb79cff);
        case pads::Kind::Color:  return juce::Colour (0xff52d6c2);
    }
    return juce::Colours::grey;
}
} // namespace

// ---------------------------------------------------------------------------
class PadsEditor::PadButton final : public juce::Component
{
public:
    PadButton (PadsEditor& e, int i) : ed (e), proc (e.proc), index (i) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void set (const pads::Pad& pad, const juce::String& steps)
    {
        colour = kindColour (pad.kind);
        name = u8 (pad.name);
        roman = u8 (pad.roman);
        stepText = steps;
        repaint();
    }

    void setState (bool l, bool c, bool n)
    {
        if (l != lit || c != current || n != next)
        {
            lit = l; current = c; next = n;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (lit ? colour : cPad.interpolatedWith (colour, 0.17f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (lit ? colour : cLine);
        g.drawRoundedRectangle (r, 10.0f, 1.0f);

        if (current)
        {
            g.setColour (cInk);
            g.drawRoundedRectangle (r.expanded (1.5f), 11.0f, 2.0f);
        }
        if (next)
        {
            juce::Path p;
            p.addRoundedRectangle (r.reduced (4.0f), 8.0f);
            juce::Path dashed;
            const float dash[] = { 5.0f, 4.0f };
            juce::PathStrokeType (1.5f).createDashedStroke (dashed, p, dash, 2);
            g.setColour (colour);
            g.fillPath (dashed);
        }

        const auto ink = lit ? cBg : cInk;
        g.setColour (ink);
        auto inner = getLocalBounds().reduced (9);
        g.setFont (juce::FontOptions ((float) juce::jlimit (14, 30, getHeight() / 4), juce::Font::bold));
        g.drawText (name, inner.removeFromBottom (getHeight() / 2).withTrimmedBottom (14), juce::Justification::bottomLeft, true);
        g.setFont (juce::FontOptions (11.5f));
        g.setColour (ink.withAlpha (0.8f));
        g.drawText (roman, getLocalBounds().reduced (9).removeFromBottom (14), juce::Justification::bottomLeft, true);

        if (stepText.isNotEmpty())
        {
            g.setColour (cInk);
            auto tag = juce::Rectangle<float> (7.0f, 7.0f, 8.0f + 6.0f * (float) stepText.length(), 16.0f);
            g.fillRoundedRectangle (tag, 4.0f);
            g.setColour (cBg);
            g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
            g.drawText (stepText, tag, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent&) override { proc.setPadHeld (index, true); if (ed.composing) ed.appendStep (index + 1); }
    void mouseUp (const juce::MouseEvent&) override { proc.setPadHeld (index, false); }

private:
    PadsEditor& ed;
    PadsProcessor& proc;
    int index;
    juce::Colour colour { juce::Colours::grey };
    juce::String name, roman, stepText;
    bool lit = false, current = false, next = false;
};

// ---------------------------------------------------------------------------
// Boîte « Enregistrer la progression » : dessinée dans la fenêtre du plugin.
// Pas de fenêtre modale de l'hôte : dans Ableton elle déforme la saisie clavier.
class PadsEditor::SaveDialog final : public juce::Component
{
public:
    SaveDialog (PadsEditor& e, const juce::String& defaultName) : ed (e)
    {
        title.setText (juce::String::fromUTF8 ("Enregistrer la progression"), juce::dontSendNotification);
        title.setFont (juce::FontOptions (17.0f, juce::Font::bold));
        title.setColour (juce::Label::textColourId, cInk);
        prompt.setText (juce::String::fromUTF8 ("Nom de la progression :"), juce::dontSendNotification);
        prompt.setFont (juce::FontOptions (12.5f));
        prompt.setColour (juce::Label::textColourId, cInk2);

        nameBox.setText (defaultName, false);
        nameBox.setSelectAllWhenFocused (true);
        nameBox.setIndents (6, 4);
        nameBox.setReturnKeyStartsNewLine (false);
        nameBox.setEscapeAndReturnKeysConsumed (false);
        nameBox.onReturnKey = [this] { ed.finishSaveDialog (true); };

        ok.setButtonText ("OK");
        cancel.setButtonText (juce::String::fromUTF8 ("Annuler"));
        ok.onClick = [this] { ed.finishSaveDialog (true); };
        cancel.onClick = [this] { ed.finishSaveDialog (false); };

        addAndMakeVisible (title);
        addAndMakeVisible (prompt);
        addAndMakeVisible (nameBox);
        addAndMakeVisible (ok);
        addAndMakeVisible (cancel);
    }

    void focusName() { nameBox.grabKeyboardFocus(); }
    juce::String name() const { return nameBox.getText(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.55f));
        auto p = panel().toFloat();
        g.setColour (cPanel);
        g.fillRoundedRectangle (p, 10.0f);
        g.setColour (cLine);
        g.drawRoundedRectangle (p, 10.0f, 1.0f);
    }

    void resized() override
    {
        auto box = panel().reduced (14);
        title.setBounds (box.removeFromTop (26));
        prompt.setBounds (box.removeFromTop (18));
        nameBox.setBounds (box.removeFromTop (30));
        box.removeFromTop (10);
        auto row = box.removeFromTop (28);
        cancel.setBounds (row.removeFromLeft (120));
        row.removeFromLeft (8);
        ok.setBounds (row.removeFromLeft (80));
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k.isKeyCode (juce::KeyPress::escapeKey)) { ed.finishSaveDialog (false); return true; }
        if (k.isKeyCode (juce::KeyPress::returnKey)) { ed.finishSaveDialog (true); return true; }
        return false;
    }

private:
    juce::Rectangle<int> panel() const
    {
        const auto area = getLocalBounds();
        const int w = juce::jlimit (300, 460, area.getWidth() * 3 / 5);
        return juce::Rectangle<int> (area.getCentreX() - w / 2, area.getCentreY() - 70, w, 140);
    }

    PadsEditor& ed;
    juce::Label title, prompt;
    juce::TextEditor nameBox;
    juce::TextButton ok, cancel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaveDialog)
};

// ---------------------------------------------------------------------------
PadsEditor::PadsEditor (PadsProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < 16; ++i)
    {
        padButtons.push_back (std::make_unique<PadButton> (*this, i));
        addAndMakeVisible (*padButtons.back());
    }

    auto setupLabel = [this] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, cInk2);
        l.setFont (juce::FontOptions (12.5f));
        addAndMakeVisible (l);
    };
    setupLabel (lKey, juce::String::fromUTF8 ("Tonalité"));
    setupLabel (lMode, "Mode");
    setupLabel (lType, "Accords");
    setupLabel (lVoicing, "Voicing");
    setupLabel (lOctave, "Octave");
    setupLabel (lVelocity, juce::String::fromUTF8 ("Vélocité"));
    setupLabel (lChannel, "Canal MIDI");
    setupLabel (lBeats, juce::String::fromUTF8 ("Durée d'un accord"));
    setupLabel (lProg, "Progression");

    auto fillCombo = [] (juce::ComboBox& c, const juce::StringArray& items)
    {
        for (int i = 0; i < items.size(); ++i)
            c.addItem (items[i], i + 1);
    };
    juce::StringArray keys, modes;
    for (auto* k : pads::kKeyLabels) keys.add (juce::String::fromUTF8 (k));
    for (auto& m : pads::kModes) modes.add (juce::String::fromUTF8 (m.name));
    fillCombo (cKey, keys);
    fillCombo (cMode, modes);
    fillCombo (cType, { "Triades", "7es", "9es" });
    fillCombo (cVoicing, { juce::String::fromUTF8 ("Registre lissé"), juce::String::fromUTF8 ("Position serrée"),
                           juce::String::fromUTF8 ("Étalé (drop 2)") });
    fillCombo (cBeats, { "2 temps", "1 mesure", "2 mesures" });

    for (auto* c : { &cKey, &cMode, &cType, &cVoicing, &cBeats, &cProg })
        addAndMakeVisible (*c);

    for (auto* s : { &sOctave, &sVelocity, &sChannel })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 22);
        addAndMakeVisible (*s);
    }

    tBass.setButtonText ("Ajouter la basse");
    tFollow.setButtonText (juce::String::fromUTF8 ("Suivre le transport de l'hôte"));
    tCompose.setButtonText (juce::String::fromUTF8 ("Composer"));
    addAndMakeVisible (tBass);
    addAndMakeVisible (tFollow);
    addAndMakeVisible (tCompose);
    addAndMakeVisible (bUndo);
    addAndMakeVisible (bClear);
    addAndMakeVisible (bGenerate);
    addAndMakeVisible (bSave);
    addAndMakeVisible (bDelete);
    addAndMakeVisible (bPanic);

    aKey = std::make_unique<CA> (proc.apvts, "key", cKey);
    aMode = std::make_unique<CA> (proc.apvts, "mode", cMode);
    aType = std::make_unique<CA> (proc.apvts, "type", cType);
    aVoicing = std::make_unique<CA> (proc.apvts, "voicing", cVoicing);
    aBeats = std::make_unique<CA> (proc.apvts, "beats", cBeats);
    aOctave = std::make_unique<SA> (proc.apvts, "octave", sOctave);
    aVelocity = std::make_unique<SA> (proc.apvts, "velocity", sVelocity);
    aChannel = std::make_unique<SA> (proc.apvts, "channel", sChannel);
    aBass = std::make_unique<BA> (proc.apvts, "bass", tBass);
    aFollow = std::make_unique<BA> (proc.apvts, "follow", tFollow);

    cProg.onChange = [this]
    {
        const int id = cProg.getSelectedId();
        if (id == 100)
            proc.setDegrees (generated);
        else if (id >= 300 && id < 300 + (int) userProgs.size())
            proc.setDegrees (userProgs[(size_t) (id - 300)].degs);
        else if (id >= 1 && id <= (int) progs.size())
            proc.setDegrees (progs[(size_t) id - 1].degs);
        updateButtons();
        refreshSteps();
    };
    bGenerate.onClick = [this]
    {
        composed.clear();
        generated = pads::randomProgression (rng);
        proc.setDegrees (generated);
        refreshProgBox();
        refreshSteps();
    };
    tCompose.onClick = [this]
    {
        composing = tCompose.getToggleState();
        if (composing)
        {
            composed.clear();
            proc.setDegrees ({});
            refreshProgBox();
        }
        refreshSteps();
    };
    bUndo.onClick = [this]
    {
        auto degs = proc.getDegrees();
        if (degs.empty())
            return;
        degs.pop_back();
        composed = degs;
        proc.setDegrees (degs);
        refreshProgBox();
        refreshSteps();
    };
    bClear.onClick = [this]
    {
        composed.clear();
        proc.setDegrees ({});
        refreshProgBox();
        refreshSteps();
    };
    bSave.onClick = [this] { openSaveDialog(); };
    bPanic.onClick = [this] { proc.resetMidi(); };
    bDelete.onClick = [this]
    {
        const int id = cProg.getSelectedId();
        if (id < 300 || id >= 300 + (int) userProgs.size())
            return;
        proc.deleteUserProg (userProgs[(size_t) (id - 300)].name);
        refreshProgBox();
        refreshSteps();
    };

    for (auto* l : { &readName, &readMeta, &readNotes })
    {
        l->setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (*l);
    }
    readName.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    readName.setColour (juce::Label::textColourId, cInk);
    readMeta.setFont (juce::FontOptions (13.0f));
    readMeta.setColour (juce::Label::textColourId, cInk2);
    readNotes.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    readNotes.setColour (juce::Label::textColourId, cInk);

    setResizable (true, true);
    setResizeLimits (720, 520, 1400, 1000);
    setSize (880, 620);

    relayout();
    startTimerHz (30);
}

PadsEditor::~PadsEditor()
{
    stopTimer();
    for (int i = 0; i < 16; ++i)
        proc.setPadHeld (i, false);
}

void PadsEditor::relayout()
{
    const auto s = proc.getSettings();
    const bool modeChanged = s.modeIndex != cachedSettings.modeIndex;
    cachedSettings = s;
    layout = pads::buildLayout (s);
    flatPref = layout.key.flatPref;

    if (modeChanged)
    {
        progs = pads::progressionsFor (s.modeIndex);
        generated.clear();
    }
    refreshProgBox();
    refreshSteps();
}

void PadsEditor::refreshProgBox()
{
    userProgs = proc.getUserProgs();

    cProg.clear (juce::dontSendNotification);
    for (size_t i = 0; i < progs.size(); ++i)
    {
        juce::String chords;
        for (int d : progs[i].degs)
            chords << (chords.isEmpty() ? "" : "  ") << u8 (layout.pads[(size_t) d - 1].name);
        cProg.addItem (u8 (progs[i].name) + "  ·  " + chords, (int) i + 1);
    }
    for (size_t i = 0; i < userProgs.size(); ++i)
    {
        juce::String chords;
        for (int d : userProgs[i].degs)
            chords << (chords.isEmpty() ? "" : "  ") << u8 (layout.pads[(size_t) d - 1].name);
        cProg.addItem (userProgs[i].name + "  ·  " + chords, 300 + (int) i);
    }

    const auto current = proc.getDegrees();
    int sel = 0;
    for (size_t i = 0; i < progs.size() && sel == 0; ++i)
        if (progs[i].degs == current)
            sel = (int) i + 1;
    for (size_t i = 0; i < userProgs.size() && sel == 0; ++i)
        if (userProgs[i].degs == current)
            sel = 300 + (int) i;

    if (sel == 0 && ! generated.empty() && generated == current)
    {
        juce::String chords;
        for (int d : generated)
            chords << (chords.isEmpty() ? "" : "  ") << u8 (layout.pads[(size_t) d - 1].name);
        cProg.addItem (juce::String::fromUTF8 ("Générée  ·  ") + chords, 100);
        sel = 100;
    }
    if (sel == 0 && ! composed.empty() && composed == current)
    {
        juce::String chords;
        for (int d : composed)
            chords << (chords.isEmpty() ? "" : "  ") << u8 (layout.pads[(size_t) d - 1].name);
        cProg.addItem (juce::String::fromUTF8 ("Composée  ·  ") + chords, 200);
        sel = 200;
    }
    if (sel != 0)
        cProg.setSelectedId (sel, juce::dontSendNotification);

    updateButtons();
}

void PadsEditor::updateButtons()
{
    const int id = cProg.getSelectedId();
    bDelete.setEnabled (id >= 300 && id < 300 + (int) userProgs.size());
    bUndo.setEnabled (composing);
    bClear.setEnabled (composing);
}

void PadsEditor::openSaveDialog()
{
    if (saveBox != nullptr || proc.getDegrees().empty())
        return;

    for (int i = 0; i < 16; ++i) // on repose tout pour repartir sur un état propre
        proc.setPadHeld (i, false);

    juce::StringArray taken;
    for (const auto& up : userProgs)
        taken.add (up.name);
    int n = 1;
    while (taken.contains (juce::String::fromUTF8 ("Ma progression ") + juce::String (n)))
        ++n;

    saveBox = std::make_unique<SaveDialog> (*this, juce::String::fromUTF8 ("Ma progression ") + juce::String (n));
    addAndMakeVisible (*saveBox);
    saveBox->setBounds (getLocalBounds());
    saveBox->toFront (true);
    saveBox->focusName();
}

void PadsEditor::finishSaveDialog (bool ok)
{
    const auto name = saveBox != nullptr ? saveBox->name() : juce::String();
    saveBox.reset();

    if (ok && name.isNotEmpty() && ! proc.getDegrees().empty())
    {
        proc.saveUserProg (name, proc.getDegrees());
        refreshProgBox();
        refreshSteps();
    }
}

bool PadsEditor::keyPressed (const juce::KeyPress& key)
{
    if (saveBox != nullptr) // la frappe du nom ne doit pas déclencher les pads
        return false;

    const auto& mods = key.getModifiers();
    if (mods.isCtrlDown() || mods.isAltDown() || mods.isCommandDown())
        return false; // on laisse passer les raccourcis

    int pad = padForCharacter ((int) key.getTextCharacter());
    if (pad < 0)
        pad = padForKeyCode (key.getKeyCode());
    if (pad < 0)
        return false;

    proc.setPadHeld (pad, true);
    return true;
}

bool PadsEditor::keyStateChanged (bool)
{
    if (saveBox != nullptr)
        return false;

    bool any = false;
    for (int i = 0; i < 16; ++i)
    {
        const bool down = isPadKeyCurrentlyDown (i);
        proc.setPadHeld (i, down);
        any = any || down;
    }
    return any;
}

void PadsEditor::refreshSteps()
{
    std::vector<juce::String> steps (16);
    const auto degs = proc.getDegrees();
    for (size_t i = 0; i < degs.size(); ++i)
    {
        auto& s = steps[(size_t) degs[i] - 1];
        s << (s.isEmpty() ? "" : "·") << (int) (i + 1);
    }
    for (size_t i = 0; i < 16; ++i)
        padButtons[i]->set (layout.pads[i], steps[i]);
}

void PadsEditor::appendStep (int pad) // mode « Composer » : ajoute un pas à la progression (8 au maximum)
{
    auto degs = proc.getDegrees();
    if (degs.size() >= 8)
        return;
    degs.push_back (pad);
    composed = degs;
    proc.setDegrees (degs);
    refreshProgBox();
    refreshSteps();
}

void PadsEditor::updateReadout()
{
    const int idx = proc.getLastPad() - 1;
    if (idx < 0 || idx > 15)
        return;
    const auto& pad = layout.pads[(size_t) idx];
    readName.setText (u8 (pad.name), juce::dontSendNotification);
    readMeta.setText (u8 (pad.roman) + "  ·  " + juce::String::fromUTF8 (pads::kKindLabels[pads::kindIndex (pad.kind)]),
                      juce::dontSendNotification);

    juce::String notes;
    for (int n : pads::voicePad (pad, proc.getVoiceOptions()))
        notes << (notes.isEmpty() ? "" : "  ") << u8 (pads::noteName (n, flatPref));
    readNotes.setText (notes, juce::dontSendNotification);
}

void PadsEditor::timerCallback()
{
    const auto s = proc.getSettings();
    if (s.tonicPc != cachedSettings.tonicPc || s.modeIndex != cachedSettings.modeIndex || s.chordType != cachedSettings.chordType)
    {
        relayout();
        shownPad = 0;
    }

    const auto sounding = proc.getSoundingMask();
    const int cur = proc.getSeqPad();
    const int last = proc.getLastPad() - 1;
    const int lastDegree = last >= 0 && last < 16 ? layout.pads[(size_t) last].degree : 0;
    const auto& nextDegs = pads::nextDegrees (lastDegree == 0 && last == 7 ? 1 : lastDegree);

    for (int i = 0; i < 16; ++i)
    {
        const bool isNext = std::find (nextDegs.begin(), nextDegs.end(), i + 1) != nextDegs.end() && last >= 0;
        padButtons[(size_t) i]->setState ((sounding >> i) & 1u, i == cur, isNext);
    }

    if (proc.getLastPad() != shownPad)
    {
        shownPad = proc.getLastPad();
        updateReadout();
    }
}

void PadsEditor::paint (juce::Graphics& g)
{
    g.fillAll (cBg);

    g.setColour (cInk);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("Pads Accords", getLocalBounds().reduced (18, 12).removeFromTop (30), juce::Justification::topLeft);

    // légende des couleurs sous la grille
    const char* labels[5] = { "Tonique", "Sous-dominante", "Dominante", "Emprunt", "Couleur" };
    const pads::Kind kinds[5] = { pads::Kind::Tonic, pads::Kind::Sub, pads::Kind::Dom, pads::Kind::Borrow, pads::Kind::Color };
    auto leg = padsArea.withY (padsArea.getBottom() + 4).withHeight (16);
    g.setFont (juce::FontOptions (11.5f));
    int x = leg.getX();
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (kindColour (kinds[i]));
        g.fillRoundedRectangle ((float) x, (float) leg.getY() + 3.0f, 10.0f, 10.0f, 2.5f);
        g.setColour (cInk2);
        const int w = juce::GlyphArrangement::getStringWidthInt (juce::Font (juce::FontOptions (11.5f)), juce::String::fromUTF8 (labels[i]));
        g.drawText (juce::String::fromUTF8 (labels[i]), x + 15, leg.getY(), w + 4, 16, juce::Justification::left);
        x += w + 28;
    }
}

void PadsEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (38);

    const int readoutH = 92;
    const int legendH = 24;
    const int side = juce::jmin (area.getHeight() - readoutH - legendH, (int) (area.getWidth() * 0.56f));
    auto left = area.removeFromLeft (side);
    padsArea = left.removeFromTop (side);
    const int gap = 8;
    const int cell = (side - gap * 3) / 4;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
        {
            const int idx = r * 4 + c;
            const int row = 3 - r; // pad 1 en bas à gauche
            padButtons[(size_t) idx]->setBounds (padsArea.getX() + c * (cell + gap), padsArea.getY() + row * (cell + gap), cell, cell);
        }

    left.removeFromTop (legendH);
    readName.setBounds (left.removeFromTop (38));
    readMeta.setBounds (left.removeFromTop (20));
    readNotes.setBounds (left.removeFromTop (22));

    area.removeFromLeft (22);
    const int rowH = 26;
    auto row = [&] (juce::Label& l, juce::Component& c)
    {
        auto r = area.removeFromTop (rowH);
        l.setBounds (r.removeFromLeft (118));
        c.setBounds (r);
        area.removeFromTop (6);
    };
    row (lKey, cKey);
    row (lMode, cMode);
    row (lType, cType);
    row (lVoicing, cVoicing);
    row (lOctave, sOctave);
    row (lVelocity, sVelocity);
    row (lChannel, sChannel);
    row (lBeats, cBeats);
    area.removeFromTop (4);
    tBass.setBounds (area.removeFromTop (rowH));
    tFollow.setBounds (area.removeFromTop (rowH));
    area.removeFromTop (10);
    lProg.setBounds (area.removeFromTop (20));
    cProg.setBounds (area.removeFromTop (rowH));
    area.removeFromTop (6);
    const int w3 = (area.getWidth() - 12) / 3;
    auto row3 = area.removeFromTop (rowH);
    tCompose.setBounds (row3.removeFromLeft (w3));
    row3.removeFromLeft (6);
    bUndo.setBounds (row3.removeFromLeft (w3));
    row3.removeFromLeft (6);
    bClear.setBounds (row3);
    area.removeFromTop (6);
    const int w4 = (area.getWidth() - 18) / 4;
    row3 = area.removeFromTop (rowH);
    bSave.setBounds (row3.removeFromLeft (w4));
    row3.removeFromLeft (6);
    bDelete.setBounds (row3.removeFromLeft (w4));
    row3.removeFromLeft (6);
    bGenerate.setBounds (row3.removeFromLeft (w4));
    row3.removeFromLeft (6);
    bPanic.setBounds (row3);

    if (saveBox != nullptr)
        saveBox->setBounds (getLocalBounds());
}
