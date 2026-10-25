# Pads Accords

Plugin VST3 (et AU sur Mac) pour Ableton Live. Une grille 4×4 façon MPC, un accord par pad, avec des progressions
suggérées selon la tonalité et le mode. Le plugin ne fait pas de son : il envoie des notes MIDI vers l'instrument de ton choix.

## Obtenir le plugin sans rien installer

1. Crée un dépôt GitHub (privé si tu veux) et envoie-y ce dossier.
2. Ouvre l'onglet **Actions**. Le workflow « Compiler le plugin » démarre tout seul (10 à 15 minutes la première fois).
3. Quand il est vert, ouvre l'exécution et télécharge `PadsAccords-Windows` ou `PadsAccords-macOS` en bas de la page.

## Installer

| Système | Dossier du fichier `Pads Accords.vst3` |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3` |
| macOS | `~/Library/Audio/Plug-Ins/VST3` (ou `/Library/Audio/Plug-Ins/VST3`) |

Dans Live : Préférences, onglet **Plug-ins**, active les dossiers de plug-ins VST3 et lance une analyse.
Sur Mac, si macOS bloque le fichier téléchargé : `xattr -dr com.apple.quarantine "Pads Accords.vst3"`.

## Brancher dans Live (Live Lite compris)

Live ne connaît pas les plug-ins VST « effet MIDI ». Le plugin est donc déclaré comme instrument qui produit du MIDI.

1. Crée une piste MIDI **A** et glisse **Pads Accords** dessus (rubrique Plug-ins, Instruments).
2. Crée une piste MIDI **B** avec l'instrument qui doit jouer les accords.
3. Sur la piste B, dans **MIDI From**, choisis la piste A, puis dans le second menu choisis **Pads Accords**.
4. Mets **Monitor** sur « In » sur la piste B et arme-la.

Les pads se jouent à la souris. Les notes MIDI 36 à 51 (do1 à ré♯2) déclenchent aussi les pads 1 à 16, donc un vrai pad MPC branché sur la piste A fonctionne.

**Suivre le transport :** coche « Suivre le transport de l'hôte » et la progression choisie se joue en boucle, un accord toutes les 2 temps, 1 mesure ou 2 mesures, calée sur la position de Live. Pour la capturer, arme la piste B et enregistre.

## Développer

- `Source/ChordEngine.*` : théorie musicale, sans dépendance. `tests/engine_test.cpp` vérifie des cas précis, et `tests/engine_dump.cpp` avec `tests/engine_dump.js` comparent les 324 combinaisons tonalité, mode et type d'accords au prototype web (les deux sorties sont identiques).
- `Source/PluginProcessor.*` : paramètres, sortie MIDI, suivi du transport.
- `Source/PluginEditor.*` : interface.

En local : `cmake -B build && cmake --build build --config Release`. Pour le test du moteur seul : `cmake --build build --target engine_test`.
