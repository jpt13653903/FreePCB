// DlgProjectOptions.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgProjectOptions.h"
#include "DlgAddWidth.h"
#include "PathDialog.h"
#include ".\dlgprojectoptions.h"

// CPT:  moved controls for autosave and auto-ratline-disable into DlgPrefs.

// global callback function for sorting
//		
int CALLBACK WidthCompare( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	if( lp1 == lp2 )
		return 0;
	else if( lp1 > lp2 )
		return 1;
	else
		return -1;
}

// CDlgProjectOptions dialog

IMPLEMENT_DYNAMIC(CDlgProjectOptions, CDialog)
CDlgProjectOptions::CDlgProjectOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgProjectOptions::IDD, pParent)
	, m_trace_w(0)
	, m_via_w(0)
	, m_hole_w(0)
	, m_layers(0)
{
	m_folder_changed = FALSE;
	m_folder_has_focus = FALSE;
}

CDlgProjectOptions::~CDlgProjectOptions()
{
}

void CDlgProjectOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	if( !pDX->m_bSaveAndValidate )
	{
		// convert NM to MILS
		m_glue_w = m_glue_w/NM_PER_MIL;
		m_trace_w = m_trace_w/NM_PER_MIL;
		m_via_w = m_via_w/NM_PER_MIL;
		m_hole_w = m_hole_w/NM_PER_MIL;
	}
	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Text(pDX, IDC_EDIT_NAME, m_name );
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_edit_folder);
	DDX_Text(pDX, IDC_EDIT_FOLDER, m_path_to_folder );
	DDX_Control(pDX, IDC_EDIT_LIBRARY_FOLDER, m_edit_lib_folder);
	DDX_Text(pDX, IDC_EDIT_LIBRARY_FOLDER, m_lib_folder );
	DDX_Control(pDX, IDC_EDIT_SYNC_FILE, m_edit_sync_file);			// CPT2 new
	DDX_Text(pDX, IDC_EDIT_SYNC_FILE, m_sync_file);
	DDX_Control(pDX, IDC_CHECK_SYNC_FILE, m_check_sync_file);		// CPT2 new
	DDX_Control(pDX, IDC_EDIT_NUM_LAYERS, m_edit_layers );
	DDX_Text(pDX, IDC_EDIT_NUM_LAYERS, m_layers );
	DDV_MinMaxInt(pDX, m_layers, 1, 16 );
	DDX_Text(pDX, IDC_EDIT_GLUE_W, m_glue_w ); 
	DDV_MinMaxInt(pDX, m_glue_w, 1, 1000 );
	DDX_Text(pDX, IDC_EDIT_DEF_TRACE_W, m_trace_w ); 
	DDV_MinMaxInt(pDX, m_trace_w, 1, 1000 );
	DDX_Text(pDX, IDC_EDIT_DEF_VIA_W, m_via_w );
	DDV_MinMaxInt(pDX, m_via_w, 1, 1000 );
	DDX_Text(pDX, IDC_EDIT_DEF_VIA_HOLE, m_hole_w );
	DDV_MinMaxInt(pDX, m_hole_w, 1, 1000 );
	DDX_Control(pDX, IDC_LIST_WIDTH_MENU, m_list_menu);
	DDX_Control(pDX, IDC_CHECK1, m_check_SMT_connect);
	// CPT:
	DDX_Control(pDX, IDC_BUTTON_PROJ, m_button_proj);
	DDX_Control(pDX, IDC_CHECK_OPTIONS_DEFAULT, m_check_default);

	if( pDX->m_bSaveAndValidate )
	{
		// leaving dialog
		if( m_name.GetLength() == 0 )
		{
			CString s ((LPCSTR) IDS_PleaseEnterNameForProject);
			pDX->PrepareEditCtrl( IDC_EDIT_NAME );
			AfxMessageBox( s );
			pDX->Fail();
		}
		else if( m_path_to_folder.GetLength() == 0 )
		{
			CString s ((LPCSTR) IDS_PleaseEnterProjectFolder);
			pDX->PrepareEditCtrl( IDC_EDIT_FOLDER );
			AfxMessageBox( s );
			pDX->Fail();
		}
		else if( m_lib_folder.GetLength() == 0 )
		{
			CString s ((LPCSTR) IDS_PleaseEnterLibraryFolder);
			pDX->PrepareEditCtrl( IDC_EDIT_LIBRARY_FOLDER );
			AfxMessageBox( s );
			pDX->Fail();
		}
		else
		{
			// save options
			m_bSMT_connect_copper = m_check_SMT_connect.GetCheck();

			// convert NM to MILS
			m_trace_w = m_trace_w*NM_PER_MIL;
			m_via_w = m_via_w*NM_PER_MIL;
			m_hole_w = m_hole_w*NM_PER_MIL;
			m_glue_w = m_glue_w*NM_PER_MIL;

			// update trace width menu
			int n = m_list_menu.GetItemCount();
			m_w->SetSize( n );
			m_v_w->SetSize( n );
			m_v_h_w->SetSize( n );
			for( int i=0; i<n; i++ )
			{
				char str[10];
				m_list_menu.GetItemText( i, 0, str, 10 );
				m_w->SetAt(i, atoi(str)*NM_PER_MIL);
				m_list_menu.GetItemText( i, 1, str, 10 );
				m_v_w->SetAt(i, atoi(str)*NM_PER_MIL);
				m_list_menu.GetItemText( i, 2, str, 10 );
				m_v_h_w->SetAt(i, atoi(str)*NM_PER_MIL);
			}
		}
		// CPT:
		m_default = m_check_default.GetCheck();
		m_bSyncFile = m_check_sync_file.GetCheck();
		// CPT2 TODO at least a warning if file already exists!
	}
}


BEGIN_MESSAGE_MAP(CDlgProjectOptions, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnEnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT_FOLDER, OnEnChangeEditFolder)
	ON_EN_CHANGE(IDC_EDIT_SYNC_FILE, OnEnChangeEditSyncFile)		// CPT2 new
	ON_EN_SETFOCUS(IDC_EDIT_FOLDER, OnEnSetfocusEditFolder)
	ON_EN_KILLFOCUS(IDC_EDIT_FOLDER, OnEnKillfocusEditFolder)
	ON_BN_CLICKED(IDC_BUTTON_LIB, OnBnClickedButtonLib)
	ON_BN_CLICKED(IDC_BUTTON_PROJ, OnBnClickedButtonProj)
	ON_BN_CLICKED(IDC_BUTTON_SYNC_FILE, OnBnClickedButtonSyncFile)	// CPT2 new
END_MESSAGE_MAP()

// initialize data
//
void CDlgProjectOptions::Init( bool new_project,
							  CString * name,
							  CString * path_to_folder,
							  CString * lib_folder,
							  bool bSyncFile,
							  CString * sync_file,
							  int num_layers,
							  bool bSMT_connect_copper,
							  int glue_w,
							  int trace_w,
							  int via_w,
							  int hole_w,
							  CArray<int> * w,
							  CArray<int> * v_w,
							  CArray<int> * v_h_w )
{
	m_new_project = new_project;
	m_name = *name;
	m_path_to_folder = *path_to_folder;
	m_lib_folder = *lib_folder;
	m_sync_file = *sync_file;				// CPT2
	m_bSyncFile = bSyncFile;				// CPT2
	m_layers = num_layers;
	m_bSMT_connect_copper = bSMT_connect_copper;
	m_glue_w = glue_w;
	m_trace_w = trace_w;
	m_via_w = via_w;
	m_hole_w = hole_w;
	m_w = w;
	m_v_w = v_w;
	m_v_h_w = v_h_w;
}

BOOL CDlgProjectOptions::OnInitDialog()
{
	CDialog::OnInitDialog();

	// initialize strings
	m_edit_folder.SetWindowText( m_path_to_folder );
	m_edit_sync_file.SetWindowTextA( m_sync_file );				// CPT2 new
	// set up list control
	DWORD old_style = m_list_menu.GetExtendedStyle();
	m_list_menu.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | old_style );
	CString colNames[3];
	for (int i=0; i<3; i++)
		colNames[i].LoadStringA(IDS_ProjectOptionsCols+i);
	m_list_menu.InsertColumn( 0, colNames[0], LVCFMT_LEFT, 77 );
	m_list_menu.InsertColumn( 1, colNames[1], LVCFMT_LEFT, 77 );
	m_list_menu.InsertColumn( 2, colNames[2], LVCFMT_LEFT, 78 );
	CString str;
	int n = m_w->GetSize();
	for( int i=0; i<n; i++ )
	{
		int nItem = m_list_menu.InsertItem( i, "" );
		m_list_menu.SetItemData( i, (LPARAM)m_w->GetAt(i) );
		str.Format( "%d", m_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 0, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", m_v_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", m_v_h_w->GetAt(i)/NM_PER_MIL );
		m_list_menu.SetItem( i, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
	}
	if( !m_new_project )
	{
		// disable some fields for existing project
		m_edit_folder.EnableWindow( FALSE );
		m_button_proj.EnableWindow(FALSE);
	}
	m_check_SMT_connect.SetCheck( m_bSMT_connect_copper );
	m_check_sync_file.SetCheck( m_bSyncFile );					// CPT2 new
	return TRUE;
}

// CDlgProjectOptions message handlers

void CDlgProjectOptions::OnBnClickedButtonAdd()
{
	CDlgAddWidth dlg;
	dlg.m_width = 0;
	dlg.m_via_w = 0;
	dlg.m_via_hole_w = 0;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString str;
		m_list_menu.InsertItem( 0, "" );
		m_list_menu.SetItemData( 0, (LPARAM)dlg.m_width * NM_PER_MIL );		// CPT:  bug fix (led to bad sorting)
		str.Format( "%d", dlg.m_width );
		m_list_menu.SetItem( 0, 0, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", dlg.m_via_w );
		m_list_menu.SetItem( 0, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
		str.Format( "%d", dlg.m_via_hole_w );
		m_list_menu.SetItem( 0, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
		m_list_menu.SortItems( ::WidthCompare, 0 );
	}
}

void CDlgProjectOptions::OnBnClickedButtonEdit()
{
	POSITION pos = m_list_menu.GetFirstSelectedItemPosition();
	int i_sel = m_list_menu.GetNextSelectedItem( pos );
	if( i_sel < 0 )
	{
		CString s ((LPCSTR) IDS_NoMenuItemSelected);
		AfxMessageBox( s );
	}
	else
	{
		CDlgAddWidth dlg;
		char str[10];
		m_list_menu.GetItemText( i_sel, 0, str, 10 );
		dlg.m_width = atoi( str );
		m_list_menu.GetItemText( i_sel, 1, str, 10 );
		dlg.m_via_w = atoi( str );
		m_list_menu.GetItemText( i_sel, 2, str, 10 );
		dlg.m_via_hole_w = atoi( str );
		int ret = dlg.DoModal();
		if( ret == IDOK )
		{
			m_list_menu.DeleteItem( i_sel );
			CString str;
			m_list_menu.InsertItem( 0, "" );
			m_list_menu.SetItemData( 0, (LPARAM)dlg.m_width * NM_PER_MIL );			// CPT bug fix (led to bad sorting)
			str.Format( "%d", dlg.m_width );
			m_list_menu.SetItem( 0, 0, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", dlg.m_via_w );
			m_list_menu.SetItem( 0, 1, LVIF_TEXT, str, 0, 0, 0, 0 );
			str.Format( "%d", dlg.m_via_hole_w );
			m_list_menu.SetItem( 0, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
			m_list_menu.SortItems( ::WidthCompare, 0 );
		}
	}
}

void CDlgProjectOptions::OnBnClickedButtonDelete()
{
	POSITION pos = m_list_menu.GetFirstSelectedItemPosition();
	CString s ((LPCSTR) IDS_NoMenuItemSelected);
	int i_sel = m_list_menu.GetNextSelectedItem( pos );
	if( i_sel < 0 )
		AfxMessageBox( s );
	else
		m_list_menu.DeleteItem( i_sel );
}

void CDlgProjectOptions::OnEnChangeEditName()
{
	/* CPT
	CString str;
	m_edit_name.GetWindowText( str ); 
	if( m_new_project == TRUE && m_folder_changed == FALSE )
		m_edit_folder.SetWindowText( m_path_to_folder + str );
	*/
}

void CDlgProjectOptions::OnEnChangeEditFolder()
{
	if( m_folder_has_focus )
		m_folder_changed = TRUE;
}

void CDlgProjectOptions::OnEnSetfocusEditFolder()
{
	m_folder_has_focus = TRUE;
}

void CDlgProjectOptions::OnEnKillfocusEditFolder()
{
	m_folder_has_focus = FALSE;
}

void CDlgProjectOptions::OnEnChangeEditSyncFile()
{
	// CPT2 new
	CString str;
	m_edit_sync_file.GetWindowText( str ); 
	m_check_sync_file.SetCheck( str.GetLength()>0 );
}

void CDlgProjectOptions::OnBnClickedButtonLib()
{
	CString s1 ((LPCSTR) IDS_LibraryFolder), s2 ((LPCSTR) IDS_SelectDefaultLibraryFolder);
	CPathDialog dlg( s1, s2, m_lib_folder );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_lib_folder = dlg.GetPathName();
		m_edit_lib_folder.SetWindowText( m_lib_folder );
	}
}

void CDlgProjectOptions::OnBnClickedButtonProj()
{
	CString s1 ((LPCSTR) IDS_ProjectFolder), s2 ((LPCSTR) IDS_SelectDefaultProjectFolder);
	CPathDialog dlg( s1, s2, m_path_to_folder );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_path_to_folder = dlg.GetPathName();
		m_edit_folder.SetWindowText( m_path_to_folder );
	}
}

void CDlgProjectOptions::OnBnClickedButtonSyncFile()
{
	// CPT2 new
	CString s ((LPCSTR) IDS_AllFiles), s2 ((LPCSTR) IDS_SynchronizeWithNetListFile), s3;
	m_edit_sync_file.GetWindowTextA( s3 );
	if (s3=="")
	{
		s3 = m_path_to_folder;
		if (s3.Right(1) != "\\") 
			s3 += "\\";
		s3 += m_name + ".txt";
	}
	CFileDialog dlg( TRUE , NULL, (LPCTSTR)s3, 0, 
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );
	dlg.m_ofn.lpstrTitle = s2;
	int ret = dlg.DoModal();
	if (ret != IDOK )
		return;
	m_sync_file = dlg.GetPathName(); 
	m_edit_sync_file.SetWindowTextA( m_sync_file );
	m_check_sync_file.SetCheck( m_sync_file.GetLength() > 0 );
}

