// SPDX-License-Identifier: MIT
#include "App.h"

uint8_t weekday(int year, uint8_t month, uint8_t day) {
  static const uint8_t table[12] = {0,3,2,5,0,3,5,1,4,6,2,4};
  if (month < 3) --year;
  return (year + year/4 - year/100 + year/400 + table[month-1] + day)%7;
}

uint32_t berlinSeconds(uint32_t epoch) {
  const time_t raw = time_t(epoch);
  const struct tm *date = gmtime(&raw);
  if (!date) return epoch%86400UL;
  const int year = date->tm_year + 1900;
  const uint8_t month = date->tm_mon + 1;
  bool dst = false;
  if (month > 3 && month < 10) dst = true;
  else if (month == 3 || month == 10) {
    const uint8_t lastSunday = 31 - weekday(year, month, 31);
    const bool afterSwitch = date->tm_mday > lastSunday
      || (date->tm_mday == lastSunday && date->tm_hour >= 1);
    dst = month == 3 ? afterSwitch : !afterSwitch;
  }
  return (epoch + (dst ? 7200UL : 3600UL))%86400UL;
}

uint32_t berlinEpoch(uint32_t epoch) {
  const uint32_t offset=(berlinSeconds(epoch)+86400UL-epoch%86400UL)%86400UL;
  return epoch+offset;
}

void requestNtp() {
  lastNtpAttempt = millis();
  ntpAttempted = true;
  if (!udpReady) {
    udpReady = ntpUDP.begin(2390) != 0;
    if (!udpReady) { Serial.println(F("NTP: UDP start failed.")); return; }
  }
  if (WiFi.hostByName(settings.ntp, ntpAddress) != 1) {
    Serial.println(F("NTP: DNS failed; retry in 30 s."));
    return;
  }
  uint8_t packet[48] = {};
  packet[0] = 0x23; // NTP v4 client.
  // Echo token for matching the response's originate timestamp.
  for (uint8_t i = 0; i < 8; ++i) {
    ntpToken[i] = uint8_t(random(1, 256));
    packet[40+i] = ntpToken[i];
  }
  if (ntpUDP.beginPacket(ntpAddress, 123) != 1) return;
  ntpUDP.write(packet, sizeof(packet));
  if (ntpUDP.endPacket() != 1) return;
  ntpSentAt = millis();
  ntpPending = true;
  Serial.println(F("NTP request sent."));
}

void receiveNtp() {
  if (!ntpPending) return;
  const int packetSize = ntpUDP.parsePacket();
  if (packetSize > 0) {
    const IPAddress source = ntpUDP.remoteIP();
    const uint16_t port = ntpUDP.remotePort();
    uint8_t packet[48] = {};
    const int count = ntpUDP.read(packet, sizeof(packet));
    bool valid = packetSize >= 48 && count == 48 && source == ntpAddress && port == 123;
    valid = valid && (packet[0]&7) == 4 && (packet[0]>>6) != 3
      && ((packet[0]>>3)&7) >= 3 && packet[1] >= 1 && packet[1] <= 15;
    for (uint8_t i = 0; i < 8; ++i) if (packet[24+i] != ntpToken[i]) valid = false;
    const uint32_t ntpSeconds = (uint32_t(packet[40])<<24)
      | (uint32_t(packet[41])<<16) | (uint32_t(packet[42])<<8) | packet[43];
    if (valid && ntpSeconds >= 2208988800UL) {
      utcEpoch = ntpSeconds - 2208988800UL;
      // Include server fraction and an approximate half round-trip delay.
      const uint32_t fraction = (uint32_t(packet[44])<<24)
        | (uint32_t(packet[45])<<16) | (uint32_t(packet[46])<<8) | packet[47];
      const uint32_t fractionMs = uint32_t((uint64_t(fraction)*1000)>>32);
      const uint32_t elapsedMs = fractionMs + (millis()-ntpSentAt)/2;
      utcEpoch += elapsedMs/1000;
      secondTick = millis() - elapsedMs%1000;
      timeSynced = true;
      secondsOfDay = berlinSeconds(utcEpoch);
      publishDisplay();
      ntpPending = false;
      ntpSucceeded = true;
      lastNtpSuccess = millis();
      Serial.print(F("NTP synchronized. Local time: "));
      char stamp[12];
      snprintf(stamp, sizeof(stamp), "%02u:%02u:%02u", unsigned((secondsOfDay/3600)%24),
               unsigned((secondsOfDay/60)%60), unsigned(secondsOfDay%60));
      Serial.println(stamp);
    }
  }
  if (ntpPending && uint32_t(millis()-ntpSentAt) >= 3000) {
    ntpPending = false;
    Serial.println(F("NTP timeout; retry in 30 s."));
  }
}

void serviceNetwork() {
  // The library owns WLAN setup, EEPROM, portal and reconnect handling.
  wifiManager.process();
  const uint32_t now = millis();
  if (uint32_t(now-lastNetworkCheck) < 250) return;
  lastNetworkCheck = now;
  const int status = WiFi.status();
  if (status == WL_NO_MODULE) {
    if (!noModuleReported) Serial.println(F("ERROR: WLAN module not responding."));
    noModuleReported = true;
  }
  const bool connected = wifiManager.getStatus() == WiFiManager::STATUS_CONNECTED
    && status == WL_CONNECTED;
  const IPAddress ip = WiFi.localIP();
  const bool validIP = ip[0] || ip[1] || ip[2] || ip[3];
  if (!connected || !validIP) {
    if (wifiConnected) Serial.println(F("WLAN disconnected; clock continues locally."));
    if (udpReady) ntpUDP.stop();
    udpReady = ntpPending = ntpSucceeded = ntpAttempted = false;
    wifiConnected = false;
    return;
  }
  if (!wifiConnected) {
    wifiConnected = true;
    Serial.print(F("WLAN connected. IP: ")); Serial.println(ip);
    startMatrixIP(ip);
    ntpAttempted = false;
    ntpSucceeded = false;
  }
  receiveNtp();
  if (ntpPending) return;
  const uint32_t current = millis();
  const bool due = ntpSucceeded
    ? uint32_t(current-lastNtpSuccess) >= 3600000UL
    : (!ntpAttempted || uint32_t(current-lastNtpAttempt) >= 30000UL);
  if (due && (!ntpAttempted || uint32_t(current-lastNtpAttempt) >= 30000UL)) requestNtp();
}

bool isNight() {
  if(!settings.nightEnabled||!timeSynced)return false;
  const uint16_t minute=secondsOfDay/60;
  if(settings.nightStart==settings.nightEnd)return true;
  return settings.nightStart<settings.nightEnd
    ? minute>=settings.nightStart && minute<settings.nightEnd
    : minute>=settings.nightStart || minute<settings.nightEnd;
}

