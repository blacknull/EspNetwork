#include <vector>
#include "basefunc.h"

#if defined(_MSG_DEBUG) || defined(_MEM_DEBUG)
#define MAX_DEBUGMSG 128
std::vector<String> vctDebugStr;

String strLine = "";
void addDebugMsg(String msg) {
  if (vctDebugStr.size() >= MAX_DEBUGMSG) {
    vctDebugStr.erase(vctDebugStr.begin());
  }

  if (!msg.endsWith("\n")) {
    strLine += msg;
    if (strLine.length() >= 128) {  // max line length
      vctDebugStr.push_back(strLine);
      strLine = "";
    }
  }
  else {
    strLine += msg;
    vctDebugStr.push_back(strLine);
    strLine = "";
  }
}

void addDebugMsg(const char* msg) {
  if (msg) addDebugMsg(String(msg));
}

void addDebugMsgln(const char* msg) {
  if (msg) addDebugMsg(String(msg) + "\n");
}

void addDebugMsgln(String msg) {
  addDebugMsg(msg + "\n");
}

void getDebugMsg(String& msg, int idx) {
  if (0 > idx) {
    //for_each(vctDebugStr.cbegin(), vctDebugStr.cend(), [](const String& str)->void{ strDebug += str; });
    for (auto iter = vctDebugStr.cbegin(); iter < vctDebugStr.cend(); iter++) {
      msg += *iter;
    }
  }
  else {
    if (vctDebugStr.size() > 0) {
      idx %= vctDebugStr.size();
      msg = vctDebugStr[idx];
    }
  }  
}

void addDebugArgsMsg(const char *format, ...) {
  char szMsg[128] = "";
	
  va_list args;
  va_start(args, format);
  //sprintf(szMsg, format, args);
  vsnprintf(szMsg, sizeof(szMsg), format, args);
  va_end(args);

  addDebugMsg(szMsg);
}
#endif

#if defined(_LOG_DEBUG)
const char* LOG_FILE = "/debug.log";
const char* TMP_FILE = "/tmp.log";

File fileLog;
void logInit(unsigned long baud_rate) {
  Serial.begin(baud_rate); // for compatibility with other debug messages
  delay(5000);

#ifdef FS_SDMMC
  #ifndef SD_MMC_CLK
    #define SD_MMC_CLK 39
    #define SD_MMC_CMD 38
    #define SD_MMC_D0  40
  #endif // SD_MMC_CLK

  MyFs.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!MyFs.begin("/sdcard", true)) {
    log_i("error: sdcard mount failed.\n");
    return;
  }
#else
  if (!MyFs.begin())
    return;
#endif

  if (!MyFs.exists(LOG_FILE)) {
    File new_log = MyFs.open(LOG_FILE, "w");
    if (!new_log) {
      log_i("error: new log file create failed\n");
      return;
    }
    new_log.println("log file.");
    new_log.close();
    delay(200);
  }

  fileLog = MyFs.open(LOG_FILE, "r+");
  if (!fileLog) {
    log_i("error: log file open failed\n");
    return;
  }
  
  const size_t MAX_LOG_SIZE = 2 * 1024 * 1024; // 2MB
  const size_t TRIM_SIZE = 1024 * 1024; // 1MB
  if (fileLog.size() > MAX_LOG_SIZE) {
    fileLog.seek(TRIM_SIZE, SeekEnd); // move to trim position

    File newFile = MyFs.open(TMP_FILE, "w");
    if (newFile) {
      uint8_t buf[1024];
      size_t bytesRead;
      while ((bytesRead = fileLog.read(buf, sizeof(buf))) > 0) {
        newFile.write(buf, bytesRead);
      }
      newFile.close();

      fileLog.close();
      MyFs.remove(LOG_FILE); // remove old file
      MyFs.rename(TMP_FILE, LOG_FILE); // rename new file to log file
      fileLog = MyFs.open(LOG_FILE, "r+");
    }
  }
}
void logPrintln(const char* msg) {
  if (!fileLog) return;
  if (msg) {
    fileLog.seek(0, SeekEnd); // move to end of file
    fileLog.println(msg);
    fileLog.flush();
  }
}
void logPrintln(String msg) { logPrintln(msg.c_str()); }
void logPrint(const char* msg) {
  if (!fileLog) return;
  if (msg) {
    fileLog.seek(0, SeekEnd); // move to end of file
    fileLog.print(msg);
    fileLog.flush();
  }
}
void logPrint(String msg) { logPrint(msg.c_str()); }
void logPrintf(const char *format, ...) {
  char szMsg[256] = "";
  
  va_list args;
  va_start(args, format);
  vsnprintf(szMsg, sizeof(szMsg), format, args);
  va_end(args);  
  
  logPrint(szMsg);
}

size_t posRead = 0;
int logRead(uint8_t* buf, size_t len) {
  if (!buf || len <= 0) {
    log_i("logRead: invalid buf and len\n");
    return 0;
  }
  if (!fileLog) {
    log_i("logRead: invalid fileLog\n");
    return 0;
  }

  fileLog.seek(posRead, SeekSet);
  size_t bytesRead = fileLog.read(buf, len);
  if (bytesRead < len) {
    posRead = 0; // reset to start
  }
  posRead += bytesRead;

  return bytesRead; // if bytesRead < len, it means end of file reached
}

#endif

uint8_t hexCharToInt(char hexChar) {
	if (hexChar >=  '0' && hexChar <= '9' )
		return hexChar - '0';
	else if (hexChar >= 'a' && hexChar <= 'f')
		return hexChar - 'a' + 10;
	else
		return 0;
}

uint32_t hexStrToUint32 (String strHex) {
	if (strHex.length() <= 0) 
		return 0;
	
	strHex.toLowerCase();
	if (strHex.startsWith("0x"))
		strHex.remove(0, 2);

	unsigned int len = strHex.length();
	if (len <= 0 || len % 2 != 0)
		return 0;

	uint32_t val = 0;		
	for (uint8_t i = 0; i < len; i += 2) {
		uint32_t low 	= hexCharToInt(strHex[len - i - 1]);
		uint32_t high 	= hexCharToInt(strHex[len - i - 2]);
		val += low * (1 << (4 * i)) + high * (1 << (4 * (i + 1)));
	}		
	
	return val;
}