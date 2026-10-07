// SPDX-License-Identifier: MIT
#include "App.h"

namespace {
bool availabilityDirty=true,availabilitySent=false,availabilityAttempted=false;
uint32_t lastAvailabilityAttempt=0,lastAvailabilitySent=0;
void queueAvailability() {
  availabilityDirty=true;availabilityAttempted=false;
}
void serviceAvailability() {
  if(!mqtt.connected())return;
  const uint32_t now=millis();
  if(!availabilityDirty&&uint32_t(now-lastAvailabilitySent)<60000)return;
  if(availabilityAttempted&&uint32_t(now-lastAvailabilityAttempt)<2000)return;
  availabilityAttempted=true;lastAvailabilityAttempt=now;
  char topic[90];mqttTopic(topic,sizeof(topic),"availability");
  if(mqtt.publish(topic,"online",true)) {
    availabilityDirty=false;availabilitySent=true;lastAvailabilitySent=now;
  } else {
    availabilityDirty=true;availabilitySent=false;
    Serial.println(F("MQTT: availability publish failed; retry follows."));
  }
}
}

// Component discovery uses stable MAC-based IDs, with retained configs.
struct DiscoveryEntity {
  const char *component,*id,*name,*command,*field,*extra;
};
const DiscoveryEntity DISCOVERY_ENTITIES[]={
  {"number","brightness","Helligkeit","brightness","brightness","\"min\":0,\"max\":100,\"step\":1,\"unit_of_measurement\":\"%\""},
  {"select","ring","Ring-Stil","ring","ring_style","\"options\":[\"point\",\"fill\",\"drain\",\"inverse\"]"},
  {"number","timer","Timer Sekunden","timer","timer_seconds","\"min\":0,\"max\":5999,\"step\":1,\"unit_of_measurement\":\"s\""},
  {"text","text","Ziffern","text","text","\"min\":0,\"max\":4,\"pattern\":\"[0-9]{0,4}\""},
  {"button","clock","Zur Uhr","clock",nullptr,"\"payload_press\":\"1\""},
  {"button","pause","Timer pausieren","pause",nullptr,"\"payload_press\":\"1\""},
  {"button","resume","Timer fortsetzen","resume",nullptr,"\"payload_press\":\"1\""},
  {"button","ack","Alarm quittieren","ack",nullptr,"\"payload_press\":\"1\""},
  {"switch","date","Datum anzeigen","date","date_active","\"payload_on\":\"1\",\"payload_off\":\"0\""},
  {"number","date_duration","Datum Anzeigedauer","date_duration","date_duration","\"min\":1,\"max\":3600,\"step\":1,\"unit_of_measurement\":\"s\""},
  {"number","text_duration","Ziffern Anzeigedauer","text_duration","text_duration","\"min\":0,\"max\":3600,\"step\":1,\"unit_of_measurement\":\"s\""},
  {"switch","night","Nachthelligkeit","night_enabled","night_enabled","\"payload_on\":\"1\",\"payload_off\":\"0\""},
  {"number","night_brightness","Nachthelligkeit Prozent","night_brightness","night_brightness","\"min\":0,\"max\":100,\"step\":1,\"unit_of_measurement\":\"%\""},
  {"switch","night_ring","Ring nachts aus","night_ring_off","night_ring_off","\"payload_on\":\"1\",\"payload_off\":\"0\""},
  {"switch","progress","Timer Fortschrittsring","timer_progress","timer_progress","\"payload_on\":\"1\",\"payload_off\":\"0\""},
  {"binary_sensor","paused","Timer pausiert",nullptr,"timer_paused","\"payload_on\":\"1\",\"payload_off\":\"0\""},
  {"binary_sensor","alarm","Timeralarm",nullptr,"alarm_active","\"payload_on\":\"1\",\"payload_off\":\"0\""},
  {"binary_sensor","ntp","NTP synchronisiert",nullptr,"ntp_synced","\"payload_on\":\"1\",\"payload_off\":\"0\""},
  {"sensor","mode","Anzeigemodus",nullptr,"mode","\"icon\":\"mdi:clock-digital\""},
  {"sensor","display","Anzeige",nullptr,"display","\"icon\":\"mdi:numeric\""},
  {"sensor","remaining","Timer Restzeit",nullptr,"timer_seconds","\"unit_of_measurement\":\"s\""},
  {"event","finished","Timer abgelaufen",nullptr,nullptr,"\"event_types\":[\"timer_finished\"]"}
};
constexpr uint8_t DISCOVERY_COUNT=sizeof(DISCOVERY_ENTITIES)/sizeof(DISCOVERY_ENTITIES[0]);


void mqttTopic(char *out,size_t n,const char *suffix) { snprintf(out,n,"%s/%s",settings.mqttBase,suffix); }

void mqttCallback(char *topic,byte *payload,unsigned int length) {
  if(!strcmp(topic,"homeassistant/status")&&length==6&&!memcmp(payload,"online",6)) {
    requestDiscovery();stateDirty=true;return;
  }
  char prefix[80];mqttTopic(prefix,sizeof(prefix),"set/");
  const size_t n=strlen(prefix);
  if(strncmp(topic,prefix,n)||length>=128) return;
  char value[128];memcpy(value,payload,length);value[length]=0;
  if(memchr(value,0,length)) return;
  if(!applyCommand(topic+n,value,true)) Serial.println(F("MQTT: invalid command or value."));
  else { updateTimer(); publishDisplay(); }
}

void configureMqtt(bool clearDiscovery) {
  if(clearDiscovery&&mqtt.connected())removeDiscovery();
  if(mqtt.connected()) { char t[90];mqttTopic(t,sizeof(t),"availability");mqtt.publish(t,"offline",true);mqtt.disconnect(); }
  mqttTransport.stop();
  mqtt.setServer(settings.mqttHost,settings.mqttPort);
  queueAvailability();availabilitySent=false;
  mqttAttempted=false;stateDirty=true;discoverySent=0;discoveryCompleted=false;discoveryPending=false;
}

String stateJSON() {
  char json[850];
  const char *mode=dateActive?"date":displayMode==MODE_CLOCK?"clock":displayMode==MODE_TIMER?"timer":"text";
  snprintf(json,sizeof(json),"{\"mode\":\"%s\",\"display\":\"%s\",\"text\":\"%s\",\"timer_seconds\":%u,\"timer_paused\":%s,\"alarm_active\":%s,\"date_active\":%s,\"date_duration\":%u,\"text_duration\":%u,\"brightness\":%u,\"effective_brightness\":%u,\"ring_style\":\"%s\",\"night_enabled\":%s,\"night_brightness\":%u,\"night_ring_off\":%s,\"timer_progress\":%s,\"ntp_synced\":%s,\"mqtt_connected\":%s}",
    mode,visibleText,displayText,unsigned(timerRemaining),timerPaused?"true":"false",alarmActive?"true":"false",dateActive?"true":"false",
    unsigned(settings.dateDuration),unsigned(settings.textDuration),unsigned(settings.brightness),unsigned(displayBrightness),RING_NAMES[settings.ringStyle],
    settings.nightEnabled?"true":"false",unsigned(settings.nightBrightness),settings.nightRingOff?"true":"false",settings.timerProgress?"true":"false",timeSynced?"true":"false",mqtt.connected()?"true":"false");
  return String(json);
}

void serviceMqtt() {
  if(!wifiConnected || !settings.mqttHost[0]) {
    if(mqtt.connected()) {
      char topic[90];mqttTopic(topic,sizeof(topic),"availability");
      mqtt.publish(topic,"offline",true);mqtt.disconnect();
    }
    mqttTransport.stop();mqttAttempted=false;availabilitySent=false;queueAvailability();
    return;
  }
  if(!mqtt.connected()) {
    if(mqttAttempted && uint32_t(millis()-lastMqttAttempt)<30000) return;
    mqttAttempted=true;lastMqttAttempt=millis();
    char availability[90];mqttTopic(availability,sizeof(availability),"availability");
    const bool connected=settings.mqttUser[0]
      ? mqtt.connect(mqttClientID,settings.mqttUser,settings.mqttPassword,availability,0,true,"offline")
      : mqtt.connect(mqttClientID,availability,0,true,"offline");
    if(!connected) {Serial.print(F("MQTT connection failed, code: "));Serial.println(mqtt.state());return;}
    Serial.print(F("MQTT connected: "));Serial.println(settings.mqttHost);
    char commands[90];mqttTopic(commands,sizeof(commands),"set/#");
    if(!mqtt.subscribe(commands)) {mqtt.disconnect();return;}
    queueAvailability();availabilitySent=false;stateDirty=true;
    if(!mqtt.subscribe("homeassistant/status"))Serial.println(F("MQTT: HA birth subscription failed."));
    requestDiscovery();
  }
  mqtt.loop();serviceDiscovery();serviceAvailability();
  if(timerEventPending) {
    char topic[90],event[160];mqttTopic(topic,sizeof(topic),"event");
    snprintf(event,sizeof(event),"{\"event_type\":\"timer_finished\",\"duration_seconds\":%u,\"finished_epoch\":%lu}",unsigned(timerEventDuration),(unsigned long)timerEventEpoch);
    if(mqtt.publish(topic,event,false))timerEventPending=false;
  }
  if(stateDirty || uint32_t(millis()-lastMqttState)>=1000) {
    char topic[90];mqttTopic(topic,sizeof(topic),"state");
    const String state=stateJSON();
    if(mqtt.publish(topic,state.c_str(),true)) {stateDirty=false;lastMqttState=millis();}
  }
}

String discoveryTopic(uint8_t i) {
  return String("homeassistant/")+DISCOVERY_ENTITIES[i].component+"/"+discoveryID+"/"+DISCOVERY_ENTITIES[i].id+"/config";
}

String discoveryJSON(uint8_t i) {
  const DiscoveryEntity &e=DISCOVERY_ENTITIES[i];
  String json;json.reserve(1000);
  json="{\"name\":\""+String(e.name)+"\",\"unique_id\":\""+discoveryID+"_"+e.id+"\",\"availability_topic\":\"";
  json+=settings.mqttBase;json+="/availability\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",\"device\":{\"identifiers\":[\"";
  json+=discoveryID;json+="\"],\"name\":\"Nerd-Clock\",\"manufacturer\":\"DIY\",\"model\":\"UNO R4 WiFi LED Clock\",\"sw_version\":\"" NERD_CLOCK_VERSION "\"}";
  if(e.command){json+=",\"command_topic\":\"";json+=settings.mqttBase;json+="/set/";json+=e.command;json+="\",\"retain\":false";}
  if(!strcmp(e.component,"number"))json+=",\"command_template\":\"{{ value | int }}\"";
  if(e.field) {
    json+=",\"state_topic\":\"";json+=settings.mqttBase;json+="/state\",\"value_template\":\"{{ ";
    const bool boolean=!strcmp(e.component,"switch")||!strcmp(e.component,"binary_sensor");
    if(boolean)json+="'1' if ";
    json+="value_json.";json+=e.field;
    if(boolean)json+=" else '0'";
    json+=" }}\"";
  }
  if(!strcmp(e.component,"event")){json+=",\"state_topic\":\"";json+=settings.mqttBase;json+="/event\"";}
  if(e.extra&&*e.extra){json+=",";json+=e.extra;}
  json+="}";return json;
}

void removeDiscovery() {
  if(!mqtt.connected())return;
  for(uint8_t i=0;i<DISCOVERY_COUNT;++i){const String topic=discoveryTopic(i);mqtt.publish(topic.c_str(),"",true);}
  discoveryPending=false;
}

void requestDiscovery() {
  queueAvailability();stateDirty=true;
  discoveryPending=settings.discovery;discoveryIndex=0;discoverySent=0;
  discoveryFailed=false;discoveryAttempted=false;discoveryCompleted=false;
  if(discoveryPending)Serial.println(F("HA discovery queued: 22 entities under homeassistant/."));
}

String mqttDiagnosticsJSON() {
  String json="{\"connected\":";json+=mqtt.connected()?"true":"false";
  json+=",\"availability_sent\":";json+=availabilitySent?"true":"false";
  json+=",\"availability_pending\":";json+=availabilityDirty?"true":"false";
  json+=",\"connection_code\":";json+=String(mqtt.state());
  json+=",\"enabled\":";json+=settings.discovery?"true":"false";
  json+=",\"pending\":";json+=discoveryPending?"true":"false";
  json+=",\"sent\":";json+=String(discoverySent);
  json+=",\"total\":";json+=String(DISCOVERY_COUNT);
  json+=",\"failed\":";json+=discoveryFailed?"true":"false";
  json+=",\"prefix\":\"homeassistant\",\"node_id\":\"";json+=discoveryID;
  json+="\",\"base\":\"";json+=settings.mqttBase;json+="\"}";return json;
}

void serviceDiscovery() {
  if(!settings.discovery||!mqtt.connected())return;
  const uint32_t now=millis();
  if(!discoveryPending&&discoveryCompleted&&uint32_t(now-lastDiscoveryComplete)>=900000)requestDiscovery();
  if(!discoveryPending)return;
  const uint32_t interval=discoveryFailed?2000:250;
  if(discoveryAttempted&&uint32_t(now-lastDiscoveryAttempt)<interval)return;
  discoveryAttempted=true;lastDiscoveryAttempt=now;
  const String topic=discoveryTopic(discoveryIndex),payload=discoveryJSON(discoveryIndex);
  if(mqtt.publish(topic.c_str(),payload.c_str(),true)) {
    discoveryFailed=false;discoverySent=++discoveryIndex;
    if(discoveryIndex>=DISCOVERY_COUNT){
      discoveryPending=false;discoveryIndex=0;discoveryCompleted=true;lastDiscoveryComplete=now;
      queueAvailability();stateDirty=true;
      Serial.println(F("HA discovery: 22/22 configs sent (retained); availability and state queued."));
    }
  } else {
    if(!discoveryFailed){Serial.print(F("HA discovery publish failed: "));Serial.println(topic);}
    discoveryFailed=true;
  }
}
