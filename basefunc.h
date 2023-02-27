#ifndef _BASE_FUNC_H
#define _BASE_FUNC_H

#include <stdarg.h>
#include <arduino.h>

extern void addDebugMsg(const char* msg);
extern void addDebugMsg(String msg);
extern void addDebugMsgln(const char* msg);
extern void addDebugMsgln(String msg);
extern void getDebugMsg(String& msg, int idx = -1);
extern void addDebugArgsMsg(const char *format, ...);
    
#ifndef DebugBegin
  #if defined(_APP_DEBUG)
    #if ARDUINO_USB_CDC_ON_BOOT && defined(USE_DEVELOP_KIT)
      #define DebugBegin(baud_rate)    Serial0.begin(baud_rate)
      #define DebugPrintln(message)    Serial0.println(message)
      #define DebugPrint(message)      Serial0.print(message)
      #define DebugPrintf(format, ...) Serial0.printf(format, ##__VA_ARGS__)
    #else  
      #define DebugBegin(baud_rate)    Serial.begin(baud_rate)
      #define DebugPrintln(message)    Serial.println(message)
      #define DebugPrint(message)      Serial.print(message)
      #define DebugPrintf(format, ...) Serial.printf(format, ##__VA_ARGS__)
    #endif
  #elif defined(_MSG_DEBUG)
    #define DebugBegin(baud_rate)
    #define DebugPrintln(message)   addDebugMsgln(message)
    #define DebugPrint(message)     addDebugMsg(message)
    #define DebugPrintf(format, ...) addDebugArgsMsg(format, ##__VA_ARGS__)
  #else
    #define DebugBegin(baud_rate)
    #define DebugPrintln(message)
    #define DebugPrint(message)
    #define DebugPrintf(format, ...)
  #endif
#endif

#define DebugPrintVersion() 	DebugPrintln("\nbuild time: " + String(__DATE__) + ", " + String(__TIME__))


#if defined(FS_SPIFFS)
  #ifdef ESP32
    #include "SPIFFS.h"
  #else
  #endif

  #define MyFs SPIFFS
#else
  #include <LittleFS.h>

  #define MyFs LittleFS
#endif


class CTimerMs {
  public:
    CTimerMs(unsigned long interval = 100) {
      _msInterval = interval;
    }

    void startUp(unsigned long interval = 0) {
      if (interval != 0)
        _msInterval = interval;

      this->update();
    }
    void reset() {
      _msUpdate = 0, _countUpdate = 0;
    }
    void update() {
      _msUpdate = millis();
      _countUpdate++;
    }
    bool isWorking() const {
		return _msUpdate != 0 && _countUpdate != 0;
    }
    bool isTimeOut() const {
      return millis() >= _msUpdate + _msInterval;
    }
    bool isActive() const {
      return _msUpdate != 0 && !this->isTimeOut();
    }
    bool toNextTime() {
      return this->isTimeOut() ? this->update(), true : false;
    }
    unsigned long getUpdateCount() const {
      return _countUpdate;
    }
  private:
    unsigned long _msInterval = 0;
    unsigned long _msUpdate = 0;
    unsigned long _countUpdate = 0;
};

struct MovingAverageTimer {
  unsigned long usAverage = 0, usBegin = 0, usMax = 0, usMin = 0xffffffff;
  void begin() { usBegin = micros(); }
  void end() { 
    unsigned long usCost = micros() - usBegin;
    usMax = usCost > usMax ? usCost : usMax;
    usMin = usCost < usMin ? usCost : usMin;
    usAverage = (usCost + usAverage) / 2; }
};

extern uint32_t hexStrToUint32 (String strHex);

#endif
