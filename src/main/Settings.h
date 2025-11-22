#pragma once

#include <afx.h>
#include "MMKV/MMKV.h"

#define SETTINGS_KEY_ENABLED            "enabled"
#define SETTINGS_KEY_RUN_AT_STARTUP     "run_at_startup"
#define SETTINGS_KEY_WAKE_ACTIONS       "wake_actions_enabled"
#define SETTINGS_KEY_HOOK_ENABLED       "hook_enabled"
#define SETTINGS_KEY_MP3_PATH           "mp3_path"
#define SETTINGS_KEY_HOOK_STEP_PERCENT  "hook_step_percent"
#define SETTINGS_KEY_WAKE_MIN_PERCENT   "wake_min_percent"
#define SETTINGS_KEY_WAKE_PLAY_SECONDS  "wake_play_seconds"
#define SETTINGS_KEY_WAKE_TRIGGER_SOURCE "wake_trigger_source"

// Wake trigger source flags (bit flags for multi-selection support)
enum WakeTriggerSourceType : int32_t {
	WAKE_TRIGGER_POWER_RESUME = 0x01,  // OnPowerBroadcast (睡眠恢复)
	WAKE_TRIGGER_USER_LOGIN   = 0x02   // OnSessionChange (用户登录)
};

// Combined flag for both triggers
constexpr int32_t WAKE_TRIGGER_ALL = WAKE_TRIGGER_POWER_RESUME | WAKE_TRIGGER_USER_LOGIN;  // 两者都触发 (全部)

class CSettings
{
public:
	CSettings();
	~CSettings();

	void Load();
	void Save();

	bool IsEnabled() const;
	void SetEnabled(bool v);

	bool IsRunAtStartup() const;
	void SetRunAtStartup(bool v);

	bool IsWakeActionsEnabled() const;
	void SetWakeActionsEnabled(bool v);

	bool IsHookEnabled() const;
	void SetHookEnabled(bool v);

	int  GetHookStepPercent() const;   // 1..20, default 1
	void SetHookStepPercent(int percent);

	int  GetWakeMinVolumePercent() const; // 1..100, default 8
	void SetWakeMinVolumePercent(int percent);

	float GetWakePlaySeconds() const; // 1.0..30.0, default 3.0 (stored as float, 0.01 precision)
	void SetWakePlaySeconds(float seconds);

	int32_t GetWakeTriggerSource() const; // Returns bit flags, default USER_LOGIN (0x02)
	void SetWakeTriggerSource(int32_t source); // Accepts bit flags

	CString GetMp3Path() const;
	void SetMp3Path(const CString& path);

	// Startup shortcut manipulation
	bool CreateStartupShortcut() const;
	bool RemoveStartupShortcut() const;

private:
	CString GetAppDataDir() const;
	CString GetStartupFolder() const;
	CString GetAppExePath() const;

private:
	void InitKvIfNeeded();
	MMKV* m_kv = nullptr;
};


