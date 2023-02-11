#include <vector>
#include "basefunc.h"

#define MAX_DEBUGMSG 64
std::vector<String> vctDebugStr;

void addDebugMsg(String msg) {
  if (vctDebugStr.size() >= MAX_DEBUGMSG) {
    vctDebugStr.erase(vctDebugStr.begin());
  }
  vctDebugStr.push_back(msg);
  //vctDebugStr.push_back(msg + String("\n"));
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
  sprintf(szMsg, format, args);
  va_end(args);  
  
  addDebugMsg(szMsg);
}

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