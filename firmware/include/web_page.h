/*
 * GENERATED FILE - DO NOT EDIT BY HAND.
 *
 * Produced by tools/embed_page.py from panel/index.html.
 * Edit the panel, then re-run the script.
 *
 * License: MIT
 */

#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char COMMAND_CENTRE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>UNICORN COMMAND CENTRE</title>
<style>
  :root {
    --green: #00ff88;
    --red: #ff3355;
    --blue: #34a0ff;
    --amber: #fbc531;
    --magenta: #ff5ce1;
    --dim: #7a8b9a;
    --line: rgba(0, 255, 136, 0.28);
  }
  * { box-sizing: border-box; }
  body {
    font-family: "Courier New", Courier, monospace;
    background: #060b10;
    color: var(--green);
    margin: 0;
    padding: 18px;
  }
  h1 {
    font-size: 1.1rem;
    letter-spacing: 5px;
    text-align: center;
    margin: 0 0 4px;
    text-shadow: 0 0 12px rgba(0, 255, 136, 0.5);
  }
  .sub {
    text-align: center;
    color: var(--dim);
    font-size: 0.72rem;
    letter-spacing: 2px;
    margin-bottom: 18px;
  }
  .badge {
    display: inline-block;
    padding: 2px 8px;
    border: 1px solid currentColor;
    border-radius: 3px;
    font-size: 0.66rem;
    letter-spacing: 1px;
  }
  .badge--device { color: var(--green); }
  .badge--sim { color: var(--amber); }
  .badge--offline { color: var(--dim); }
  .grid { display: flex; flex-wrap: wrap; gap: 14px; justify-content: center; }
  .panel {
    border: 1px solid var(--line);
    background: rgba(0, 255, 136, 0.04);
    border-radius: 6px;
    padding: 16px;
    min-width: 290px;
    flex: 1;
  }
  .panel h2 {
    margin: 0 0 12px;
    font-size: 0.74rem;
    letter-spacing: 2px;
    color: var(--dim);
    font-weight: normal;
  }
  .panel--danger { border-color: var(--red); background: rgba(255, 51, 85, 0.06); }
  .panel--ack { border-color: var(--blue); background: rgba(52, 160, 255, 0.06); }
  .wide { width: 100%; max-width: 960px; margin: 14px auto 0; }
  .state { font-size: 1.1rem; font-weight: bold; margin-bottom: 10px; }
  .row { display: flex; justify-content: space-between; font-size: 0.82rem; color: var(--dim); padding: 4px 0; }
  .row b { color: #cfe8dd; font-weight: normal; }
  .meter { height: 10px; border: 1px solid var(--line); border-radius: 3px; margin-top: 6px; position: relative; overflow: hidden; }
  .meter-fill { height: 100%; width: 0%; background: var(--green); transition: width 0.15s linear; }
  .meter-mark { position: absolute; top: -3px; bottom: -3px; width: 2px; background: var(--amber); }
  .btn {
    width: 100%;
    background: transparent;
    border: 1px solid var(--green);
    color: var(--green);
    padding: 12px;
    margin-top: 8px;
    font-family: inherit;
    font-size: 0.8rem;
    font-weight: bold;
    letter-spacing: 1px;
    cursor: pointer;
    text-transform: uppercase;
  }
  .btn:hover { background: var(--green); color: #000; }
  .btn--danger { border-color: var(--red); color: var(--red); }
  .btn--danger:hover { background: var(--red); color: #fff; }
  .btn--ack { border-color: var(--blue); color: var(--blue); }
  .btn--ack:hover { background: var(--blue); color: #fff; }
  .btn--dim { border-color: var(--dim); color: var(--dim); }
  .btn--dim:hover { background: var(--dim); color: #000; }
  .pad { text-align: center; margin-top: 6px; }
  .pad button {
    width: 74px; padding: 9px; margin: 3px;
    background: transparent; border: 1px solid var(--line);
    color: var(--green); font-family: inherit; cursor: pointer; border-radius: 4px;
  }
  .pad button:hover:not(:disabled) { background: var(--green); color: #000; }
  .pad button:disabled { opacity: 0.3; cursor: not-allowed; }
  .word {
    text-align: center;
    font-size: 1.9rem;
    letter-spacing: 12px;
    color: var(--amber);
    border: 1px dashed var(--amber);
    padding: 12px;
    margin: 4px 0 14px;
  }
  .decode { font-size: 0.78rem; line-height: 1.9; color: var(--dim); }
  .decode span { color: #cfe8dd; }
  .decode .on { color: var(--green); }
  .decode .warn { color: var(--red); }
  .lamp { display: inline-block; width: 11px; height: 11px; border-radius: 50%; margin-right: 7px; vertical-align: middle; }
  .note { font-size: 0.68rem; color: var(--dim); margin-top: 12px; line-height: 1.6; }
  @media (max-width: 640px) { .panel { min-width: 100%; } }
</style>
</head>
<body>

<h1>UNICORN COMMAND CENTRE</h1>
<div class="sub">
  SEARCH &amp; RESCUE ROVER &middot; <span id="conn" class="badge badge--offline">CONNECTING</span>
</div>

<div class="grid">

  <div class="panel" id="statusPanel">
    <h2>ROVER STATUS</h2>
    <div class="state" id="stateText"><span class="lamp" id="lamp"></span>—</div>
    <div class="row"><span>Path ahead</span><b id="distText">—</b></div>
    <div class="row"><span>Survivor</span><b id="survText">—</b></div>
    <div class="row"><span>Uptime</span><b id="uptimeText">—</b></div>
  </div>

  <div class="panel">
    <h2>MICROPHONE (MAX9814)</h2>
    <div class="state" id="micText" style="font-size:0.95rem;">—</div>
    <div class="row"><span>Peak-to-peak</span><b id="micVal">—</b></div>
    <div class="meter"><div class="meter-fill" id="micFill"></div><div class="meter-mark" id="micMark"></div></div>
    <div class="note">The bar is the peak-to-peak swing over a 50&nbsp;ms window, not a
      single ADC sample. The marker is the detection threshold.</div>
  </div>

  <div class="panel">
    <h2>POSITION (DEAD RECKONING)</h2>
    <div class="row"><span>Displacement</span><b id="navDisp">—</b></div>
    <div class="row"><span>Heading</span><b id="navHead">—</b></div>
    <div class="row"><span>Ground covered</span><b id="navPath">—</b></div>
    <div class="row"><span>Error radius</span><b id="navErr">—</b></div>
    <div class="row"><span>Speed model</span><b id="navCal">—</b></div>
    <div class="note">No GNSS and no wheel encoders: this is displacement from where
      the rover was put down, integrated from the motion it was commanded to make.
      The error radius only grows — treat the position as a disc, never a point.</div>
  </div>

  <div class="panel">
    <h2>DRIVE</h2>
    <div class="pad">
      <button id="btnFwd" onclick="drive('forward')">FWD</button><br>
      <button id="btnLeft" onclick="drive('left')">LEFT</button>
      <button id="btnStop" onclick="drive('stop')">STOP</button>
      <button id="btnRight" onclick="drive('right')">RIGHT</button><br>
      <button id="btnBack" onclick="drive('back')">BACK</button>
    </div>
    <div class="note" id="driveNote">Manual drive is locked during an alert: the rover
      holds position so four gearmotors are not drowning the microphone.</div>
  </div>

  <div class="panel">
    <h2>PROTOCOLS</h2>
    <button class="btn btn--danger" onclick="post('trigger')">Declare seismic event</button>
    <button class="btn btn--ack" onclick="post('ack')">Acknowledge — help is on the way</button>
    <button class="btn btn--dim" onclick="post('reset')">Reset to idle</button>
  </div>

</div>

<div class="panel wide" id="wordPanel">
  <h2>9-BIT TELEMETRY WORD</h2>
  <div class="word" id="wordText">—</div>
  <div class="decode" id="decode"></div>
  <div class="note">Fixed width with a leading 0 and a trailing 1. A receiver frames the
    word without a length field, and a stuck line (all zeros or all ones) fails the
    marker check instead of decoding as a valid state. This is an encoding, not
    encryption — the link carries no secrets.</div>
</div>

<script>
const $ = id => document.getElementById(id);
let offlineTicks = 0;

function post(path) { fetch('/' + path, { method: 'POST' }).catch(() => {}); }
function drive(dir) { post(dir); }

function fmtUptime(ms) {
  const s = Math.floor(ms / 1000);
  return String(Math.floor(s / 60)).padStart(2, '0') + ':' + String(s % 60).padStart(2, '0');
}

function render(d) {
  const conn = $('conn');
  if (d.source === 'simulator') {
    conn.className = 'badge badge--sim';
    conn.textContent = 'SIMULATOR — NO ROVER CONNECTED';
  } else {
    conn.className = 'badge badge--device';
    conn.textContent = 'LIVE ROVER';
  }

  const panel = $('statusPanel');
  const lamp = $('lamp');
  panel.className = 'panel';

  if (!d.alert) {
    $('stateText').innerHTML = '<span class="lamp"></span>IDLE — CHARGING';
    lamp.style.background = 'var(--magenta)';
    lamp.style.boxShadow = '0 0 10px var(--magenta)';
    $('stateText').firstChild.style.background = 'var(--magenta)';
  } else if (d.centreAck) {
    panel.className = 'panel panel--ack';
    $('stateText').innerHTML = '<span class="lamp"></span>HELP IS ON THE WAY';
  } else {
    panel.className = 'panel panel--danger';
    $('stateText').innerHTML = '<span class="lamp"></span>SEISMIC ALERT';
  }
  const l = $('stateText').querySelector('.lamp');
  const colour = !d.alert ? 'var(--magenta)' : (d.centreAck ? 'var(--blue)' : 'var(--red)');
  l.style.background = colour;
  l.style.boxShadow = '0 0 10px ' + colour;

  $('distText').textContent = d.distance > 0 ? d.distance + ' cm' : 'no echo';
  $('survText').textContent = ['listening', 'responsive', 'silent'][d.survivor] || '—';
  $('uptimeText').textContent = fmtUptime(d.uptime);

  $('navDisp').textContent = 'x ' + d.navX.toFixed(0) + ' cm, y ' + d.navY.toFixed(0) + ' cm';
  $('navHead').textContent = d.navHeading + '\u00B0 from launch';
  $('navPath').textContent = d.navPath + ' cm';
  $('navErr').textContent = '\u00B1 ' + d.navError + ' cm';
  $('navErr').style.color = d.navError > 60 ? 'var(--red)' : '#cfe8dd';
  $('navCal').textContent = d.navCalibrated ? 'measured' : 'derived — uncalibrated';
  $('navCal').style.color = d.navCalibrated ? 'var(--green)' : 'var(--amber)';

  const pct = Math.min(100, (d.micLevel / (d.micThreshold * 2)) * 100);
  $('micFill').style.width = pct + '%';
  $('micFill').style.background = d.micLevel >= d.micThreshold ? 'var(--red)' : 'var(--green)';
  $('micMark').style.left = '50%';
  $('micVal').textContent = d.micLevel + ' / 4095';
  $('micText').textContent = !d.alert ? 'Standby'
    : (d.survivor === 1 ? 'SURVIVOR HEARD' : (d.survivor === 2 ? 'No response' : 'Listening…'));

  ['btnFwd', 'btnLeft', 'btnRight', 'btnBack'].forEach(b => { $(b).disabled = d.alert; });

  $('wordText').textContent = d.word;
  const w = d.word;
  const bit = (v, on, off, cls) => v ? '<span class="' + (cls || 'on') + '">' + on + '</span>'
                                     : '<span>' + off + '</span>';
  $('decode').innerHTML =
    'bit 0 &nbsp; start marker &nbsp;&rarr;&nbsp; <span>[' + w[0] + '] always 0</span><br>' +
    'bit 1 &nbsp; mission &nbsp;&rarr;&nbsp; ' + bit(w[1] === '1', '[1] SEISMIC ALERT', '[0] idle', 'warn') + '<br>' +
    'bits 2-3 &nbsp; survivor &nbsp;&rarr;&nbsp; <span class="' +
      (w.substr(2, 2) === '01' ? 'on' : (w.substr(2, 2) === '10' ? 'warn' : '')) + '">[' +
      w.substr(2, 2) + '] ' + ({ '00': 'listening', '01': 'RESPONSIVE', '10': 'SILENT' }[w.substr(2, 2)] || '?') +
      '</span><br>' +
    'bits 4-5 &nbsp; path ahead &nbsp;&rarr;&nbsp; <span class="' +
      (w.substr(4, 2) === '01' ? 'warn' : '') + '">[' + w.substr(4, 2) + '] ' +
      ({ '00': 'no echo', '01': 'BLOCKED', '11': 'clear' }[w.substr(4, 2)] || '?') + '</span><br>' +
    'bits 6-7 &nbsp; command centre &nbsp;&rarr;&nbsp; ' +
      bit(w.substr(6, 2) === '11', '[11] ACKNOWLEDGED', '[00] waiting') + '<br>' +
    'bit 8 &nbsp; stop marker &nbsp;&rarr;&nbsp; <span>[' + w[8] + '] always 1</span>';
}

function tick() {
  fetch('/status', { cache: 'no-store' })
    .then(r => r.json())
    .then(d => { offlineTicks = 0; render(d); })
    .catch(() => {
      if (++offlineTicks >= 3) {
        const conn = $('conn');
        conn.className = 'badge badge--offline';
        conn.textContent = 'OFFLINE — NO RESPONSE';
      }
    });
}

setInterval(tick, 500);
tick();
</script>
</body>
</html>
)rawliteral";

#endif  // WEB_PAGE_H
