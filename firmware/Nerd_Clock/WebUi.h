// SPDX-License-Identifier: MIT
#pragma once

const char APP_HTML[] = R"HTML(<!doctype html><html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Nerd-Clock</title><style>
:root{color-scheme:dark}body{font:16px system-ui;background:#17191d;color:#eee;max-width:780px;margin:auto;padding:20px}section{background:#23262d;padding:20px;border-radius:12px;margin:18px 0}h1,h2{margin-top:0}label{display:block;margin-top:14px}input,select,button{font:inherit;padding:10px;border-radius:6px;border:1px solid #666;box-sizing:border-box}input:not([type=checkbox]),select{width:100%;background:#17191d;color:#eee}button{background:#ae3040;color:white;cursor:pointer;margin-top:14px}a{color:#ffc0ca}small{color:#bbb}#digits{font:48px monospace;white-space:pre}.row{display:flex;gap:16px}.row>div{flex:1}#notice{min-height:24px;color:#ffc0ca}.save-feedback{display:block;margin-top:12px;min-height:24px}.save-feedback[data-kind=success]{color:#a4edb1}.save-feedback[data-kind=error]{color:#ffadb5}button:disabled{opacity:.65;cursor:wait}code{overflow-wrap:anywhere}@media(max-width:550px){.row{display:block}}</style></head><body>
<h1>Nerd-Clock</h1><div id="notice" role="status"></div><section><div id="digits">----</div><div id="status">Status wird geladen ...</div><p><a id="wlan" href="#">WLAN einrichten / wechseln</a></p></section>
<section><h2>Anzeige steuern</h2><form id="timerForm"><label>Timer in Sekunden (1–5999, maximal 99:59)</label><div class="row"><div><input id="timer" type="number" min="1" max="5999" value="300" required></div><div><button>Timer starten</button></div></div></form><small>MM:SS · In der letzten Minute blinkt der gesamte Ring. Danach erscheint wieder die Uhr.</small>
<form id="textForm"><label>Ziffern (1–4 Zeichen)</label><input id="text" inputmode="numeric" pattern="[0-9]{1,4}" maxlength="4" placeholder="0032" required><button>Ziffern anzeigen</button></form><button id="clock">Zur Uhr / Timer abbrechen</button> <button id="pause">Pausieren</button> <button id="resume">Fortsetzen</button> <button id="ack">Alarm quittieren</button></section>
<section><h2>Einstellungen</h2><form id="config"><label>Helligkeit: <span id="brightValue">100</span> %</label><input id="brightness" name="brightness" type="range" min="0" max="100" value="100"><small>100 % entspricht der bisherigen Helligkeit; 0 % schaltet die externe Anzeige aus.</small>
<label>Ring-Stil</label><select id="ring" name="ring"><option value="point">Einzelner Sekundenpunkt</option><option value="fill">Sich auffüllender Kreis</option><option value="drain">Sich reduzierender Kreis</option><option value="inverse">Sekundenpunkt aus, alle anderen an</option></select>
<label><input id="timer_progress" type="checkbox"> Timer-Ring als Gesamtfortschritt</label>
<label for="text_duration">Dauer der Zahlenanzeige per Text-Befehl (Sekunden)</label><input id="text_duration" name="text_duration" type="number" min="0" max="3600" required value="0"><small>Gilt für „Ziffern anzeigen“ und MQTT /set/text. Danach erscheint wieder die Uhr. 0 = bis zum nächsten Befehl; gilt ab der nächsten Zahlenanzeige.</small>
<h2 style="margin-top:28px">Nachtmodus</h2><label><input id="night_enabled" type="checkbox"> Nachthelligkeit aktivieren</label>
<div class="row"><div><label>Von</label><input id="night_start" name="night_start" type="time" value="22:00" required></div><div><label>Bis</label><input id="night_end" name="night_end" type="time" value="07:00" required></div></div>
<label>Helligkeit nachts (%)</label><input id="night_brightness" name="night_brightness" type="number" min="0" max="100" value="10" required><label><input id="night_ring_off" type="checkbox"> Sekundenring nachts ausschalten</label><small>Wirkt nach NTP-Synchronisierung. Timeralarm bleibt aktiv. Gleiche Start-/Endzeit bedeutet ganztägig.</small>
<label>NTP-Server</label><input id="ntp" name="ntp" maxlength="63" required placeholder="de.pool.ntp.org"><small>Hostname oder IPv4-Adresse; lokale Zeit Europe/Berlin mit Sommerzeit.</small>
<h2 style="margin-top:28px">MQTT</h2><label>Broker (leer = MQTT deaktiviert)</label><input id="host" name="host" maxlength="63" placeholder="192.168.1.10"><div class="row"><div><label>Port</label><input id="port" name="port" type="number" min="1" max="65535" value="1883" required></div><div><label>Benutzername</label><input id="user" name="user" maxlength="63" autocomplete="username"></div></div>
<label>Passwort</label><input id="password" name="password" type="password" maxlength="63" autocomplete="new-password"><small id="passwordHint">Leer lassen, um das gespeicherte Passwort zu behalten.</small><label><input type="checkbox" name="clear_password" value="1"> Gespeichertes Passwort löschen</label>
<label><input id="discovery" type="checkbox" checked> Home-Assistant-Autodiscovery</label><label>Basis-Topic</label><input id="base" name="base" maxlength="63" value="nerd-clock" required pattern="[A-Za-z0-9_-]+(/[A-Za-z0-9_-]+)*"><button id="saveSettings" type="submit">Alle Einstellungen speichern</button><span id="saveNotice" class="save-feedback" role="status" aria-live="polite" aria-atomic="true"></span></form></section>
<section><h2>Firmware-Update (OTA)</h2><p id="otaStatus" role="status">OTA-Status wird geladen …</p><form id="otaForm"><label for="otaUrl">Firmware-URL (.ota)</label><input id="otaUrl" type="url" required maxlength="383" placeholder="http://192.168.1.20:8000/Nerd-Clock.ota"><button id="otaDownload">Herunterladen und prüfen</button></form><button id="otaInstall" type="button" disabled>Geprüfte Firmware installieren und neu starten</button> <button id="otaCancel" type="button">Abbrechen / verwerfen</button><p id="otaNotice" role="status"></p><small>Lokale HTTP-URL einer für UNO R4 WiFi erzeugten .ota-Datei. Nach der Prüfung separat installieren. WLAN-Firmware ab 0.5.0 erforderlich. Während der Installation Versorgung eingeschaltet lassen. Anleitung: docs/OTA.md.</small></section>
<section><h2>Home Assistant</h2><p id="discoveryStatus" role="status">Discovery-Status wird geladen …</p><button id="resendDiscovery" type="button">HA-Discovery erneut senden</button><p><small>HA und Uhr müssen denselben Broker verwenden. Discovery-Präfix in HA: homeassistant. „Gesendet“ bestätigt die lokale Übertragung, nicht die Erkennung durch HA.</small></p></section>
<section><h2>MQTT-Befehle</h2><p>Basis-Topic: <code id="topic">nerd-clock</code></p><p><code>/set/timer</code>: Sekunden, z. B. <code>300</code><br><code>/set/text</code>: Ziffern, z. B. <code>0032</code><br><code>/set/clock</code>: beliebiger Inhalt, z. B. <code>1</code><br><code>/set/brightness</code>: <code>0</code>–<code>100</code><br><code>/set/ring</code>: <code>point</code>, <code>fill</code>, <code>drain</code>, <code>inverse</code><br><code>/set/ntp</code>: NTP-Hostname<br><code>/set/date_duration</code>: 1–3600 Sekunden (Standard 5)<br><code>/set/date</code>: 1 = Datum anzeigen, 0 = ausblenden<br><code>/set/pause</code>, <code>/set/resume</code>, <code>/set/ack</code>: beliebiger Inhalt</p><small>Befehle ohne Retain senden. Status unter /state, online/offline unter /availability. Neue Timer-/Text-Befehle ersetzen den aktuellen Modus. Datum ausschließlich per MQTT, nur im Uhrmodus und nach Zeitsynchronisierung. Bei Timerende folgen 10 Sekunden Doppelblitze.</small></section>
<script>
const el=id=>document.getElementById(id);const note=t=>el('notice').textContent=t;
let otaRestartExpected=false;
async function otaAction(action){try{
 const data={action};if(action==='download')data.url=el('otaUrl').value;
 await post('/api/ota',data);otaRestartExpected=action==='install';
 el('otaNotice').textContent=action==='install'?'Installation angefordert. Die Uhr startet neu; Seite danach neu laden.':action==='download'?'Download angefordert. Status wird aktualisiert.':'OTA verworfen.';
 await refreshOta();
}catch(e){el('otaNotice').textContent=e.message;}}
el('otaForm').onsubmit=e=>{e.preventDefault();otaAction('download');};
el('otaInstall').onclick=()=>otaAction('install');el('otaCancel').onclick=()=>otaAction('cancel');
async function refreshOta(){try{
 const r=await fetch('/api/ota');if(!r.ok)throw Error('OTA-Status nicht erreichbar');const s=await r.json();
 const labels={idle:'bereit',queued:'Download startet',downloading:'Download',ready:'geprüft – bereit zur Installation',install_queued:'Installation startet',installing:'Installation / Neustart',error:'Fehler'};
 const busy=['queued','downloading','install_queued','installing'].includes(s.stage);
 const progress=s.total>0?' · '+Math.min(100,Math.floor(s.received*100/s.total))+' %':'';
 el('otaStatus').textContent='Firmware '+s.version+' · WLAN-Firmware '+s.wifi_firmware+' · '+(labels[s.stage]||s.stage)+progress+(s.error?' · '+s.error:'')+(!s.supported?' · OTA benötigt WLAN-Firmware ab 0.5.0':'');
 el('otaDownload').disabled=busy||s.stage==='ready'||!s.supported||!s.connected;
 el('otaInstall').disabled=s.stage!=='ready'||!s.connected;
 el('otaCancel').disabled=['install_queued','installing'].includes(s.stage);
 if(otaRestartExpected&&s.stage==='idle'){otaRestartExpected=false;el('otaNotice').textContent='Uhr wieder erreichbar. Seite neu laden, um die neue Weboberfläche zu öffnen.';}
}catch(e){el('otaStatus').textContent=otaRestartExpected?'Installation / Neustart – warte auf die Uhr …':'OTA-Status nicht erreichbar';}}
el('resendDiscovery').onclick=()=>command('discovery_resend','1');
el('wlan').href='http://'+location.hostname+':8080/';
el('brightness').oninput=()=>el('brightValue').textContent=el('brightness').value;
async function post(path,data){const r=await fetch(path,{method:'POST',body:new URLSearchParams(data)});const j=await r.json();if(!r.ok)throw Error(j.error||'Fehler');return j;}
async function command(cmd,value){try{await post('/api/command',{command:cmd,value});note('Befehl ausgeführt.');await refresh();}catch(e){note(e.message);}}
el('timerForm').onsubmit=e=>{e.preventDefault();command('timer',el('timer').value);};el('textForm').onsubmit=e=>{e.preventDefault();command('text',el('text').value);};el('clock').onclick=()=>command('clock','1');['pause','resume','ack'].forEach(k=>el(k).onclick=()=>command(k,'1'));
const saveNote=(text,kind)=>{el('saveNotice').textContent=text;el('saveNotice').dataset.kind=kind;};
el('config').onsubmit=async e=>{
 e.preventDefault();const button=el('saveSettings');if(button.disabled)return;
 button.disabled=true;button.textContent='Wird gespeichert …';saveNote('Wird gespeichert …','pending');
 try{
  const data=new FormData(e.target);['night_enabled','night_ring_off','timer_progress','discovery'].forEach(k=>data.set(k,el(k).checked?'1':'0'));
  const result=await post('/api/settings',data);if(result.ok!==true)throw Error('Speichern wurde nicht bestätigt.');
  el('password').value='';saveNote('✓ Einstellungen erfolgreich gespeichert.','success');
  try{await load();}catch(err){saveNote('✓ Einstellungen gespeichert. Erneutes Laden fehlgeschlagen: '+err.message,'error');}
 }catch(err){saveNote('Speichern nicht bestätigt: '+err.message+' Bitte erneut versuchen.','error');}
 finally{button.disabled=false;button.textContent='Alle Einstellungen speichern';}
};
async function load(){const r=await fetch('/api/settings');if(!r.ok)throw Error('Einstellungen nicht erreichbar');const s=await r.json();['brightness','ring','ntp','host','port','user','base','night_start','night_end','night_brightness','text_duration'].forEach(k=>el(k).value=s[k]);['night_enabled','night_ring_off','timer_progress','discovery'].forEach(k=>el(k).checked=!!s[k]);el('brightValue').textContent=s.brightness;el('topic').textContent=s.base;el('passwordHint').textContent=s.password_saved?'Passwort gespeichert. Leer lassen, um es zu behalten.':'Kein Passwort gespeichert.';el('config').elements.clear_password.checked=false;}
async function refresh(){try{const r=await fetch('/api/state');const s=await r.json();el('digits').textContent=(s.mode==='timer'||s.mode==='date'||(s.mode==='clock'&&s.ntp_synced))?(s.display.slice(0,2)+':'+s.display.slice(2)).trimStart():s.display;el('status').textContent=(s.mode==='clock'?'Uhr':s.mode==='timer'?(s.timer_paused?'Timer pausiert':'Timer'):s.mode==='date'?'Datum':'Ziffern')+(s.alarm_active?' · Timeralarm':'')+' · NTP: '+(s.ntp_synced?'synchronisiert':'wartet')+' · MQTT: '+(s.mqtt_connected?'verbunden':'aus / getrennt');}catch(e){el('status').textContent='Verbindung zur Uhr unterbrochen';}}
async function refreshDiscovery(){try{
 const r=await fetch('/api/mqtt');if(!r.ok)throw Error('Status nicht erreichbar');const s=await r.json();
 const connection=s.connected?'verbunden':'getrennt (Code '+s.connection_code+')';
 const discovery=!s.enabled?'deaktiviert':!s.connected?'wartet auf MQTT':s.failed?'Senden fehlgeschlagen – erneuter Versuch folgt':s.pending?'wird gesendet: '+s.sent+'/'+s.total:s.sent===s.total?'gesendet: '+s.sent+'/'+s.total:'wartet';
 el('discoveryStatus').textContent='MQTT: '+connection+' · HA-Discovery: '+discovery+' · Online-Signal: '+(!s.connected?'wartet':s.availability_pending?'wartet / Wiederholung':s.availability_sent?'gesendet':'noch nicht gesendet')+' · Topic: '+s.prefix+'/…/'+s.node_id+'/…/config · Basis: '+s.base;
 el('resendDiscovery').disabled=!s.connected||!s.enabled||s.pending;
}catch(e){el('discoveryStatus').textContent='Discovery-Status nicht erreichbar';}}
load().catch(e=>note(e.message));refresh();refreshDiscovery();refreshOta();setInterval(refreshOta,1500);setInterval(refresh,1000);setInterval(refreshDiscovery,3000);
</script></body></html>)HTML";
