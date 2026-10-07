// SPDX-License-Identifier: MIT
#include "App.h"

uint32_t configCRC(const uint8_t *bytes, size_t count) {
  uint32_t crc = 0xFFFFFFFFUL;
  while (count--) {
    crc ^= *bytes++;
    for (uint8_t i=0; i<8; ++i) crc = (crc>>1)^((crc&1)?0xEDB88320UL:0);
  }
  return ~crc;
}

bool terminated(const char *p, size_t n) { return memchr(p, 0, n) != nullptr; }

bool hostValid(const char *p, bool emptyAllowed) {
  if (!*p) return emptyAllowed;
  for (; *p; ++p) {
    const char c=*p;
    if (!((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='.' || c=='-')) return false;
  }
  return true;
}

bool baseValid(const char *p) {
  const size_t n=strlen(p);
  if (!n || p[0]=='/' || p[n-1]=='/') return false;
  for (; *p; ++p) {
    const char c=*p;
    if (!((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='/' || c=='_' || c=='-')) return false;
  }
  return true;
}

bool configValid(const ClockSettings &c) {
  return c.brightness<=100 && c.ringStyle<4 && c.mqttPort>0
    && c.dateDuration>=1 && c.dateDuration<=3600 && c.textDuration<=3600
    && c.nightStart<1440 && c.nightEnd<1440 && c.nightBrightness<=100
    && c.nightEnabled<=1 && c.nightRingOff<=1 && c.timerProgress<=1 && c.discovery<=1
    && terminated(c.ntp,sizeof(c.ntp)) && terminated(c.mqttHost,sizeof(c.mqttHost))
    && terminated(c.mqttUser,sizeof(c.mqttUser)) && terminated(c.mqttPassword,sizeof(c.mqttPassword))
    && terminated(c.mqttBase,sizeof(c.mqttBase)) && hostValid(c.ntp,false)
    && hostValid(c.mqttHost,true) && baseValid(c.mqttBase);
}

void loadSettings() {
  EEPROM.get(SETTINGS_ADDRESS,settings);
  if(settings.magic==SETTINGS_MAGIC && configValid(settings)
     && settings.crc==configCRC((const uint8_t*)&settings,offsetof(ClockSettings,crc))) {
    displayBrightness=settings.brightness;return;
  }
  // Upgrade the previous single-file settings without touching WLAN profiles.
  struct LegacySettings {
    uint32_t magic;uint8_t brightness,ringStyle;uint16_t mqttPort;
    char ntp[64],mqttHost[64],mqttUser[64],mqttPassword[64],mqttBase[64];uint32_t crc;
  } old;
  EEPROM.get(SETTINGS_ADDRESS,old);
  const bool migrate=old.magic==0x55485232UL
    && old.crc==configCRC((const uint8_t*)&old,offsetof(LegacySettings,crc));
  memset(&settings,0,sizeof(settings));
  if(migrate)memcpy(&settings,&old,offsetof(LegacySettings,crc));
  else {
    settings.brightness=100;settings.mqttPort=1883;
    strcpy(settings.ntp,"de.pool.ntp.org");strcpy(settings.mqttBase,"nerd-clock");
  }
  settings.magic=SETTINGS_MAGIC;settings.dateDuration=5;
  settings.nightStart=1320;settings.nightEnd=420;settings.nightBrightness=10;
  settings.nightRingOff=1;settings.timerProgress=1;settings.discovery=1;
  // CRC validity alone is insufficient for corrupted or manually edited settings.
  if(!configValid(settings)) {
    settings.brightness=100;settings.ringStyle=0;settings.mqttPort=1883;
    strcpy(settings.ntp,"de.pool.ntp.org");settings.mqttHost[0]=settings.mqttUser[0]=settings.mqttPassword[0]=0;
    strcpy(settings.mqttBase,"nerd-clock");
  }
  displayBrightness=settings.brightness;
}

bool saveSettings() {
  settings.crc=configCRC((const uint8_t*)&settings,offsetof(ClockSettings,crc));
  EEPROM.put(SETTINGS_ADDRESS,settings);
  ClockSettings check;
  EEPROM.get(SETTINGS_ADDRESS,check);
  return !memcmp(&settings,&check,sizeof(settings));
}

void settingsUpdated() {
  displayBrightness=isNight()?settings.nightBrightness:settings.brightness;
  displayDirty=stateDirty=settingsDirty=true;
  settingsChanged=millis();
}

