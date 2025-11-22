#include "HookInstaller.h"
#include "../hook/AudioHook.h"

typedef LRESULT(CALLBACK* PFN_GETMSGPROC)(int, WPARAM, LPARAM);

CHookInstaller::CHookInstaller()
{
	m_hHook = NULL;
	m_hDll = NULL;
}

CHookInstaller::~CHookInstaller()
{
	Uninstall();
}

bool CHookInstaller::Install()
{
	if (m_hHook) return true;

	// Delegate hook install to AudioHook.dll
	HookInstall();
	// We cannot retrieve HHOOK from DLL directly; mark as installed
	m_hHook = (HHOOK)1;

	return true;
}

void CHookInstaller::Uninstall()
{
	if (m_hHook) {
		// Delegate uninstall to AudioHook.dll
		HookUninstall();
		m_hHook = NULL;
	}
}

bool CHookInstaller::IsInstalled() const
{
	return m_hHook != NULL;
}


