#pragma once

#include <afxwin.h>
#include <wtsapi32.h>

class CHiddenWnd : public CWnd
{
public:
	CHiddenWnd();
	virtual ~CHiddenWnd();

	BOOL CreateHidden();

protected:
	// Custom tray icon notification (WM_TRAYICON)
	afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
	// Override of CWnd::OnPowerBroadcast
	afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, LPARAM lParam);
	// Override of CWnd::OnSessionChange — forwards WTS events
	afx_msg void OnSessionChange(UINT nSessionState, UINT nSessionId);

	DECLARE_MESSAGE_MAP()

private:
	// Kept placeholders for power notifications; enable registration if needed later.
	// HPOWERNOTIFY m_hNotifyDisplay = nullptr;
	// HPOWERNOTIFY m_hNotifyScheme = nullptr;
	// HPOWERNOTIFY m_hNotifyAway = nullptr;
	bool m_sessionNotifyRegistered = false;
};


