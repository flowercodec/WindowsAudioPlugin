#include "Settings.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <atlbase.h>

#include "MMKV/MMKV.h"
#include <string>

static const TCHAR* kAppDirName = _T("WindowsAudioPlugin");
static const TCHAR* kStartupLnkName = _T("WindowsAudioPlugin.lnk");

CSettings::CSettings()
{
}

CSettings::~CSettings()
{
	if (m_kv) {
		m_kv->close();
		m_kv = nullptr;
	}
}

void CSettings::InitKvIfNeeded()
{
	if (m_kv != nullptr) return;

	// Initialize MMKV at %APPDATA%\WindowsAudioPlugin using wide path (MMKVPath_t = std::wstring)
	CString rootDir = GetAppDataDir();
	std::wstring rootW(rootDir.GetString());

	MMKV::initializeMMKV(rootW);
	m_kv = MMKV::mmkvWithID("config.mmkv", MMKVMode::MMKV_MULTI_PROCESS, nullptr, &rootW);
}

void CSettings::Load()
{
	InitKvIfNeeded();
	if (!m_kv) {
		// MMKV init failed, keep defaults via getters
		return;
	}
	// No-op: live reads happen in getters
}

void CSettings::Save()
{
	InitKvIfNeeded();
	if (!m_kv) {
		// If MMKV is not ready, skip saving to avoid crashes
		return;
	}
	m_kv->sync();
}

bool CSettings::IsEnabled() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return false;
	return m_kv->getBool(SETTINGS_KEY_ENABLED, false);
}
void CSettings::SetEnabled(bool v)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	m_kv->set(v, SETTINGS_KEY_ENABLED);
}

bool CSettings::IsRunAtStartup() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return false;
	return m_kv->getBool(SETTINGS_KEY_RUN_AT_STARTUP, false);
}
void CSettings::SetRunAtStartup(bool v)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	m_kv->set(v, SETTINGS_KEY_RUN_AT_STARTUP);
}

bool CSettings::IsWakeActionsEnabled() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return false;
	return m_kv->getBool(SETTINGS_KEY_WAKE_ACTIONS, false);
}
void CSettings::SetWakeActionsEnabled(bool v)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	m_kv->set(v, SETTINGS_KEY_WAKE_ACTIONS);
}

bool CSettings::IsHookEnabled() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return false;
	return m_kv->getBool(SETTINGS_KEY_HOOK_ENABLED, false);
}
void CSettings::SetHookEnabled(bool v)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	m_kv->set(v, SETTINGS_KEY_HOOK_ENABLED);
}

int CSettings::GetHookStepPercent() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return 2;
	// Clamp on read to [1,20]
	int v = (int)m_kv->getInt32(SETTINGS_KEY_HOOK_STEP_PERCENT, 1);
	if (v < 1) v = 1;
	if (v > 20) v = 20;
	return v;
}
void CSettings::SetHookStepPercent(int percent)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	if (percent < 1) percent = 1;
	if (percent > 20) percent = 20;
	m_kv->set((int64_t)percent, SETTINGS_KEY_HOOK_STEP_PERCENT);
}

int CSettings::GetWakeMinVolumePercent() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return 8;
	int v = (int)m_kv->getInt32(SETTINGS_KEY_WAKE_MIN_PERCENT, 8);
	if (v < 1) v = 1;
	if (v > 100) v = 100;
	return v;
}
void CSettings::SetWakeMinVolumePercent(int percent)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	if (percent < 1) percent = 1;
	if (percent > 100) percent = 100;
	m_kv->set((int64_t)percent, SETTINGS_KEY_WAKE_MIN_PERCENT);
}
float CSettings::GetWakePlaySeconds() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return 3.0f;
	float v = m_kv->getFloat(SETTINGS_KEY_WAKE_PLAY_SECONDS, 3.0f); // default 3.0 seconds
	// Clamp to valid range
	if (v < 1.0f) v = 1.0f;
	if (v > 30.0f) v = 30.0f;
	return v;
}
void CSettings::SetWakePlaySeconds(float seconds)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	if (seconds < 1.0f) seconds = 1.0f;
	if (seconds > 30.0f) seconds = 30.0f;
	// Store directly as float (e.g., 3.00)
	m_kv->set(seconds, SETTINGS_KEY_WAKE_PLAY_SECONDS);
}

int32_t CSettings::GetWakeTriggerSource() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return WAKE_TRIGGER_USER_LOGIN; // default
	int32_t v = (int32_t)m_kv->getInt32(SETTINGS_KEY_WAKE_TRIGGER_SOURCE, WAKE_TRIGGER_USER_LOGIN);
	// Validate: ensure only valid flags are set
	v &= (WAKE_TRIGGER_POWER_RESUME | WAKE_TRIGGER_USER_LOGIN);
	// If no valid flags, default to USER_LOGIN
	if (v == 0) v = WAKE_TRIGGER_USER_LOGIN;
	return v;
}

void CSettings::SetWakeTriggerSource(int32_t source)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	// Validate: only allow valid flags
	int32_t v = source & (WAKE_TRIGGER_POWER_RESUME | WAKE_TRIGGER_USER_LOGIN);
	// If no valid flags, default to USER_LOGIN
	if (v == 0) v = WAKE_TRIGGER_USER_LOGIN;
	m_kv->set((int64_t)v, SETTINGS_KEY_WAKE_TRIGGER_SOURCE);
}

CString CSettings::GetMp3Path() const
{
	const_cast<CSettings*>(this)->InitKvIfNeeded();
	if (!m_kv) return CString();
	std::string u8;
	if (!m_kv->getString(SETTINGS_KEY_MP3_PATH, u8)) return CString();
	int lenW2 = MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), (int)u8.size(), nullptr, 0);
	CString w;
	LPWSTR buf = w.GetBuffer(lenW2);
	MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), (int)u8.size(), buf, lenW2);
	w.ReleaseBuffer(lenW2);
	return w;
}
void CSettings::SetMp3Path(const CString& path)
{
	InitKvIfNeeded();
	if (!m_kv) return;
	int lU8 = WideCharToMultiByte(CP_UTF8, 0, path, path.GetLength(), nullptr, 0, nullptr, nullptr);
	std::string u8; u8.resize(lU8);
	WideCharToMultiByte(CP_UTF8, 0, path, path.GetLength(), u8.data(), lU8, nullptr, nullptr);
	m_kv->set(u8, SETTINGS_KEY_MP3_PATH);
}

CString CSettings::GetAppDataDir() const
{
	TCHAR appdata[MAX_PATH] = {0};
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata);
	CString dir = CString(appdata) + CString(_T("\\")) + kAppDirName;
	if (!PathFileExists(dir)) {
		CreateDirectory(dir, NULL);
	}
	return dir;
}

CString CSettings::GetStartupFolder() const
{
	TCHAR path[MAX_PATH] = {0};
	SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, SHGFP_TYPE_CURRENT, path);
	return CString(path);
}

CString CSettings::GetAppExePath() const
{
	TCHAR exe[MAX_PATH] = {0};
	GetModuleFileName(NULL, exe, MAX_PATH);
	return CString(exe);
}

bool CSettings::CreateStartupShortcut() const
{
	CString lnk = GetStartupFolder() + CString(_T("\\")) + kStartupLnkName;
	CString target = GetAppExePath();

	CComPtr<IShellLink> psl;
	HRESULT hr = psl.CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr)) return false;

	psl->SetPath(target);
	psl->SetDescription(_T("WindowsAudioPlugin"));

	CComPtr<IPersistFile> ppf;
	hr = psl.QueryInterface(&ppf);
	if (FAILED(hr)) return false;

	hr = ppf->Save(lnk, TRUE);
	return SUCCEEDED(hr);
}

bool CSettings::RemoveStartupShortcut() const
{
	CString lnk = GetStartupFolder() + CString(_T("\\")) + kStartupLnkName;
	if (PathFileExists(lnk)) {
		return DeleteFile(lnk) ? true : false;
	}
	return true;
}


