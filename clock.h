#ifndef _CLOCK_H_3147
#define _CLOCK_H_3147

#include <ArduinoJson.h>

typedef struct {
  uint8_t ucWeekday = 0, ucHour = 0, ucMinute = 0;
  String strAlarmMode = "F";
  String strAlarmSound = "none";
} ALARM_STRUCT;

typedef struct {
  String    strNtpServer = "ntp1.aliyun.com";
  int8_t    cTimeZone = 8;
  uint8_t   ucFontSlant = 0;
  String    strFontMode = "T";
  String    strBkgroundMode = "R";
  std::vector<ALARM_STRUCT> vctAlarm;
} CLOCK_STRUCT;
extern CLOCK_STRUCT clockCfg;

extern void clockBegin(std::function<void (uint8_t idx)> cb_alarm = NULL);
extern void clockProcess();
extern void clockGetTime(int& hours, int& minutes, int& seconds);
extern void clockSaveCfg(DynamicJsonDocument* pdoc = NULL);

extern void handleAlarm();
extern void handleAlarmCfg();

#endif
