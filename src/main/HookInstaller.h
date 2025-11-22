#pragma once

#include <windows.h>
#include <tchar.h>

class CHookInstaller
{
public:
	CHookInstaller();
	~CHookInstaller();

	bool Install();
	void Uninstall();
	bool IsInstalled() const;

private:
	HHOOK   m_hHook;
	HMODULE m_hDll;
};


