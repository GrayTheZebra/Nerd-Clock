#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#define F(x) x
#define ARDUINO_UNOR4_WIFI 1
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define LOW 0
#define HIGH 1
#define OUTPUT 1
extern uint32_t testMillis;
inline uint32_t millis(){return testMillis;}
inline long random(long a,long){return a;}
extern uint8_t pinLevels[32];
inline void digitalWrite(uint8_t p,int value){pinLevels[p]=value;}
inline void pinMode(uint8_t,int){}
inline void noInterrupts(){}
inline void interrupts(){}
class String : public std::string {public: using std::string::string;using std::string::operator+=;using std::string::operator=;String(const std::string&s):std::string(s){}String(unsigned int n):std::string(std::to_string(n)){}String(int n):std::string(std::to_string(n)){}void reserve(size_t n){std::string::reserve(n);}};
using byte=uint8_t;
using std::min;

struct SerialStub {void begin(int){} int available(){return 0;} int read(){return -1;} template<class T> void print(T const&){} template<class T> void println(T const&){} void println(){} }; inline SerialStub Serial;

inline void NVIC_SystemReset(){}
