#ifndef _Esp_Network_h
#define _Esp_Network_h

#include <Arduino.h>
#ifdef ESP32
	#include <WebServer.h>		
#else
	#include <ESP8266WebServer.h>
#endif

#include <WebSocketsServer.h>

#include "basefunc.h"

typedef struct
{
  uint8_t buf[128]; // reserve for other data want to keep in eeprom
  char ssid[32];
  char psw[64];
  char host[32];
} CONFIG_TYPE;
extern CONFIG_TYPE config;
extern void configLoad();
extern void configSave();

#ifdef ESP32
	extern WebServer webServer;
#else
	extern ESP8266WebServer webServer;
#endif

extern WebSocketsServer webSocket;
extern void wifiInit( const String ssid, const String psw,  const String host = "",
                      std::function<void ()> userWebService = NULL,
                      std::function<void (uint8_t num, WStype_t type, uint8_t * payload, size_t length)> userWebSocketEvent = NULL);
extern void wifiConfig();
extern void wifiProcess();

#endif
