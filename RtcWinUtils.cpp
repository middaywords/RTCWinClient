#include "pch.h"
#include "resource.h"
#include "RtcWinUtils.h"

CString GetModulePath()
{
	CString strFilePath, tempStr;
	TCHAR   tszCurrentDirectory[MAX_PATH];
	int     nStartIndex = -1;
	::GetModuleFileName(NULL, tszCurrentDirectory, MAX_PATH);
	tempStr.Format(_T("%s"), tszCurrentDirectory);
	nStartIndex = tempStr.ReverseFind(_T('\\'));
	if (nStartIndex == -1) return _T("");
	wsprintf(tszCurrentDirectory, _T("%s"), tempStr.Left(nStartIndex + 1));
	strFilePath.Format(_T("%s"), tszCurrentDirectory);
	return strFilePath;
}

CString GetAbsFilePath(CString strFilePath)
{
	CString strAbsPath = strFilePath, tempStr;
	UINT    uLen = 0;
	tempStr = GetModulePath();
	if (tempStr.IsEmpty()) return strAbsPath;
	uLen = tempStr.GetLength();
	if (strAbsPath.Left(uLen).CompareNoCase(tempStr) == 0)
		return strAbsPath;
	strAbsPath = tempStr + strAbsPath;
	return strAbsPath;
}

BOOL FileExist(CString strFilePath)
{
	HANDLE          hFind;
	WIN32_FIND_DATA fdata;
	hFind = ::FindFirstFile(strFilePath, &fdata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		::FindClose(hFind); return TRUE;
	}
	return FALSE;
}

BOOL FolderExist(CString strFolderPath)
{
	return FileExist(strFolderPath + _T("\\*.*"));
}

std::string CStringToStdString(CString strValue) {
	std::wstring wbuffer;
	size_t length;
#ifdef _UNICODE
	wbuffer.assign(strValue.GetString(), strValue.GetLength());
#else
	length = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, (LPCTSTR)strValue, -1, NULL, 0);
	wbuffer.resize(length);
	MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strValue, -1, (LPWSTR)(wbuffer.data()), wbuffer.length());
#endif
	length = WideCharToMultiByte(CP_UTF8, 0, wbuffer.data(), wbuffer.size(), NULL, 0, NULL, NULL);
	std::string buffer;
	buffer.resize(length);
	WideCharToMultiByte(CP_UTF8, 0, strValue, -1, (LPSTR)(buffer.data()), length, NULL, NULL);
	return (buffer);
}

CString StdStringToCString(std::string utf8str) {
	int nLen = ::MultiByteToWideChar(CP_UTF8, NULL, utf8str.data(), utf8str.size(), NULL, 0);
	std::wstring wbuffer;
	wbuffer.resize(nLen);
	::MultiByteToWideChar(CP_UTF8, NULL, utf8str.data(), utf8str.size(), (LPWSTR)(wbuffer.data()), wbuffer.length());
#ifdef UNICODE
	return (CString(wbuffer.data(), wbuffer.length()));
#else
	nLen = WideCharToMultiByte(CP_ACP, 0, wbuffer.data(), wbuffer.length(), NULL, 0, NULL, NULL);
	std::string ansistr;
	ansistr.resize(nLen);
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)(wbuffer.data()), wbuffer.length(), (LPSTR)(ansistr.data()), ansistr.size(), NULL, NULL);
	return (CString(ansistr.data(), ansistr.length()));
#endif
}

BOOL CreateCACert(CString strFilePath) {
	HGLOBAL hResourceLoaded;
	HRSRC hRes;
	void* lpResLock;
	DWORD dwSize = 0;
	BOOL  bRst = FALSE;

	if (FileExist(strFilePath)) return TRUE;
	
	hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_BINARY1), _T("Binary"));
	if (hRes) {
		hResourceLoaded = LoadResource(NULL, hRes);
		if (hResourceLoaded) {
			lpResLock = LockResource(hResourceLoaded);

			dwSize = SizeofResource(NULL, hRes);
			if (dwSize > 0 && lpResLock) {
				HANDLE hFile = CreateFile(strFilePath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					bRst = WriteFile(hFile, lpResLock, dwSize, NULL, NULL);
					CloseHandle(hFile);
				}
			}

			FreeResource(hResourceLoaded);
		}
	}
	return bRst;
}

void LogPrintf(const char* fmt, ...) {
	char buffer[8192] = { 0 };
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	CStringA strLog = buffer;
	OutputDebugStringA(strLog + "\n");
}

std::string GenerateUUID() {
	std::string uuid;
	GUID guid;
	HRESULT hr = CoCreateGuid(&guid);
	if (hr == S_OK) {
		RPC_WSTR strGUID;
		if (UuidToString(&guid, &strGUID) == RPC_S_OK) {
			uuid = CStringToStdString((LPTSTR)strGUID);
			RpcStringFree(&strGUID);
		}
	}
	return uuid;
}