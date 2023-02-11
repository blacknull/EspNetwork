#ifdef ESP32
	#include <WiFi.h>
	#include <WebServer.h>
	#include <ESPmDNS.h>
	#include <HTTPUpdateServer.h>
#else
	#include <ESP8266WiFi.h>
	#include <ESP8266WebServer.h>
	#include <ESP8266mDNS.h>
	#include <ESP8266HTTPUpdateServer.h>
#endif

#include <WiFiClient.h>
#include <DNSServer.h>
#include <FS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#include "flash_strings.h"
#include "EspNetwork.h"

/*
typedef struct DEVICE_SETTING_TYPE {
  String mdns;
  String ap_name;  
};
DEVICE_SETTING_TYPE deviceSetting;
//*/
const char* DEFAULT_HOST = "esp_device";

const unsigned long TIMEOUT_WIFI        = 20 * 1000;      // 20s
const unsigned long TIMEOUT_WIFICONFIG  = 5 * 60 * 1000;  // 5 minutes

const int MAX_WIFI_CNCT = 3;  // wifi connect retry times

CONFIG_TYPE config;
#ifdef ESP32
	WebServer webServer(80);
	HTTPUpdateServer httpUpdater;
#else
	ESP8266WebServer webServer(80);
	ESP8266HTTPUpdateServer httpUpdater;
#endif

const int CONFIG_SIZE = 256;
void configLoad() {
  memset(&config, 0L, sizeof(config));
  EEPROM.begin(CONFIG_SIZE);
  uint8_t *p = (uint8_t*)(&config);
  for (int i = 0; i < sizeof(config); i++)
  {
    *(p + i) = EEPROM.read(i);
  }
  EEPROM.end();
}

void configSave() {
  uint8_t *p = (uint8_t*)(&config);
  EEPROM.begin(CONFIG_SIZE);
  for (int i = 0; i < sizeof(config); i++)
  {
    EEPROM.write(i, *(p + i));
  }
  EEPROM.end();
}

/////////////////////////////////////////////////////////////////////////////////
/*
void deviceSettingLoad() {
  String jsonSetting;

  SPIFFS.begin();
  File f = SPIFFS.open("/setting.json", "r");
  if (f) {
    jsonSetting = f.readString();
    f.close();

    DebugPrintln(jsonSetting);
  }
  else {
    DebugPrintln("open setting.json failed. set mdns[esp], ap_name[esp]");
    deviceSetting.mdns = "esp";
    deviceSetting.ap_name = "esp";
    return;
  }

  DynamicJsonDocument doc(256);
  deserializeJson(doc, jsonSetting);

  deviceSetting.mdns = (const char*)(doc["mdns"]);
  deviceSetting.ap_name = (const char*)(doc["ap_name"]);

  DebugPrintln("set mdns: " +  deviceSetting.mdns);
  DebugPrintln("set ap_name: " +  deviceSetting.ap_name);
}
//*/

/////////////////////////////////////////////////////////////////////////////////
String htmlLogin = "<span style='font-size:24px'><h1>Accessing WiFi...</h1><br>\
      <h1>Press relogin button if no respones after 30 seconds.</h1><br></span>\
      <form name='input' action='/'>\r\n\
        <input type='submit' value='relogin'  style='width:120px;height:60px'>\
      </form>";

void handleSetCfg() {
  String ssid  = webServer.arg("ssid");
  String psw   = webServer.arg("psw");
  if (ssid.length() <= 0 || psw.length() <= 0)
    return;

  strcpy(config.ssid, ssid.c_str());
  strcpy(config.psw, psw.c_str());
  configSave();    

  delay(500);
  ESP.restart();    
}

void handleLogin() {
  String ssid  = webServer.arg("Wifi");
  String psw   = webServer.arg("Password");
  String host = webServer.arg("Host");
  DebugPrintln("get the wifi ssid:" + ssid);
  DebugPrintln("get the wifi password:" + psw);
  
  if (host == "")  host = DEFAULT_HOST;
  DebugPrintln("get the host name:" + host);

  // network init
  DebugPrintln("connet to " + ssid);
  webServer.send(200, "text/html", htmlLogin);
  delay(1000);

  WiFi.begin(ssid.c_str(), psw.c_str());  
  unsigned long time_begin = millis();
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    DebugPrint ( "." );
    if (millis() > time_begin + TIMEOUT_WIFI) {
      DebugPrintln ( "\nconnection time out. restarting..." );
      delay(50);
      ESP.restart();
      break;
    }
  }

  if (millis() < time_begin + TIMEOUT_WIFI) {
    webServer.send(200, "text/html", "<h1>Congratuation! Wifi connect succeed.</h1>");
    delay(1000);
    
    strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid));
    strncpy(config.psw, psw.c_str(), sizeof(config.psw));
    strncpy(config.host, host.c_str(), sizeof(config.host));
    configSave();
    
    DebugPrintln ("wifi connected with " + ssid);
    delay(1000);
    WiFi.softAPdisconnect(true);
    ESP.restart();
  }
}

/////////////////////////////////////////////////////////////////////////////////
void handleSetting() {
  /*
  if (!webServer.hasArg("mdns") || !webServer.hasArg("ap_name")) {
    webServer.send(200, "text/html", "<h2>wrong args!</h2>");
    return;
  }

  String mdns     = webServer.arg("mdns");
  String ap_name  = webServer.arg("ap_name");
  String setting = "{\n\"mdns\":\"" + mdns + "\",";
  setting += "\"ap_name\":\"";
  setting += ap_name;
  setting += "\"}\n";

  //DebugPrintln("new setting:\n" + setting);
  File f = SPIFFS.open("/setting.json", "w");
  if (f) {
    f.write(setting.c_str(), setting.length());
    f.close();

    DebugPrintln("new setting:\n" + setting);
  }
  else {
    DebugPrintln("json file open failed.");
  }
  //*/
  if (!webServer.hasArg("host"))  {
     webServer.send(200, "text/html", "<h2>no host, wrong args!</h2>");
    return;
  }
  String host = webServer.arg("host");
  if (host != "") {
    webServer.send(200, "text/html", "<h2>setting host</h2>");
	  strncpy(config.host, host.c_str(), sizeof(config.host));
	  configSave();
	  delay(500);
	  ESP.restart();
  }
}

/////////////////////////////////////////////////////////////////////////////////
void handleReset() {
  if (webServer.hasArg("clr_cfg")) {
    webServer.send(200, "text/html", "clear cfg info and restart...");
    delay(3000);
    
    memset(&config, 0L, sizeof(CONFIG_TYPE));
    configSave();
  }
  else {
  	webServer.send(200, "text/html", "restarting...");
  	delay(3000);
  }
  
  ESP.restart();
}

/////////////////////////////////////////////////////////////////////////////////
static const char DIR_HTML[] PROGMEM = "<!DOCTYPE html><html lang='en'> \
<head><style>\
  dt { text-align:left; font-size: 20px;} \
  .light { background-color:#eeeeee;}\
  .dark { background-color:#c0c0c0;}\
</style></head> \
<body>replace</body></html>";

#ifdef ESP32
void handleDir()
{
  String strExt = "";
  if (webServer.hasArg("ext")) {
    strExt = webServer.arg("ext");
  }

  //SPIFFS.begin();
  String infoDir;
  File root = SPIFFS.open("/");
  if (!root) {
    DebugPrintln("- failed to open directory");
    return;
  }
  
  int count = 0;
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      DebugPrint("  DIR : ");
      DebugPrintln(file.name());

      file = root.openNextFile();
      continue;
    }

    String fileName = file.name();
    if (strExt != "") {
      if (!fileName.endsWith(strExt)) {
        file = root.openNextFile();
        continue;
      }
    }
    size_t fileSize = file.size();

    String infoFile;
    if (count % 2 == 0) {
      infoFile = "<dt class='dark'";
    }
    else {
      infoFile = "<dt class='light'";
    }

    infoFile += " name=" + String(count);
    infoFile += " onclick=\"choose(this)\">/";  // '/' must needed.
    infoFile += fileName;
    infoFile += "--------";
    infoFile += String(fileSize);
    infoFile += " bytes</dt>";

    infoDir += infoFile;
    count++;
    file = root.openNextFile();
  }

  String html = FPSTR(DIR_HTML);
  html.replace("replace", infoDir);
  //DebugPrintln(infoDir);
  webServer.send(200, "text/html", html);
}

#else
void handleDir() {
  String strExt = "";
  if (webServer.hasArg("ext")) {
  	strExt = webServer.arg("ext");
  }
  
  SPIFFS.begin();
  String infoDir;
  Dir dir = SPIFFS.openDir("/");
  int count = 0;
  while (dir.next()) {
    String fileName = dir.fileName();
    if (strExt != "") {
    		if (!fileName.endsWith(strExt))
    			continue;
    }
    size_t fileSize = dir.fileSize();

    String infoFile;
    if (count % 2 == 0) {
      infoFile = "<dt class='dark'";
    }
    else {
      infoFile = "<dt class='light'";
    }
	
	infoFile += " name=" + String(count);
    infoFile += " onclick=\"choose(this)\">";
    infoFile += fileName;
    infoFile += "--------";
    infoFile += String(fileSize);
    infoFile += " bytes</dt>";

    infoDir += infoFile;

    count++;
  }

  String html = FPSTR(DIR_HTML);
  html.replace("replace", infoDir);
  webServer.send(200, "text/html", html);
}
#endif

/////////////////////////////////////////////////////////////////////////////////
static const char HELP_HTML[] PROGMEM = "<!DOCTYPE html><html lang='en'> \
<head><style>body { text-align:left; font-size: 20px;}</style></head>\
<body>\
/help -- show this message</br>\
/dir -- list files in spiffs</br>\
/file -- file manager</br>\
/debugmsg -- show debug msg</br>\
/reset -- restart this device</br>\
/reset?clr_cfg=1 -- clear wifi config and restart this device</br>\
/setting?host=host_name</br>\
</body></html>";

void handleHelp() {
  String html = FPSTR(HELP_HTML);
  webServer.send(200, "text/html", html);
}

void handleFile() {
  String html = FPSTR(FILE_MGR_HTML);
  webServer.send(200, "text/html", html);
}

void handleDebugMsg() {
  String strDebug = "debug msg:\n";
  getDebugMsg(strDebug);
  webServer.send(200, "text/plain", strDebug);
}

/////////////////////////////////////////////////////////////////////////////////
void handleRemove() {
  if (!webServer.hasArg("file")) {
    webServer.send(200, "text/html", "<h2>wrong args! must be: file=fullpath</h2>");
    return;
  }

  String target = webServer.arg("file");
  if (!target.startsWith("/")) {
    target = "/" + target;
  }
  
  if (SPIFFS.remove(target)) {
    DebugPrintln(target + " has removed!");
  }
  else {
    DebugPrintln("failed to remove " + target);
  }

  handleDir();
}

/////////////////////////////////////////////////////////////////////////////////
File fsUploadFile;
void handleFileUpload() {
  //判断http requestUri
  if (webServer.uri() != "/upload") {
    webServer.send(404, "text/plain", "not an upload command.");
    return;
  }

  //获得 Http上传文件处理对象
  HTTPUpload& upload = webServer.upload();

  //文件开始上传
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    DebugPrintln("handleFileUpload Name: " + filename);

    //本地文件系统创建一个文件用来保存内容
    fsUploadFile = SPIFFS.open(filename, "w");
    if (!fsUploadFile) {
      DebugPrintln(filename + "open failed.");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //文件开始写入文件
    //DebugPrintln("handleFileUpload Data: " + String(upload.currentSize));
    if (fsUploadFile) {
      //写入文件
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    //文件上传结束
    if (fsUploadFile) {
      fsUploadFile.close();
      DebugPrintln("handleFileUpload done. total size: " + String(upload.totalSize));
      handleDir();  // send file list page
    }    
  }
}

/////////////////////////////////////////////////////////////////////////////////
// 获取文件类型
String getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".mp3")) return "audio/mp3";
  return "text/plain";
} 

/////////////////////////////////////////////////////////////////////////////////
bool handleFileRead(String path, String type = "") {            //处理浏览器HTTP访问  
  String contentType = getContentType(path);  // 获取文件类型
  if (type != "")
    contentType = type;
    
  //SPIFFS.begin();
  if (!SPIFFS.exists(path))
    return false;

  File file = SPIFFS.open(path, "r");          // 则尝试打开该文件
  if (!file) {
    webServer.send(404, "text/plain", "file open error.");
    return false;
  }
  else {
    if (contentType == "application/octet-stream") {
      int idx = path.lastIndexOf('/');
      if (idx == -1) 
        idx = 0;
  
      webServer.sendHeader("Content-Disposition", "attachment; filename=" + path.substring(idx + 1));
      DebugPrintln("download mode set save name:" + path.substring(idx + 1));
    }

    DebugPrintln("webServer stream file: " + path + "," + contentType);
    webServer.streamFile(file, contentType);    // 并且将该文件返回给浏览器
    file.close();                               // 并且关闭文件
    return true;                                // 返回true
  }
}

/////////////////////////////////////////////////////////////////////////////////
void handleUserRequest() {
  bool fileReadOK = handleFileRead(webServer.uri());
  if (!fileReadOK) {
    webServer.send(404, "text/plain", "404 Not Found");
    DebugPrintln(webServer.uri() + " 404 Not Found.");
  }
  else {
    DebugPrintln(webServer.uri() + " send ok.");
  }
}

/////////////////////////////////////////////////////////////////////////////////
void handleDownload() {
  if (!webServer.hasArg("file")) {
    webServer.send(200, "text/html", "<h2>wrong args! must be: file=fullpath</h2>");
    return;
  }

  String target = webServer.arg("file");
  if (!target.startsWith("/")) {
    target = "/" + target;
  }

  handleFileRead(target, "application/octet-stream");
}

/////////////////////////////////////////////////////////////////////////////////
// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

/* Soft AP network parameters */
//IPAddress apIP(192, 168, 4, 1);
IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);

/** Is this an IP? */
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(webServer.hostHeader()) && webServer.hostHeader() != "esp_cfg.local") {
    DebugPrintln("Request redirected to captive portal");
    webServer.sendHeader("Location", String("http://") + toStringIp(webServer.client().localIP()), true);
    webServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
    webServer.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void handleCfgNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }

  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += webServer.uri();
  message += F("\nMethod: ");
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += webServer.args();
  message += F("\n");

  for (uint8_t i = 0; i < webServer.args(); i++) {
    message += String(F(" ")) + webServer.argName(i) + F(": ") + webServer.arg(i) + F("\n");
  }
  webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  webServer.sendHeader("Pragma", "no-cache");
  webServer.sendHeader("Expires", "-1");
  webServer.send(404, "text/plain", message);
}

void handleCfgIndex() {
	String html = FPSTR(PORTAL_HTML);
	webServer.send(200, "text/html", html);
}

/////////////////////////////////////////////////////////////////////////////////
void wifiConfig() {
  
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP("esp_cfg");//, password);
  delay(500);

  DebugPrintln("IP address: " + WiFi.softAPIP().toString());

  // init spiffs
  SPIFFS.begin();

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

//  webServer.serveStatic("/generate_204", SPIFFS, "/index.html");  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
//  webServer.serveStatic("/fwlink", SPIFFS, "/index.html");  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
//  webServer.serveStatic("/", SPIFFS, "/index.html");
  webServer.on("/generate_204", handleCfgIndex);
  webServer.on("/fwlink", handleCfgIndex);
  webServer.on("/", handleCfgIndex);
  
  webServer.on("/login", handleLogin);
  webServer.on("/setcfg", handleSetCfg);
  webServer.onNotFound(handleCfgNotFound);

  httpUpdater.setup(&webServer);
  webServer.begin();
  DebugPrintln("HTTP webServer started");

  unsigned long time_ap = millis();
  while (1) {
    dnsServer.processNextRequest();
    webServer.handleClient();

    delay(10);
    if (millis() > time_ap + TIMEOUT_WIFICONFIG) {
      ESP.restart();
    }
  }
}

WebSocketsServer webSocket = WebSocketsServer(81);
/////////////////////////////////////////////////////////////////////////////////
void wifiInit(const String ssid, const String psw, 
			const String host/* = ""*/, 
             std::function<void ()> userWebService/* = NULL*/,
             std::function<void (uint8_t num, WStype_t type, uint8_t * payload, size_t length)> userWebSocketEvent/* = NULL*/) {
  
  SPIFFS.begin();

  for (int i = 0; i < MAX_WIFI_CNCT; i++) {
    WiFi.softAPdisconnect(true);
    WiFi.disconnect();
    WiFi.setHostname(host.length() > 0 ? host.c_str() : config.host);
    WiFi.begin(ssid.c_str(), psw.c_str());  
    unsigned long time_begin = millis();
    while ( WiFi.status() != WL_CONNECTED ) {
      delay(500);
      DebugPrint ( "." );
      if (millis() > time_begin + TIMEOUT_WIFI) {
        DebugPrintln ( "connection time out. " );
        break;
      }
    }  

    if ( WiFi.status() == WL_CONNECTED ) {
      DebugPrintln("\nWifi connected with ip: " + WiFi.localIP().toString());
      WiFi.setAutoReconnect(true);
      /*
      if (WiFi.hostname(host != "" ? host : config.host)) {
        DebugPrintln ("set host name: " + String(config.host));
      }
      //WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(8,8,8,8));
      //*/


      // user web service setup
      if (userWebService) {
        userWebService();
      }

      webServer.on("/", handleFile);      
      webServer.on("/dir", handleDir);
      webServer.on("/reset", handleReset);
      webServer.on("/setting", handleSetting);
      webServer.on("/remove", handleRemove);
      webServer.on("/upload", HTTP_ANY, []() { webServer.send(200, "text/plain", "");}, handleFileUpload);
      webServer.on("/download", handleDownload);
      webServer.on("/help", handleHelp);
      webServer.on("/file", handleFile);
      webServer.on("/debugmsg", handleDebugMsg);

      // cmd not found, turn to web file
      webServer.onNotFound(handleUserRequest);
      
      MDNS.begin(config.host);
      httpUpdater.setup(&webServer);

      webServer.begin();
      MDNS.addService("http", "tcp", 80);        

      // setup for websocket
      if (userWebSocketEvent) {
        webSocket.begin();
        DebugPrintln ( "set websocket callback event. " );
        webSocket.onEvent(userWebSocketEvent);
      } 
      break;
    }
  }
  
  if (WiFi.status() != WL_CONNECTED)
    wifiConfig();
}

void wifiProcess() {
  webServer.handleClient();
  
#ifndef ESP32
  MDNS.update();
#endif
  
  webSocket.loop();
}
