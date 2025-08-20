#ifndef _BASE_FUNC_H
#define _BASE_FUNC_H

#include <stdarg.h>
#include <arduino.h>

// 3 options: _APP_DEBUG/_SERIAL_DEBUG, _MSG_DEBUG/_MEM_DEBUG, _LOG_DEBUG
// first one debug msg to Serial
// second one goes to a buffer with max 128 lines
// third one goes to a log file on flush mem or sdcard, max size 2MB, auto trim to 1MB

#ifndef DebugBegin
  #if defined(_APP_DEBUG) || defined(_SERIAL_DEBUG)
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
  #elif defined(_MSG_DEBUG) || defined(_MEM_DEBUG)
    extern void addDebugMsg(const char* msg);
    extern void addDebugMsg(String msg);
    extern void addDebugMsgln(const char* msg);
    extern void addDebugMsgln(String msg);
    extern void getDebugMsg(String& msg, int idx = -1);
    extern void addDebugArgsMsg(const char *format, ...);

    #define DebugBegin(baud_rate)
    #define DebugPrintln(message)   addDebugMsgln(message)
    #define DebugPrint(message)     addDebugMsg(message)
    #define DebugPrintf(format, ...) addDebugArgsMsg(format, ##__VA_ARGS__)
  #elif defined(_LOG_DEBUG)
    extern void logInit(unsigned long baud_rate = 115200UL);
    extern void logPrintln(const char* msg);
    extern void logPrintln(String msg);
    extern void logPrint(const char* msg);
    extern void logPrint(String msg);
    extern void logPrintf(const char *format, ...);
    extern int logRead(uint8_t* buf, size_t len);
    
    #define DebugBegin(baud_rate)    logInit(baud_rate)
    #define DebugPrintln(message)    logPrintln(message)
    #define DebugPrint(message)      logPrint(message)
    #define DebugPrintf(format, ...) logPrintf(format, ##__VA_ARGS__)
  #else
    #define DebugBegin(baud_rate)
    #define DebugPrintln(message)
    #define DebugPrint(message)
    #define DebugPrintf(format, ...)
  #endif
#endif

#define DebugPrintVersion() 	DebugPrintln("\nbuild time: " + String(__DATE__) + ", " + String(__TIME__))

// in platformio.ini set board_build.filesystem = spiffs/littlefs/fatfs
// by default, SPIFFS is used if no other defined
#if defined(FS_LITTLEFS)
  #include <LittleFS.h>

  #define MyFs LittleFS
  //bool begin(bool formatOnFail=false, const char * basePath="/littlefs", uint8_t maxOpenFiles=10, const char * partitionLabel="spiffs");

#elif defined(FS_SDMMC)
  #include <SD_MMC.h>

  #define MyFs SD_MMC
  //bool begin(const char * mountpoint="/sdcard", bool mode1bit=false, bool format_if_mount_failed=false, int sdmmc_frequency=BOARD_MAX_SDMMC_FREQ, uint8_t maxOpenFiles = 5);
  // mode1bit: true for 1-line SD mode, false for 4-line SD mode, it's more fast than 1-line but need more pins
  // make sure define SD_MMC_CLK, SD_MMC_CMD, and SD_MMC_D0 in your platformio.ini
  // or by default, SD_MMC_CLK = 39, SD_MMC_CMD = 38, SD_MMC_D0 = 40

#else // default is SPIFFS
  #ifdef ESP32 || defined(ESP32C3) || defined(ESP32S2) || defined(ESP32S3)
    #include "SPIFFS.h"
  #else
  #endif

  #define MyFs SPIFFS
  //bool begin(bool formatOnFail=false, const char * basePath="/spiffs", uint8_t maxOpenFiles=10, const char * partitionLabel=NULL);
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
