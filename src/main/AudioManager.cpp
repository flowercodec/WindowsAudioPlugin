#include "AudioManager.h"
#include <shlwapi.h>
#include <shellapi.h>
#include <afxdlgs.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <atlbase.h>

CAudioManager::CAudioManager()
{
	m_workerThread = std::thread(&CAudioManager::WorkerThreadLoop, this);
}

CAudioManager::~CAudioManager()
{
	{
		std::lock_guard<std::mutex> lock(m_workerMutex);
		m_workerRunning = false;
		m_workerCv.notify_all();
	}
	if (m_workerThread.joinable()) {
		m_workerThread.join();
	}
	if (m_endpoint) {
		m_endpoint.Release();
	}
	if (m_comInitialized) {
		CoUninitialize();
		m_comInitialized = false;
	}
}

void CAudioManager::SetMp3Path(const CString& path)
{
	std::lock_guard<std::mutex> lock(m_stateMutex);
	m_mp3Path = path;
}

CString CAudioManager::GetMp3Path() const
{
	std::lock_guard<std::mutex> lock(m_stateMutex);
	return m_mp3Path;
}

CString CAudioManager::BrowseMp3File()
{
	CFileDialog dlg(TRUE, _T("mp3"), NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("MP3 Files (*.mp3)|*.mp3|All Files (*.*)|*.*||"));
	if (dlg.DoModal() == IDOK) {
		return dlg.GetPathName();
	}
	return _T("");
}

void CAudioManager::SetWakeMinVolumePercent(int percent)
{
	if (percent < 1) percent = 1;
	if (percent > 100) percent = 100;
	std::lock_guard<std::mutex> lock(m_stateMutex);
	m_wakeMinScalar = (float)percent / 100.0f;
}
void CAudioManager::SetWakePlaySeconds(float seconds)
{
	if (seconds < 1.0f) seconds = 1.0f;
	if (seconds > 30.0f) seconds = 30.0f;
	std::lock_guard<std::mutex> lock(m_stateMutex);
	m_wakePlaySeconds = seconds;
}

void CAudioManager::OnSystemWake()
{
	bool shouldNotify = false;
	{
		std::lock_guard<std::mutex> lock(m_workerMutex);

		TCHAR buf[256];
		wsprintf(buf, _T("[WindowsAudioPlugin] CAudioManager::OnSystemWake m_workerRunning = %d, m_wakeRequested = %d, m_wakeInProgress = %d\n"),
			m_workerRunning, m_wakeRequested, m_wakeInProgress);
		OutputDebugString(buf);

		if (m_workerRunning && !m_wakeRequested && !m_wakeInProgress) {
			m_wakeRequested = true;
			shouldNotify = true;
		}
	}
	if (shouldNotify) {
		m_workerCv.notify_one();
	}
}

void CAudioManager::WorkerThreadLoop()
{
	while (true) {
		std::unique_lock<std::mutex> lock(m_workerMutex);
		m_workerCv.wait(lock, [this] { return !m_workerRunning || m_wakeRequested; });
		if (!m_workerRunning) {
			break;
		}
		m_wakeRequested = false;
		m_wakeInProgress = true;
		lock.unlock();

		ExecuteWakeSequence();

		lock.lock();
		m_wakeInProgress = false;
		lock.unlock();
	}

	if (m_endpoint) {
		m_endpoint.Release();
	}
	if (m_comInitialized) {
		CoUninitialize();
		m_comInitialized = false;
	}
}

CString CAudioManager::GetMp3PathCopy() const
{
	return GetMp3Path();
}

float CAudioManager::GetWakeMinScalarCopy() const
{
	std::lock_guard<std::mutex> lock(m_stateMutex);
	return m_wakeMinScalar;
}
float CAudioManager::GetWakePlaySecondsCopy() const
{
	std::lock_guard<std::mutex> lock(m_stateMutex);
	return m_wakePlaySeconds;
}

void CAudioManager::ExecuteWakeSequence()
{
	TCHAR buf[256];
	wsprintf(buf, _T("[WindowsAudioPlugin] CAudioManager::ExecuteWakeSequence\n"));
	OutputDebugString(buf);

	CString mp3Path = GetMp3PathCopy();
	float wakeMinScalar = GetWakeMinScalarCopy();
	float wakePlaySeconds = GetWakePlaySecondsCopy();

	do {
		if (!EnsureEndpoint()) {
			break;
		}

		// check resources before any audio state change
		TCHAR exePath[MAX_PATH] = {0};
		GetModuleFileName(NULL, exePath, MAX_PATH);
		PathRemoveFileSpec(exePath);
		CString ffplayPath = CString(exePath) + _T("\\ffplay.exe");
		if (mp3Path.IsEmpty() || !PathFileExists(mp3Path) || !PathFileExists(ffplayPath)) {
			// missing prerequisites, skip action
			break;
		}

		float original = 0.0f;
		bool haveVolume = GetEndpointVolume(original);

		BOOL wasMuted = FALSE;
		m_endpoint->GetMute(&wasMuted);
		if (wasMuted) {
			m_endpoint->SetMute(FALSE, NULL);
		}

		if (haveVolume) {
			float boosted = original;
			if (original <= wakeMinScalar) {
				boosted = wakeMinScalar;
			}
			if (boosted > 1.0f) {
				boosted = 1.0f;
			}
			SetEndpointVolume(boosted);
		}

		// play mp3 then restore
		if (PathFileExists(ffplayPath)) {
			CString cmd;
			cmd.Format(_T("\"%s\" -nodisp -autoexit -t %.1f \"%s\""), ffplayPath.GetString(), wakePlaySeconds, mp3Path.GetString());
			STARTUPINFO si = {0};
			PROCESS_INFORMATION pi = {0};
			si.cb = sizeof(si);
			LPTSTR cmdLine = cmd.GetBuffer();
			if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
				// wait until playback finishes
				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			}
			cmd.ReleaseBuffer();
		}

		if (haveVolume) {
			SetEndpointVolume(original);
		}

		if (wasMuted) {
			m_endpoint->SetMute(TRUE, NULL);
		}
	} while (0);
}

bool CAudioManager::GetEndpointVolume(float& outScalar)
{
	outScalar = 0.0f;
	if (!EnsureEndpoint()) return false;
	return SUCCEEDED(m_endpoint->GetMasterVolumeLevelScalar(&outScalar));
}

bool CAudioManager::SetEndpointVolume(float scalar)
{
	if (scalar < 0.0f) scalar = 0.0f;
	if (scalar > 1.0f) scalar = 1.0f;
	if (!EnsureEndpoint()) return false;
	return SUCCEEDED(m_endpoint->SetMasterVolumeLevelScalar(scalar, NULL));
}

bool CAudioManager::EnsureEndpoint()
{
	if (m_endpoint) return true;

	if (!m_comInitialized) {
		HRESULT hr = CoInitialize(NULL);
		if (SUCCEEDED(hr)) {
			m_comInitialized = true;
		}
	}

	CComPtr<IMMDeviceEnumerator> enumr;
	if (FAILED(enumr.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) return false;

	CComPtr<IMMDevice> device;
	if (FAILED(enumr->GetDefaultAudioEndpoint(eRender, eConsole, &device))) return false;

	CComPtr<IAudioEndpointVolume> epv;
	if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&epv))) return false;

	m_endpoint = epv;
	return m_endpoint != nullptr;
}



