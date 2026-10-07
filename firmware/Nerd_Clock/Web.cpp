// SPDX-License-Identifier: MIT
#include "App.h"

#include "WebUi.h"

int hexValue(char c) {
  if(c>='0'&&c<='9')return c-'0';
  if(c>='A'&&c<='F')return c-'A'+10;
  if(c>='a'&&c<='f')return c-'a'+10;
  return -1;
}

bool formField(const char *body,const char *key,char *out,size_t capacity) {
  bool found=false;size_t keySize=strlen(key);
  for(const char *part=body;*part;) {
    const char *end=strchr(part,'&');if(!end)end=part+strlen(part);
    if(size_t(end-part)>keySize&&!strncmp(part,key,keySize)&&part[keySize]=='=') {
      if(found)return false;
      found=true;size_t used=0;
      for(const char *p=part+keySize+1;p<end;++p) {
        uint8_t c=*p;
        if(c=='+')c=' ';
        else if(c=='%') {if(end-p<3||hexValue(p[1])<0||hexValue(p[2])<0)return false;c=hexValue(p[1])*16+hexValue(p[2]);p+=2;}
        if(!c||used+1>=capacity)return false;
        out[used++]=c;
      }
      out[used]=0;
    }
    part=*end?end+1:end;
  }
  return found;
}

String jsonEscape(const char *s) {
  String out;out.reserve(strlen(s)+16);
  for(;*s;++s){const uint8_t c=*s;if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c<32){char b[7];snprintf(b,sizeof(b),"\\u%04x",c);out+=b;}else out+=char(c);}
  return out;
}

String settingsJSON() {
  String json="{\"brightness\":"+String(settings.brightness)+",\"ring\":\""+RING_NAMES[settings.ringStyle];
  json+="\",\"ntp\":\""+jsonEscape(settings.ntp)+"\",\"host\":\""+jsonEscape(settings.mqttHost);
  json+="\",\"port\":"+String(settings.mqttPort)+",\"user\":\""+jsonEscape(settings.mqttUser);
  json+="\",\"base\":\""+jsonEscape(settings.mqttBase)+"\",\"password_saved\":";
  json+=settings.mqttPassword[0]?"true":"false";
  char extra[350];snprintf(extra,sizeof(extra),",\"night_enabled\":%u,\"night_ring_off\":%u,\"timer_progress\":%u,\"discovery\":%u,\"night_brightness\":%u,\"night_start\":\"%02u:%02u\",\"night_end\":\"%02u:%02u\",\"text_duration\":%u}",settings.nightEnabled,settings.nightRingOff,settings.timerProgress,settings.discovery,settings.nightBrightness,settings.nightStart/60,settings.nightStart%60,settings.nightEnd/60,settings.nightEnd%60,settings.textDuration);
  json+=extra;return json;
}

void webReply(const char *status,const char *type,const char *body) {
  appClient.print("HTTP/1.1 ");appClient.println(status);
  appClient.print("Content-Type: ");appClient.println(type);
  appClient.println("Cache-Control: no-store");appClient.println("Connection: close");
  appClient.print("Content-Length: ");appClient.println(strlen(body));appClient.println();
  // Small writes avoid large WiFiS3 command payloads.
  const size_t n=strlen(body);
  for(size_t pos=0;pos<n;pos+=256) appClient.write((const uint8_t*)body+pos,min(size_t(256),n-pos));
  appClient.stop();appClientActive=false;memset(appRequest,0,sizeof(appRequest));
}

bool applyWebSettings(const char *body) {
  ClockSettings next=settings;char val[128];uint32_t number;
  if(!formField(body,"brightness",val,sizeof(val))||!unsignedValue(val,100,number))return false;
  next.brightness=number;
  if(!formField(body,"ring",val,sizeof(val)))return false;
  bool found=false;for(uint8_t i=0;i<4;++i)if(!strcmp(val,RING_NAMES[i])){next.ringStyle=i;found=true;}
  if(!found)return false;
  if(!formField(body,"ntp",next.ntp,sizeof(next.ntp))||!formField(body,"host",next.mqttHost,sizeof(next.mqttHost))
     ||!formField(body,"port",val,sizeof(val))||!unsignedValue(val,65535,number)||!number)return false;
  next.mqttPort=number;
  if(!formField(body,"user",next.mqttUser,sizeof(next.mqttUser))||!formField(body,"base",next.mqttBase,sizeof(next.mqttBase)))return false;
  if(!formField(body,"password",val,sizeof(val)))return false;
  if(strlen(val)>=sizeof(next.mqttPassword))return false;
  if(*val)strcpy(next.mqttPassword,val); // Empty field keeps the existing password.
  char clear[4];if(formField(body,"clear_password",clear,sizeof(clear))&&!strcmp(clear,"1"))next.mqttPassword[0]=0;
  memset(val,0,sizeof(val));
  const char *flags[]={"night_enabled","night_ring_off","timer_progress","discovery"};
  uint8_t *dest[]={&next.nightEnabled,&next.nightRingOff,&next.timerProgress,&next.discovery};
  for(uint8_t i=0;i<4;++i){bool flag;if(formField(body,flags[i],val,sizeof(val))){if(!boolValue(val,flag))return false;*dest[i]=flag;}}
  if(formField(body,"night_brightness",val,sizeof(val))){if(!unsignedValue(val,100,number))return false;next.nightBrightness=number;}
  if(formField(body,"text_duration",val,sizeof(val))){if(!unsignedValue(val,3600,number))return false;next.textDuration=number;}
  if(formField(body,"night_start",val,sizeof(val))&&!minuteValue(val,next.nightStart))return false;
  if(formField(body,"night_end",val,sizeof(val))&&!minuteValue(val,next.nightEnd))return false;
  if(!configValid(next))return false;
  const bool mqttChanged=next.discovery!=settings.discovery||strcmp(next.mqttHost,settings.mqttHost)||next.mqttPort!=settings.mqttPort
    ||strcmp(next.mqttUser,settings.mqttUser)||strcmp(next.mqttPassword,settings.mqttPassword)||strcmp(next.mqttBase,settings.mqttBase);
  const bool ntpChanged=strcmp(next.ntp,settings.ntp);
  // Publish offline under the OLD base before replacing the connection config.
  if(mqttChanged)configureMqtt(true);
  const ClockSettings old=settings;settings=next;
  if(!saveSettings()){settings=old;configureMqtt(false);return false;}
  if(mqttChanged)configureMqtt(false);
  if(ntpChanged)ntpPending=ntpSucceeded=ntpAttempted=false;
  requestDiscovery();
  settingsDirty=false;displayBrightness=isNight()?settings.nightBrightness:settings.brightness;displayDirty=stateDirty=true;
  return true;
}

void handleWebRequest() {
  const char *body=appRequest+appHeaderSize;
  if(!strncmp(appRequest,"GET / HTTP/1.",13)||!strncmp(appRequest,"GET /index.html HTTP/1.",23)) {
    webReply("200 OK","text/html; charset=utf-8",APP_HTML);return;
  }
  if(!strncmp(appRequest,"GET /api/ota HTTP/1.",20)) {const String s=otaStatusJSON();webReply("200 OK","application/json",s.c_str());return;}
  if(!strncmp(appRequest,"POST /api/ota HTTP/1.",21)) {
    char action[16],url[384]={};
    const bool parsed=formField(body,"action",action,sizeof(action));
    const bool urlOk=!parsed||strcmp(action,"download")||formField(body,"url",url,sizeof(url));
    const bool ok=parsed&&urlOk&&otaCommand(action,url);
    String reply=ok?"{\"ok\":true}":String("{\"error\":\"")+jsonEscape(otaLastError()[0]?otaLastError():"Ungueltige OTA-Anfrage")+"\"}";
    webReply(ok?"200 OK":"400 Bad Request","application/json",reply.c_str());return;
  }
  if(!strncmp(appRequest,"GET /api/mqtt HTTP/1.",21)) {const String s=mqttDiagnosticsJSON();webReply("200 OK","application/json",s.c_str());return;}
  if(!strncmp(appRequest,"GET /api/state HTTP/1.",22)) {const String s=stateJSON();webReply("200 OK","application/json",s.c_str());return;}
  if(!strncmp(appRequest,"GET /api/settings HTTP/1.",25)) {const String s=settingsJSON();webReply("200 OK","application/json",s.c_str());return;}
  if(!strncmp(appRequest,"POST /api/settings HTTP/1.",26)) {
    const bool ok=applyWebSettings(body);
    webReply(ok?"200 OK":"400 Bad Request","application/json",ok?"{\"ok\":true}":"{\"error\":\"Einstellungen ungueltig oder Speichern fehlgeschlagen.\"}");return;
  }
  if(!strncmp(appRequest,"POST /api/command HTTP/1.",25)) {
    char command[24],value[128];
    const bool ok=formField(body,"command",command,sizeof(command))&&formField(body,"value",value,sizeof(value))&&applyCommand(command,value,false);
    if(ok) {updateTimer();publishDisplay();}
    webReply(ok?"200 OK":"400 Bad Request","application/json",ok?"{\"ok\":true}":"{\"error\":\"Befehl oder Wert ungueltig.\"}");return;
  }
  webReply("404 Not Found","application/json","{\"error\":\"Nicht gefunden\"}");
}

void serviceWeb() {
  // WiFiS3 loses server sockets when the AP/STA mode changes.
  const int status=wifiManager.getStatus();
  if(status!=previousManagerStatus) {
    previousManagerStatus=status;appClient.stop();appClientActive=false;
    if(status==WiFiManager::STATUS_AP_MODE||status==WiFiManager::STATUS_CONNECTED)appServer.begin();
  }
  if(status!=WiFiManager::STATUS_AP_MODE && !wifiConnected)return;
  if(!appClientActive) {
    appClient=appServer.available();if(!appClient)return;
    appClientActive=true;appClientSince=millis();appUsed=appHeaderSize=appBodySize=0;memset(appRequest,0,sizeof(appRequest));
  }
  if(uint32_t(millis()-appClientSince)>=5000){webReply("408 Request Timeout","application/json","{\"error\":\"Zeitlimit\"}");return;}
  for(unsigned budget=0;budget<192&&appClient.available();++budget) {
    const int c=appClient.read();if(c<0)break;
    if(!c||appUsed+1>=sizeof(appRequest)){webReply("413 Content Too Large","application/json","{\"error\":\"Anfrage zu gross\"}");return;}
    appRequest[appUsed++]=char(c);appRequest[appUsed]=0;
    if(!appHeaderSize&&appUsed>=4&&!memcmp(appRequest+appUsed-4,"\r\n\r\n",4)) {
      appHeaderSize=appUsed;bool lengthSeen=false;
      for(char *line=strstr(appRequest,"\r\n");line&&line[2];) {
        line+=2;
        if(!strncasecmp(line,"Content-Length:",15)) {
          char *end;long n=strtol(line+15,&end,10);
          if(lengthSeen||end==line+15||strncmp(end,"\r\n",2)||n<0||n>1400){webReply("400 Bad Request","application/json","{\"error\":\"Ungueltige Laenge\"}");return;}
          lengthSeen=true;appBodySize=n;
        }
        if(!strncasecmp(line,"Transfer-Encoding:",18)){webReply("400 Bad Request","application/json","{\"error\":\"Transfer-Encoding nicht unterstuetzt\"}");return;}
        line=strstr(line,"\r\n");
      }
    }
    if(appHeaderSize&&appUsed>=appHeaderSize+appBodySize){handleWebRequest();return;}
  }
  if(!appClient.connected()&&!appClient.available()){appClient.stop();appClientActive=false;memset(appRequest,0,sizeof(appRequest));}
}

