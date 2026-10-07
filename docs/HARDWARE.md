# Verdrahtung der reverse-engineerten Uhr

Diese Firmware richtet sich an die im Projekt vermessene Platine, nicht an beliebige Nerd-Clocken. Holtek **HT48R066 aus dem Sockel entfernen**. Pinzählung bei DIP-Gehäusen beginnt an der Kerbe bei Pin 1 und läuft gegen den Uhrzeigersinn.

Die Uhrplatine erhält 12 V; am 74HC595 liegen gemessene 5 V. Der R4 wird separat über USB versorgt. Gemeinsame Masse verbinden; **R4-5V und Platinenversorgung nicht verbinden**. Die acht Ring-Anodenleitungen besitzen zusätzlich zu den Platinenwiderständen je einen **1-kΩ-Serienwiderstand** zum Arduino.

## Arduino zu Holtek-Sockel

| Arduino | Sockel-Pin | Signal |
|---|---|---|
| D2 | 17 | 595 Daten, DS Pin 14 |
| D3 | 18 | 595 gemeinsamer Schiebe-/Latch-Takt, Pins 11 und 12 |
| D4 | 16 | T1 |
| D5 | 15 | T2 |
| D6 | 14 | T3 |
| D7 | 13 | T4 |
| D8 | 12 | T5 |
| D9 | 11 | T6 |
| D10 | 10 | T7 |
| D11 | 9 | T8 |
| A0 | 19 | Ring-Anodenleitung 0, zusätzlicher 1 kΩ |
| A1 | 1 | Ring-Anodenleitung 1, zusätzlicher 1 kΩ |
| A2 | 2 | Ring-Anodenleitung 2, zusätzlicher 1 kΩ |
| A3 | 3 | Ring-Anodenleitung 3, zusätzlicher 1 kΩ |
| A4 | 4 | Ring-Anodenleitung 4, zusätzlicher 1 kΩ |
| A5 | 6 | Ring-Anodenleitung 5, zusätzlicher 1 kΩ |
| D12 | 7 | Ring-Anodenleitung 6, zusätzlicher 1 kΩ |
| D13 | 8 | Ring-Anodenleitung 7, zusätzlicher 1 kΩ |
| GND | gemeinsame Platinenmasse / 595 Pin 8 | Masse |

T1–T8 haben vorhandene 2-kΩ-Basiswiderstände; ihre Emitter liegen an GND. Ungerade Transistoren schalten `a,b,f,g` ihrer Ziffer, gerade Transistoren `c,d,e`. T1/T2 gehören Z1, T3/T4 Z2, T5/T6 Z3 und T7/T8 Z4. Oberer Doppelpunkt: T3; unterer: T4.

## 74HC595

| Pin | Funktion in dieser Uhr |
|---|---|
| 1 / QB | a und e, Byte-Bit 1 |
| 2 / QC | unterer Doppelpunkt, Bit 2 |
| 3 / QD | f und c, Bit 3 |
| 4 / QE | oberer Doppelpunkt, Bit 4 |
| 5 / QF | b und d, Bit 5 |
| 6 / QG | nicht belegt |
| 7 / QH | g, Bit 7 |
| 8 | GND |
| 10 | /MR fest an VCC |
| 11, 12 | miteinander verbunden, Arduino D3 |
| 13 | /OE fest an GND |
| 14 | Daten, Arduino D2 |
| 15 / QA | nicht belegt |
| 16 | gemessene 5 V |

Segment-LEDs sind parallel geschaltet; vorhandene Widerstände liegen bei etwa 52 Ω. Weil Shift- und Latch-Takt verbunden sind, sendet die Firmware **acht Datenbits MSB zuerst und eine neunte Taktflanke**. Während des Schiebens sind alle Gruppen abgeschaltet.

## Ring

R0 liegt oben bei 12 Uhr, R1–R59 folgen im Uhrzeigersinn. R1–R8 an T8, R9–R16 an T7, weiter abwechselnd bis R49–R56 an T2. Die Anodenreihenfolge wechselt zwischen `19,1,2,3,4,6,7,8` und der Umkehrung. R57/R58/R59 an T1 nutzen Sockel-Pins 8/7/6, R0 Pin 4.

Die externe Anzeige scannt acht Gruppen mit 1 ms Gruppenzeit. Bei Helligkeit 100 % beträgt die nominelle Einschaltzeit 750 µs pro Gruppe, entsprechend 9,375 % Gesamtduty. Kleinere Helligkeiten verteilen Bruchteile von 125-µs-Phasen. Sehr niedrige Werte können sichtbar flimmern. Die Statusmatrix verwendet einen eigenen Timer; ihr Scan ist vom externen Display getrennt.
