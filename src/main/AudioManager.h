#pragma once

#include <afx.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <thread>
#include <mutex>
#include <condition_variable>

class CAudioManager
{
public:
	CAudioManager();
	~CAudioManager();

	void SetMp3Path(const CString& path);
	CString GetMp3Path() const;

	CString BrowseMp3File();

	void SetWakeMinVolumePercent(int percent); // 1..100 (converted to scalar internally)
	void SetWakePlaySeconds(float seconds);     // 1.0..30.0 seconds (supports decimal)

	void OnSystemWake();

private:
	// Worker thread helpers
	void WorkerThreadLoop();
	void ExecuteWakeSequence();
	CString GetMp3PathCopy() const;
	float GetWakeMinScalarCopy() const;
	float GetWakePlaySecondsCopy() const;

private:
	CString m_mp3Path;
	float   m_wakeMinScalar = 0.08f; // default 8%
	float   m_wakePlaySeconds = 3.0f;

	// Shared state protection
	mutable std::mutex m_stateMutex;

	// Worker thread synchronization
	std::thread m_workerThread;
	std::mutex m_workerMutex;
	std::condition_variable m_workerCv;
	bool m_workerRunning = true;
	bool m_wakeRequested = false;
	bool m_wakeInProgress = false;

private:
	bool GetEndpointVolume(float& outScalar);
	bool SetEndpointVolume(float scalar);

private:
	bool EnsureEndpoint();

private:
	CComPtr<IAudioEndpointVolume> m_endpoint;
	bool m_comInitialized = false;
};

