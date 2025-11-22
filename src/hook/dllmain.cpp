#include <windows.h>
#include <tchar.h>
#include "AudioHook.h"

HMODULE g_hThisModule = NULL;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);
		g_hThisModule = hModule;

		do {
			TCHAR path[MAX_PATH] = {0};
			if (!GetModuleFileName(NULL, path, MAX_PATH)) {
				break;
			}
			const TCHAR* exe = _tcsrchr(path, _T('\\'));
			exe = exe ? exe + 1 : path;

			if (_tcsicmp(exe, _T("explorer.exe")) != 0) {
				break;
			}

			HMODULE hAudioses = GetModuleHandle(_T("audioses.dll"));
			if (!hAudioses) {
				break;
			}

			InitAudioHook();
		} while (0);
		break;
	}
	case DLL_PROCESS_DETACH:
		UninitAudioHook();
		break;
	}
	return TRUE;
}


