# Pads Accords

[![Build plugin](https://github.com/pieroudeillac/pads_chord/actions/workflows/build.yml/badge.svg)](https://github.com/pieroudeillac/pads_chord/actions/workflows/build.yml) [![Licence : AGPL v3](https://img.shields.io/badge/licence-AGPL_v3-blue.svg)](LICENSE)

The live-music companion for chord players: a MIDI plugin (VST3, and AU on macOS) built for Ableton Live. A 4×4 MPC-style grid, one chord on each pad, with suggested progressions for every key and mode.

> 🇫🇷 Version française : [README.fr.md](README.fr.md)

**The plugin makes no sound** — it drives the instrument of your choice with MIDI notes.

## Features

- **4×4 pad grid** rebuilt for the selected key (12) and mode (ionian, dorian, natural minor, melodic minor…): tonics, subdominants, dominants, borrowed and colour chords, labelled with roman numerals
- **Factory progressions** for each mode (pop, jazz ii–V–I, Canon, folk…) plus a **random generator**
- **Compose your own**: click pads to add chords step by step, save your progressions under a name (stored inside the Live project), delete them anytime
- **Follow host transport**: play the selected progression in sync with Live — one chord every 2 beats, 1 bar or 2 bars
- **Voicing options**: octave transposition, smooth register / close position / drop 2 spread, optional root bass
- General settings: velocity, MIDI channel, triads / 7ths / 9ths mode
- **Plays nice**: trigger pads with the mouse, a genuine MPC-style controller (notes 36–51), or the computer keyboard
- **MIDI reset button** to silence everything (All Notes Off) when things get stuck

## Download & install

Grab a build from the [Releases](https://github.com/pieroudeillac/pads_chord/releases) page:

- **Numbered releases** (e.g. `v1.0.0`) are the stable ones to keep;
- **`latest`** follows the current development build.

Download `PadsAccords-Windows.zip` or `PadsAccords-macOS.zip` (or grab the `PadsAccords-Windows` / `PadsAccords-macOS` artifacts from the Actions tab).

| System | Unzip and copy the folder `Pads Accords.vst3` to |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3` |
| macOS | `~/Library/Audio/Plug-Ins/VST3` (or `/Library/Audio/Plug-Ins/VST3` for all users) |

Then in Live: **Preferences → Plug-Ins**, enable VST3 folder scanning, then hit *Rescan*. If macOS blocks the downloaded file: `xattr -dr com.apple.quarantine "Pads Accords.vst3"`.

## Using it in Live (works with Live Lite too)

Live only knows an **instrument** plugin that produces MIDI. So it goes **track A** producing MIDI → feeding an **instrument on track B**:

1. Create a MIDI track **A** and drop **Pads Accords** on it (from *Plug-ins → Instruments*).
2. Create MIDI track **B** with the instrument that should play the chords.
3. On track B set **MIDI From** = track A, then in the second menu pick **Pads Accords**. Set **Monitor** to *In* and arm the track.
4. Play: click the pads. On a real pad controller connected to track A, notes 36–51 (C1 to D#2) trigger pads 1 to 16.

**Follow host transport:** tick *Follow host transport* and the selected progression loops in sync with Live — one chord every 2 beats, 1 bar or 2 bars. To capture it, record-arm track B.

**Compose and save your own progressions:**

- **Compose**: with the switch on, every pad click adds a chord to the progression (up to 8) while playing. Undo removes the last step, Clear starts over; the Progression drop-down shows it as *Composée*.
- **Save**: name the displayed progression — it is saved inside the Live project and reappears in the **Progression** menu, after the built-in presets. **Delete** only works on a saved progression.
- **MIDI reset**: instantly silences everything (note-offs + All Notes Off on every channel) and resets the plugin's MIDI state — handy if the synth ends up holding stuck notes.

**Computer keyboard:** click inside the plugin window first so it gets keyboard focus, then the keys drive the pads bottom-up: `Z X C V` (pads 1–4), `A S D F` (5–8), `Q W E R` (9–12), and the number row `1 2 3 4` (13–16). A letter always means the label printed on your key (AZERTY *or* QWERTY); on AZERTY the number row also accepts the unshifted characters `& é " '`. Hold a key to sustain the chord, release to stop.

## Build from source

Prerequisites: **git**, **CMake ≥ 3.22** and a C++17 compiler (MSVC 2019+, Xcode, or gcc/clang). JUCE 8.0.8 is downloaded automatically on first build.

```sh
cmake -B build
cmake --build build --config Release
```

Tests (music-theory engine, no JUCE needed):

```sh
ctest --test-dir build -C Release --output-on-failure
```

The plugin ends up in `build/PadsAccords_artefacts/Release/` (VST3, and AU on macOS).

**Cutting a numbered release:** tag the commit and push the tag — the CI builds and publishes a stable release automatically:

```sh
git tag v1.0.0
git push origin v1.0.0
```

## Project layout

| Path | Purpose |
|---|---|
| `Source/ChordEngine.*` | Free-standing music theory engine — pure C++, verified against the original web prototype (324 key/mode/type combinations) |
| `Source/PluginProcessor.*` | Parameters, MIDI output, host transport sync, saved progressions |
| `Source/PluginEditor.*` | Interface (pads grid, readouts, progressions menu) |
| `tests/` | Engine unit tests plus a JS/C++ comparison harness versus the prototype |

## Licence

Copyright © 2026 Pierre-Yves R. — released under the [GNU AGPL-3.0](LICENSE). This plugin builds on the [JUCE](https://juce.com/) framework 8, used under the JUCE AGPL v3 license terms.
