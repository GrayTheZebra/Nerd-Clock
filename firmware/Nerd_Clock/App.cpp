// SPDX-License-Identifier: MIT
#include "App.h"

void serviceSerial() {
  while (Serial.available()) {
    const char c = char(Serial.read());
    if (c == '\n' || c == '\r') {
      serialCommand[serialUsed] = 0;
      if (!strcmp(serialCommand, "resetwifi")) {
        // Explicit command deletes the manager's saved networks and reboots.
        Serial.println(F("Clearing WLAN settings and restarting into setup mode."));
        wifiManager.resetSettings();
        NVIC_SystemReset();
      }
      serialUsed = 0;
    } else if (size_t(serialUsed)+1 < sizeof(serialCommand)) serialCommand[serialUsed++] = c;
    else serialUsed = 0;
  }
}

void appSetup() {
  for (uint8_t pin : GROUP_PINS) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  for (uint8_t pin : RING_PINS) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  digitalWrite(DATA_PIN, LOW);
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(CLOCK_PIN, LOW);
  pinMode(CLOCK_PIN, OUTPUT);
  write595(0);
  Serial.begin(115200);
  Serial.println(F("Nerd-Clock: waiting for NTP; timer scan, 75% group brightness."));
  Serial.println(F("Local time: Europe/Berlin. Serial command: resetwifi"));
  loadSettings();
  secondTick = millis();
  publishDisplay();
  displayRunning = startDisplayTimer();
  if (!displayRunning) {
    allOff();
    write595(0);
    Serial.println(F("ERROR: display timer could not be started; outputs off."));
  } else Serial.println(F("Display timer running."));
  // Each display allocates its own FspTimer. Start both before networking.
  matrixReady = matrix.begin() != 0;
  if (matrixReady) clearMatrix();
  else Serial.println(F("ERROR: matrix timer could not be started."));
  mqtt.setCallback(mqttCallback);
  if(!mqtt.setBufferSize(2048))Serial.println(F("ERROR: MQTT buffer allocation failed."));
  mqtt.setSocketTimeout(2);
  mqtt.setKeepAlive(20);
  configureMqtt(false);
  wifiManager.setPort(8080); // Dedicated WiFi portal; application UI is port 80.
  wifiManager.setAPCallback(onSetupAP);
  // Allows changing WLAN settings at the displayed IP after connection.
  wifiManager.setKeepServerAlive(true);
  wifiManager.autoConnect(SETUP_SSID, SETUP_PASSWORD);
  uint8_t mac[6];WiFi.macAddress(mac);
  snprintf(mqttClientID,sizeof(mqttClientID),"nerd-clock-%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  // Keep discovery IDs stable across the project rename: existing HA entities survive.
  snprintf(discoveryID,sizeof(discoveryID),"ledclock-%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  setupOta();
  appServer.begin();

}

void appLoop() {
  if(otaInstalling())return; // Bridge is programming/resetting the RA4M1.
  const uint32_t now = millis();
  while (uint32_t(now-secondTick) >= 1000) {
    secondTick += 1000;
    if (timeSynced) {
      ++utcEpoch;
      secondsOfDay = berlinSeconds(utcEpoch);
    }
  }
  serviceSerial();
  serviceNetwork();
  serviceMatrix();
  serviceWeb();
  serviceModes();
  if (displayDirty || secondsOfDay != renderedTime) publishDisplay();
  serviceMqtt();
  // MQTT callbacks may have changed the active mode immediately.
  serviceOta();
  serviceModes();
  if (displayDirty) publishDisplay();
  if(settingsDirty && uint32_t(millis()-settingsChanged)>=2000) {
    if(saveSettings()) settingsDirty=false;
    else {settingsChanged=millis();Serial.println(F("ERROR: saving settings failed."));}
  }
}

