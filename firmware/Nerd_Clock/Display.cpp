// SPDX-License-Identifier: MIT
#include "App.h"

void allOff() {
  for (uint8_t pin : GROUP_PINS) digitalWrite(pin, LOW);
  for (uint8_t pin : RING_PINS) digitalWrite(pin, LOW);
}

void write595(uint8_t value) {
#if defined(__AVR_ATmega328P__)
  // Uno D2=PD2 data, D3=PD3 shared shift/latch clock.
  // Preserve all other PORTD bits, including group and serial pins.
  for (int8_t bit = 7; bit >= 0; --bit) {
    if (value & (uint8_t(1)<<bit)) PORTD |= _BV(PD2);
    else PORTD &= ~_BV(PD2);
    PORTD |= _BV(PD3);
    PORTD &= ~_BV(PD3);
  }
  PORTD &= ~_BV(PD2);
  PORTD |= _BV(PD3); // Ninth edge latches the requested byte.
  PORTD &= ~_BV(PD3);
#else
  for (int8_t bit = 7; bit >= 0; --bit) {
    digitalWrite(DATA_PIN, value & (uint8_t(1)<<bit) ? HIGH : LOW);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);
  }
  digitalWrite(DATA_PIN, LOW);
  digitalWrite(CLOCK_PIN, HIGH);
  digitalWrite(CLOCK_PIN, LOW);
#endif
}

void addRingLED(uint8_t led,uint8_t *ring) {
  uint8_t group=0,line=4;
  if(led) { const uint8_t block=(led-1)/8,offset=(led-1)%8;
    group=7-block;line=block%2?7-offset:offset;
  }
  ring[group]|=uint8_t(1)<<line;
}

void buildRing(uint8_t position,uint8_t style,uint8_t *ring) {
  memset(ring,0,8);
  for(uint8_t led=0;led<60;++led) {
    const bool on=style==0?led==position:style==1?led<=position:style==2?led>=position:led!=position;
    if(on) addRingLED(led,ring);
  }
}

void publishDisplay() {
  const uint8_t digits[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
  const uint8_t bits[7]={1,5,3,5,1,3,7};
  const bool lower[7]={false,false,true,true,true,false,false};
  uint8_t values[4]={255,255,255,255};
  uint8_t segments[8]={},ring[8]={};
  if(dateActive) {
    const time_t local=berlinEpoch(utcEpoch);const struct tm *date=gmtime(&local);
    if(date){values[0]=date->tm_mday/10;values[1]=date->tm_mday%10;values[2]=(date->tm_mon+1)/10;values[3]=(date->tm_mon+1)%10;}
  } else if(displayMode==MODE_TEXT) {
    const size_t n=strlen(displayText);
    for(size_t i=0;i<n;++i)values[4-n+i]=displayText[i]-'0';
  } else {
    const bool ready=displayMode==MODE_TIMER||timeSynced;
    if(ready) {
      const uint16_t left=displayMode==MODE_TIMER?timerRemaining/60:secondsOfDay/3600;
      const uint16_t right=displayMode==MODE_TIMER?timerRemaining%60:(secondsOfDay/60)%60;
      values[0]=left>=10?left/10:255;values[1]=left%10;values[2]=right/10;values[3]=right%10;
    }
  }
  for(uint8_t digit=0;digit<4;++digit)visibleText[digit]=values[digit]<=9?'0'+values[digit]:' ';
  visibleText[4]=0;
  if(displayMode==MODE_CLOCK&&!timeSynced)strcpy(visibleText,"----");
  for(uint8_t group=0;group<8;++group) {
    const uint8_t value=values[group/2];const uint8_t mask=value<=9?digits[value]:0;
    for(uint8_t seg=0;seg<7;++seg)if(lower[seg]==bool(group%2)&&(mask&(uint8_t(1)<<seg)))segments[group]|=uint8_t(1)<<bits[seg];
  }
  if(dateActive||displayMode==MODE_TIMER||(displayMode==MODE_CLOCK&&timeSynced&&secondsOfDay%2==0)) {
    segments[2]|=uint8_t(1)<<4;segments[3]|=uint8_t(1)<<2;
  }
  if(displayMode==MODE_TIMER) {
    if(timerRemaining<=60) {if(timerRemaining%2==0)for(uint8_t r=0;r<60;++r)addRingLED(r,ring);}
    else if(settings.timerProgress) {
      const uint8_t lit=(uint64_t(timerMilliseconds())*60+uint32_t(timerDuration)*1000-1)/(uint32_t(timerDuration)*1000);
      for(uint8_t r=0;r<lit;++r)addRingLED(r,ring);
    } else buildRing(timerRemaining%60,settings.ringStyle,ring);
  } else if(displayMode==MODE_CLOCK&&timeSynced&&!dateActive&&!(isNight()&&settings.nightRingOff))buildRing(secondsOfDay%60,settings.ringStyle,ring);
  if(displayMode==MODE_CLOCK&&!timeSynced)for(uint8_t g=0;g<8;++g)segments[g]=g%2?0:uint8_t(1)<<7;
  noInterrupts();
  for(uint8_t i=0;i<8;++i){pendingSegments[i]=segments[i];pendingRing[i]=ring[i];}
  pendingReady=true;interrupts();renderedTime=secondsOfDay;displayDirty=false;
}

void displayTick(timer_callback_args_t *args) {
  if(args->event!=TIMER_EVENT_CYCLE_END)return;
  static uint8_t phase=0,group=7,onPhases=0;
  static uint16_t brightnessError[8]={};
  if(phase==0) {
    allOff();group=(group+1)%8;
    if(group==0&&pendingReady){for(uint8_t i=0;i<8;++i){activeSegments[i]=pendingSegments[i];activeRing[i]=pendingRing[i];}pendingReady=false;}
    brightnessError[group]+=uint16_t(displayBrightness)*6;
    onPhases=brightnessError[group]/100;brightnessError[group]%=100;
    write595(activeSegments[group]);
    if(onPhases) {
      uint8_t ringMask=activeRing[group];
      const uint32_t alarmElapsed=millis()-alarmStarted;
      if(alarmActive&&alarmElapsed<10000) {
        const uint16_t phaseMs=alarmElapsed%1000;
        const bool flash=phaseMs<100||(phaseMs>=200&&phaseMs<300);
        ringMask=flash?(group==0?0xF0:0xFF):0;
      }
      for(uint8_t line=0;line<8;++line)if(ringMask&(uint8_t(1)<<line))digitalWrite(RING_PINS[line],HIGH);
      digitalWrite(GROUP_PINS[group],HIGH);
    }
  } else if(phase==onPhases)allOff();
  phase=(phase+1)%8;
}

bool startDisplayTimer() {
  uint8_t timerType = GPT_TIMER;
  int8_t index = FspTimer::get_available_timer(timerType);
  if (index < 0) {
    index = FspTimer::get_available_timer(timerType, true);
    if (index < 0) return false;
    FspTimer::force_use_of_pwm_reserved_timer();
  }
  if (!displayTimer.begin(TIMER_MODE_PERIODIC, timerType, uint8_t(index),
                          8000.0f, 0.0f, displayTick)) return false;
  if (!displayTimer.setup_overflow_irq()) return false;
  if (!displayTimer.open()) return false;
  return displayTimer.start();
}

