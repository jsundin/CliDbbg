#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <Psapi.h>
#include "support.h"

DWORD dwFileCounter = 0;
int nLogLevel = LVL_DEBUG;

void log(int level, LPWSTR fmt, ...) {
	if (level < nLogLevel) {
		return;
	}
	WCHAR out[_BUFSZ];
	va_list args;
	va_start(args, fmt);
	vswprintf(out, _BUFSZ, fmt, args);
	va_end(args);

	wprintf(L"<%d> %s\n", level, out);
}

DWORD FindProcessByString(LPWSTR lpszSearchString) {
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!hSnap) {
		log(LVL_ERROR, L"CreateToolhelp32Snapshot failed");
		return 0;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	DWORD dwPid = 0;
	BOOL ok = Process32First(hSnap, &pe32);
	while (ok) {
		if (wcsstr(pe32.szExeFile, lpszSearchString)) {
			if (!dwPid) {
				wprintf(L"* ");
				dwPid = pe32.th32ProcessID;
			}
			else {
				wprintf(L"  ");
			}
			wprintf(L"%d: %s\n", pe32.th32ProcessID, pe32.szExeFile);
		}

		ok = Process32Next(hSnap, &pe32);
	}
	return dwPid;
}

// https://learn.microsoft.com/en-us/windows/win32/memory/obtaining-a-file-name-from-a-file-handle
BOOL GetFilenameFromHandleW(HANDLE hProc, HANDLE hFile, OUT LPWSTR lpszFilename, DWORD dwFilenameSize) {
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

	if (!dwFileSizeLo && !dwFileSizeHi) {
		log(LVL_DEBUG, L"- file handle does not have a size\n");
		return FALSE;
	}

	HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL);
	if (!hFileMap) {
		log(LVL_DEBUG, L"CreateFileMapping failed");
		return FALSE;
	}

	void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
	BOOL bSuccess = FALSE;
	if (pMem) {
		if (GetMappedFileName(hProc, pMem, lpszFilename, dwFilenameSize)) {
			bSuccess = TRUE;
		}
		else {
			log(LVL_DEBUG, L"GetMappedFileName failed");
		}
		UnmapViewOfFile(pMem);
	}
	else {
		log(LVL_DEBUG, L"MapVieWOfFile failed");
	}

	CloseHandle(hFileMap);
	return bSuccess;
}

// https://learn.microsoft.com/en-us/windows/win32/memory/obtaining-a-file-name-from-a-file-handle
BOOL TranslateResourceFilename(LPWSTR lpszResource) {
	WCHAR szTemp[_BUFSZ];
	szTemp[0] = 0;

	if (!GetLogicalDriveStrings(_BUFSZ, szTemp)) {
		return FALSE;
	}

	WCHAR szName[MAX_PATH + 1];
	WCHAR szDrive[3] = L" :";
	LPWSTR p = szTemp;

	do {
		*szDrive = *p;
		if (QueryDosDevice(szDrive, szName, MAX_PATH)) {
			SIZE_T dwNameLen = wcslen(szName);
			if (dwNameLen < MAX_PATH) {
				if (_wcsnicmp(lpszResource, szName, dwNameLen) == 0 && *(lpszResource + dwNameLen) == L'\\') {
					WCHAR szTempFile[MAX_PATH + 1];
					_snwprintf_s(szTempFile, MAX_PATH-1, MAX_PATH-1, L"%s%s", szDrive, lpszResource + dwNameLen);
					wcsncpy_s(lpszResource, MAX_PATH, szTempFile, wcslen(szTempFile));
					return TRUE;
				}
			}

		}
	} while (*p++);
	return FALSE;
}

LPWSTR GetFilename(LPWSTR lpszFullPath) {
	LPWSTR szFilename = wcsrchr(lpszFullPath, '\\');
	return szFilename != NULL ? szFilename + 1 : NULL;
}

void SaveFile(LPWSTR lpszSrc, LPWSTR lpszDestDir) {
	if (!lpszDestDir) {
		return;
	}

	LPWSTR lpszFilename = GetFilename(lpszSrc);
	if (!lpszFilename) {
		return;
	}

	WCHAR szDst[MAX_PATH + 1];
	_snwprintf_s(szDst, MAX_PATH - 1, MAX_PATH - 1, L"%s\\%s.%d", lpszDestDir, lpszFilename, ++dwFileCounter);
	if (CopyFileW(lpszSrc, szDst, TRUE)) {
		log(LVL_INFO, L"  saved file to: %s", szDst);
	}
}