#pragma once
#include <string>
#define WL_NO_MODULE 255
#define WL_CONNECTED 3
#define WL_AP_LISTENING 7
#define WL_AP_CONNECTED 8
struct IPAddress {uint8_t operator[](int i)const {return i==0?192:i==1?168:i==2?1:42;} bool operator==(const IPAddress&)const{return true;}};
struct WiFiUDP {int begin(int){return 1;} void stop(){} int beginPacket(IPAddress,int){return 1;} void write(uint8_t*,size_t){} int endPacket(){return 1;} int parsePacket(){return 0;} IPAddress remoteIP(){return {};} int remotePort(){return 123;} int read(uint8_t*,size_t){return 0;}};
struct WiFiClient {std::string input,output;size_t pos=0;bool live=false;explicit operator bool()const{return live;} bool connected(){return live;} int available(){return input.size()-pos;} int read(){return pos<input.size()?input[pos++]:-1;} void stop(){live=false;} size_t write(const uint8_t *p,size_t n){output.append((const char*)p,n);return n;} void print(const char*p){output+=p;} void print(const String&p){output+=p;} void print(size_t n){output+=std::to_string(n);} void println(const char*p){print(p);output+="\r\n";} void println(size_t n){print(n);output+="\r\n";} void println(){output+="\r\n";}};
struct WiFiServer {WiFiClient queued;WiFiServer(int){} void begin(){} WiFiClient available(){auto c=queued;queued={};return c;}};
struct WiFiStub{const char* fw="0.5.1";const char* firmwareVersion(){return fw;}int current=0;int status(){return current;} void end(){current=0;} int beginAP(const char*,const char*){return current=WL_AP_LISTENING;} int begin(const char*,const char* = nullptr){return current=WL_CONNECTED;} IPAddress localIP(){return {};} void macAddress(uint8_t *p){memset(p,42,6);} int hostByName(const char*,IPAddress&){return 1;}};inline WiFiStub WiFi;
