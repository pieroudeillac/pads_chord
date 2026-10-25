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

**Clavier de l'ordinateur :** cliquez d'abord dans la fenêtre du plugin pour qu'elle ait le focus, puis les touches pilotent les pads du bas vers le haut : `Z X C V` (pads 1–4), `A S D F` (pads 5–8), `Q W E R` (9–12) et la rangée des chiffres `1 2 3 4` (pads 13–16). Une lettre correspond à l'étiquette affichée sur la touche (AZERTY ou QWERTY) ; maintenez la touche pour tenir l'accord, relâchez pour couper. Sur AZERTY, la rangée des chiffres sans Maj (`& é " '`) fonctionne aussi.

**Suivre le transport :** coche « Suivre le transport de l'hôte » et la progression choisie se joue en boucle, un accord toutes les 2 temps, 1 mesure ou 2 mesures, calée sur la position de Live. Pour la capturer, arme la piste B et enregistre.

## Composer et enregistrer une progression

- Coche **Composer** : chaque pad cliqué ajoute alors un accord à la progression (8 accords au maximum, il s'entend toujours en même temps). **Annuler** retire le dernier pas, **Effacer** repart de zéro.
- Le menu **Progression** affiche « Composée » pendant la composition. Décoche **Composer** : la progression continue de sonner avec « Suivre le transport de l'hôte ».
- **Enregistrer** ouvre une boîte pour nommer la progression affichée : elle est sauvegardée dans le projet Live (comme tout l'état du plugin) et réapparaît ensuite dans le menu **Progression**, à la suite des préréglages d'usine. **Supprimer** n'est actif que sur une progression enregistrée.
- **RàZ MIDI** coupe net tout ce qui sonne (note-off et *All Notes Off* sur chaque canal) et remet à zéro le MIDI du plugin : utile si le synthé garde des notes bloquées, par exemple après un changement de projet ou de routage.

## Développer

- `Source/ChordEngine.*` : théorie musicale, sans dépendance. `tests/engine_test.cpp` vérifie des cas précis, et `tests/engine_dump.cpp` avec `tests/engine_dump.js` comparent les 324 combinaisons tonalité, mode et type d'accords au prototype web (les deux sorties sont identiques).
- `Source/PluginProcessor.*` : paramètres, sortie MIDI, suivi du transport.
- `Source/PluginEditor.*` : interface.

En local : `cmake -B build && cmake --build build --config Release`. Pour le test du moteur seul : `cmake --build build --target engine_test`.
