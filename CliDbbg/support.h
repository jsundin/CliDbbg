#pragma once
#include <Windows.h>

#define LVL_DEBUG	1
#define LVL_INFO	2
#define LVL_ERROR	3

#define _BUFSZ 500

void log(int level, LPWSTR fmt, ...);
DWORD FindProcessByString(LPWSTR lpszSearchString);
BOOL GetFilenameFromHandleW(HANDLE hProc, HANDLE hFile, OUT LPWSTR lpszFilename, DWORD dwFilenameSize);
BOOL TranslateResourceFilename(LPWSTR lpszResource);
LPWSTR GetFilename(LPWSTR lpszFullPath);
void SaveFile(LPWSTR lpszSrc, LPWSTR lpszDestDir);
