#include "ApiInfo.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <atlbase.h>

bool ComputeAudioApiInfo(AudioApiInfo& info)
{
	info = AudioApiInfo();

	bool didCoInit = false;
	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr)) {
		didCoInit = true;
	} else if (hr != RPC_E_CHANGED_MODE) {
		return false;
	}

	CComPtr<IMMDeviceEnumerator> enumr;
	if (FAILED(enumr.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) return false;

	CComPtr<IMMDevice> device;
	if (FAILED(enumr->GetDefaultAudioEndpoint(eRender, eConsole, &device))) return false;

	CComPtr<IAudioEndpointVolume> epv;
	if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&epv))) return false;

	uintptr_t* vtable = *(uintptr_t**)epv.p;
	if (!vtable) return false;

	// IAudioEndpointVolume vtable layout (after IUnknown: 0=QI,1=AddRef,2=Release):
	//  3 RegisterControlChangeNotify
	//  4 UnregisterControlChangeNotify
	//  5 GetChannelCount
	//  6 SetMasterVolumeLevel
	//  7 SetMasterVolumeLevelScalar
	//  8 GetMasterVolumeLevel
	//  9 GetMasterVolumeLevelScalar
	// 10 SetChannelVolumeLevel
	// 11 SetChannelVolumeLevelScalar
	// 12 GetChannelVolumeLevel
	// 13 GetChannelVolumeLevelScalar
	// 14 SetMute
	// 15 GetMute
	// 16 GetVolumeStepInfo
	// 17 VolumeStepUp
	// 18 VolumeStepDown
	// 19 QueryHardwareSupport
	// 20 GetVolumeRange
	const int VOLUME_STEP_UP_INDEX = 17;
	const int VOLUME_STEP_DOWN_INDEX = 18;

	HMODULE hmmdevapi = GetModuleHandleA("audioses.dll");
	if (!hmmdevapi) return false;
	uintptr_t base = (uintptr_t)hmmdevapi;

	info.offset_step_up = (uint64_t)((uintptr_t)vtable[VOLUME_STEP_UP_INDEX] - base);
	info.offset_step_down = (uint64_t)((uintptr_t)vtable[VOLUME_STEP_DOWN_INDEX] - base);
	info.got_success = (info.offset_step_up != 0 && info.offset_step_down != 0);
	if (didCoInit) {
		CoUninitialize();
	}
	return info.got_success;
}


