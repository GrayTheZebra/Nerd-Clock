# Firmware über WLAN aktualisieren

Nerd-Clock verwendet die im Arduino-R4-Boardpaket enthaltene **OTAUpdate**-Bibliothek. Die Firmware wird auf dem WLAN-Modul zwischengespeichert und von dort auf den RA4M1 übertragen. Damit funktioniert OTA auch bei unserem Sketch mit mehr als 128 KiB. Es wird keine zusätzliche ArduinoOTA-Bibliothek installiert.

## Erste Einrichtung

1. Die Version mit OTA einmal per USB aufspielen: `firmware/Nerd_Clock/Nerd_Clock.ino`.
2. Die Weboberfläche öffnen. Unter **Firmware-Update (OTA)** stehen Sketchversion und WLAN-Firmwareversion.
3. Für den nicht blockierenden Download wird WLAN-/USB-Bridge-Firmware **ab 0.5.0** benötigt. Bei einer älteren Version zeigt die Oberfläche den Grund und sperrt den Download. Das Arduino-Boardpaket auf dem PC aktualisiert diese Firmware auf dem Board nicht automatisch. Beim Freenove-Nachbau die zum konkreten Board passende Anleitung für das WLAN-Firmwareupdate verwenden.

Es gibt keinen Arduino-IDE-Netzwerkport. Updates werden über die Weboberfläche angefordert. Die Uhr braucht eine WLAN-Verbindung; AP-Einrichtung allein reicht nicht.

## Eine neue Firmware bauen und bereitstellen

In der Arduino IDE **Sketch → Kompilierte Binärdatei exportieren** für das Board **Arduino UNO R4 WiFi** verwenden. Die benötigte Datei endet auf `.ino.bin`. Keine ESP32-Firmware, HEX-Datei oder ELF-Datei verwenden.

Der eigene Python-Helfer benötigt nur Python 3 und die Standardbibliothek:

```sh
python3 tools/make_ota.py PFAD/Nerd_Clock.ino.bin Nerd-Clock.ota --serve
```

Er erzeugt das Arduino-OTA-Paket mit R4-Kennung, CRC32 und einem kompatiblen LZSS-Datenstrom. Der portable Encoder verwendet ausschließlich Literale; dadurch wird die Datei etwa 12,5 % größer als die Rohfirmware. Es sind keine Compiler oder Python-Pakete für die Konvertierung nötig.

Der Helfer startet einen HTTP-Server auf Port 8000, der ausschließlich diese eine Firmware-Datei anbietet. Die URL ist:

```text
http://IP-DEINES-PCs:8000/Nerd-Clock.ota
```

Die IP des PCs verwenden, nicht `localhost`. Die Uhr muss den PC erreichen können; gegebenenfalls TCP-Port 8000 in der lokalen Firewall erlauben. Den Server bis zum Abschluss des Downloads laufen lassen, danach mit Strg+C beenden.

Eine bereits vorhandene `.ota`-Datei kann direkt angeboten werden:

```sh
python3 tools/make_ota.py PFAD/Nerd-Clock.ota --serve
```

Alternativ die `.ota` auf einem eigenen HTTP-Server im Heimnetz bereitstellen. Der Server muss HTTP 200 mit korrekter `Content-Length` liefern. Direkte Dateiadresse verwenden, ohne Weiterleitung, Anmeldung oder Query-Parameter. Diese Version unterstützt lokale **HTTP-URLs**, keine HTTPS-URLs und keinen Datei-Upload im Browser.

Arduino CLI:

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --output-dir build firmware/Nerd_Clock
python3 tools/make_ota.py build/Nerd_Clock.ino.bin build/Nerd-Clock.ota --serve
```

GitHub Actions erzeugt ebenfalls `.bin` und `.ota` als Build-Artefakt. Das ZIP enthält zusätzlich ein lokal kompiliertes OTA-Paket unter `release/`; es lässt sich nach der ersten USB-Installation zum Testen des OTA-Ablaufs verwenden.

## Installation in der Uhr

1. URL im Bereich **Firmware-Update (OTA)** eintragen.
2. **Herunterladen und prüfen** anklicken. Fortschritt und Fehler erscheinen darunter. Uhr, Timer und Weboberfläche laufen während des Downloads weiter; einzelne Modemaufrufe können kurz warten.
3. Nach **geprüft – bereit zur Installation** auf **Geprüfte Firmware installieren und neu starten** klicken. Bis dahin kann die Datei mit **Abbrechen / verwerfen** verworfen werden.
4. Beim eigentlichen Übertragen wird die Anzeige ausgeschaltet und MQTT meldet offline. **Die Versorgung bis zum Neustart eingeschaltet lassen.** Die Seite kann vorübergehend nicht erreichbar sein. Danach neu laden.

WLAN-Profile und App-Einstellungen bleiben bei einem normalen Sketchupdate erhalten. Aktive Timer, Text, Datum und Alarm werden nach dem Neustart nicht fortgesetzt. Das Update ersetzt den Sketch auf dem RA4M1, nicht die Firmware des WLAN-Moduls.

Die Prüfung kontrolliert Arduino-R4-Dateikennung und CRC32; sie ist keine kryptografische Signatur und garantiert nicht, dass fremde Firmware zur Verdrahtung passt. Nur eigene oder vertrauenswürdige Nerd-Clock-Firmware verwenden. Wie die übrige Weboberfläche ist OTA im lokalen Heimnetz ohne eigene Anmeldung erreichbar.

## Fehler und Wiederherstellung

- Kein Downloadfortschritt für 30 Sekunden oder Gesamtdauer über 5 Minuten: Abbruch mit Fehlermeldung.
- Download-/CRC-Fehler: Es wird nicht installiert; erneut herunterladen.
- WLAN getrennt: Vorgang abbrechen und nach Wiederverbindung erneut starten.
- Bei einem Fehler vor dem Flashen läuft der bisherige Sketch weiter. Bei unterbrochener Installation oder einer ungeeigneten Firmware kann eine Wiederherstellung per USB nötig sein; automatische Rückkehr zur alten Version gibt es nicht.

Technischer Status: `GET /api/ota`. Bedienung: `POST /api/ota` mit Formulardaten `action=download&url=...`, `action=install` oder `action=cancel`. Die Installationsanforderung wird erst nach der HTTP-Antwort ausgeführt.

## Quellen

- [Offizielles OTAUpdate-Beispiel](https://github.com/arduino/ArduinoCore-renesas/tree/main/libraries/OTAUpdate/examples/OTANonBlocking)
- [Arduino-OTA-Dateiformat](https://github.com/arduino-libraries/ArduinoIoTCloud/blob/master/extras/tools/bin2ota.py)
- [Arduino-LZSS-Format](https://github.com/arduino-libraries/ArduinoIoTCloud/blob/master/extras/tools/lzss.c)
- [ArduinoOTA: Grenze des internen Zwischenspeichers](https://github.com/JAndrassy/ArduinoOTA#installation)
