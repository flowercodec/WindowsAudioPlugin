#pragma once

#include <afxwinappex.h>
#include "SystemTray.h"
#include "PowerMonitor.h"
#include "AudioManager.h"
#include "Settings.h"
#include "HiddenWnd.h"
#include "HookInstaller.h"

class CWindowsAudioPluginApp : public CWinAppEx
{
public:
	CWindowsAudioPluginApp();

	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;

private:
	CHiddenWnd    m_hiddenWnd;
	CSystemTray   m_tray;
	CPowerMonitor m_powerMonitor;
	CAudioManager m_audioManager;
	CSettings     m_settings;
	CHookInstaller m_hookInstaller;

	bool          m_enabled;
	bool          m_runAtStartup;
	bool          m_settingsOpen = false;

	BOOL InitTrayIcon();
	void LoadConfig();
	void SaveConfig();
	void ShowConfigDialog();

public:
	LRESULT HandlePowerBroadcast(WPARAM wParam, LPARAM lParam);
	void     HandleSessionChange(WPARAM wParam, LPARAM lParam);
	bool IsEnabled() const { return m_enabled; }
	bool IsRunAtStartup() const { return m_runAtStartup; }

	afx_msg void OnTrayEnable();
	afx_msg void OnTrayDisable();
	afx_msg void OnTrayRunAtStartup();
	afx_msg void OnTraySettings();
	afx_msg void OnTrayExit();

	DECLARE_MESSAGE_MAP()
};

CWindowsAudioPluginApp* GetPluginApp();


