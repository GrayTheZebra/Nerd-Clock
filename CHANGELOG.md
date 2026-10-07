# Changelog

## 1.1.1 – 2026-10-07

- Führende Null der Stunden bzw. Timer-Minuten ausgeblendet: `0:00` statt `00:00`, `9:05` statt `09:05`.
- Webanzeige entsprechend formatiert; MQTT-Anzeigefeld bleibt vierstellig mit Leerzeichen links.

## 1.1.0 – 2026-10-06

- R4-OTA mit der im Boardpaket vorhandenen OTAUpdate-Bibliothek.
- Weboberfläche: Firmware-URL, Downloadfortschritt, Prüfung, separate Installation und Abbruch.
- WLAN-Firmwareversion und Voraussetzungen sichtbar; Fehler, Zeitlimits und Wiederholbarkeit geprüft.
- Python-Helfer für Arduino-R4-OTA-Pakete und lokalen Firmware-HTTP-Server, ohne Zusatzpakete.
- OTA-Anleitung und GitHub-Build-Artefakte ergänzt.

## 1.0.3 – 2026-10-06

- MQTT-Online-Signal nach Discovery, bei HA-Start und alle 60 Sekunden erneut senden.
- Fehlgeschlagenes Availability-Publish alle 2 Sekunden wiederholen.
- Zustand nach Discovery erneut senden; Online-Sendestatus in Web-Diagnose anzeigen.

## 1.0.2 – 2026-10-06

- Projekt, Sketch, Weboberfläche, HA-Gerätename und Setup-AP heißen Nerd-Clock.
- Neuer Standard-Basis-Topic `nerd-clock`; gespeicherte Einstellungen bleiben erhalten.
- HA-IDs bleiben kompatibel, MQTT-Client-ID trägt den neuen Projektnamen.
- Discovery-Status und Button zum erneuten Senden im Web; MQTT-Fehlercode im seriellen Monitor.
- Discovery nach jedem Speichern, bei HA-Start, bei Verbindung und alle 15 Minuten erneut senden.
- Konfigurationen mit 250 ms Abstand senden; Fehler mit 2 Sekunden Abstand erneut versuchen.

## 1.0.1 – 2026-10-06

- Sichtbare Speicherbestätigung direkt am Button, einschließlich Warte- und Fehleranzeige.
- Speichern-Button während der Anfrage gesperrt; Fehler beim Nachladen getrennt vom Speicherergebnis.
- Dauer der Zahlenanzeige verständlicher benannt und direkt im Formular erklärt.

## 1.0.0 – 2026-10-06

Erste strukturierte Repository-Version, basierend auf dem bisherigen Einzeldatei-Sketch.

- Separate C++-Module für Anzeige, NTP, Einstellungen, Befehle, MQTT, Matrix und Web.
- Zehn Sekunden zeitgesteuerte Doppelblitze bei Timerende und MQTT-/HA-Ende-Ereignis.
- Pause/Fortsetzen, Alarm quittieren, zeitlich begrenzte Ziffernanzeige.
- Datum `TT:MM` ausschließlich per MQTT-Flag und MQTT-Anzeigedauer.
- Nachthelligkeit mit lokalem Zeitfenster und optional ausgeschaltetem Sekundenring.
- Gesamtfortschrittsring als Timer-Option; Blinkmuster in letzter Minute bleibt erhalten.
- Home-Assistant-Discovery mit 22 Entitäten und MAC-basierten IDs.
- Automatische Migration bestehender App-Einstellungen, WLAN-Profile unverändert.
- Host-Regressionstests, GitHub-Actions-Build, Verdrahtungs- und MQTT-Dokumentation.
