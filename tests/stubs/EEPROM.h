#pragma once
#include <cstring>
struct EEPROMStub {uint8_t memory[4096]={}; template<class T> T& get(int address,T& value){memcpy(&value,memory+address,sizeof(T));return value;} template<class T> const T& put(int address,const T& value){memcpy(memory+address,&value,sizeof(T));return value;}}; inline EEPROMStub EEPROM;
