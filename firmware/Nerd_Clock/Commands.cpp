// SPDX-License-Identifier: MIT
#include "App.h"

bool unsignedValue(const char *text, uint32_t maximum, uint32_t &out) {
  if (!*text) return false;
  uint32_t value=0;
  for (; *text; ++text) {
    if (*text<'0' || *text>'9') return false;
    const uint8_t digit=*text-'0';
    if (value>maximum/10 || (value==maximum/10 && digit>maximum%10)) return false;
    value=value*10+digit;
  }
  out=value; return true;
}

bool boolValue(const char *value, bool &out) {
  if(!strcmp(value,"1")||!strcasecmp(value,"ON")||!strcasecmp(value,"true")){out=true;return true;}
  if(!strcmp(value,"0")||!strcasecmp(value,"OFF")||!strcasecmp(value,"false")){out=false;return true;}
  return false;
}

bool minuteValue(const char *value,uint16_t &out) {
  if(strlen(value)!=5||value[2]!=':')return false;
  char hours[3]={value[0],value[1],0},minutes[3]={value[3],value[4],0};uint32_t h,m;
  if(!unsignedValue(hours,23,h)||!unsignedValue(minutes,59,m))return false;
  out=h*60+m;return true;
}

uint32_t timerMilliseconds() {
  if(timerPaused)return timerSegmentMs;
  const uint32_t elapsed=millis()-timerStarted;
  return elapsed>=timerSegmentMs?0:timerSegmentMs-elapsed;
}

void returnToClock() {
  displayMode=MODE_CLOCK;
  timerDuration=timerRemaining=0;timerSegmentMs=0;timerPaused=false;
  dateActive=false;alarmActive=false;displayText[0]=0;textTimeoutMs=0;
  displayDirty=stateDirty=true;
}

bool applyCommand(const char *command,const char *value,bool fromMqtt) {
  uint32_t number;bool flag;
  if(!strcmp(command,"discovery_resend")) {
    if(!settings.discovery||!mqtt.connected())return false;
    requestDiscovery();return true;
  }
  if(!strcmp(command,"clock")){returnToClock();return true;}
  if(!strcmp(command,"ack")){alarmActive=false;displayDirty=stateDirty=true;return true;}
  if(!strcmp(command,"pause")) {
    if(displayMode!=MODE_TIMER||timerPaused)return false;
    updateTimer();if(displayMode!=MODE_TIMER)return false;
    timerSegmentMs=timerMilliseconds();timerPaused=true;stateDirty=displayDirty=true;return true;
  }
  if(!strcmp(command,"resume")) {
    if(displayMode!=MODE_TIMER||!timerPaused)return false;
    timerStarted=millis();timerPaused=false;stateDirty=displayDirty=true;return true;
  }
  if(!strcmp(command,"date")) {
    if(!fromMqtt||!boolValue(value,flag))return false;
    if(flag) {
      if(!timeSynced||displayMode!=MODE_CLOCK||alarmActive)return false;
      dateActive=true;dateStarted=millis();dateTimeoutMs=uint32_t(settings.dateDuration)*1000;
    } else dateActive=false;
    stateDirty=displayDirty=true;return true;
  }
  if(!strcmp(command,"date_duration")||!strcmp(command,"text_duration")) {
    const bool date=!strcmp(command,"date_duration");
    if((date&&!fromMqtt)||!unsignedValue(value,3600,number)||(date&&!number))return false;
    if(date)settings.dateDuration=number;else settings.textDuration=number;
    settingsUpdated();return true;
  }
  if(!strcmp(command,"timer")) {
    if(!unsignedValue(value,5999,number))return false;
    returnToClock();
    if(number) {
      displayMode=MODE_TIMER;timerDuration=timerRemaining=number;
      timerSegmentMs=number*1000;timerStarted=millis();
    }
    return true;
  }
  if(!strcmp(command,"text")) {
    const size_t n=strlen(value);if(n>4)return false;
    for(size_t i=0;i<n;++i)if(value[i]<'0'||value[i]>'9')return false;
    returnToClock();if(!n)return true;
    strcpy(displayText,value);displayMode=MODE_TEXT;textStarted=millis();
    textTimeoutMs=uint32_t(settings.textDuration)*1000;return true;
  }
  if(!strcmp(command,"brightness")||!strcmp(command,"night_brightness")) {
    if(!unsignedValue(value,100,number))return false;
    if(!strcmp(command,"brightness"))settings.brightness=number;else settings.nightBrightness=number;
    settingsUpdated();return true;
  }
  if(!strcmp(command,"night_enabled")||!strcmp(command,"night_ring_off")||!strcmp(command,"timer_progress")||!strcmp(command,"discovery")) {
    if(!boolValue(value,flag))return false;
    if(!strcmp(command,"night_enabled"))settings.nightEnabled=flag;
    else if(!strcmp(command,"night_ring_off"))settings.nightRingOff=flag;
    else if(!strcmp(command,"timer_progress"))settings.timerProgress=flag;
    else {
      if(!flag&&mqtt.connected())removeDiscovery();
      settings.discovery=flag;requestDiscovery();
    }
    settingsUpdated();return true;
  }
  if(!strcmp(command,"night_start")||!strcmp(command,"night_end")) {
    uint16_t minute;if(!minuteValue(value,minute))return false;
    if(!strcmp(command,"night_start"))settings.nightStart=minute;else settings.nightEnd=minute;
    settingsUpdated();return true;
  }
  if(!strcmp(command,"ring")) {
    for(uint8_t i=0;i<4;++i)if(!strcmp(value,RING_NAMES[i])){settings.ringStyle=i;settingsUpdated();return true;}
    return false;
  }
  if(!strcmp(command,"ntp")) {
    if(strlen(value)>=sizeof(settings.ntp)||!hostValid(value,false))return false;
    strcpy(settings.ntp,value);ntpPending=ntpSucceeded=ntpAttempted=false;settingsUpdated();return true;
  }
  return false;
}

void updateTimer() {
  if(displayMode!=MODE_TIMER)return;
  const uint32_t remainingMs=timerMilliseconds();
  if(!remainingMs) {
    timerEventEpoch=timeSynced?utcEpoch:0;timerEventDuration=timerDuration;timerEventPending=true;
    returnToClock();alarmStarted=millis();alarmActive=true;return;
  }
  const uint16_t remaining=(remainingMs+999)/1000;
  if(remaining!=timerRemaining){timerRemaining=remaining;displayDirty=stateDirty=true;}
}

void serviceModes() {
  updateTimer();const uint32_t now=millis();
  if(alarmActive&&uint32_t(now-alarmStarted)>=10000){alarmActive=false;displayDirty=stateDirty=true;}
  if(dateActive&&uint32_t(now-dateStarted)>=dateTimeoutMs){dateActive=false;displayDirty=stateDirty=true;}
  if(displayMode==MODE_TEXT&&textTimeoutMs&&uint32_t(now-textStarted)>=textTimeoutMs)returnToClock();
  const uint8_t brightness=isNight()?settings.nightBrightness:settings.brightness;
  if(displayBrightness!=brightness){displayBrightness=brightness;displayDirty=stateDirty=true;}
}

