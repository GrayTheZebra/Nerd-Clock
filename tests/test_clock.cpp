#include <cassert>
#include <cstdint>
#include <iostream>
uint32_t testMillis=1000;
uint8_t pinLevels[32]={};
#include "App.h"
#include <OTAUpdate.h>
int ringCount(const uint8_t *r){int n=0;for(int i=0;i<8;i++)for(int b=0;b<8;b++)n+=(r[i]>>b)&1;return n;}
int pendingCount(){uint8_t r[8];for(int i=0;i<8;i++)r[i]=pendingRing[i];return ringCount(r);}
void send(const std::string &request){WiFiClient c;c.input=request;c.live=true;appServer.queued=c;for(int i=0;i<40;i++){serviceWeb();if(!appClientActive&&!appClient.output.empty())break;}}
int main(){
 loadSettings();assert(settings.brightness==100);assert(settings.mqttPort==1883);assert(!strcmp(settings.mqttBase,"nerd-clock"));assert(SETTINGS_ADDRESS+sizeof(settings)<4096);
 strcpy(settings.mqttPassword,"secret123");assert(saveSettings());settings.brightness=11;loadSettings();assert(settings.brightness==100);EEPROM.memory[SETTINGS_ADDRESS+10]^=1;loadSettings();assert(!settings.mqttPassword[0]);
 timeSynced=true;secondsOfDay=0;publishDisplay();assert(!strcmp(visibleText," 000"));assert(!pendingSegments[0]&&!pendingSegments[1]);
 secondsOfDay=9*3600+5*60;publishDisplay();assert(!strcmp(visibleText," 905"));
 secondsOfDay=10*3600+5*60;publishDisplay();assert(!strcmp(visibleText,"1005"));
 assert(applyCommand("timer","1"));publishDisplay();assert(!strcmp(visibleText," 001"));
 assert(applyCommand("timer","600"));publishDisplay();assert(!strcmp(visibleText,"1000"));
 returnToClock();timeSynced=false;
 uint8_t r[8];for(int pos=0;pos<60;pos++){buildRing(pos,0,r);assert(ringCount(r)==1);buildRing(pos,1,r);assert(ringCount(r)==pos+1);buildRing(pos,2,r);assert(ringCount(r)==60-pos);buildRing(pos,3,r);assert(ringCount(r)==59);}
 assert(applyCommand("timer","300"));publishDisplay();assert(!strcmp(visibleText," 500"));
 testMillis+=239000;updateTimer();assert(timerRemaining==61);testMillis+=1000;updateTimer();publishDisplay();assert(timerRemaining==60);assert(pendingCount()==60);testMillis+=1000;updateTimer();publishDisplay();assert(timerRemaining==59);assert(pendingCount()==0);testMillis+=59000;updateTimer();assert(displayMode==MODE_CLOCK);
 assert(applyCommand("timer","5999"));publishDisplay();assert(!strcmp(visibleText,"9959"));assert(!applyCommand("timer","6000"));assert(!applyCommand("timer","-1"));assert(!applyCommand("timer","12abc"));assert(applyCommand("timer","0"));assert(displayMode==MODE_CLOCK);
 assert(applyCommand("text","0032"));publishDisplay();assert(!strcmp(visibleText,"0032"));assert(!pendingCount());assert(applyCommand("text","7"));publishDisplay();assert(!strcmp(visibleText,"   7"));assert(!applyCommand("text","12345"));assert(!applyCommand("text","ab"));assert(applyCommand("text",""));assert(applyCommand("clock","whatever"));assert(!displayText[0]);
 testMillis=0xFFFFFF00;assert(applyCommand("timer","2"));testMillis+=1000;updateTimer();assert(timerRemaining==1);testMillis+=1000;updateTimer();assert(displayMode==MODE_CLOCK);
 uint32_t n;assert(!unsignedValue("42949672950",5999,n));assert(!applyCommand("brightness","101"));assert(applyCommand("brightness","0"));assert(displayBrightness==0);assert(applyCommand("ring","inverse"));assert(!applyCommand("ring","invalid"));assert(applyCommand("ntp","192.168.1.1"));assert(!applyCommand("ntp","http://server/"));
 char topic[]="nerd-clock/set/text";byte payload[]={'0','0','4','2'};mqttCallback(topic,payload,4);publishDisplay();assert(!strcmp(visibleText,"0042"));
 char out[16];assert(formField("text=0032&host=A%26B","host",out,sizeof(out)));assert(!strcmp(out,"A&B"));assert(!formField("x=%00","x",out,sizeof(out)));assert(!formField("x=a&x=b","x",out,sizeof(out)));assert(!formField("x=abcd","x",out,4));
 const char *body="brightness=85&ring=fill&ntp=pool.ntp.org&host=192.168.1.10&port=1883&user=gray&password=secret123&base=home%2Fclock";
 assert(applyWebSettings(body));assert(settings.brightness==85);assert(!strcmp(settings.mqttPassword,"secret123"));assert(settingsJSON().find("secret123")==std::string::npos);
 assert(applyWebSettings("brightness=50&ring=drain&ntp=pool.ntp.org&host=&port=1883&user=&password=&base=nerd-clock"));assert(!strcmp(settings.mqttPassword,"secret123"));
 assert(applyWebSettings("brightness=50&ring=drain&ntp=pool.ntp.org&host=&port=1883&user=&password=&base=nerd-clock&clear_password=1"));assert(!settings.mqttPassword[0]);assert(!applyWebSettings("brightness=101"));
 wifiManager.state=WiFiManager::STATUS_AP_MODE;send("GET / HTTP/1.1\r\nHost: x\r\n\r\n");assert(appClient.output.find("200 OK")!=std::string::npos);
 send("GET /api/settings HTTP/1.1\r\nHost: x\r\n\r\n");assert(appClient.output.find("password_saved")!=std::string::npos);
 std::string b="command=timer&value=120";send("POST /api/command HTTP/1.1\r\nContent-Length: "+std::to_string(b.size())+"\r\n\r\n"+b);assert(timerRemaining==120);assert(!strcmp(visibleText," 200"));
 send("POST /api/settings HTTP/1.1\r\nContent-Length: 1500\r\n\r\n");assert(appClient.output.find("400 Bad Request")!=std::string::npos);
 timer_callback_args_t args{TIMER_EVENT_CYCLE_END};
 auto duty=[&](uint8_t brightness){displayBrightness=brightness;int count=0;for(int i=0;i<8000;i++){displayTick(&args);for(uint8_t pin:GROUP_PINS)count+=pinLevels[pin];}return count;};
 assert(duty(100)==6000);assert(duty(50)==3000);assert(duty(0)==0);assert(duty(1)>40);

 // Pause preserves sub-second time; resume does not count paused time.
 testMillis=1000;assert(applyCommand("timer","120"));testMillis+=1250;
 assert(applyCommand("pause","1"));const uint32_t frozen=timerMilliseconds();
 testMillis+=30000;serviceModes();assert(timerMilliseconds()==frozen);
 assert(applyCommand("resume","1"));testMillis+=500;assert(timerMilliseconds()==frozen-500);
 assert(applyCommand("clock","1"));assert(!timerPaused);
 // Date is MQTT-only, never interrupts timer/text/alarm, and times out.
 timeSynced=true;utcEpoch=1791280800;secondsOfDay=berlinSeconds(utcEpoch);
 assert(!applyCommand("date","1"));assert(applyCommand("date_duration","3",true));
 assert(applyCommand("date","ON",true));publishDisplay();assert(!strcmp(visibleText,"0610"));assert(!pendingCount());
 assert(pendingSegments[2]&(1<<4));assert(pendingSegments[3]&(1<<2));
 testMillis+=2999;serviceModes();assert(dateActive);testMillis++;serviceModes();assert(!dateActive);
 assert(applyCommand("timer","5"));assert(!applyCommand("date","1",true));
 // Timer expiry queues one MQTT event and starts exactly ten seconds of alarm.
 testMillis+=5000;serviceModes();assert(displayMode==MODE_CLOCK&&alarmActive&&timerEventPending);
 const uint32_t ended=alarmStarted;testMillis=ended+9999;serviceModes();assert(alarmActive);
 testMillis=ended+10000;serviceModes();assert(!alarmActive);
 mqtt.online=true;wifiConnected=true;strcpy(settings.mqttHost,"testbroker");serviceMqtt();assert(!timerEventPending);
 bool eventSeen=false;for(const auto &m:mqtt.messages)if(m.topic=="nerd-clock/event"){assert(!m.retained);assert(m.payload.find("timer_finished")!=std::string::npos);eventSeen=true;}assert(eventSeen);
 // Quitting the alarm does not overwrite time; a new mode cancels it.
 assert(applyCommand("timer","1"));testMillis+=1000;serviceModes();assert(alarmActive);
 assert(applyCommand("ack","1"));assert(!alarmActive);
 assert(applyCommand("text_duration","2"));assert(applyCommand("text","0042"));
 testMillis+=2000;serviceModes();assert(displayMode==MODE_CLOCK);
 // Midnight schedule, daytime schedule and pre-NTP behavior.
 settings.nightEnabled=1;settings.nightStart=1320;settings.nightEnd=420;settings.nightBrightness=10;
 secondsOfDay=23*3600;assert(isNight());serviceModes();assert(displayBrightness==10);
 secondsOfDay=6*3600;assert(isNight());secondsOfDay=7*3600;assert(!isNight());
 settings.nightStart=600;settings.nightEnd=900;secondsOfDay=12*3600;assert(isNight());
 timeSynced=false;assert(!isNight());timeSynced=true;settings.nightEnabled=0;
 // Full-progress ring shrinks proportionally before last-minute blinking.
 settings.timerProgress=1;testMillis=0;assert(applyCommand("timer","600"));publishDisplay();assert(pendingCount()==60);
 testMillis=300000;updateTimer();publishDisplay();assert(pendingCount()==30);
 // Alarm pulse edges are generated by the actual scan ISR, not network loop timing.
 returnToClock();alarmActive=true;alarmStarted=10000;displayBrightness=100;
 auto ringAt=[&](uint32_t ms){testMillis=alarmStarted+ms;int count=0;timer_callback_args_t arg{TIMER_EVENT_CYCLE_END};for(int i=0;i<64;i++){displayTick(&arg);for(uint8_t pin:RING_PINS)count+=pinLevels[pin];}return count;};
 assert(ringAt(0)>0);assert(ringAt(99)>0);assert(ringAt(100)==0);assert(ringAt(200)>0);assert(ringAt(300)==0);
 assert(ringAt(1000)>0);testMillis=alarmStarted+10000;serviceModes();assert(!alarmActive);
 // Previous EEPROM layout migrates credentials and initializes new options.
 struct OldSettings {uint32_t magic;uint8_t brightness,ringStyle;uint16_t port;char ntp[64],host[64],user[64],password[64],base[64];uint32_t crc;} old{};
 old.magic=0x55485232;old.brightness=85;old.ringStyle=2;old.port=1883;
 strcpy(old.ntp,"de.pool.ntp.org");strcpy(old.host,"broker");strcpy(old.password,"oldsecret");strcpy(old.base,"custom-old-topic");
 old.crc=configCRC((const uint8_t*)&old,offsetof(OldSettings,crc));EEPROM.put(SETTINGS_ADDRESS,old);loadSettings();
 assert(settings.brightness==85&&settings.ringStyle==2);assert(!strcmp(settings.mqttPassword,"oldsecret"));
 assert(settings.dateDuration==5&&settings.discovery==1&&settings.nightEnabled==0);assert(configValid(settings));
 assert(!applyCommand("date_duration","5"));
 assert(!strcmp(settings.mqttBase,"custom-old-topic"));
 strcpy(settings.mqttBase,"nerd-clock");strcpy(mqttClientID,"nerd-clock-123456789abc");
 strcpy(discoveryID,"ledclock-123456789abc");settings.discovery=1;
 // Configs must use custom base, while stable HA identities survive renaming.
 assert(discoveryTopic(0)=="homeassistant/number/ledclock-123456789abc/brightness/config");
 mqtt.online=true;mqtt.messages.clear();requestDiscovery();
 const int before=mqtt.publishAttempts;serviceDiscovery();assert(discoverySent==1);
 serviceDiscovery();assert(mqtt.publishAttempts==before+1); // throttle within same tick
 for(int i=1;i<22;i++){testMillis+=250;serviceDiscovery();}
 assert(discoverySent==22&&!discoveryPending&&discoveryCompleted);
 assert(mqtt.messages.size()==22);for(const auto &m:mqtt.messages){assert(m.retained);assert(m.payload.find("nerd-clock/availability")!=std::string::npos);assert(m.payload.find("Nerd-Clock")!=std::string::npos);}
 // Failed publications are retried at the same entity, without a busy loop.
 requestDiscovery();mqtt.publishFails=true;serviceDiscovery();assert(discoveryFailed&&discoverySent==0);
 testMillis+=1999;serviceDiscovery();assert(discoverySent==0);mqtt.publishFails=false;testMillis++;serviceDiscovery();assert(discoverySent==1&&!discoveryFailed);
 // HA birth and a manual resend requeue all retained configs.
 char birthTopic[]="homeassistant/status";byte birth[]={'o','n','l','i','n','e'};
 mqttCallback(birthTopic,birth,6);assert(discoveryPending&&discoverySent==0);
 assert(applyCommand("discovery_resend","1"));settings.discovery=0;assert(!applyCommand("discovery_resend","1"));settings.discovery=1;
 // Saving an unchanged MQTT config must also requeue discovery.
 assert(applyWebSettings("brightness=50&ring=point&ntp=pool.ntp.org&host=broker&port=1883&user=&password=&base=nerd-clock&discovery=1"));
 assert(discoveryPending&&discoverySent==0);
 send("GET /api/mqtt HTTP/1.1\r\nHost: x\r\n\r\n");assert(appClient.output.find("homeassistant")!=std::string::npos);
 mqtt.online=true;requestDiscovery();for(int i=0;i<22;i++){testMillis+=250;serviceDiscovery();}
 testMillis+=900000;serviceDiscovery();assert(discoveryPending&&discoverySent==1);

 // A reconnect sends online; HA birth and completed discovery resend it with state.
 mqtt.messages.clear();mqtt.online=false;mqttAttempted=false;wifiConnected=true;serviceMqtt();
 auto onlineCount=[&](){int count=0;for(const auto &m:mqtt.messages)if(m.topic=="nerd-clock/availability"&&m.payload=="online"){assert(m.retained);count++;}return count;};
 assert(onlineCount()==1);
 for(int i=1;i<22;i++){testMillis+=250;serviceMqtt();}assert(onlineCount()==2);
 mqttCallback(birthTopic,birth,6);serviceMqtt();assert(onlineCount()==3);
 // Heartbeat renews online without a new discovery config.
 settings.discovery=0;discoveryPending=false;testMillis+=60000;serviceMqtt();assert(onlineCount()==4);
 // A failed online send remains pending and recovers after two seconds.
 mqtt.publishFails=true;testMillis+=60000;serviceMqtt();assert(onlineCount()==4);
 assert(mqttDiagnosticsJSON().find("\"availability_pending\":true")!=std::string::npos);
 mqtt.publishFails=false;testMillis+=1999;serviceMqtt();assert(onlineCount()==4);
 testMillis++;serviceMqtt();assert(onlineCount()==5);
 assert(mqttDiagnosticsJSON().find("\"availability_pending\":false")!=std::string::npos);
 settings.discovery=1;

 // OTA: URL validation, bridge capability, async download, verification, install errors.
 assert(otaUrlValid("http://192.168.1.20:8000/Nerd-Clock.ota"));
 assert(!otaUrlValid("https://example.com/file.ota"));assert(!otaUrlValid("http://host/file.bin"));
 assert(!otaUrlValid("http://user@host/file.ota"));assert(!otaUrlValid("http://host:0/file.ota"));
 assert(!otaUrlValid("http://host/a,b.ota"));assert(!otaUrlValid("http://host/a\r\n.ota"));
 WiFi.fw="0.4.1";setupOta();assert(!otaCommand("download","http://host/file.ota"));
 WiFi.fw="0.5.1";setupOta();wifiConnected=true;
 assert(otaCommand("download","http://host/file.ota"));assert(otaBusy());
 serviceOta();assert(OTAUpdate::startCalls==1);
 OTAUpdate::progress=500;testMillis+=500;serviceOta();assert(OTAUpdate::verifyCalls==0);
 OTAUpdate::progress=1000;testMillis+=500;serviceOta();assert(OTAUpdate::verifyCalls==1);
 assert(otaStatusJSON().find("\"stage\":\"ready\"")!=std::string::npos);
 assert(!otaCommand("download","http://host/other.ota"));assert(otaCommand("install",""));
 const int installs=OTAUpdate::updateCalls;serviceOta();assert(OTAUpdate::updateCalls==installs);
 testMillis+=1500;serviceOta();assert(OTAUpdate::updateCalls==installs+1);assert(!otaInstalling());
 assert(otaStatusJSON().find("\"stage\":\"error\"")!=std::string::npos);
 assert(otaCommand("cancel",""));OTAUpdate::verifyResult=-10;
 assert(otaCommand("download","http://host/file.ota"));serviceOta();testMillis+=500;serviceOta();
 assert(!otaCommand("install","")); // CRC failure never programs the board
 assert(otaCommand("cancel",""));OTAUpdate::verifyResult=0;OTAUpdate::progress=0;
 assert(otaCommand("download","http://host/file.ota"));serviceOta();testMillis+=30001;serviceOta();
 assert(otaStatusJSON().find("Download-Zeitlimit")!=std::string::npos);
 assert(otaCommand("cancel",""));OTAUpdate::progress=-26;
 assert(otaCommand("download","http://host/file.ota"));serviceOta();testMillis+=500;serviceOta();
 assert(otaStatusJSON().find("Download fehlgeschlagen")!=std::string::npos);
 assert(otaCommand("cancel",""));OTAUpdate::progress=1000;
 send("GET /api/ota HTTP/1.1\r\nHost: x\r\n\r\n");assert(appClient.output.find("wifi_firmware")!=std::string::npos);
 std::string otaBody="action=download&url=http%3A%2F%2Fhost%2Ffile.ota";
 send("POST /api/ota HTTP/1.1\r\nContent-Length: "+std::to_string(otaBody.size())+"\r\n\r\n"+otaBody);
 assert(otaBusy());serviceOta();testMillis+=500;serviceOta();
 OTAUpdate::updateResult=0;assert(otaCommand("install",""));testMillis+=1500;serviceOta();assert(otaInstalling());

 for(uint8_t i=0;i<22;++i)std::cout<<discoveryJSON(i)<<"\n";
 std::cout<<stateJSON()<<"\n";
 std::cerr<<"Host behavior tests passed.\n";
}
