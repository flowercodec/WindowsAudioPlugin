#pragma once

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>

class CSystemTray
{
public:
	CSystemTray();
	~CSystemTray();

	BOOL Create(HWND hWnd, UINT uCallbackMessage, LPCTSTR pszTip, HICON hIcon);
	void Destroy();

private:
	NOTIFYICONDATA m_nid;
	bool m_created;
};


