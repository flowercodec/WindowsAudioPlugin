#include "ConfigDialog.h"
#include "resource.h"
#include <afxdlgs.h>

BEGIN_MESSAGE_MAP(CConfigDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BROWSE, &CConfigDialog::OnBrowse)
	ON_BN_CLICKED(IDC_ENABLE_HOOK, &CConfigDialog::OnToggleHook)
	ON_BN_CLICKED(IDC_ENABLE_WAKE, &CConfigDialog::OnToggleWake)
	ON_BN_CLICKED(IDC_TEST_WAKE, &CConfigDialog::OnTestWake)
	ON_NOTIFY(UDN_DELTAPOS, IDC_WAKE_PLAY_SPIN, &CConfigDialog::OnSpinWakePlay)
END_MESSAGE_MAP()

CConfigDialog::CConfigDialog(CSettings& settings, CAudioManager& audioMgr, CWnd* pParent)
	: CDialogEx(IDD_CONFIG, pParent)
	, m_settings(settings)
	, m_audioMgr(audioMgr)
{
}

void CConfigDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ENABLE_WAKE, m_chkWake);
	DDX_Control(pDX, IDC_ENABLE_HOOK, m_chkHook);
	DDX_Control(pDX, IDC_MP3_PATH, m_edtMp3);
	DDX_Control(pDX, IDC_HOOK_STEP_EDIT, m_edtHookStep);
	DDX_Control(pDX, IDC_HOOK_STEP_SPIN, m_spinHookStep);
	DDX_Control(pDX, IDC_WAKE_MIN_EDIT, m_edtWakeMin);
	DDX_Control(pDX, IDC_WAKE_MIN_SPIN, m_spinWakeMin);
	DDX_Control(pDX, IDC_WAKE_PLAY_EDIT, m_edtWakePlay);
	DDX_Control(pDX, IDC_WAKE_PLAY_SPIN, m_spinWakePlay);
	DDX_Control(pDX, IDC_WAKE_TRIGGER_COMBO, m_cmbWakeTrigger);
}

BOOL CConfigDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Ensure dialog shows in taskbar even though main window is hidden
	LONG_PTR exStyle = GetWindowLongPtr(GetSafeHwnd(), GWL_EXSTYLE);
	exStyle &= ~WS_EX_TOOLWINDOW;
	exStyle |= WS_EX_APPWINDOW;
	SetWindowLongPtr(GetSafeHwnd(), GWL_EXSTYLE, exStyle);
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	m_chkWake.SetCheck(m_settings.IsWakeActionsEnabled() ? BST_CHECKED : BST_UNCHECKED);
	m_chkHook.SetCheck(m_settings.IsHookEnabled() ? BST_CHECKED : BST_UNCHECKED);
	m_edtMp3.SetWindowText(m_settings.GetMp3Path());

	// Initialize wake trigger source combo box
	m_cmbWakeTrigger.AddString(_T("Power Resume"));
	m_cmbWakeTrigger.AddString(_T("User Login"));
	m_cmbWakeTrigger.AddString(_T("All"));
	int32_t currentSource = m_settings.GetWakeTriggerSource();
	int selIndex = 1; // default to User Login
	if (currentSource == WAKE_TRIGGER_POWER_RESUME) {
		selIndex = 0;
	} else if (currentSource == WAKE_TRIGGER_USER_LOGIN) {
		selIndex = 1;
	} else if (currentSource == WAKE_TRIGGER_ALL) {
		selIndex = 2;
	}
	m_cmbWakeTrigger.SetCurSel(selIndex);

	// Init hook step percent controls
	m_spinHookStep.SetBuddy(&m_edtHookStep);
	m_spinHookStep.SetRange(1, 20);
	m_spinHookStep.SetPos(m_settings.GetHookStepPercent());

	// Init wake min volume percent controls
	m_spinWakeMin.SetBuddy(&m_edtWakeMin);
	m_spinWakeMin.SetRange(1, 100);
	m_spinWakeMin.SetPos(m_settings.GetWakeMinVolumePercent());
	{
		CString tmp; tmp.Format(_T("%d"), m_settings.GetWakeMinVolumePercent());
		m_edtWakeMin.SetWindowText(tmp);
	}
	// Initialize wake play seconds (supports decimal with 0.01 precision, e.g., 1.50)
	m_spinWakePlay.SetBuddy(&m_edtWakePlay);
	m_spinWakePlay.SetRange(1, 30); // Range in seconds (1.00 to 30.00 seconds)
	{
		float playSeconds = m_settings.GetWakePlaySeconds();
		CString tmp; tmp.Format(_T("%.2f"), playSeconds);
		m_edtWakePlay.SetWindowText(tmp);
		// Set spin position (rounded to nearest integer)
		m_spinWakePlay.SetPos((int)(playSeconds + 0.5f));
	}

	UpdateHookStepEnable();
	UpdateWakeControlsEnable();

	return TRUE;
}

void CConfigDialog::OnBrowse()
{
	CFileDialog dlg(TRUE, _T("mp3"), NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("MP3 Files (*.mp3)|*.mp3|All Files (*.*)|*.*||"));
	if (dlg.DoModal() == IDOK) {
		m_edtMp3.SetWindowText(dlg.GetPathName());
	}
}

void CConfigDialog::OnOK()
{
	BOOL wake = (m_chkWake.GetCheck() == BST_CHECKED);
	BOOL hook = (m_chkHook.GetCheck() == BST_CHECKED);
	m_settings.SetWakeActionsEnabled(wake != FALSE);
	m_settings.SetHookEnabled(hook != FALSE);

	// hook step percent
	CString stepStr;
	m_edtHookStep.GetWindowText(stepStr);
	int step = _ttoi(stepStr);
	if (step < 1) step = 1;
	if (step > 20) step = 20;
	m_settings.SetHookStepPercent(step);

	// wake min volume percent
	CString wakeStr;
	m_edtWakeMin.GetWindowText(wakeStr);
	int wakeMin = _ttoi(wakeStr);
	if (wakeMin < 1) wakeMin = 1;
	if (wakeMin > 100) wakeMin = 100;
	m_settings.SetWakeMinVolumePercent(wakeMin);
	m_audioMgr.SetWakeMinVolumePercent(wakeMin);

	// wake play seconds (supports decimal, e.g., 1.5)
	CString playStr;
	m_edtWakePlay.GetWindowText(playStr);
	float playSeconds = (float)_tstof(playStr);
	if (playSeconds < 1.0f) playSeconds = 1.0f;
	if (playSeconds > 30.0f) playSeconds = 30.0f;
	m_settings.SetWakePlaySeconds(playSeconds);
	m_audioMgr.SetWakePlaySeconds(playSeconds);

	CString path;
	m_edtMp3.GetWindowText(path);
	m_settings.SetMp3Path(path);
	m_audioMgr.SetMp3Path(path);

	// Save wake trigger source
	int selIndex = m_cmbWakeTrigger.GetCurSel();
	int32_t triggerValue = WAKE_TRIGGER_USER_LOGIN; // default
	if (selIndex == 0) {
		triggerValue = WAKE_TRIGGER_POWER_RESUME;
	} else if (selIndex == 1) {
		triggerValue = WAKE_TRIGGER_USER_LOGIN;
	} else if (selIndex == 2) {
		triggerValue = WAKE_TRIGGER_ALL;
	}
	m_settings.SetWakeTriggerSource(triggerValue);

	m_settings.Save();

	CDialogEx::OnOK();
}

void CConfigDialog::OnToggleHook()
{
	UpdateHookStepEnable();
}

void CConfigDialog::OnToggleWake()
{
	UpdateWakeControlsEnable();
}

void CConfigDialog::UpdateHookStepEnable()
{
	BOOL enabled = (m_chkHook.GetCheck() == BST_CHECKED);
	m_edtHookStep.EnableWindow(enabled);
	m_spinHookStep.EnableWindow(enabled);
	GetDlgItem(IDC_HOOK_STEP_LABEL)->EnableWindow(enabled);
}

void CConfigDialog::UpdateWakeControlsEnable()
{
	BOOL enabled = (m_chkWake.GetCheck() == BST_CHECKED);
	m_edtWakeMin.EnableWindow(enabled);
	m_spinWakeMin.EnableWindow(enabled);
	GetDlgItem(IDC_WAKE_MIN_LABEL)->EnableWindow(enabled);
	m_edtWakePlay.EnableWindow(enabled);
	m_spinWakePlay.EnableWindow(enabled);
	GetDlgItem(IDC_WAKE_PLAY_LABEL)->EnableWindow(enabled);
	m_edtMp3.EnableWindow(enabled);
	GetDlgItem(IDC_BROWSE)->EnableWindow(enabled);
	GetDlgItem(IDC_TEST_WAKE)->EnableWindow(enabled);
	m_cmbWakeTrigger.EnableWindow(enabled);
	GetDlgItem(IDC_WAKE_TRIGGER_LABEL)->EnableWindow(enabled);
}

void CConfigDialog::OnTestWake()
{
	// Update AudioManager with current dialog values before testing
	CString mp3Path;
	m_edtMp3.GetWindowText(mp3Path);
	m_audioMgr.SetMp3Path(mp3Path);

	CString wakeMinStr;
	m_edtWakeMin.GetWindowText(wakeMinStr);
	int wakeMin = _ttoi(wakeMinStr);
	if (wakeMin < 1) wakeMin = 1;
	if (wakeMin > 100) wakeMin = 100;
	m_audioMgr.SetWakeMinVolumePercent(wakeMin);

	CString playStr;
	m_edtWakePlay.GetWindowText(playStr);
	float playSeconds = (float)_tstof(playStr);
	if (playSeconds < 1.0f) playSeconds = 1.0f;
	if (playSeconds > 30.0f) playSeconds = 30.0f;
	m_audioMgr.SetWakePlaySeconds(playSeconds);

	// Test the wake actions
	m_audioMgr.OnSystemWake();
}

void CConfigDialog::OnSpinWakePlay(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	// Get current value from edit box
	CString playStr;
	m_edtWakePlay.GetWindowText(playStr);
	float currentValue = (float)_tstof(playStr);
	
	// Adjust by 1.0 second per click (minimum unit is 0.01 second)
	// Note: direction is reversed, iDelta > 0 means up arrow (increase), iDelta < 0 means down arrow (decrease)
	if (pNMUpDown->iDelta > 0) {
		// Up arrow: increase by 1.0 second
		currentValue += 1.0f;
	} else {
		// Down arrow: decrease by 1.0 second
		currentValue -= 1.0f;
	}
	
	// Clamp to valid range
	if (currentValue < 1.0f) currentValue = 1.0f;
	if (currentValue > 30.0f) currentValue = 30.0f;
	
	// Update edit box with 2 decimal places (0.01 precision)
	CString tmp; tmp.Format(_T("%.2f"), currentValue);
	m_edtWakePlay.SetWindowText(tmp);
	
	// Prevent default behavior
	*pResult = 1;
}

