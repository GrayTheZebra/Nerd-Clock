#pragma once
#include <vector>
struct FakePublish {std::string topic,payload;bool retained;};
struct PubSubClient {
 bool publishFails=false;int publishAttempts=0;
 int state(){return online?0:-1;}
 bool online=false;std::vector<FakePublish> messages;void(*cb)(char*,byte*,unsigned int)=nullptr;
 PubSubClient(WiFiClient&){} bool connected(){return online;}void disconnect(){online=false;}
 void setServer(const char*,uint16_t){}void setCallback(void(*p)(char*,byte*,unsigned int)){cb=p;}
 bool setBufferSize(uint16_t){return true;}void setSocketTimeout(uint16_t){}void setKeepAlive(uint16_t){}
 bool publish(const char*t,const char*p,bool r){++publishAttempts;if(publishFails)return false;messages.push_back({t,p,r});return true;}
 bool subscribe(const char*){return true;}
 bool connect(const char*,const char*,int,bool,const char*){return online=true;}
 bool connect(const char*,const char*,const char*,const char*,int,bool,const char*){return online=true;}
 bool loop(){return true;}
};
