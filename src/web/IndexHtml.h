#pragma once

#include <Arduino.h>

/**
 * @file IndexHtml.h
 * @brief Strona panelu testera (HTML, CSS i JavaScript) zapisana we flashu (PROGMEM).
 *
 * Edytuj HTML/CSS/JS bezposrednio miedzy znacznikami R"rawhtml( ... )rawhtml".
 * Strona korzysta z API opisanego w WebServerApp.
 */

/// Tresc strony glownej wysylana pod adresem "/".
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>LcdTester</title>
<style>
  :root { --bg:#f4f5f7; --card:#fff; --text:#1d2129; --muted:#667085; --accent:#2563eb; --ok:#16a34a; --err:#dc2626; --line:#d0d5dd; }
  @media (prefers-color-scheme: dark) {
    :root { --bg:#14161a; --card:#1e2127; --text:#e6e8eb; --muted:#9aa1ac; --accent:#4f8cff; --line:#333842; }
  }
  * { box-sizing: border-box; }
  body { margin:0; font-family: system-ui, sans-serif; background:var(--bg); color:var(--text); }
  main { max-width: 640px; margin: 0 auto; padding: 16px; }
  h1 { font-size: 1.4rem; margin: 8px 0 16px; }
  h1 small { color: var(--muted); font-weight: normal; font-size: .9rem; }
  section { background: var(--card); border: 1px solid var(--line); border-radius: 10px; padding: 14px 16px; margin-bottom: 14px; }
  h2 { font-size: 1rem; margin: 0 0 10px; }
  .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(140px, 1fr)); gap: 10px; }
  label { display: flex; flex-direction: column; gap: 4px; font-size: .85rem; color: var(--muted); }
  label.check { flex-direction: row; align-items: center; gap: 6px; color: var(--text); }
  select, input[type=number] { padding: 0 8px; height: 36px; border: 1px solid var(--line); border-radius: 6px; background: var(--bg); color: var(--text); font-size: .95rem; width: 100%; }
  /* Pola w siatce zawsze na dole komorki - rowno, nawet gdy opis zawija sie w 2 linie */
  .grid label > select, .grid label > input[type=number] { margin-top: auto; }
  details summary { cursor: pointer; color: var(--muted); font-size: .9rem; margin-bottom: 8px; }
  .buttons { display: flex; flex-wrap: wrap; gap: 10px; }
  .buttons button { min-width: 120px; }
  button { flex: 1; padding: 12px; font-size: 1rem; border: 0; border-radius: 8px; cursor: pointer; color: #fff; background: var(--accent); }
  button.stop { background: var(--err); }
  button.alt { background: var(--ok); }
  pre { background: var(--bg); border: 1px solid var(--line); border-radius: 6px; padding: 8px; font-size: .8rem; overflow-x: auto; }
  button:disabled { opacity: .45; cursor: default; }
  .bar { height: 10px; background: var(--line); border-radius: 5px; overflow: hidden; margin: 10px 0; }
  .bar div { height: 100%; width: 0; background: var(--accent); transition: width .4s; }
  #message { font-weight: 600; }
  #message.ok { color: var(--ok); }
  #message.err { color: var(--err); }
  table { width: 100%; border-collapse: collapse; font-size: .9rem; }
  td, th { text-align: left; padding: 5px 4px; border-bottom: 1px solid var(--line); }
  th { color: var(--muted); font-weight: normal; }
  .muted { color: var(--muted); font-size: .8rem; }
  input[type=range] { width: 100%; accent-color: var(--accent); }
  #brightnessValue { color: var(--text); }
  .wide { grid-column: 1 / -1; }
  input[type=checkbox] { width: 18px; height: 18px; accent-color: var(--accent); flex: none; }
  /* Panel z przyciskami zawsze widoczny na dole ekranu */
  .actions { position: sticky; bottom: 0; z-index: 10; margin-bottom: 0;
             box-shadow: 0 -4px 16px rgba(0, 0, 0, .12);
             padding-bottom: calc(14px + env(safe-area-inset-bottom)); }
  .actions .bar { margin: 10px 0 6px; }

  /* Telefon */
  @media (max-width: 600px) {
    main { padding: 10px 10px 0; }
    h1 { font-size: 1.25rem; margin: 4px 2px 10px; }
    section { padding: 12px; margin-bottom: 10px; }
    .grid { grid-template-columns: 1fr 1fr; gap: 8px; }
    /* 16 px - przegladarka nie powieksza strony przy wpisywaniu */
    select, input[type=number] { height: 44px; font-size: 16px; }
    label.check { min-height: 40px; font-size: .95rem; }
    input[type=checkbox] { width: 22px; height: 22px; }
    input[type=range] { height: 32px; }
    details summary { padding: 8px 0; font-size: 1rem; }
    .actions { margin: 0 -10px; border-radius: 12px 12px 0 0; border-bottom: 0; }
    .buttons { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
    .buttons button { min-width: 0; padding: 13px 6px; font-size: .95rem; }
    #message { font-size: .9rem; }
    table { font-size: .85rem; }
    pre { white-space: pre-wrap; word-break: break-word; font-size: .75rem; }
  }
</style>
</head>
<body>
<main>
  <h1>LcdTester <small id="board"></small></h1>

  <section>
    <h2>Wyświetlacz</h2>
    <div class="grid">
      <label>Model <select id="model"></select></label>
      <label>Obrót <select id="rot">
        <option value="0">0°</option><option value="1">90°</option>
        <option value="2">180°</option><option value="3">270°</option>
      </select></label>
      <label class="wide">Rozdzielczość <select id="res"></select></label>
    </div>
    <div class="grid" style="margin-top:10px">
      <label class="check"><input type="checkbox" id="invert"> Inwersja kolorów</label>
      <label class="check"><input type="checkbox" id="swaprb"> Zamień czerwony i niebieski</label>
    </div>
    <label style="margin-top:12px">Jasność podświetlenia (pin LED): <b id="brightnessValue">100 %</b>
      <input type="range" id="brightness" min="0" max="100" step="1" value="100">
    </label>
  </section>

  <section>
    <h2>Szyna SPI</h2>
    <div class="grid">
      <label>Szybkość <select id="freq"></select></label>
      <label>Tryb SPI <select id="mode">
        <option>0</option><option>1</option><option>2</option><option>3</option>
      </select></label>
    </div>
    <label class="check" style="margin-top:10px"><input type="checkbox" id="spi3wire">
      Odczyt ID przez SDA (3-wire, moduły bez MISO/SDO)</label>
    <details style="margin-top:12px">
      <summary>Piny GPIO (-1 = nieużywany)</summary>
      <div class="grid" id="pins"></div>
      <p class="muted">Domyślne w firmware: <span id="pinDefaults"></span>
        <a href="#" id="pinReset">Przywróć domyślne</a></p>
      <p class="muted">Bez MISO sterownik nie jest odczytywany - test sprawdza tylko zapis.</p>
    </details>
  </section>

  <section>
    <h2>Ograniczenie wysyłania</h2>
    <div class="grid">
      <label>Odstęp paczek, ms (testy 1-2)
        <input type="number" id="delay" min="1" max="100" step="1" placeholder="brak"></label>
      <label>Limit FPS (animacje) <select id="fps">
        <option value="">brak</option><option>30</option><option>10</option>
        <option>5</option><option>1</option>
      </select></label>
    </div>
    <p class="muted">Odstęp w ms (1-100) po każdym kształcie lub napisie. Limit FPS dotyczy pełnych klatek.</p>
  </section>

  <section>
    <h2>Wyniki</h2>
    <table>
      <tr><th>Faza</th><th>Klatki / operacje</th><th>Na sekundę</th></tr>
      <tr><td>1. Kształty</td><td id="f1">-</td><td id="s1">-</td></tr>
      <tr><td>2. Napisy</td><td id="f2">-</td><td id="s2">-</td></tr>
      <tr><td>3. Animacje (FPS)</td><td id="f3">-</td><td id="s3">-</td></tr>
    </table>
    <pre id="idInfo" hidden></pre>
    <p class="muted" id="heap"></p>
  </section>

  <section class="actions">
    <div class="buttons">
      <button id="readid" class="alt">Odczytaj ID</button>
      <button id="pattern" class="alt">Obraz testowy</button>
      <button id="start">Uruchom test</button>
      <button id="stop" class="stop" disabled>Zatrzymaj</button>
    </div>
    <div class="bar"><div id="progress"></div></div>
    <div id="message">Ładowanie…</div>
  </section>
</main>

<script>
const PIN_NAMES = ['sclk', 'mosi', 'miso', 'cs', 'dc', 'rst', 'led'];
// Nazwy pinow, jakie zwykle sa nadrukowane na modulach LCD
const PIN_LABELS = {
  sclk: 'SCLK (SCK/SCL)', mosi: 'MOSI (SDI/SDA)', miso: 'MISO (SDO)', cs: 'CS',
  dc: 'DC (RS/A0)', rst: 'RST (RESET)', led: 'LED (BL)',
};
const $ = id => document.getElementById(id);
let cfg = null, polling = null;

function saved() {
  try { return JSON.parse(localStorage.getItem('lcdtester') || '{}'); } catch (e) { return {}; }
}
function save(data) {
  try { localStorage.setItem('lcdtester', JSON.stringify(data)); } catch (e) {}
}

function fillResolutions(applyDefaults) {
  const m = cfg.models.find(x => x.id === $('model').value);
  $('res').innerHTML = m.resolutions
    .map((r, i) => `<option value="${i}">${r.w}x${r.h} - ${r.note}</option>`).join('');
  if (applyDefaults) {
    $('invert').checked = m.invert;
    $('swaprb').checked = m.swapRb;
    $('freq').value = m.maxFreq;
  }
}

async function loadConfig() {
  cfg = await (await fetch('/api/config')).json();
  $('board').textContent = cfg.board;
  $('model').innerHTML = cfg.models.map(m => `<option value="${m.id}">${m.name}</option>`).join('');
  $('freq').innerHTML = cfg.frequencies
    .map(f => `<option value="${f}">${(f / 1e6).toFixed(f % 1e6 ? 1 : 0)} MHz</option>`).join('');
  $('pins').innerHTML = PIN_NAMES
    .map(p => `<label>${PIN_LABELS[p]} <input type="number" min="-1" max="48" id="pin_${p}" value="${cfg.pins[p]}"></label>`).join('');
  $('pinDefaults').textContent = PIN_NAMES.map(p => `${p.toUpperCase()} ${cfg.pins[p]}`).join(', ');

  const s = saved();
  // Zapamietane piny uzywamy tylko, gdy domyslne w firmware sie nie zmienily
  if (s.pins && JSON.stringify(s.pinDefaults) !== JSON.stringify(cfg.pins)) delete s.pins;
  if (s.model && cfg.models.some(m => m.id === s.model)) $('model').value = s.model;
  fillResolutions(true);
  for (const k of ['res', 'rot', 'freq', 'mode', 'delay', 'fps']) if (s[k] !== undefined) $(k).value = s[k];
  for (const k of ['invert', 'swaprb', 'spi3wire']) if (s[k] !== undefined) $(k).checked = s[k] === '1';
  if (s.pins) PIN_NAMES.forEach(p => { if (s.pins[p] !== undefined) $('pin_' + p).value = s.pins[p]; });
}

function formData() {
  const d = {
    model: $('model').value, res: $('res').value, rot: $('rot').value,
    freq: $('freq').value, mode: $('mode').value,
    invert: $('invert').checked ? '1' : '0', swaprb: $('swaprb').checked ? '1' : '0',
    spi3wire: $('spi3wire').checked ? '1' : '0',
    brightness: $('brightness').value,
    delay: $('delay').value, fps: $('fps').value,
  };
  PIN_NAMES.forEach(p => d[p] = $('pin_' + p).value);
  return d;
}

async function startTest(endpoint) {
  const d = formData();
  save({ ...d, pins: Object.fromEntries(PIN_NAMES.map(p => [p, d[p]])), pinDefaults: cfg.pins });
  const r = await fetch(endpoint, { method: 'POST', body: new URLSearchParams(d) });
  const j = await r.json();
  if (!j.ok) { showMessage(j.error, 'err'); return; }
  startPolling();
}

async function stopTest() {
  await fetch('/api/stop', { method: 'POST' });
}

// Suwak jasnosci - wysyla wartosc na biezaco (max co 100 ms), dziala tez w trakcie testu
let brightnessTimer = null, brightnessSynced = false;
function showBrightness(v) {
  $('brightness').value = v;
  $('brightnessValue').textContent = v + ' %';
}
function onBrightnessInput() {
  $('brightnessValue').textContent = $('brightness').value + ' %';
  if (brightnessTimer) return;
  brightnessTimer = setTimeout(async () => {
    brightnessTimer = null;
    await fetch('/api/brightness', { method: 'POST', body: new URLSearchParams({ value: $('brightness').value }) });
  }, 100);
}

function showMessage(text, cls) {
  $('message').textContent = text;
  $('message').className = cls || '';
}

async function updateStatus() {
  let s;
  try { s = await (await fetch('/api/status')).json(); } catch (e) { return; }
  const running = s.state === 'running';
  $('start').disabled = running;
  $('pattern').disabled = running;
  $('readid').disabled = running;
  $('idInfo').hidden = !s.idInfo;
  $('idInfo').textContent = s.idInfo;
  $('stop').disabled = !running;

  const total = 3 * cfg.phaseSeconds * 1000;
  let done = 0;
  if (running && s.phase > 0) done = (s.phase - 1) * cfg.phaseSeconds * 1000 + s.phaseElapsedMs;
  else if (s.state === 'done') done = total;
  $('progress').style.width = Math.min(100, done / total * 100) + '%';

  let text = s.message;
  if (running && s.phase > 0) text += ` (${Math.floor(s.phaseElapsedMs / 1000)} / ${cfg.phaseSeconds} s)`;
  showMessage(text, s.state === 'done' ? 'ok' : s.state === 'error' ? 'err' : '');

  s.results.forEach((r, i) => {
    $('f' + (i + 1)).textContent = r.frames || '-';
    $('s' + (i + 1)).textContent = r.frames ? r.fps.toFixed(1) : '-';
  });
  $('heap').textContent = `Wolna pamięć: ${(s.freeHeap / 1024).toFixed(0)} kB`;
  if (!brightnessSynced) { showBrightness(s.brightness); brightnessSynced = true; }

  if (!running && polling) { clearInterval(polling); polling = null; }
}

function startPolling() {
  if (!polling) polling = setInterval(updateStatus, 500);
  updateStatus();
}

$('model').addEventListener('change', () => fillResolutions(true));
$('start').addEventListener('click', () => startTest('/api/start'));
$('pattern').addEventListener('click', () => startTest('/api/pattern'));
$('readid').addEventListener('click', () => startTest('/api/readid'));
$('stop').addEventListener('click', stopTest);
$('brightness').addEventListener('input', onBrightnessInput);
$('pinReset').addEventListener('click', e => {
  e.preventDefault();
  PIN_NAMES.forEach(p => $('pin_' + p).value = cfg.pins[p]);
});

loadConfig().then(startPolling).catch(e => showMessage('Błąd połączenia z ESP: ' + e, 'err'));
</script>
</body>
</html>
)rawhtml";
