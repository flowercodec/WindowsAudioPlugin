#include "AudioHook.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <atlbase.h>
#include "detours.h"
#include "ApiInfo.h"
#include "MMKV/MMKV.h"
#include <shlobj.h>
#include <shlwapi.h>

#ifndef SETTINGS_KEY_HOOK_STEP_PERCENT
#define SETTINGS_KEY_HOOK_STEP_PERCENT "hook_step_percent"
#endif
#ifndef SETTINGS_KEY_HOOK_ENABLED
#define SETTINGS_KEY_HOOK_ENABLED "hook_enabled"
#endif

static AudioApiInfo g_apiInfo;
static HHOOK g_hHook = NULL;
extern HMODULE g_hThisModule;

// Function types (stdcall for COM methods via vtable; real signature unknown here)
typedef HRESULT(__stdcall* PFN_VolumeStepUp)(IAudioEndpointVolume* self, LPCGUID pguidEventContext);
typedef HRESULT(__stdcall* PFN_VolumeStepDown)(IAudioEndpointVolume* self, LPCGUID pguidEventContext);

static PFN_VolumeStepUp   g_origStepUp = nullptr;
static PFN_VolumeStepDown g_origStepDown = nullptr;

static MMKV* g_kv = nullptr;

static void InitMMKVMultiProcess()
{
	if (g_kv) return;
	wchar_t appdata[MAX_PATH] = {0};
	if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata))) {
		return;
	}
	std::wstring rootW = std::wstring(appdata) + L"\\WindowsAudioPlugin";
	// ensure directory exists
	if (!PathFileExistsW(rootW.c_str())) {
		CreateDirectoryW(rootW.c_str(), NULL);
	}
	MMKV::initializeMMKV(rootW);
	g_kv = MMKV::mmkvWithID("config.mmkv", MMKVMode::MMKV_MULTI_PROCESS, nullptr, &rootW);
}

static int GetHookStepPercent()
{
	InitMMKVMultiProcess();
	if (!g_kv) return 2;
	// If hook is disabled, treat as native OS behavior by returning 2
	bool hookEnabled = g_kv->getBool(SETTINGS_KEY_HOOK_ENABLED, false);
	if (!hookEnabled) {
		return 2;
	}
	int v = (int)g_kv->getInt32(SETTINGS_KEY_HOOK_STEP_PERCENT, 1);
	if (v < 1) v = 1;
	if (v > 20) v = 20;
	return v;
}

static HRESULT __stdcall Hook_StepUp(IAudioEndpointVolume* self, LPCGUID ctx)
{
	if (!self) return E_POINTER;
	int percent = GetHookStepPercent();
	if (percent == 2 && g_origStepUp) {
		return g_origStepUp(self, ctx);
	}
	float vol = 0.0f;
	if (SUCCEEDED(self->GetMasterVolumeLevelScalar(&vol))) {
		float step = (float)percent / 100.0f;
		float v = vol + step;
		if (v > 1.0f) v = 1.0f;
		return self->SetMasterVolumeLevelScalar(v, ctx);
	}
	// fallback
	return g_origStepUp ? g_origStepUp(self, ctx) : E_FAIL;
}

static HRESULT __stdcall Hook_StepDown(IAudioEndpointVolume* self, LPCGUID ctx)
{
	if (!self) return E_POINTER;
	int percent = GetHookStepPercent();
	if (percent == 2 && g_origStepDown) {
		return g_origStepDown(self, ctx);
	}
	float vol = 0.0f;
	if (SUCCEEDED(self->GetMasterVolumeLevelScalar(&vol))) {
		float step = (float)percent / 100.0f;
		float v = vol - step;
		if (v < 0.0f) v = 0.0f;
		return self->SetMasterVolumeLevelScalar(v, ctx);
	}
	// fallback
	return g_origStepDown ? g_origStepDown(self, ctx) : E_FAIL;
}

void InitAudioHook()
{
	InitMMKVMultiProcess();
	ComputeAudioApiInfo(g_apiInfo);

	if (!g_apiInfo.got_success) {
		return;
	}

	HMODULE hmmdevapi = GetModuleHandleA("audioses.dll");
	if (!hmmdevapi) return;
	uint8_t* base = (uint8_t*)hmmdevapi;

	g_origStepUp = (PFN_VolumeStepUp)(base + g_apiInfo.offset_step_up);
	g_origStepDown = (PFN_VolumeStepDown)(base + g_apiInfo.offset_step_down);

	if (!g_origStepUp || !g_origStepDown) return;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)g_origStepUp, Hook_StepUp);
	DetourAttach(&(PVOID&)g_origStepDown, Hook_StepDown);
	DetourTransactionCommit();
}

void UninitAudioHook()
{
	if (g_origStepUp || g_origStepDown) {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		if (g_origStepUp) DetourDetach(&(PVOID&)g_origStepUp, Hook_StepUp);
		if (g_origStepDown) DetourDetach(&(PVOID&)g_origStepDown, Hook_StepDown);
		DetourTransactionCommit();
	}
}

extern "C" 
LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(NULL, code, wParam, lParam);
}

// Install/uninstall global message hook inside the DLL (exported via .def)
extern "C" void HookInstall()
{
	if (g_hHook) return;
	g_hHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, g_hThisModule, 0);
}

extern "C" void HookUninstall()
{
	if (g_hHook) {
		UnhookWindowsHookEx(g_hHook);
		g_hHook = NULL;
	}
}

