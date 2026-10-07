// SPDX-License-Identifier: MIT
#include "App.h"
#include <OTAUpdate.h>

namespace {
OTAUpdate updater;
enum Stage {IDLE,QUEUED,DOWNLOADING,READY,INSTALL_QUEUED,INSTALLING,FAILED};
Stage stage=IDLE;
const char *const names[]={"idle","queued","downloading","ready","install_queued","installing","error"};
char updateUrl[384]={},errorText[160]={},wifiFirmware[32]={};
bool supported=false,opened=false;
int expected=0,received=0;
uint32_t started=0,lastProgress=0,lastPoll=0,installQueued=0;
void fail(const char *message,int code=0) {
  snprintf(errorText,sizeof(errorText),"%s (Code %d)",message,code);
  Serial.print(F("OTA: "));Serial.println(errorText);
  if(opened)updater.reset();opened=false;stage=FAILED;
}
}

void setupOta() {
  snprintf(wifiFirmware,sizeof(wifiFirmware),"%s",WiFi.firmwareVersion());
  unsigned major=0,minor=0,patch=0;
  supported=sscanf(wifiFirmware,"%u.%u.%u",&major,&minor,&patch)==3&&(major>0||minor>=5);
  Serial.print(F("OTA WiFi firmware: "));Serial.println(wifiFirmware);
}

bool otaInstalling() {return stage==INSTALLING;}
bool otaBusy() {return stage==QUEUED||stage==DOWNLOADING||stage==INSTALL_QUEUED||stage==INSTALLING;}
const char *otaLastError() {return errorText;}

bool otaUrlValid(const char *url) {
  const size_t length=strlen(url);
  if(length<12||length>=sizeof(updateUrl)||strncmp(url,"http://",7))return false;
  for(const char *p=url;*p;p++)if((uint8_t)*p<33||(uint8_t)*p>126||strchr(",@\\\"#",*p))return false;
  const char *path=strchr(url+7,'/');if(!path||path==url+7||strcmp(url+length-4,".ota"))return false;
  char host[64];const size_t authority=path-(url+7);if(authority>=sizeof(host))return false;
  memcpy(host,url+7,authority);host[authority]=0;
  char *port=strchr(host,':');if(port){*port++=0;uint32_t n;if(!unsignedValue(port,65535,n)||!n)return false;}
  return hostValid(host,false);
}

bool otaCommand(const char *action,const char *url) {
  if(!strcmp(action,"cancel")) {
    if(stage==INSTALL_QUEUED||stage==INSTALLING)return false;
    if(opened)updater.reset();opened=false;stage=IDLE;errorText[0]=updateUrl[0]=0;expected=received=0;return true;
  }
  if(!strcmp(action,"install")) {
    if(stage!=READY||!wifiConnected){snprintf(errorText,sizeof(errorText),"Keine gepruefte Firmware oder WLAN getrennt.");return false;}
    errorText[0]=0;stage=INSTALL_QUEUED;installQueued=millis();return true;
  }
  if(strcmp(action,"download"))return false;
  if(otaBusy()||stage==READY){snprintf(errorText,sizeof(errorText),"OTA bereits aktiv. Erst abbrechen oder installieren.");return false;}
  if(!supported){snprintf(errorText,sizeof(errorText),"WLAN-Firmware ab 0.5.0 erforderlich.");return false;}
  if(!wifiConnected){snprintf(errorText,sizeof(errorText),"WLAN nicht verbunden.");return false;}
  if(!otaUrlValid(url)){snprintf(errorText,sizeof(errorText),"HTTP-URL zu einer .ota-Datei erforderlich.");return false;}
  strcpy(updateUrl,url);errorText[0]=0;expected=received=0;stage=QUEUED;return true;
}

String otaStatusJSON() {
  String json="{\"stage\":\"";json+=names[stage];json+="\",\"version\":\"" NERD_CLOCK_VERSION "\",\"wifi_firmware\":\"";
  json+=jsonEscape(wifiFirmware);json+="\",\"supported\":";json+=supported?"true":"false";
  json+=",\"connected\":";json+=wifiConnected?"true":"false";
  json+=",\"received\":";json+=String(received);json+=",\"total\":";json+=String(expected);
  json+=",\"error\":\"";json+=jsonEscape(errorText);json+="\"}";return json;
}

void serviceOta() {
  const uint32_t now=millis();
  if(stage==QUEUED) {
    if(!wifiConnected){fail("WLAN getrennt");return;}
    const int result=updater.begin("/nerd-clock-update.bin");
    if(result!=OTAUpdate::OTA_ERROR_NONE){fail("OTA-Speicher nicht bereit",result);return;}
    opened=true;
    expected=updater.startDownload(updateUrl,"/nerd-clock-update.bin");
    if(expected<=0){fail("Downloadstart fehlgeschlagen",expected);return;}
    // Our literal LZSS packaging adds at most 12.5% to the 256 KiB firmware.
    if(expected>300000){fail("OTA-Datei zu gross",expected);return;}
    stage=DOWNLOADING;started=lastProgress=millis();lastPoll=started-500;return;
  }
  if(stage==DOWNLOADING) {
    if(!wifiConnected){fail("WLAN getrennt");return;}
    if(uint32_t(now-started)>=300000||uint32_t(now-lastProgress)>=30000){fail("Download-Zeitlimit");return;}
    if(uint32_t(now-lastPoll)<500)return;lastPoll=now;
    const int progress=updater.downloadProgress();
    if(progress<0){fail("Download fehlgeschlagen",progress);return;}
    if(progress>received){received=progress;lastProgress=now;}
    if(received<expected)return;
    const int result=updater.verify();
    if(result!=OTAUpdate::OTA_ERROR_NONE){fail("Dateipruefung fehlgeschlagen",result);return;}
    stage=READY;Serial.println(F("OTA verified; waiting for installation."));return;
  }
  if(stage==INSTALL_QUEUED&&uint32_t(now-installQueued)>=1500) {
    if(!wifiConnected){fail("WLAN getrennt");return;}
    if(settingsDirty&&!saveSettings()){fail("Einstellungen konnten nicht gespeichert werden");return;}
    settingsDirty=false;stage=INSTALLING;
    configureMqtt(false); // Retained offline; boot reconnects and announces online.
    const bool wasRunning=displayRunning;
    if(wasRunning)displayTimer.stop();
    allOff();write595(0);if(matrixReady)clearMatrix();
    const int result=updater.update("/nerd-clock-update.bin");
    // A successful bridge transfer programs and resets the RA4M1.
    if(result==OTAUpdate::OTA_ERROR_NONE)return;
    fail("Installation fehlgeschlagen",result);
    if(wasRunning)displayTimer.start();displayDirty=true;
  }
}
