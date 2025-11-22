#include "PowerMonitor.h"
#include "AudioManager.h"

#include <wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")

CPowerMonitor::CPowerMonitor()
{
}

bool CPowerMonitor::Initialize(CAudioManager& audioManager)
{
	m_audioManager = &audioManager;
	return true;
}

void CPowerMonitor::Enable(bool enable)
{
	m_enabled = enable;
}

void CPowerMonitor::SetWakeTriggerSource(int32_t source)
{
	m_triggerSource = source;
}

LRESULT CPowerMonitor::OnPowerBroadcast(WPARAM wParam, LPARAM lParam)
{
	if (!m_enabled || !m_audioManager) {
		return 0;
	}

	// Check if PowerBroadcast trigger is enabled (using bit flag)
	if ((m_triggerSource & WAKE_TRIGGER_POWER_RESUME) == 0) {
		return 0;
	}

	TCHAR buf[256];
	wsprintf(buf, _T("[WindowsAudioPlugin] CPowerMonitor::OnPowerBroadcast wParam=0x%lx, lParam=0x%lx\n"),
		(unsigned long)wParam, (unsigned long)lParam);
	OutputDebugString(buf);

	switch (wParam) {
	case PBT_APMSUSPEND:
		break;
	case PBT_APMRESUMEAUTOMATIC:
	case PBT_APMRESUMESUSPEND:
		RequestWake(_T("Power resume"));
		break;
	default:
		break;
	}
	return 0;
}

void CPowerMonitor::OnSessionChange(WPARAM wParam, LPARAM lParam)
{
	if (!m_enabled) {
		return;
	}

	// Check if SessionChange trigger is enabled (using bit flag)
	if ((m_triggerSource & WAKE_TRIGGER_USER_LOGIN) == 0) {
		return;
	}

	TCHAR buf[256];
	wsprintf(buf, _T("[WindowsAudioPlugin] CPowerMonitor::OnSessionChange wParam=0x%lx, lParam=0x%lx\n"),
		(unsigned long)wParam, (unsigned long)lParam);
	OutputDebugString(buf);

	switch (wParam) {
	case WTS_SESSION_UNLOCK:
	case WTS_SESSION_LOGON:
	case WTS_REMOTE_CONNECT:
		RequestWake(_T("Session active"));
		break;
	default:
		break;
	}
}

void CPowerMonitor::RequestWake(const TCHAR* reason)
{
	if (!m_enabled || !m_audioManager) {
		return;
	}

	TCHAR buf[256];
	wsprintf(buf, _T("[WindowsAudioPlugin] RequestWake: %s\n"), reason ? reason : _T("(none)"));
	OutputDebugString(buf);

	m_audioManager->OnSystemWake();
}

