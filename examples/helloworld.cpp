#include "EspNetwork.h""
#define _APP_DEBUG

typedef enum {
  NONE_STATE,
  AP_STATE,
  WIFI_STATE,
  WORK_STATE,
} APP_STATE;
volatile APP_STATE app_state = NONE_STATE;

void handleTest() {
  webServer.send(200, "text/plain", "this is a web service test.");
}

void webServerServiceSetup() {
  webServer.on("/test", handleTest);  
}

void initWifi() {
  configLoad();

  int16_t ssid_len = strlen(config.ssid);
  if (ssid_len <= 0 || ssid_len >= 32) {
    DebugPrintln("no valid ssid found, config wifi now...");
    app_state = AP_STATE;
    
    wifiConfig();
  }
  else {
    DebugPrintln("found wifi config with ssid:" + String(config.ssid) + " psw:" + (String)config.psw);
    app_state = WIFI_STATE;
        
    wifiInit(config.ssid, config.psw, "", webServerServiceSetup);
  }
}

void setup() {
  DebugBegin(9600);
  delay(100);

  // version
  DebugPrintVersion();

  // info
  DebugPrintln("core version: " + ESP.getCoreVersion());
  DebugPrintln("flash size: " + String(ESP.getFlashChipRealSize() / 1024) + "KBytes");
  DebugPrintln("mcu frequence: " + String(ESP.getCpuFreqMHz()) + "Mhz");
  
  // network
  initWifi();

  app_state = WORK_STATE;
}

void loop() {
  wifiProcess();
  delay(10);
}