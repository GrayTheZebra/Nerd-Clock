# MQTT-Schnittstelle

Standardbasis `nerd-clock`, frei konfigurierbar. Ganzzahlen als reine Dezimalzeichenketten. Flags: `1`/`0`, `ON`/`OFF` oder `true`/`false`. Alle Kommandos ohne Retain senden; `clock`, `pause`, `resume` und `ack` ignorieren ihren Inhalt.

| Topic-Suffix | Werte | Wirkung |
|---|---|---|
| `set/timer` | 0–5999 | Timer in Sekunden, 0 = abbrechen |
| `set/pause` | z. B. `1` | Aktiven Timer pausieren |
| `set/resume` | z. B. `1` | Pausierten Timer fortsetzen |
| `set/text` | 0–4 Ziffern | Führende Nullen bleiben; leer = Uhr |
| `set/clock` | z. B. `1` | Sofort Uhr; Timer/Text/Datum/Alarm beenden |
| `set/ack` | z. B. `1` | Abschlussalarm quittieren |
| `set/date_duration` | 1–3600 | Dauer für nächste Datumsanzeige in Sekunden, gespeichert |
| `set/date` | Flag | Datum starten/ausblenden; ausschließlich MQTT |
| `set/text_duration` | 0–3600 | Dauer für nächsten Text, 0 = unbegrenzt |
| `set/brightness` | 0–100 | Normale Helligkeit |
| `set/ring` | `point`, `fill`, `drain`, `inverse` | Sekundenring-Stil |
| `set/timer_progress` | Flag | Gesamtfortschrittsring beim Timer |
| `set/night_enabled` | Flag | Nachtzeitplan aktivieren |
| `set/night_start` | `HH:MM` | Beginn lokaler Nachtzeit |
| `set/night_end` | `HH:MM` | Ende lokaler Nachtzeit |
| `set/night_brightness` | 0–100 | Helligkeit nachts |
| `set/night_ring_off` | Flag | Sekundenring nachts aus |
| `set/ntp` | Hostname oder IPv4 | NTP-Server, sofort neuer Synchronisierungsversuch |
| `set/discovery_resend` | z. B. `1` | Alle 22 Discovery-Konfigurationen erneut senden, wenn verbunden und aktiviert |
| `set/discovery` | Flag | Home-Assistant-Discovery an/aus |

Ungültige Werte und Datumsanforderungen während Timer/Text/Alarm werden abgewiesen. Der serielle Monitor meldet ungültige MQTT-Befehle. Im Web gibt ein ungültiger Befehl HTTP 400 zurück. Es wird immer nur ein Ziffernmodus angezeigt; Datum ist eine zeitlich begrenzte Einblendung im Uhrmodus.

## Rückmeldungen

- `availability`: `online`/`offline`, retained, mit Last Will.
- `state`: JSON, retained; nach Änderungen und ungefähr jede Sekunde.
- `event`: Timerende als JSON, **nicht retained**.

Beispiel für `state` (gekürzt):

```json
{"mode":"timer","display":" 459","timer_seconds":299,"timer_paused":false,"alarm_active":false,"date_active":false,"date_duration":5,"brightness":100,"effective_brightness":100,"ring_style":"point","ntp_synced":true,"mqtt_connected":true}
```

Weitere Felder: `text`, `text_duration`, `night_enabled`, `night_brightness`, `night_ring_off`, `timer_progress`. `mode` ist `clock`, `timer`, `text` oder `date`. `display` hat vier Stellen ohne Doppelpunkt; bei Uhr und Timer ist die erste Stelle unter 10 Stunden bzw. Minuten ein Leerzeichen (`" 905"` entspricht `9:05`); Leerzeichen links bei kurzem Text, `----` vor der ersten Zeitsynchronisierung. Während der Abschlussblitze bleibt `mode=clock`, `alarm_active=true`.

Beispiel für `event`:

```json
{"event_type":"timer_finished","duration_seconds":300,"finished_epoch":1791280800}
```

`finished_epoch` ist Unix-Zeit in UTC, 0 bei fehlender Zeitsynchronisierung. Die vollständige Dauer steht unter `duration_seconds`, nicht die bei einer Pause übrig gebliebene Zeit.

## Beispiel Home-Assistant-Automation

```yaml
alias: Datum an Nerd-Clock zeigen
triggers:
  - trigger: time
    at: "12:00:00"
actions:
  - action: mqtt.publish
    data:
      topic: nerd-clock/set/date_duration
      payload: "5"
      retain: false
  - action: mqtt.publish
    data:
      topic: nerd-clock/set/date
      payload: "1"
      retain: false
```

Alternativ die automatisch erkannten Datums-Entitäten verwenden. Für eine Timer-Ende-Automation die Event-Entität im Home-Assistant-Automationseditor auswählen oder direkt auf `nerd-clock/event` reagieren.
