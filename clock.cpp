
#ifdef ESP32
	#include <WiFi.h>
	#include "SPIFFS.h"
#else
	#include <ESP8266WiFi.h>
#endif

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <vector>
#include <StreamString.h>

//#define _MSG_DEBUG
#include "EspNetwork.h"
#include "clock.h"


// clock config struct
CLOCK_STRUCT clockCfg;


// for NTP Client
String serverPool[] = {"ntp1.aliyun.com", "ntp2.aliyun.com", "ntp3.aliyun.com", "time1.cloud.tencent.com", "time2.cloud.tencent.com", "time3.cloud.tencent.com" };
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 8 * 3600);


///////////////////////////////////////////////////////////////////////
void clockGetTime(int& hours, int& minutes, int& seconds) {
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  seconds = timeClient.getSeconds();
}

///////////////////////////////////////////////////////////////////////
const char* szClockJson = "/clock.json";
void clockLoadCfg() {
  String jsonClock;

  SPIFFS.begin();
  File f = SPIFFS.open(szClockJson, "r");
  if (f) {
    jsonClock = f.readString();
    f.close();
  }
  else {
    DebugPrintln("open clock.json failed. set all data as default value.");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonClock);
  if (error) {
    DebugPrint(F("deserializeJson() failed: "));
    DebugPrintln(error.f_str());
    return;
  }

  clockCfg.strNtpServer   = (const char*)doc["ntp_server"];
  clockCfg.cTimeZone      = (uint8_t)doc["time_zone"];
  clockCfg.ucFontSlant    = (uint8_t)doc["font_slant"];
  clockCfg.strFontMode    = (const char*)doc["font_mode"];
  clockCfg.strBkgroundMode= (const char*)doc["background"];

  int nAlarmCount = doc["alarm"]["count"];
  for (int i = 0; i < nAlarmCount; i++) {
    ALARM_STRUCT alarm;
    alarm.ucWeekday = doc["alarm"]["set"][i]["weekday"];
    alarm.ucHour    = doc["alarm"]["set"][i]["hour"];
    alarm.ucMinute  = doc["alarm"]["set"][i]["minute"];
    alarm.strAlarmMode    = (const char*)doc["alarm"]["set"][i]["mode"];
    alarm.strAlarmSound   = (const char*)doc["alarm"]["set"][i]["sound"];
    if (alarm.strAlarmSound.length() <= 0) {
      alarm.strAlarmSound = "none";
    }
    
    clockCfg.vctAlarm.push_back(alarm);
    
    char szBuf[256] = "";
    sprintf(szBuf, "alarm%d --- weekday:%x, hour:%d, minute:%d, alarm mode:%s, alarm sound:%s", i,
        alarm.ucWeekday, alarm.ucHour, alarm.ucMinute, alarm.strAlarmMode.c_str(), alarm.strAlarmSound.c_str());
    DebugPrintln(szBuf);
  }
}

void clockSaveCfg(DynamicJsonDocument* pdoc) { 
  DynamicJsonDocument local_doc(1024);
  DynamicJsonDocument& doc = pdoc ? *pdoc : local_doc; 
    
  doc["ntp_server"]   = clockCfg.strNtpServer;
  doc["time_zone"]    = clockCfg.cTimeZone;
  doc["font_slant"]   = clockCfg.ucFontSlant;
  doc["font_mode"]    = clockCfg.strFontMode;
  doc["background"]   = clockCfg.strBkgroundMode;

  doc["alarm"]["count"] = clockCfg.vctAlarm.size();
  for (int i = 0; i < clockCfg.vctAlarm.size(); i++) {
    doc["alarm"]["set"][i]["weekday"] = clockCfg.vctAlarm[i].ucWeekday;
    doc["alarm"]["set"][i]["hour"]    = clockCfg.vctAlarm[i].ucHour;
    doc["alarm"]["set"][i]["minute"]  = clockCfg.vctAlarm[i].ucMinute;
    doc["alarm"]["set"][i]["mode"]    = clockCfg.vctAlarm[i].strAlarmMode.c_str();
    doc["alarm"]["set"][i]["sound"]   = clockCfg.vctAlarm[i].strAlarmSound.c_str();
  }

  File f = SPIFFS.open(szClockJson, "w");
  if (f) {
    serializeJsonPretty(doc, f);
    f.close();
  }
}

///////////////////////////////////////////////////////////////////////
const int SERVER_TEST_MAX = 5;		
int timeServerTest() {
  int i = 0;
  for (i = 0; i < SERVER_TEST_MAX; i++) {
  	int numServers = sizeof(serverPool);
  	DebugPrintln("ntp server nums: " + String(numServers));  	
  	
  	timeClient.begin();
  
  	const unsigned long INTERVAL_MAX = 300;	// ms
  	unsigned long msLast = millis();
  	if (timeClient.update() && millis() < msLast + INTERVAL_MAX) {
  		break;
  	}
  	
  	// test failed, try another NTP time server
  	timeClient.end();
  	
  	String newServer = serverPool[random(numServers)];
  	timeClient.setPoolServerName(newServer.c_str());
  	clockCfg.strNtpServer = newServer;
  	DebugPrintln("try another server: " + newServer);
  }
  return i + 1;	// return  how many time to test
}

///////////////////////////////////////////////////////////////////////
std::function<void (uint8_t idx)> callbackAlarm = NULL;
void clockBegin(std::function<void (uint8_t idx)> cb_alarm) {
  clockLoadCfg();
  
  timeClient.setTimeOffset(clockCfg.cTimeZone * 3600);  
  DebugPrintln("time zone: " + String(clockCfg.cTimeZone));
  timeClient.setPoolServerName(clockCfg.strNtpServer.c_str());
  DebugPrintln("ntp server: " + clockCfg.strNtpServer);  
		
  //timeClient.begin();
  int rval = timeServerTest();
  if (rval > 1) {
  	DebugPrintln("set new ntp server." + clockCfg.strNtpServer);  
  }
  else if (rval >= SERVER_TEST_MAX) {
  	DebugPrintln("error: too many times trying server.");
  }

  callbackAlarm = cb_alarm;
}

///////////////////////////////////////////////////////////////////////
void clockProcess() {
  timeClient.update();
    
  static int nLastMinute = 0;
  int nMinute = timeClient.getMinutes();
  if (nMinute == nLastMinute) // prevent reentry in the same minute
    return;

  // keep minute if the num is different with last minute
  nLastMinute = nMinute;
          
  int idxOnce = -1;
  int count = clockCfg.vctAlarm.size();
  for (int i = 0; i < count; i++) {
    ALARM_STRUCT alarm = clockCfg.vctAlarm[i];
    if (alarm.ucWeekday & (1 << timeClient.getDay())) {
      if (alarm.ucHour == timeClient.getHours()) {
        if (alarm.ucMinute == timeClient.getMinutes()) {
          if (alarm.ucWeekday & 0x80) {  // alarm once only
            idxOnce = i;            
          }
          
          // alarm performance
          char szBuf[256] = "";
          sprintf(szBuf, "alarm%d triggered --- weekday:%x, hour:%d, minute:%d, alarm mode:%s, alarm sound:%s", i,
              alarm.ucWeekday, alarm.ucHour, alarm.ucMinute, alarm.strAlarmMode.c_str(), alarm.strAlarmSound.c_str());              
          DebugPrintln(szBuf);
          
          if (callbackAlarm)
            callbackAlarm(i);
          break;
        }
      }      
    }
  }

  if (idxOnce != -1) {
    clockCfg.vctAlarm.erase(clockCfg.vctAlarm.begin() + idxOnce);
    clockSaveCfg();
  }
}

///////////////////////////////////////////////////////////////////////
void handleAlarm() {
  String htmlAlarm;
  
  SPIFFS.begin();
  File f = SPIFFS.open("/alarm.html", "r");
  if(f) {
    htmlAlarm = f.readString();
    f.close();
  }
  else {
    webServer.send(404, "text/plain", "alarm.html not found.");
    return;
  }
  
  String strAlarmSet = "";
  int countAlarm = clockCfg.vctAlarm.size();
  for (int i = 0; i < countAlarm; i++) {
    // dt
    String strAlarm = "<dt ";
    strAlarm += (i % 2) == 0 ? "class='light'>" : "class='dark'>";

    // time input
    strAlarm += "<input type='time' ";
    char szTime[16] = "";
    sprintf(szTime, "%02d:%02d", clockCfg.vctAlarm[i].ucHour, clockCfg.vctAlarm[i].ucMinute);
    strAlarm += "value='" + String(szTime) + "'> ";

    // weekday buttons
    uint8_t ucWeekday = clockCfg.vctAlarm[i].ucWeekday;
    for (int d = 0; d < 7; d++) {
      strAlarm += "<button onclick='btn_switch(this)' ";      
      strAlarm += ucWeekday & (1 << d) ? "class='enable'" : "class='disable'";
      strAlarm += ">" + String(d) + "</button> ";
    }

    // once only button
    strAlarm += "<button onclick='btn_switch(this)' ";
    strAlarm += ucWeekday & 0x80 ? "class='enable'" : "class='disable'";
    strAlarm += " style='width:10%;'>once</button> ";

    // mode button
    strAlarm += "<button onclick='btn_switch(this)' class='enable' name='mode'>";
    strAlarm += clockCfg.vctAlarm[i].strAlarmMode;
    strAlarm += "</button> ";

    // sound button
    strAlarm += "<button onclick='btn_switch(this)' name='sound' style='width:7%;' ";
    strAlarm += clockCfg.vctAlarm[i].strAlarmSound != "" ? "class='enable' " : "class='disable' ";
    strAlarm += String("sound='") + clockCfg.vctAlarm[i].strAlarmSound + String("'>");
    strAlarm += "S</button>";

    // delete button
    strAlarm += "-";
    strAlarm += "<button onclick='btn_switch(this)' class='enable' style='width:6%;' name='del'>x</button>";

    // close dt
    strAlarm += "</dt>\n";

    // add to set
    strAlarmSet += strAlarm;
  }
  
  htmlAlarm.replace("alarm_list_replace", strAlarmSet);
  webServer.send(200, "text/html", htmlAlarm);
}

///////////////////////////////////////////////////////////////////////
void handleAlarmCfg() {
  clockCfg.vctAlarm.clear();
  
  int numAlarm = 0;
  while(true) {
    String strAlarm = "alarm" + String(numAlarm);
    if (!webServer.hasArg(strAlarm))
      break;
      
    String strParam = webServer.arg(strAlarm);
    DebugPrintln(strAlarm + ": " + strParam);
    
    String strHour = strParam.substring(0, 2);
    String strMinute = strParam.substring(3, 5);
    String strWeekday = strParam.substring(6, 8); // hex num
    String strMode = strParam.substring(9, 10);
    String strSound = strParam.substring(11);

    ALARM_STRUCT alarm;
    alarm.ucHour = strHour.toInt();
    alarm.ucMinute = strMinute.toInt();

    uint8_t high = strWeekday[0] >= 'a' ? strWeekday[0] - 'a' + 10 : strWeekday[0] - '0';
    uint8_t low = strWeekday[1] >= 'a' ? strWeekday[1] - 'a' + 10 : strWeekday[1] - '0';
    alarm.ucWeekday = high * 16 + low;
    
    alarm.strAlarmMode = strMode;
    alarm.strAlarmSound = strSound;
    
    clockCfg.vctAlarm.push_back(alarm);      
    numAlarm++;
  }

  clockSaveCfg();

  String strData = "done!";
  webServer.send(200, "text/plain", strData);
}

