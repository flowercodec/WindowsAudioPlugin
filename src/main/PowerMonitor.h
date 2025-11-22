#pragma once

#include <windows.h>
#include "Settings.h"

class CAudioManager;

class CPowerMonitor
{
public:
	CPowerMonitor();
	bool Initialize(CAudioManager& audioManager);
	void Enable(bool enable);
	void SetWakeTriggerSource(int32_t source);

	LRESULT OnPowerBroadcast(WPARAM wParam, LPARAM lParam);
	void OnSessionChange(WPARAM wParam, LPARAM lParam);

private:
	void RequestWake(const TCHAR* reason);

private:
	CAudioManager* m_audioManager = nullptr;
	bool m_enabled = false;
	int32_t m_triggerSource = WAKE_TRIGGER_USER_LOGIN;
};

