// Référence : exécute le moteur du prototype web (pads-accords.html) et affiche le même format que engine_test.cpp.
// Usage : node tests/engine_dump.js ../pads-accords.html
const fs = require('fs');
const src = fs.readFileSync(process.argv[2], 'utf8');
const a = src.indexOf('/* ---------- Théorie');
const b = src.indexOf('/* ---------- Audio de la page');
const KIND = { T: 0, S: 1, D: 2, B: 3, C: 4 };
const code = src.slice(a, b) + `
for (let pc = 0; pc < 12; pc++) for (const m of MODES) for (const type of [3, 7, 9]) {
  st.tonicPc = pc; st.modeId = m.id; st.type = type; key = computeKey(); pads = build();
  console.log('## ' + pc + ' ' + m.id + ' ' + type + ' | ' + key.names[0] + (key.flatPref ? ' flat' : ''));
  for (const p of pads) {
    let line = p.idx + ' ' + KIND[p.kind] + ' ' + p.name + ' [' + p.rn + '] |';
    for (const v of ['close', 'smooth', 'open']) {
      st.voicing = v; st.oct = v === 'open' ? 1 : 0; st.bass = v === 'open';
      line += ' ' + voicePad(p).join(' ') + ' |';
    }
    console.log(line);
  }
}`;
eval(code);
