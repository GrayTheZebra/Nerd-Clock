# Beiträge

Bitte vor einem Pull Request `./tests/run.sh` ausführen und den Sketch für `arduino:renesas_uno:unor4wifi` kompilieren. Geänderte MQTT-Befehle, Einstellungen und Hardwareannahmen auch in README bzw. `docs/` dokumentieren.

LED-Pinbelegung und neunte 595-Taktflanke sind spezifisch für diese Platine. Netzwerkzugriffe, String-Verarbeitung und EEPROM-Schreiben gehören nicht in den Display-Interrupt. Monotone Zeitabläufe mit `millis()` und überlaufsicherer unsigned Subtraktion implementieren. Persistente Layoutänderungen brauchen eine Migration.

Die Tests verwenden simulierte Arduino-Schnittstellen. Änderungen an GPIO-/Timer-Code zusätzlich auf realer Hardware prüfen und klar zwischen Build, Host-Test und Hardware-Test unterscheiden.
