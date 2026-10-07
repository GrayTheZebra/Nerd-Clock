// SPDX-License-Identifier: MIT
#include "App.h"

const uint8_t IP_FONT[11][5] = {
  {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7},
  {5,5,7,1,1}, {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1},
  {7,5,7,5,7}, {7,5,7,1,7}, {0,0,0,0,2}
};

void clearMatrix() {
  memset(matrixFrame, 0, sizeof(matrixFrame));
  if (matrixReady) matrix.renderBitmap(matrixFrame, 8, 12);
}

void showMatrixAP() {
  if (!matrixReady) return;
  memset(matrixFrame, 0, sizeof(matrixFrame));
  const uint8_t a[7] = {14,17,17,31,17,17,17};
  const uint8_t p[7] = {30,17,17,30,16,16,16};
  for (uint8_t y = 0; y < 7; ++y) {
    for (uint8_t x = 0; x < 5; ++x) {
      matrixFrame[y][x] = (a[y] >> (4-x)) & 1;
      matrixFrame[y][x+7] = (p[y] >> (4-x)) & 1;
    }
  }
  matrix.renderBitmap(matrixFrame, 8, 12);
  matrixAPShown = true;
  matrixIPActive = false;
}

void onSetupAP() {
  // Called by the manager before beginAP(), so AP is visible immediately.
  showMatrixAP();
  Serial.print(F("Setup WLAN: ")); Serial.println(SETUP_SSID);
  Serial.print(F("Password: ")); Serial.println(SETUP_PASSWORD);
  Serial.println(F("Portal: http://192.168.4.1"));
}

void startMatrixIP(const IPAddress &ip) {
  if (!matrixReady) return;
  snprintf(matrixIP, sizeof(matrixIP), "%u.%u.%u.%u", unsigned(ip[0]),
           unsigned(ip[1]), unsigned(ip[2]), unsigned(ip[3]));
  matrixIPStarted = millis();
  lastMatrixStep = millis()-MATRIX_SCROLL_STEP_MS;
  matrixScrollX = 12;
  matrixAPShown = false;
  matrixIPActive = true;
}

void serviceMatrix() {
  if (!matrixReady) return;
  const bool inAP = wifiManager.getStatus() == WiFiManager::STATUS_AP_MODE;
  if (inAP) {
    if (!matrixAPShown) showMatrixAP();
    return;
  }
  if (matrixAPShown) { matrixAPShown = false; clearMatrix(); }
  const uint32_t now = millis();
  if (!wifiConnected || !matrixIPActive
      || uint32_t(now-matrixIPStarted) >= MATRIX_IP_DURATION_MS) {
    if (matrixIPActive) { matrixIPActive = false; clearMatrix(); }
    return;
  }
  if (uint32_t(now-lastMatrixStep) < MATRIX_SCROLL_STEP_MS) return;
  lastMatrixStep = now;
  memset(matrixFrame, 0, sizeof(matrixFrame));
  const size_t count = strlen(matrixIP);
  for (size_t i = 0; i < count; ++i) {
    const char c = matrixIP[i];
    const uint8_t glyph = c == '.' ? 10 : uint8_t(c-'0');
    if (glyph > 10) continue;
    for (uint8_t y = 0; y < 5; ++y) {
      for (uint8_t x = 0; x < 3; ++x) {
        const int16_t column = matrixScrollX + int16_t(i*4) + x;
        if (column >= 0 && column < 12)
          matrixFrame[y+1][column] = (IP_FONT[glyph][y] >> (2-x)) & 1;
      }
    }
  }
  matrix.renderBitmap(matrixFrame, 8, 12);
  --matrixScrollX;
  // Include a short blank gap before the address repeats.
  if (matrixScrollX < -int16_t(count*4)-4) matrixScrollX = 12;
}

