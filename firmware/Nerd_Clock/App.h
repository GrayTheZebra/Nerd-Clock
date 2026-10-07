// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <FspTimer.h>
#include <WiFiS3.h>
#include <HiTECH_R4_Wifi_Manager.h>
#include <Arduino_LED_Matrix.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <stddef.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <string.h>
#if !defined(ARDUINO_UNOR4_WIFI)
#error "Select Arduino UNO R4 WiFi in the Arduino IDE."
#endif

#define NERD_CLOCK_VERSION "1.1.1"

constexpr uint32_t MATRIX_IP_DURATION_MS = 60000UL;
constexpr uint32_t MATRIX_SCROLL_STEP_MS = 100UL;
constexpr uint8_t DATA_PIN = 2;   // Holtek socket 17
constexpr uint8_t CLOCK_PIN = 3;  // Holtek socket 18: SCK and RCK tied
constexpr uint8_t GROUP_PINS[8] = {4, 5, 6, 7, 8, 9, 10, 11};
constexpr uint8_t RING_PINS[8] = {A0, A1, A2, A3, A4, A5, 12, 13};
constexpr int SETTINGS_ADDRESS = 1024;
constexpr uint32_t SETTINGS_MAGIC = 0x55485233UL;

struct ClockSettings {
  uint32_t magic;
  uint8_t brightness;
  uint8_t ringStyle;
  uint16_t mqttPort;
  char ntp[64];
  char mqttHost[64];
  char mqttUser[64];
  char mqttPassword[64];
  char mqttBase[64];
  uint16_t dateDuration;
  uint16_t textDuration;
  uint16_t nightStart;
  uint16_t nightEnd;
  uint8_t nightEnabled;
  uint8_t nightBrightness;
  uint8_t nightRingOff;
  uint8_t timerProgress;
  uint8_t discovery;
  uint8_t reserved[3];
  uint32_t crc;
};
enum DisplayMode : uint8_t { MODE_CLOCK, MODE_TIMER, MODE_TEXT };

extern const char SETUP_SSID[];
extern const char SETUP_PASSWORD[];
extern WiFiManager wifiManager;
extern ArduinoLEDMatrix matrix;
extern bool matrixReady;
extern uint8_t matrixFrame[8][12];
extern bool matrixAPShown;
extern bool matrixIPActive;
extern char matrixIP[16];
extern uint32_t matrixIPStarted;
extern uint32_t lastMatrixStep;
extern int16_t matrixScrollX;
extern char serialCommand[24];
extern uint8_t serialUsed;
extern uint32_t secondsOfDay;
extern uint32_t utcEpoch;
extern bool timeSynced;
extern WiFiUDP ntpUDP;
extern IPAddress ntpAddress;
extern uint8_t ntpToken[8];
extern bool udpReady;
extern bool ntpPending;
extern bool wifiConnected;
extern bool ntpAttempted;
extern bool ntpSucceeded;
extern bool noModuleReported;
extern uint32_t lastNetworkCheck;
extern uint32_t lastNtpAttempt;
extern uint32_t lastNtpSuccess;
extern uint32_t ntpSentAt;
extern uint32_t secondTick;
extern FspTimer displayTimer;
extern volatile uint8_t pendingSegments[8];
extern volatile uint8_t pendingRing[8];
extern volatile bool pendingReady;
extern uint8_t activeSegments[8];
extern uint8_t activeRing[8];
extern bool displayRunning;
extern uint32_t renderedTime;
extern ClockSettings settings;
extern DisplayMode displayMode;
extern const char *const RING_NAMES[4];
extern char displayText[5];
extern char visibleText[5];
extern uint32_t timerStarted;
extern uint16_t timerDuration;
extern uint16_t timerRemaining;
extern uint32_t timerSegmentMs;
extern bool timerPaused;
extern bool dateActive;
extern uint32_t dateStarted;
extern uint32_t dateTimeoutMs;
extern uint32_t textStarted;
extern uint32_t textTimeoutMs;
extern volatile bool alarmActive;
extern volatile uint32_t alarmStarted;
extern bool timerEventPending;
extern uint32_t timerEventEpoch;
extern uint16_t timerEventDuration;
extern bool discoveryPending;
extern uint8_t discoveryIndex;
extern char discoveryID[32];
extern uint8_t discoverySent;
extern bool discoveryFailed;
extern bool discoveryAttempted;
extern uint32_t lastDiscoveryAttempt, lastDiscoveryComplete;
extern bool discoveryCompleted;
extern bool displayDirty;
extern volatile uint8_t displayBrightness;
extern bool settingsDirty;
extern uint32_t settingsChanged;
extern WiFiClient mqttTransport;
extern PubSubClient mqtt;
extern char mqttClientID[32];
extern uint32_t lastMqttAttempt;
extern bool mqttAttempted;
extern bool stateDirty;
extern uint32_t lastMqttState;
extern WiFiServer appServer;
extern WiFiClient appClient;
extern bool appClientActive;
extern char appRequest[3072];
extern size_t appUsed, appHeaderSize, appBodySize;
extern uint32_t appClientSince;
extern int previousManagerStatus;

uint32_t configCRC(const uint8_t *bytes, size_t count);
bool terminated(const char *p, size_t n);
bool hostValid(const char *p, bool emptyAllowed);
bool baseValid(const char *p);
bool configValid(const ClockSettings &c);
void loadSettings();
bool saveSettings();
void settingsUpdated();
bool unsignedValue(const char *text, uint32_t maximum, uint32_t &out);
bool boolValue(const char *value, bool &out);
bool minuteValue(const char *value,uint16_t &out);
bool isNight();
uint32_t timerMilliseconds();
void returnToClock();
bool applyCommand(const char *command,const char *value,bool fromMqtt = false);
void updateTimer();
void serviceModes();
uint32_t berlinEpoch(uint32_t epoch);
void addRingLED(uint8_t led,uint8_t *ring);
void buildRing(uint8_t position,uint8_t style,uint8_t *ring);
void mqttTopic(char *out,size_t n,const char *suffix);
void mqttCallback(char *topic,byte *payload,unsigned int length);
void configureMqtt(bool clearDiscovery = false);
String stateJSON();
void serviceMqtt();
String discoveryTopic(uint8_t i);
String discoveryJSON(uint8_t i);
void removeDiscovery();
void serviceDiscovery();
void requestDiscovery();
String mqttDiagnosticsJSON();
int hexValue(char c);
bool formField(const char *body,const char *key,char *out,size_t capacity);
String jsonEscape(const char *s);
String settingsJSON();
void webReply(const char *status,const char *type,const char *body);
bool applyWebSettings(const char *body);
void handleWebRequest();
void serviceWeb();
void allOff();
void write595(uint8_t value);
void clearMatrix();
void showMatrixAP();
void onSetupAP();
void startMatrixIP(const IPAddress &ip);
void serviceMatrix();
void serviceSerial();
void appSetup();
uint8_t weekday(int year, uint8_t month, uint8_t day);
uint32_t berlinSeconds(uint32_t epoch);
void requestNtp();
void receiveNtp();
void serviceNetwork();
void publishDisplay();
void displayTick(timer_callback_args_t *args);
bool startDisplayTimer();
void appLoop();

void setupOta();
void serviceOta();
bool otaBusy();
bool otaUrlValid(const char *url);
bool otaCommand(const char *action,const char *url);
const char *otaLastError();
String otaStatusJSON();
bool otaInstalling();
