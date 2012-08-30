#pragma once
#include "afxwin.h"


// CDlgImportOptions dialog

class CDlgImportOptions : public CDialog
{
	DECLARE_DYNAMIC(CDlgImportOptions)

public:
	CDlgImportOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgImportOptions();
	void Initialize( int flags );

// Dialog Data
	enum { IDD = IDD_IMPORT_OPTIONS };
	enum { FREEPCB, PADSPCB };							// CPT

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_flags;
	CButton m_radio_remove_parts;
	CButton m_radio_keep_parts_no_connections;
	CButton m_radio_keep_parts_and_connections;
	CButton m_radio_change_fp;
	CButton m_radio_keep_fp;
	CButton m_radio_remove_nets;
	CButton m_radio_keep_nets;
	CButton m_button_save_and_import;
	// CPT added (used to be in file-open dialog):
	CButton m_radio_parts;
	CButton m_radio_nets;
	CButton m_radio_parts_and_nets;
	CButton m_radio_padspcb;
	CButton m_radio_freepcb;
	CButton m_check_file_sync;				// CPT2 added
	int m_format;

	void SetPartsNetsFlags();
	void EnableDisableButtons();
	afx_msg void OnBnClickedPartsAndNets();
	afx_msg void OnBnClickedParts();
	afx_msg void OnBnClickedNets();
	afx_msg void OnBnClickedFileSync();
	afx_msg void OnBnClickedSaveAndImport();
	afx_msg void OnBnClickedOk();
};
