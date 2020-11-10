#ifndef __RTC_WIN_UTILS_H__
#define __RTC_WIN_UTILS_H__

#include <string>

BOOL    CreateCACert(CString strFilePath);

CString GetModulePath();
CString GetAbsFilePath(CString strFilePath);
BOOL    FileExist(CString strFilePath);

std::string CStringToStdString(CString strValue);
CString StdStringToCString(std::string utf8str);

std::string GenerateUUID();

void LogPrintf(const char* fmt, ...);

#endif