#include "SystemTray.h"

CSystemTray::CSystemTray()
{
	memset(&m_nid, 0, sizeof(m_nid));
	m_created = false;
}

CSystemTray::~CSystemTray()
{
	Destroy();
}

BOOL CSystemTray::Create(HWND hWnd, UINT uCallbackMessage, LPCTSTR pszTip, HICON hIcon)
{
	if (m_created) {
		return TRUE;
	}

	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.hWnd = hWnd;
	m_nid.uID = 1;
	m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	m_nid.uCallbackMessage = uCallbackMessage;
	m_nid.hIcon = hIcon;
	_tcsncpy_s(m_nid.szTip, pszTip, _TRUNCATE);

	BOOL ok = Shell_NotifyIcon(NIM_ADD, &m_nid);
	if (ok) {
		m_created = true;
	}
	return ok;
}

void CSystemTray::Destroy()
{
	if (m_created) {
		Shell_NotifyIcon(NIM_DELETE, &m_nid);
		m_created = false;
	}
}


