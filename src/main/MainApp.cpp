#include "MainApp.h"
#include "resource.h"
#include "ConfigDialog.h"
#include "../hook/ApiInfo.h"
#include <commctrl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const UINT WM_TRAYICON = WM_USER + 100;

CWindowsAudioPluginApp theApp;

BEGIN_MESSAGE_MAP(CWindowsAudioPluginApp, CWinAppEx)
END_MESSAGE_MAP()

CWindowsAudioPluginApp::CWindowsAudioPluginApp()
	: m_enabled(false)
	, m_runAtStartup(false)
{
#if defined(_DEBUG)
	AudioApiInfo api_info;
	ComputeAudioApiInfo(api_info);
#endif
	//TCHAR buf[256];
	//wsprintf(buf, _T("[WindowsAudioPlugin] CWindowsAudioPluginApp\n"));
	//OutputDebugString(buf);
}

CWindowsAudioPluginApp* GetPluginApp()
{
	return &theApp;
}

BOOL CWindowsAudioPluginApp::InitInstance()
{
	CWinAppEx::InitInstance();

	// Enable Windows visual styles (theming)
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS | ICC_BAR_CLASSES;
	InitCommonControlsEx(&icex);

	LoadConfig();

	if (!m_hiddenWnd.CreateHidden()) {
		return FALSE;
	}
	// Set main window to hidden window to keep MFC message loop alive
	m_pMainWnd = &m_hiddenWnd;

	if (!InitTrayIcon()) {
		return FALSE;
	}

	m_powerMonitor.Initialize(m_audioManager);
	m_powerMonitor.Enable(m_enabled && m_settings.IsWakeActionsEnabled());
	m_powerMonitor.SetWakeTriggerSource(m_settings.GetWakeTriggerSource());

	// install hook if enabled
	if (m_enabled && m_settings.IsHookEnabled()) {
		m_hookInstaller.Install();
	}

	// Show configuration dialog on startup
	ShowConfigDialog();

	return TRUE;
}

int CWindowsAudioPluginApp::ExitInstance()
{
	// uninstall hook
	m_hookInstaller.Uninstall();
	SaveConfig();
	m_powerMonitor.Enable(false);
	return CWinAppEx::ExitInstance();
}

BOOL CWindowsAudioPluginApp::InitTrayIcon()
{
	// Load custom application icon
	HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	if (!hIcon) {
		// Fallback to system icon if custom icon fails to load
		hIcon = ::LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	}
	return m_tray.Create(m_hiddenWnd.GetSafeHwnd(), WM_TRAYICON, _T("WindowsAudioPlugin"), hIcon);
}

void CWindowsAudioPluginApp::LoadConfig()
{
	m_settings.Load();
	m_enabled = m_settings.IsEnabled();
	m_runAtStartup = m_settings.IsRunAtStartup();
	m_audioManager.SetMp3Path(m_settings.GetMp3Path());
	m_audioManager.SetWakeMinVolumePercent(m_settings.GetWakeMinVolumePercent());
	m_audioManager.SetWakePlaySeconds(m_settings.GetWakePlaySeconds());
}

void CWindowsAudioPluginApp::SaveConfig()
{
	m_settings.SetEnabled(m_enabled);
	m_settings.SetRunAtStartup(m_runAtStartup);
	m_settings.Save();
}

LRESULT CWindowsAudioPluginApp::HandlePowerBroadcast(WPARAM wParam, LPARAM lParam)
{
	return m_powerMonitor.OnPowerBroadcast(wParam, lParam);
}

void CWindowsAudioPluginApp::HandleSessionChange(WPARAM wParam, LPARAM lParam)
{
	m_powerMonitor.OnSessionChange(wParam, lParam);
}

void CWindowsAudioPluginApp::OnTrayEnable()
{
	m_enabled = true;
	m_powerMonitor.Enable(m_settings.IsWakeActionsEnabled());
	if (m_settings.IsHookEnabled() && !m_hookInstaller.IsInstalled()) {
		m_hookInstaller.Install();
	}
}

void CWindowsAudioPluginApp::OnTrayDisable()
{
	m_enabled = false;
	m_powerMonitor.Enable(false);
	m_hookInstaller.Uninstall();
}

void CWindowsAudioPluginApp::OnTraySettings()
{
	if (m_settingsOpen) {
		return;
	}
	ShowConfigDialog();
}

void CWindowsAudioPluginApp::ShowConfigDialog()
{
	m_settingsOpen = true;
	{
		CConfigDialog dlg(m_settings, m_audioManager);
		dlg.DoModal();
	}
	m_settingsOpen = false;
	// Re-apply feature toggles based on latest settings and global enable
	m_powerMonitor.Enable(m_enabled && m_settings.IsWakeActionsEnabled());
	m_powerMonitor.SetWakeTriggerSource(m_settings.GetWakeTriggerSource());
	if (m_enabled && m_settings.IsHookEnabled()) {
		if (!m_hookInstaller.IsInstalled()) m_hookInstaller.Install();
	} else {
		if (m_hookInstaller.IsInstalled()) m_hookInstaller.Uninstall();
	}
}

void CWindowsAudioPluginApp::OnTrayRunAtStartup()
{
	m_runAtStartup = m_settings.IsRunAtStartup();
	m_runAtStartup = !m_runAtStartup;
	m_settings.SetRunAtStartup(m_runAtStartup);
	if (m_runAtStartup) {
		m_settings.CreateStartupShortcut();
	} else {
		m_settings.RemoveStartupShortcut();
	}
	m_settings.Save();
}

void CWindowsAudioPluginApp::OnTrayExit()
{
	PostQuitMessage(0);
}


