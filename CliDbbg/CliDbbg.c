#include <Windows.h>
#include <stdio.h>
#include "support.h"

void ProcessCreateProcessEvent(DEBUG_EVENT* ev, LPWSTR lpszTargetDirectory) {
	WCHAR szFilename[MAX_PATH + 1];
	DWORD dwPid;

	szFilename[0] = 0;
	BOOL bHasFilename = GetFilenameFromHandleW(GetCurrentProcess(), ev->u.CreateProcessInfo.hFile, szFilename, MAX_PATH);
	BOOL bIsReadable = FALSE;
	if (bHasFilename) {
		bIsReadable = TranslateResourceFilename(szFilename);
	}
	dwPid = GetProcessId(ev->u.CreateProcessInfo.hProcess);
	log(LVL_INFO, L"event: process created: %d @ 0x%p - %s", dwPid, ev->u.CreateProcessInfo.lpBaseOfImage, szFilename);
	SaveFile(szFilename, lpszTargetDirectory);
}

void ProcessCreateThreadEvent(DEBUG_EVENT* ev) {
	DWORD dwTid = GetThreadId(ev->u.CreateThread.hThread);
	log(LVL_INFO, L"event: thread created: %d @ 0x%p", dwTid, ev->u.CreateThread.lpStartAddress);
}

void ProcessLoadDLLDebugEvent(HANDLE hProc, DEBUG_EVENT* ev, LPWSTR lpszTargetDirectory) {
	wchar_t szFilename[MAX_PATH + 1];

	BOOL bHasFilename = GetFilenameFromHandleW(GetCurrentProcess(), ev->u.LoadDll.hFile, szFilename, MAX_PATH);
	BOOL bIsReadable = FALSE;
	if (bHasFilename) {
		bIsReadable = TranslateResourceFilename(szFilename);
	}

	log(LVL_INFO, L"event: dll loaded: 0x%p - %s", ev->u.LoadDll.lpBaseOfDll, szFilename);
	SaveFile(szFilename, lpszTargetDirectory);
}

BOOL ProcessDebugEvent(HANDLE hProc, DEBUG_EVENT* ev, LPWSTR lpszTargetDirectory) {
	switch (ev->dwDebugEventCode) {
	case CREATE_PROCESS_DEBUG_EVENT:
		ProcessCreateProcessEvent(ev, lpszTargetDirectory);
		break;

	case CREATE_THREAD_DEBUG_EVENT:
		ProcessCreateThreadEvent(ev);
		break;

	case EXCEPTION_DEBUG_EVENT:
		log(LVL_INFO, L"event: exception");
		break;

	case EXIT_PROCESS_DEBUG_EVENT:
		log(LVL_INFO, L"event: process exit (code: 0x%x)", ev->u.ExitProcess.dwExitCode);
		return FALSE;
		break;

	case EXIT_THREAD_DEBUG_EVENT:
		log(LVL_INFO, L"event: thread exit");
		break;

	case LOAD_DLL_DEBUG_EVENT:
		ProcessLoadDLLDebugEvent(hProc, ev, lpszTargetDirectory);
		break;

	case OUTPUT_DEBUG_STRING_EVENT:
		log(LVL_INFO, L"event: output-debugging-string");
		break;

	case RIP_EVENT:
		log(LVL_INFO, L"event: system debugging error");
		break;

	case UNLOAD_DLL_DEBUG_EVENT:
		log(LVL_INFO, L"event: dll unloaded");
		break;

	default:
		log(LVL_INFO, L"event: unhandled debug event: %d", ev->dwDebugEventCode);
		break;
	}

	return TRUE;
}

int wmain(int ac, wchar_t *av[])
{
	if (ac < 2) {
		log(LVL_ERROR, L"Usage: %s <pid|procsearch> [<target directory>]", av[0]);
		return -1;
	}

	int nPid = wcstol(av[1], 0, 10);
	if (nPid <= 0) {
		nPid = FindProcessByString(av[1]);
		if (nPid <= 0) {
			log(LVL_ERROR, L"No PID :(");
			return -1;
		}
	}
	log(LVL_INFO, L"using pid: %d", nPid);

	wchar_t* szTargetDirectory = NULL;
	if (ac > 2) {
		szTargetDirectory = av[2];
		log(LVL_INFO, L"target directory: %s", szTargetDirectory);
	}

	if (!DebugActiveProcess(nPid)) {
		log(LVL_ERROR, L"DebugActiveProcess failed");
		return -1;
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, nPid);
	if (!hProc) {
		log(LVL_ERROR, L"OpenProcess failed");
		return -1;
	}

	BOOL bStillDebugging = TRUE;
	DEBUG_EVENT ev;
	while (bStillDebugging) {
		if (!WaitForDebugEvent(&ev, INFINITE)) {
			log(LVL_ERROR, L"WaitForDebugEvent failed");
			break;
		}

		if (!ProcessDebugEvent(hProc, &ev, szTargetDirectory)) {
			break;
		}

		ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
	}

	CloseHandle(hProc);
	return 0;
}
