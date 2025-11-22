#pragma once

#include <afxdialogex.h>
#include "resource.h"
#include "Settings.h"
#include "AudioManager.h"

class CConfigDialog : public CDialogEx
{
public:
	CConfigDialog(CSettings& settings, CAudioManager& audioMgr, CWnd* pParent = nullptr);
	enum { IDD = IDD_CONFIG };

protected:
	virtual BOOL OnInitDialog() override;
	virtual void DoDataExchange(CDataExchange* pDX) override;
	afx_msg void OnBrowse();
	afx_msg void OnTestWake();
	afx_msg void OnSpinWakePlay(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK() override;

	DECLARE_MESSAGE_MAP()

private:
	CSettings&     m_settings;
	CAudioManager& m_audioMgr;

	CButton m_chkWake;
	CButton m_chkHook;
	CEdit   m_edtMp3;
	CEdit   m_edtHookStep;
	CSpinButtonCtrl m_spinHookStep;
	CEdit   m_edtWakeMin;
	CSpinButtonCtrl m_spinWakeMin;
	CEdit   m_edtWakePlay;
	CSpinButtonCtrl m_spinWakePlay;
	CComboBox m_cmbWakeTrigger;

	afx_msg void OnToggleHook();
	afx_msg void OnToggleWake();
	void UpdateHookStepEnable();
	void UpdateWakeControlsEnable();
};


