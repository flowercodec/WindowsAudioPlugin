#include "HiddenWnd.h"
#include "resource.h"
#include "MainApp.h"
#include "PowerMonitor.h"
#pragma comment(lib, "Wtsapi32.lib")
#include <wtsapi32.h>

static const UINT WM_TRAYICON = WM_USER + 100;

BEGIN_MESSAGE_MAP(CHiddenWnd, CWnd)
	ON_MESSAGE(WM_TRAYICON, &CHiddenWnd::OnTrayIcon)
	ON_WM_POWERBROADCAST()
	ON_WM_WTSSESSION_CHANGE()
END_MESSAGE_MAP()

CHiddenWnd::CHiddenWnd() {}
CHiddenWnd::~CHiddenWnd()
{
	if (m_sessionNotifyRegistered) {
		WTSUnRegisterSessionNotification(GetSafeHwnd());
		m_sessionNotifyRegistered = false;
	}
}

BOOL CHiddenWnd::CreateHidden()
{
	LPCTSTR cls = _T("WindowsAudioPluginHiddenWnd");
	WNDCLASS wc = {0};
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = cls;
	RegisterClass(&wc);
	// Create a hidden top-level window (no parent) to receive WM_POWERBROADCAST broadcasts
	BOOL ok = CreateEx(0, cls, _T(""), WS_POPUP, 0, 0, 0, 0, NULL, 0);
	if (!ok) return FALSE;

	// Power setting notifications (leave commented; enable if needed later)
	// m_hNotifyDisplay = RegisterPowerSettingNotification(GetSafeHwnd(), &GUID_CONSOLE_DISPLAY_STATE, DEVICE_NOTIFY_WINDOW_HANDLE);
	// m_hNotifyScheme  = RegisterPowerSettingNotification(GetSafeHwnd(), &GUID_POWERSCHEME_PERSONALITY, DEVICE_NOTIFY_WINDOW_HANDLE);
	// m_hNotifyAway    = RegisterPowerSettingNotification(GetSafeHwnd(), &GUID_SYSTEM_AWAYMODE, DEVICE_NOTIFY_WINDOW_HANDLE);

	if (ok) {
		m_sessionNotifyRegistered = WTSRegisterSessionNotification(GetSafeHwnd(), NOTIFY_FOR_THIS_SESSION) == TRUE;
	}

	return TRUE;
}

LRESULT CHiddenWnd::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
	if (lParam == WM_LBUTTONUP) {
		auto* app = GetPluginApp();
		if (app) {
			app->OnTraySettings();
		}
		return 0;
	}
	if (lParam == WM_RBUTTONUP) {
		// Build popup menu (English captions)
		CMenu menu;
		menu.CreatePopupMenu();
		auto* app = GetPluginApp();
		bool enabled = app ? app->IsEnabled() : false;
		bool runAtStartup = app ? app->IsRunAtStartup() : false;
		UINT enableFlags = MF_STRING | (enabled ? MF_CHECKED : 0);
		UINT startupFlags = MF_STRING | (runAtStartup ? MF_CHECKED : 0);
		menu.AppendMenu(enableFlags, IDM_TRAY_ENABLE, _T("Enable"));
		menu.AppendMenu(startupFlags, IDM_TRAY_RUN_AT_STARTUP, _T("Run at startup"));
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, IDM_TRAY_SETTINGS, _T("Settings..."));
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, IDM_TRAY_EXIT, _T("Exit"));

		POINT pt;
		GetCursorPos(&pt);
		SetForegroundWindow();
		UINT cmd = (UINT)menu.TrackPopupMenu(TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, this);

		if (!app) return 0;

		switch (cmd) {
		case IDM_TRAY_ENABLE:
			if (app->IsEnabled()) app->OnTrayDisable(); else app->OnTrayEnable();
			break;
		case IDM_TRAY_RUN_AT_STARTUP:
			app->OnTrayRunAtStartup();
			break;
		case IDM_TRAY_SETTINGS: app->OnTraySettings(); break;
		case IDM_TRAY_EXIT: app->OnTrayExit(); break;
		default: break;
		}
	}
	return 0;
}

UINT CHiddenWnd::OnPowerBroadcast(UINT nPowerEvent, LPARAM lParam)
{
	//TCHAR buf[256];
	//wsprintf(buf, _T("[WindowsAudioPlugin] WM_POWERBROADCAST received: wParam=0x%lx, lParam=0x%lx\n"), (unsigned long)nPowerEvent, (unsigned long)lParam);
	//OutputDebugString(buf);

	if (nPowerEvent == PBT_POWERSETTINGCHANGE && lParam) {
		const POWERBROADCAST_SETTING* s = (const POWERBROADCAST_SETTING*)lParam;
		TCHAR gbuf[64];
		wsprintf(gbuf, _T("{%08lX-%04hX-%04hX-????-????????????}"), (unsigned long)s->PowerSetting.Data1, s->PowerSetting.Data2, s->PowerSetting.Data3);
		TCHAR sbuf[160];
		wsprintf(sbuf, _T("PBT_POWERSETTINGCHANGE Guid=%s, DataLength=%lu\r\n"), gbuf, (unsigned long)s->DataLength);
		OutputDebugString(sbuf);
	}

	auto* app = GetPluginApp();
	if (app) {
		app->HandlePowerBroadcast(nPowerEvent, lParam);
	}

	// Return TRUE to indicate the broadcast message is processed
	return TRUE;
}

void CHiddenWnd::OnSessionChange(UINT nSessionState, UINT nSessionId)
{
	auto* app = GetPluginApp();
	if (app) {
		app->HandleSessionChange(nSessionState, nSessionId);
	}
	CWnd::OnSessionChange(nSessionState, nSessionId);
}



