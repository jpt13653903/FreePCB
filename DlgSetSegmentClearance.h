#pragma once
#include "afxwin.h"


// DlgSetSegmentClearance dialog

class DlgSetSegmentClearance : public CDialog
{
	DECLARE_DYNAMIC(DlgSetSegmentClearance)

public:
	DlgSetSegmentClearance(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgSetSegmentClearance();

// Dialog Data
	enum { IDD = IDD_SEGMENT_CLEARANCE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedApplySeg();
	afx_msg void OnBnClickedApplyCon();
	afx_msg void OnBnClickedApplyNet();

public:
	// these variables should be set on entry
	int m_mode;                 // 0=segment, 1=connection, 2=net
	CClearanceInfo m_clearance; // clearance

	// these variables will be set on exit
	int m_def;		    // set default clearance (1=con, 2=net)
	int m_apply;	    // apply clearance (1=seg, 2=con, 3=net)
	int m_apply_pinvia; // apply clearance to pin/via (1=no, 2=auto, 3=use selected clearance)

protected:
	CButton m_radio1_use_net_default;
	CButton m_radio1_set_trace_clearance;
	CEdit m_edit_clearance;

	CButton m_check_def_net;

	CButton m_radio2_apply_seg;
	CButton m_radio2_apply_con;
	CButton m_radio2_apply_net;

	CButton m_radio3_no;
	CButton m_radio3_auto;
	CButton m_radio3_as_sel;

	afx_msg void OnBnClickedRadioUseNetClearance();
	afx_msg void OnBnClickedRadioSetTraceClearance();
};
