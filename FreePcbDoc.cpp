// FreePcbDoc.cpp : implementation of the CFreePcbDoc class
//
#pragma once

#include "stdafx.h"
#include <direct.h>
#include <shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "PcbFont.h"
#include "DlgAddPart.h"
#include "DlgEditNet.h"
#include "DlgAssignNet.h"
#include "DlgNetlist.h"
#include "DlgProjectOptions.h"
#include "DlgImportOptions.h" 
#include "freepcbdoc.h"
#include "DlgLayers.h"
#include "DlgPartlist.h"
#include "MyFileDialog.h"
#include "DlgExportOptions.h"
#include "DlgIvex.h"
#include "DlgIndexing.h"
#include "UndoBuffer.h"
#include "UndoList.h"
#include "DlgCAD.h"
#include "DlgWizQuad.h"
#include "utility.h"
#include "gerber.h"
#include "dlgdrc.h"
#include "DlgGroupPaste.h"
#include "DlgReassignLayers.h"
#include "DlgExportDsn.h"
#include "DlgImportSes.h"
#include "RTcall.h"
#include "DlgReport.h"
#include "DlgNetCombine.h"
#include "DlgMyMessageBox.h"
#include "DlgSaveLib.h"
//CPT
#include "DlgPrefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFreePcbApp theApp;

CFreePcbDoc * this_Doc;		// global for callback

BOOL m_bShowMessageForClearance = TRUE;

// global array to map file_layers to actual layers
int m_layer_by_file_layer[MAX_LAYERS];


/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc

IMPLEMENT_DYNCREATE(CFreePcbDoc, CDocument)

BEGIN_MESSAGE_MAP(CFreePcbDoc, CDocument)
	//{{AFX_MSG_MAP(CFreePcbDoc)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_ADD_PART, OnAddPart)
	ON_COMMAND(ID_NONE_ADDPART, OnAddPart)
	ON_COMMAND(ID_VIEW_NETLIST, OnProjectNetlist)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_COMMAND(ID_VIEW_LAYERS, OnViewLayers)
	ON_COMMAND(ID_VIEW_PARTLIST, OnProjectPartlist)
	ON_COMMAND(ID_PART_PROPERTIES, OnPartProperties)
	ON_COMMAND(ID_FILE_IMPORT, OnFileImport)
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_COMMAND(ID_FILE_CONVERT, OnFileConvert)
	ON_COMMAND(ID_FILE_GENERATECADFILES, OnFileGenerateCadFiles)
	ON_COMMAND(ID_TOOLS_FOOTPRINTWIZARD, OnToolsFootprintwizard)
	ON_COMMAND(ID_PROJECT_OPTIONS, OnProjectOptions)
	ON_COMMAND(ID_FILE_EXPORTNETLIST, OnFileExport)
	ON_COMMAND(ID_TOOLS_CHECK_PARTS_NETS, OnToolsCheckPartsAndNets)
	ON_COMMAND(ID_TOOLS_DRC, OnToolsDrc)
	ON_COMMAND(ID_TOOLS_CLEAR_DRC, OnToolsClearDrc)
	ON_COMMAND(ID_TOOLS_SHOWDRCERRORLIST, OnToolsShowDRCErrorlist)
	ON_COMMAND(ID_TOOLS_CHECK_CONNECTIVITY, OnToolsCheckConnectivity)
	ON_COMMAND(ID_VIEW_LOG, OnViewLog)
	ON_COMMAND(ID_TOOLS_CHECKCOPPERAREAS, OnToolsCheckCopperAreas)
	ON_COMMAND(ID_TOOLS_CHECKTRACES, OnToolsCheckTraces)
	ON_COMMAND(ID_EDIT_PASTEFROMFILE, OnEditPasteFromFile)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_DSN_FILE_EXPORT, OnFileExportDsn)
	ON_COMMAND(ID_SES_FILE_IMPORT, OnFileImportSes)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_NONE_REPEATDRC, OnRepeatDrc)
	ON_COMMAND(ID_TOOLS_REPEATDRC, OnRepeatDrc)
	ON_COMMAND(ID_FILE_GENERATEREPORTFILE, OnFileGenerateReportFile)
	ON_COMMAND(ID_PROJECT_COMBINENETS, OnProjectCombineNets)
	ON_COMMAND(ID_FILE_LOADLIBRARYASPROJECT, OnFileLoadLibrary)
	ON_COMMAND(ID_FILE_SAVEPROJECTASLIBRARY, OnFileSaveLibrary)
	// CPT
	ON_COMMAND(ID_TOOLS_PREFERENCES, OnToolsPreferences)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc construction/destruction

CFreePcbDoc::CFreePcbDoc()
{
	// get application directory
	// (there must be a better way to do this!!!)
	int token_start = 0;
	CString delim = " ";
	CString cmdline = GetCommandLine();
	if( cmdline[0] == '\"' )
	{
		delim = "\"";
		token_start = 1;
	}
	CString app_dir = cmdline.Tokenize( delim, token_start );
	int pos = app_dir.ReverseFind( '\\' );
	if( pos == -1 )
		pos = app_dir.ReverseFind( ':' ); 
	if( pos == -1 )
		ASSERT(0);	// failed to find application folder
	app_dir = app_dir.Left( pos );
	m_app_dir = app_dir;
	m_app_dir.Trim();
	int err = _chdir( m_app_dir );	// change to application folder 
	if( err )
		ASSERT(0);	// failed to switch to application folder

	m_smfontutil = new SMFontUtil( &m_app_dir );
	m_pcbu_per_wu = 25400;	// default nm per world unit
	DWORD dwVersion = ::GetVersion();
	DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if( dwWindowsMajorVersion > 4 )
		m_pcbu_per_wu = 2540;		// if Win2000 or XP or vista
	m_dlist = new CDisplayList( m_pcbu_per_wu, m_smfontutil );
	m_dlist_fp = new CDisplayList( m_pcbu_per_wu, m_smfontutil );
	m_plist = new CPartList( m_dlist );
	m_nlist = new CNetList( m_dlist, m_plist );
	m_plist->UseNetList( m_nlist );
	m_plist->SetShapeCacheMap( &m_footprint_cache_map );
	m_tlist = new CTextList( m_dlist, m_smfontutil );
	m_drelist = new DRErrorList;
	m_drelist->SetLists( m_plist, m_nlist, m_dlist );
	m_pcb_filename = "";
	m_pcb_full_path = "";
	m_board_outline.RemoveAll();
	m_project_open = FALSE;
	m_project_modified = FALSE;
	m_project_modified_since_autosave = FALSE;
	m_footprint_modified = FALSE;
	m_footprint_name_changed = FALSE;
	theApp.m_Doc = this;
	m_undo_list = new CUndoList( 10000 );
	m_redo_list = new CUndoList( 10000 );
	this_Doc = this;
	m_auto_interval = 0;
	m_auto_elapsed = 0;
	m_dlg_log = NULL;
	bNoFilesOpened = TRUE;
	m_version = 1.359;
	m_file_version = 1.344;
	m_dlg_log = new CDlgLog;
	m_dlg_log->Create( IDD_LOG );
	m_import_flags = IMPORT_PARTS | IMPORT_NETS | KEEP_FP | KEEP_NETS | KEEP_TRACES | KEEP_STUBS | KEEP_AREAS;
	m_num_copper_layers = 1;
	m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;

	// initialize pseudo-clipboard
	clip_plist = new CPartList( NULL );
	clip_nlist = new CNetList( NULL, clip_plist );
	clip_plist->UseNetList( clip_nlist );
	clip_plist->SetShapeCacheMap( &m_footprint_cache_map );
	clip_tlist = new CTextList( NULL, m_smfontutil );

	// CPT
	ReadPrefs();
}

CFreePcbDoc::~CFreePcbDoc()
{
	// delete group clipboard
	delete clip_nlist;
	delete clip_plist;
	delete clip_tlist;
	clip_sm_cutout.RemoveAll();
	// delete partlist, netlist, displaylist, etc.
	m_sm_cutout.RemoveAll();
	delete m_drelist;
	delete m_undo_list;
	delete m_redo_list;
	m_board_outline.RemoveAll();
	delete m_nlist;
	delete m_plist;
	delete m_tlist;
	delete m_dlist;
	delete m_dlist_fp;
	delete m_smfontutil;
	// delete all footprints from local cache
	POSITION pos = m_footprint_cache_map.GetStartPosition();
	while( pos != NULL )
	{
		void * ptr;
		CShape * shape;
		CString string;
		m_footprint_cache_map.GetNextAssoc( pos, string, ptr );
		shape = (CShape*)ptr;
		delete shape;
	}
	m_footprint_cache_map.RemoveAll();
	// delete log window
	if( m_dlg_log )
	{
		m_dlg_log->DestroyWindow();
		delete m_dlg_log;
	}
}

void CFreePcbDoc::SendInitialUpdate()
{
	CDocument::SendInitialUpdate();
}

// this is only executed once, at beginning of app
//
BOOL CFreePcbDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	CString s1 ((LPCSTR) IDS_AppName), s2 ((LPCSTR) IDS_NoProjectOpen);
	m_window_title = s1 + " - " + s2;
	m_parent_folder = "..\\projects\\";
	m_lib_dir = "..\\lib\\" ;
	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc serialization

void CFreePcbDoc::Serialize(CArchive& ar)
{
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc diagnostics

#ifdef _DEBUG
void CFreePcbDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFreePcbDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc commands


BOOL CFreePcbDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	return CDocument::OnSaveDocument(lpszPathName);
}

void CFreePcbDoc::OnFileNew()
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_View_fp->OnFootprintFileNew();
		return;
	}

	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();

	// now set default project options
	InitializeNewProject();
	CDlgProjectOptions dlg;
	dlg.Init( TRUE, &m_name, &m_parent_folder, &m_lib_dir,
		m_num_copper_layers, m_bSMT_copper_connect, m_default_glue_w,
		m_trace_w, m_via_w, m_via_hole_w,
		m_auto_interval, m_auto_ratline_disable, m_auto_ratline_min_pins,
		&m_w, &m_v_w, &m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// set up project file name and path
		m_name = dlg.GetName();
		m_pcb_filename = m_name + ".fpc";
		CString fullpath;
		char full[_MAX_PATH];
		fullpath = _fullpath( full, (LPCSTR)dlg.GetPathToFolder(), MAX_PATH );
		m_path_to_folder = (CString)fullpath;

		// Check if project folder exists and create it if necessary
		struct _stat buf;
		int err = _stat( m_path_to_folder, &buf );
		if( err )
		{
			CString str, s ((LPCSTR) IDS_FolderDoesntExistCreateIt);
			str.Format( s, m_path_to_folder );
			int ret = AfxMessageBox( str, MB_YESNO );
			if( ret == IDYES )
			{
				err = _mkdir( m_path_to_folder );
				if( err )
				{
					CString s ((LPCSTR) IDS_UnableToCreateFolder);
					str.Format( s, m_path_to_folder );
					AfxMessageBox( str, MB_OK );
				}
			}
		}
		if( err )
			return;

		CString str;
		m_pcb_full_path = (CString)fullpath	+ "\\" + m_pcb_filename;
		CString s1 ((LPCSTR) IDS_AppName);
		m_window_title = s1 + " - " + m_pcb_filename;
		CWnd* pMain = AfxGetMainWnd();
		pMain->SetWindowText( m_window_title );

		// make path to library folder and index libraries
		m_lib_dir = dlg.GetLibFolder();
		fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
		if( fullpath[fullpath.GetLength()-1] == '\\' )	
			fullpath = fullpath.Left(fullpath.GetLength()-1);
		m_full_lib_dir = fullpath;
		MakeLibraryMaps( &m_full_lib_dir );
		CMenu* pMenu = pMain->GetMenu();
		pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_ENABLED ); 
		pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_ENABLED ); 
		pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_ENABLED ); 
		pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_ENABLED ); 
		pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_ENABLED ); 
		CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
		submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_GENERATEREPORTFILE, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_DSN_FILE_EXPORT, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_SES_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
		submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
		submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		pMain->DrawMenuBar();

		// set options from dialog
		m_auto_ratline_disable = dlg.GetAutoRatlineDisable();
		m_auto_ratline_min_pins = dlg.GetAutoRatlineMinPins();
		m_num_copper_layers = dlg.GetNumCopperLayers();
		m_plist->SetNumCopperLayers( m_num_copper_layers );
		m_nlist->SetNumCopperLayers( m_num_copper_layers );
		m_nlist->SetSMTconnect( m_bSMT_copper_connect );
		m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
		m_trace_w = dlg.GetTraceWidth();
		m_via_w = dlg.GetViaWidth();
		m_via_hole_w = dlg.GetViaHoleWidth();
		m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
		for( int i=0; i<m_num_layers; i++ )
		{
			m_vis[i] = 1;
			m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
		}

		// CPT:  fixed bug where m_view->m_dlist->SetMapping() didn't get called, with unpredictable results for the initial
		// display
		m_project_open = TRUE;
		m_view->OnViewAllElements();

		// force redraw of function key text
		m_view->m_cursor_mode = 999;
		m_view->SetCursorMode( CUR_NONE_SELECTED );

		// force redraw of window title
		m_project_modified = FALSE;
		m_project_modified_since_autosave = FALSE;
		m_auto_elapsed = 0;

		// save project
		OnFileSave();
	}
}

void CFreePcbDoc::OnFileOpen()
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_View_fp->OnFootprintFileImport();
		return;
	}

	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();
	InitializeNewProject();		// set defaults

	// get project file name
	// force old-style file dialog by setting size of OPENFILENAME struct (for Win98)
	CString s ((LPCSTR) IDS_PCBFiles);
	CFileDialog dlg( 1, "fpc", LPCTSTR(m_pcb_filename), 0, 
		s, NULL, /* CPT:  OPENFILENAME_SIZE_VERSION_400 */ 0 );
	dlg.AssertValid();

	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_parent_folder;
	// now show dialog
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		CString pathname = dlg.GetPathName();
		CString filename = dlg.GetFileName();
		if( filename.Right(4) == ".fpl" )
		{
			CString mess ((LPCSTR) IDS_YouAreOpeningAFileWithExtensionFPL); 
			int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
			if( ret == IDCANCEL )
				return;
			else if( ret == IDYES )
				FileLoadLibrary( pathname );
		}
		else
		{
			// read project file
			FileOpen( pathname );
		}
	}
	else
	{
		// CANCEL or error
		DWORD dwError = ::CommDlgExtendedError();
		if( dwError )
		{
			CString str, s ((LPCSTR) IDS_FileOpenDialogErrorCode);
			str.Format( s, (unsigned long)dwError );
			AfxMessageBox( str );
		}
	}
}

void CFreePcbDoc::OnFileAutoOpen( LPCTSTR fn )
{
	CString pathname = fn;
	if( pathname.Right(4) == ".fpl" )
	{
		CString mess ((LPCSTR) IDS_YouAreOpeningAFileWithExtensionFPL);
		int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
		{
			FileLoadLibrary( fn );
			return;
		}
		else
			FileOpen( fn );
			return;
	}
	else
		FileOpen( fn );
}

// open project file, fn = full path to file
// if bLibrary = true, open library file and read footprints
// return TRUE if success
//
BOOL CFreePcbDoc::FileOpen( LPCTSTR fn, BOOL bLibrary )
{
	// if another file open, offer to save before closing
	if( FileClose() == IDCANCEL )
		return FALSE;		// file close cancelled
	
	// reset before opening new project
	m_view->CancelSelection();
	InitializeNewProject();		// set defaults

	// now open it
	CStdioFile pcb_file;
	int err = pcb_file.Open( fn, CFile::modeRead, NULL );
	if( !err )
	{
		// error opening project file
		CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
		mess.Format( s, fn );
		AfxMessageBox( mess );
		return FALSE;
	}

	try
	{
		if( !bLibrary )
		{
			// read project from file
			CString key_str;
			CString in_str;
			CArray<CString> p;

			ReadOptions( &pcb_file );
			m_plist->SetPinAnnularRing( m_annular_ring_pins );
			m_nlist->SetViaAnnularRing( m_annular_ring_vias );
			ReadFootprints( &pcb_file );
			ReadBoardOutline( &pcb_file );
			ReadSolderMaskCutouts( &pcb_file );
			m_plist->ReadParts( &pcb_file );
			m_nlist->ReadNets( &pcb_file, m_read_version );
			m_tlist->ReadTexts( &pcb_file );

			// make path to library folder and index libraries
			if( m_full_lib_dir == "" )
			{
				CString fullpath;
				char full[MAX_PATH];
				fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
				if( fullpath[fullpath.GetLength()-1] == '\\' )	
					fullpath = fullpath.Left(fullpath.GetLength()-1);
				m_full_lib_dir = fullpath;
			}
			MakeLibraryMaps( &m_full_lib_dir );
		}
		else
		{
			// read library as project
			ReadFootprints( &pcb_file, NULL, FALSE );
		}
		m_pcb_full_path = fn;
		int fpl = m_pcb_full_path.GetLength();
		int isep = m_pcb_full_path.ReverseFind( '\\' );
		if( isep == -1 )
			isep = m_pcb_full_path.ReverseFind( ':' );
		if( isep == -1 )
			ASSERT(0);		// unable to parse filename
		m_pcb_filename = m_pcb_full_path.Right( fpl - isep - 1);
		int fnl = m_pcb_filename.GetLength();
		m_path_to_folder = m_pcb_full_path.Left( m_pcb_full_path.GetLength() - fnl - 1 );
		CString s1 ((LPCSTR) IDS_AppName);
		m_window_title = s1 + " - " + m_pcb_filename;
		CWnd* pMain = AfxGetMainWnd();
		pMain->SetWindowText( m_window_title );
		if( m_name == "" )
		{
			m_name = m_pcb_filename;
			if( m_name.Right(4) == ".fpc" )
				m_name = m_name.Left( m_name.GetLength() - 4 );
		}
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_ENABLED ); 
			CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
			submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_GENERATEREPORTFILE, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_DSN_FILE_EXPORT, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_SES_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
			submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
			pMain->DrawMenuBar();
		}
		m_project_open = TRUE;
		theApp.AddMRUFile( &m_pcb_full_path );
		// now set layer visibility
		for( int i=0; i<m_num_layers; i++ )
		{
			m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
			m_dlist->SetLayerVisible( i, m_vis[i] );
		}
		// force redraw of function key text
		m_view->m_cursor_mode = 999;
		m_view->SetCursorMode( CUR_NONE_SELECTED );
		m_view->InvalidateLeftPane();
		m_view->Invalidate( FALSE );
		ProjectModified( FALSE );
		m_view->OnViewAllElements();
		m_auto_elapsed = 0;
		CDC * pDC = m_view->GetDC();
		m_view->OnDraw( pDC );
		m_view->ReleaseDC( pDC );
		m_plist->CheckForProblemFootprints();
		bNoFilesOpened = FALSE;
		return TRUE;
	}
	catch( CString * err_str )
	{
		// parsing error
		AfxMessageBox( *err_str );
		delete err_str;
		CDC * pDC = m_view->GetDC();
		m_view->OnDraw( pDC );
		m_view->ReleaseDC( pDC );
		return FALSE;
	}
}

void CFreePcbDoc::OnFileClose()
{
	FileClose();
}

// return IDCANCEL if closing cancelled by user
//
int CFreePcbDoc::FileClose()
{
	if( m_project_open && m_project_modified )
	{
		CString s ((LPCSTR) IDS_ProjectModifiedSaveIt);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return IDCANCEL;
		else if( ret == IDYES )
			OnFileSave();
	}
	m_view->CancelSelection();

	// destroy existing project
	// delete undo list, partlist, netlist, displaylist, etc.
	m_sm_cutout.RemoveAll();
	m_drelist->Clear();
	ResetUndoState();
	m_board_outline.RemoveAll();
	m_nlist->RemoveAllNets();
	m_plist->RemoveAllParts();
	m_tlist->RemoveAllTexts();
	m_dlist->RemoveAll();
	// clear clipboard
	clip_nlist->RemoveAllNets();
	clip_plist->RemoveAllParts();
	clip_tlist->RemoveAllTexts();
	clip_sm_cutout.RemoveAll();
	clip_board_outline.SetSize(0);

	// delete all shapes from local cache
	POSITION pos = m_footprint_cache_map.GetStartPosition();
	while( pos != NULL )
	{
		void * ptr;
		CShape * shape;
		CString string;
		m_footprint_cache_map.GetNextAssoc( pos, string, ptr );
		shape = (CShape*)ptr;
		delete shape;
	}
	m_footprint_cache_map.RemoveAll();
	CWnd* pMain = AfxGetMainWnd();
	if (pMain != NULL)
	{
		CMenu* pMenu = pMain->GetMenu();
		pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
		submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_GENERATEREPORTFILE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_DSN_FILE_EXPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_SES_FILE_IMPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
		submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		pMain->DrawMenuBar();
	}

	m_view->Invalidate( FALSE );
	m_project_open = FALSE;
	ProjectModified( FALSE );
	m_auto_elapsed = 0;
	// force redraw
	m_view->m_cursor_mode = 999;
	m_view->SetCursorMode( CUR_NONE_SELECTED );
	CString s1 ((LPCSTR) IDS_AppName), s2 ((LPCSTR) IDS_NoProjectOpen);
	m_window_title = s1 + " - " + s2;
	pMain->SetWindowText( m_window_title );
	CDC * pDC = m_view->GetDC();
	m_view->OnDraw( pDC );
	m_view->ReleaseDC( pDC );
	return IDOK;
}

void CFreePcbDoc::OnFileSave() 
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_View_fp->OnFootprintFileSaveAs();
		return;
	}

	PurgeFootprintCache();
	FileSave( &m_path_to_folder, &m_pcb_filename, &m_path_to_folder, &m_pcb_filename );
	ProjectModified( FALSE );
	ResetUndoState();
	bNoFilesOpened = FALSE;
}

// Autosave
//
BOOL CFreePcbDoc::AutoSave()
{
	CString str;
	CString auto_folder = m_path_to_folder + "\\Autosave";
	int len = m_pcb_filename.GetLength();
	CString search_str = m_pcb_filename + ".0??";
	struct _stat s;
	int err = _stat( auto_folder, &s );
	if( err )
	{
		if( err )
		{
			err = _mkdir( auto_folder );
			if( err )
			{
				CString s ((LPCSTR) IDS_UnableToCreateAutosaveFolder);
				str.Format( s, auto_folder );
				AfxMessageBox( str, MB_OK );
				return FALSE;
			}
		}
	}
	CFileFind finder;
	CString list_str = "";
	CTime time;
	time_t bin_time;
	time_t max_time = 0;
	int max_suffix = 0;
	if( _chdir( auto_folder ) != 0 )
	{
		CString mess, s ((LPCSTR) IDS_UnableToOpenAutosaveFolder);
		mess.Format( s, auto_folder );
		AfxMessageBox( mess );
	}
	else
	{
		BOOL bWorking = finder.FindFile( search_str );
		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			CString fn = finder.GetFileName();
			if( !finder.IsDirectory() )
			{
				// found a file
				int suffix = atoi( fn.Right(2) );
				finder.GetLastWriteTime( time );
				bin_time = time.GetTime();
				if( bin_time > max_time )
				{
					max_time = bin_time;
					max_suffix = suffix;
				}
			}
		}
	}
	CString new_file_name;
	int new_suffix = max_suffix%25 + 1;
	new_file_name.Format( "%s.%03d", m_pcb_filename, new_suffix );
	FileSave( &auto_folder, &new_file_name, NULL, NULL, FALSE );
	m_project_modified_since_autosave = FALSE;
	finder.Close();
	return TRUE;
}

// save project file
// make backup if the new file has the same path and filename as the old file
// returns TRUE if successful, FALSE if fails
//
BOOL CFreePcbDoc::FileSave( CString * folder, CString * filename, 
						   CString * old_folder, CString * old_filename,
						   BOOL bBackup ) 
{
	if( !m_project_open )
		return FALSE;

	// write project file
	CString full_path = *folder + "\\" + *filename;
	// see if we need to make a backup file
	if( old_folder != NULL && old_filename != NULL )
	{
		if( bBackup && *folder == *old_folder && *filename == *old_filename )
		{
			// rename file
			CString backup = full_path + ".bak";
			remove( backup );
			rename( full_path, backup );
		}
	}
	CStdioFile pcb_file;
	int err = pcb_file.Open( LPCSTR(full_path), CFile::modeCreate | CFile::modeWrite, NULL );
	if( !err )
	{
		// error opening file
		CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
		mess.Format( s, full_path ); 
		AfxMessageBox( mess );
		return FALSE;
	} 
	else
	{
		// write project to file
		try
		{
			WriteOptions( &pcb_file );
			WriteFootprints( &pcb_file );
			WriteBoardOutline( &pcb_file );
			WriteSolderMaskCutouts( &pcb_file );
			m_plist->WriteParts( &pcb_file );
			m_nlist->WriteNets( &pcb_file );
			m_tlist->WriteTexts( &pcb_file );
			pcb_file.WriteString( "[end]\n" );
			pcb_file.Close();
			theApp.AddMRUFile( &m_pcb_full_path );
			bNoFilesOpened = FALSE;
			m_auto_elapsed = 0;
		}
		catch( CString * err_str )
		{
			// error
			AfxMessageBox( *err_str );
			delete err_str;
			CDC * pDC = m_view->GetDC();
			m_view->OnDraw( pDC ) ;
			m_view->ReleaseDC( pDC );
			return FALSE;
		}
	}
	return TRUE;
}

void CFreePcbDoc::OnFileSaveAs() 
{
	// force old-style file dialog by setting size of OPENFILENAME struct
	CString s ((LPCSTR) IDS_PCBFiles);
	CFileDialog dlg( 0, "fpc", LPCTSTR(m_pcb_filename), 0, 
		s, NULL, 0 /* CPT eliminated, doesn't work in VS 10.0: OPENFILENAME_SIZE_VERSION_400 */ );
	OPENFILENAME  * myOFN = dlg.m_pOFN;
	myOFN->Flags |= OFN_OVERWRITEPROMPT;
	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_parent_folder;
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		// get new filename and folder
		CString new_pathname = dlg.GetPathName();
		CString new_filename = dlg.GetFileName();
		int fnl = new_filename.GetLength();
		CString new_folder = new_pathname.Left( new_pathname.GetLength() - fnl - 1 );
		// write project file
		BOOL ok = FileSave( &new_folder, &new_filename, &m_path_to_folder, &m_pcb_filename );
		if( ok )
		{
			// update member variables, MRU files and window title
			m_pcb_filename = new_filename;
			m_pcb_full_path = new_pathname;
			m_path_to_folder = new_folder;
			theApp.AddMRUFile( &m_pcb_full_path );
			CString s1 ((LPCSTR) IDS_AppName);
			m_window_title = s1 + " - " + m_pcb_filename;
			CWnd* pMain = AfxGetMainWnd();
			pMain->SetWindowText( m_window_title );
			ProjectModified( FALSE );
		}
		else
		{
			CString s ((LPCSTR) IDS_FileSaveFailed);
			AfxMessageBox( s );
		}
	}
}

void CFreePcbDoc::OnAddPart()
{
	// invoke dialog
	CDlgAddPart dlg;
	partlist_info pl;
	m_plist->ExportPartListInfo( &pl, NULL );
	dlg.Initialize( &pl, -1, TRUE, TRUE, FALSE, 0, &m_footprint_cache_map, 
		&m_footlibfoldermap, m_units, m_dlg_log );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// select new part, and start dragging it if requested
		m_plist->ImportPartListInfo( &pl, 0 );
		int n_parts = pl.GetSize();
		cpart * part = m_plist->GetPart( pl[n_parts-1].ref_des );
		ProjectModified( TRUE );
		m_view->SaveUndoInfoForPart( part, 
			CPartList::UNDO_PART_ADD, &part->ref_des, TRUE, m_undo_list );
		m_view->SelectPart( part );
		if( dlg.GetDragFlag() )
		{
			m_view->m_dragging_new_item = TRUE;
			m_view->OnPartMove();
		}
	}
}

void CFreePcbDoc::OnProjectNetlist()
{
	CFreePcbView * view = (CFreePcbView*)m_view;
	CDlgNetlist dlg;
	dlg.Initialize( m_nlist, m_plist, &m_w, &m_v_w, &m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		ResetUndoState();
		m_nlist->ImportNetListInfo( dlg.m_nl, 0, NULL, m_trace_w, m_via_w, m_via_hole_w );
		ProjectModified( TRUE );
		view->CancelSelection();
		if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
			m_nlist->OptimizeConnections();
		view->Invalidate( FALSE );
	}
}

// write footprint info from local cache to file
//
int CFreePcbDoc::WriteFootprints( CStdioFile * file, CMapStringToPtr * cache_map )
{
	CMapStringToPtr * use_map = cache_map;
	if( use_map == NULL )
		use_map = &m_footprint_cache_map;

	void * ptr;
	CShape * s;
	POSITION pos;
	CString key;

	file->WriteString( "[footprints]\n\n" );
	for( pos = use_map->GetStartPosition(); pos != NULL; )
	{
		use_map->GetNextAssoc( pos, key, ptr );
		s = (CShape*)ptr;
		s->WriteFootprint( file );
	}
	return 0;
}

// get shape from cache
// if necessary, make shape from library file and put into cache first
// returns NULL if shape not found
//
CShape * CFreePcbDoc::GetFootprintPtr( CString name )
{
	// lookup shape, first in cache
	void * ptr;
	int err = m_footprint_cache_map.Lookup( name, ptr );
	if( err )
	{
		// found in cache
		return (CShape*)ptr; 
	}
	else
	{
		// not in cache, lookup in library file
		int ilib;
		CString file_name;
		int offset;
		CString * project_lib_folder_str;
		project_lib_folder_str = m_footlibfoldermap.GetDefaultFolder();
		CFootLibFolder * project_footlibfolder = m_footlibfoldermap.GetFolder( project_lib_folder_str, m_dlg_log );
		BOOL ok = project_footlibfolder->GetFootprintInfo( &name, NULL, &ilib, NULL, &file_name, &offset );
		if( !ok )
		{
			// unable to find shape, return NULL
			return NULL;
		}
		else
		{
			// make shape from library file and put into cache
			CShape * shape = new CShape;
			CString lib_name = *project_footlibfolder->GetFullPath();
			err = shape->MakeFromFile( NULL, name, file_name, offset ); 
			if( err )
			{
				// failed
				CString mess, s ((LPCSTR) IDS_UnableToMakeShapeFromFile);
				mess.Format( s, name );
				AfxMessageBox( mess );
				return NULL;
			}
			else
			{
				// success, put into cache and return pointer
				m_footprint_cache_map.SetAt( name, shape );
				ProjectModified( TRUE );
				return shape;
			}
		}
	}
	return NULL;
}

// read shapes from file
//
void CFreePcbDoc::ReadFootprints( CStdioFile * pcb_file, 
								  CMapStringToPtr * cache_map,
								  BOOL bFindSection )
{
	// set up cache map to use
	CMapStringToPtr * use_map = cache_map;
	if( use_map == NULL )
		use_map = &m_footprint_cache_map;

	// find beginning of shapes section
	ULONGLONG pos;
	int err;
	CString key_str;
	CString in_str;
	CArray<CString> p;

	// delete all shapes from local cache
	POSITION mpos = use_map->GetStartPosition();
	while( mpos != NULL )
	{
		void * ptr;
		CShape * shape;
		CString string;
		use_map->GetNextAssoc( mpos, string, ptr );
		shape = (CShape*)ptr;
		delete shape;
	}
	use_map->RemoveAll();

	if( bFindSection )
	{
		// find beginning of shapes section
		do
		{
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				// error reading pcb file
				CString mess ((LPCSTR) IDS_UnableToFindFootprintsSectionInFile);
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str != "[shapes]" && in_str != "[footprints]" );
	}

	// get each shape and add it to the cache
	while( 1 )
	{
		pos = pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			if( bFindSection )
			{
				CString * err_str = new CString( (LPCSTR) IDS_UnexpectedEOFInProjectFile);
				throw err_str;
			}
			else
				break;
		}
		in_str.Trim();
		if( in_str[0] == '[' )
		{
			pcb_file->Seek( pos, CFile::begin );
			break;		// next section, exit
		}
		else if( in_str.Left(5) == "name:" )
		{
			CString name = in_str.Right( in_str.GetLength()-5 );
			name.Trim();
			if( name.Right(1) == '\"' )
				name = name.Left( name.GetLength() - 1 );
			if( name.Left(1) == '\"' )
				name = name.Right( name.GetLength() - 1 );
			name = name.Left( CShape::MAX_NAME_SIZE );
			CShape * s = new CShape;
			pcb_file->Seek( pos, CFile::begin );	// back up
			err = s->MakeFromFile( pcb_file, "", "", 0 );
			if( !err )
				use_map->SetAt( name, s );
			else
				delete s;
		}
	}
}

// write board outline to file
//
// throws CString * exception on error
//
void CFreePcbDoc::WriteBoardOutline( CStdioFile * file, CArray<CPolyLine> * bbd )
{
	CString line;
	CArray<CPolyLine> * bd = bbd;
	if( bd == NULL )
		bd = &m_board_outline;

	try
	{
		line.Format( "[board]\n" );
		file->WriteString( line );
		for( int ib=0; ib<bd->GetSize(); ib++ )
		{
			line.Format( "\noutline: %d %d\n", (*bd)[ib].GetNumCorners(), ib );
			file->WriteString( line );
			for( int icor=0; icor<(*bd)[ib].GetNumCorners(); icor++ )
			{
				line.Format( "  corner: %d %d %d %d\n", icor+1,
					(*bd)[ib].GetX( icor ),
					(*bd)[ib].GetY( icor ),
					(*bd)[ib].GetSideStyle( icor )
					);
				file->WriteString( line );
			}
		}
		file->WriteString( "\n" );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteBoardOutline()\n" + *err_str;
		throw err_str;
	}
}
void CFreePcbDoc::WriteSolderMaskCutouts( CStdioFile * file, CArray<CPolyLine> * sm )
{
	CString line;
	CArray<CPolyLine> * smc = sm;
	if( sm == NULL )
		smc = &m_sm_cutout;

	try
	{
		line.Format( "[solder_mask_cutouts]\n\n" );
		file->WriteString( line );
		for( int i=0; i<smc->GetSize(); i++ )
		{
			line.Format( "sm_cutout: %d %d %d\n", (*smc)[i].GetNumCorners(),
				(*smc)[i].GetHatch(), m_sm_cutout[i].GetLayer() );
			file->WriteString( line );
			for( int icor=0; icor<(*smc)[i].GetNumCorners(); icor++ )
			{
				line.Format( "  corner: %d %d %d %d\n", icor+1,
					(*smc)[i].GetX( icor ),
					(*smc)[i].GetY( icor ),
					(*smc)[i].GetSideStyle( icor )
					);
				file->WriteString( line );
			}
			file->WriteString( "\n" );
		}
		file->WriteString( "\n" );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteSolderMaskCutouts()\n" + *err_str;
		throw err_str;
	}
}

// read board outline from file
//
// throws CString * exception on error
//
void CFreePcbDoc::ReadBoardOutline( CStdioFile * pcb_file, CArray<CPolyLine> * bbd )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;
	int last_side_style = CPolyLine::STRAIGHT;
	CArray<CPolyLine> * bd = bbd;
	if( bd == NULL )
		bd = &m_board_outline;

	try
	{
		// find beginning of [board] section
		do
		{
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				// error reading pcb file
				CString mess ((LPCSTR) IDS_UnableToFindBoardSectionInFile);
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str != "[board]" );

		// get data
		while( 1 )
		{
			pos = pcb_file->GetPosition();
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				throw err_str;
			}
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				return;
			}
			np = ParseKeyString( &in_str, &key_str, &p );
			if( np && key_str == "outline" )
			{
				if( np != 2 && np != 3 )
				{
					CString * err_str = new CString((LPCSTR) IDS_ErrorParsingBoardSectionOfProjectFile);
					throw err_str;
				}
				int ncorners = my_atoi( &p[0] );
				int ib = 0;
				if( np == 3 )
					ib = my_atoi( &p[1] );
				for( int icor=0; icor<ncorners; icor++ )
				{
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || np < 4 )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingBoardSectionOfProjectFile);
						throw err_str;
					}
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingBoardSectionOfProjectFile);
						throw err_str;
					}
					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					if( icor == 0 )
					{
						// make new board outline
						bd->SetSize( ib + 1 );
						if( bbd )
							(*bd)[ib].SetDisplayList( NULL );
						else
							(*bd)[ib].SetDisplayList( m_dlist );
						id bid( ID_BOARD, ID_BOARD_OUTLINE, ib );
						(*bd)[ib].Start( LAY_BOARD_OUTLINE, 1, 20*NM_PER_MIL, x, y, 
							0, &bid, NULL );
					}
					else
						(*bd)[ib].AppendCorner( x, y, last_side_style );
					if( np == 5 )
						last_side_style = my_atoi( &p[3] );
					else
						last_side_style = CPolyLine::STRAIGHT;
					if( icor == (ncorners-1) )
						(*bd)[ib].Close( last_side_style );
				}
			}
		}
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteBoardOutline()\n" + *err_str;
		throw err_str;
	}
}

// read solder mask cutouts from file
//
// throws CString * exception on error
//
void CFreePcbDoc::ReadSolderMaskCutouts( CStdioFile * pcb_file, CArray<CPolyLine> * ssm )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;
	int last_side_style = CPolyLine::STRAIGHT;
	CArray<CPolyLine> * sm = ssm;
	if( sm == NULL )
		sm = &m_sm_cutout;

	try
	{
		// find beginning of [solder_mask_cutouts] section
		int pos = pcb_file->GetPosition();
		do
		{
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				// error reading pcb file
				CString mess ((LPCSTR) IDS_UnableToFindSolderMaskCutoutsSectionInFile);
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str[0] != '[' );

		if( in_str != "[solder_mask_cutouts]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			return;
		}

		// get data
		while( 1 )
		{
			pos = pcb_file->GetPosition();
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				throw err_str;
			}
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				return;
			}
			np = ParseKeyString( &in_str, &key_str, &p );
			if( np && key_str == "sm_cutout" )
			{
				if( np < 4 ) 
				{
					CString * err_str = new CString((LPCSTR) IDS_ErrorParsingSolderMaskCutoutsSectionOfProjectFile);
					throw err_str;
				}
				int ncorners = my_atoi( &p[0] );
				int hatch = my_atoi( &p[1] );
				int lay = my_atoi( &p[2] );
				int ic = sm->GetSize();
				sm->SetSize(ic+1);
				for( int icor=0; icor<ncorners; icor++ )
				{
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || np < 4 )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingSolderMaskCutoutsSectionOfProjectFile);
						throw err_str;
					}
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingSolderMaskCutoutsSectionOfProjectFile);
						throw err_str;
					}
					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					id id_sm( ID_SM_CUTOUT, ID_SM_CUTOUT, ic );
					if( icor == 0 )
					{
						// make new cutout 
						(*sm)[ic].Start( lay, 0, 10*NM_PER_MIL, x, y, hatch, &id_sm, NULL );
						if( ssm )
							(*sm)[ic].SetDisplayList( NULL );
						else
							(*sm)[ic].SetDisplayList( m_dlist );
					}
					else
						(*sm)[ic].AppendCorner( x, y, last_side_style );
					if( np == 5 )
						last_side_style = my_atoi( &p[3] );
					else
						last_side_style = CPolyLine::STRAIGHT;
					if( icor == (ncorners-1) )
						(*sm)[ic].Close( last_side_style );
				}
			}
		}
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::ReadSolderMaskCutouts()\n" + *err_str;
		throw err_str;
	}
}

// read project options from file
//
// throws CString * exception on error
//
void CFreePcbDoc::ReadOptions( CStdioFile * pcb_file )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;

	// initalize
	CFreePcbView * view = (CFreePcbView*)m_view;
	m_visible_grid.SetSize( 0 );
	m_part_grid.SetSize( 0 );
	// CPT: m_routing_grid.SetSize( 0 );
	m_fp_visible_grid.SetSize( 0 );
	m_fp_part_grid.SetSize( 0 );
	m_name = "";
	m_auto_interval = 0;
	m_dr.bCheckUnrouted = FALSE;
	for( int i=0; i<MAX_LAYERS; i++ )
		m_layer_by_file_layer[i] = i;
	m_bSMT_copper_connect = FALSE;
	m_default_glue_w = 25*NM_PER_MIL;
	m_report_flags = 0;

	try
	{
		// find beginning of [options] section
		do
		{
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				// error reading pcb file
				CString mess ((LPCSTR) IDS_UnableToFindOptionsSectionInFile);
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str != "[options]" );

		// get data
		while( 1 )
		{
			pos = pcb_file->GetPosition();
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				throw err_str;
			}
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				break;
			}
			np = ParseKeyString( &in_str, &key_str, &p );
			if( np == 2 && key_str == "project_name" )
			{
				m_name = p[0];
			}
			else if( np == 2 && key_str == "version" )
			{
				m_read_version = my_atof( &p[0] );
			}
			else if( np == 2 && key_str == "file_version" )
			{
				double file_version = my_atof( &p[0] );
				if( file_version > m_version )
				{
					CString mess, s ((LPCSTR) IDS_WarningTheFileVersionIs);
					mess.Format( s, file_version, m_version );
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret == IDCANCEL )
					{
						CString * err_str = new CString((LPCSTR) IDS_ReadingProjectFileFailedCancelledByUser);
						throw err_str;
					}
				}
			}
			else if( np && key_str == "parent_folder" )
			{
				m_parent_folder = p[0];
			}
			else if( np && key_str == "library_folder" )
			{
				m_lib_dir = p[0];
			}
			else if( np && key_str == "full_library_folder" )
			{
				m_full_lib_dir = p[0];
			}
			else if( np && key_str == "CAM_folder" )
			{
				m_cam_full_path = p[0];
			}
			else if( np && key_str == "netlist_file_path" )
			{
				m_netlist_full_path = p[0];
			}
			else if( np && key_str == "ses_file_path" )
			{
				m_ses_full_path = p[0];
			}
			else if( np && key_str == "dsn_flags" )
			{
				m_dsn_flags = my_atoi( &p[0] );
			}
			else if( np && key_str == "dsn_bounds_poly" )
			{
				m_dsn_bounds_poly = my_atoi( &p[0] );
			}
			else if( np && key_str == "dsn_signals_poly" )
			{
				m_dsn_signals_poly = my_atoi( &p[0] );
			}
			else if( np && key_str == "SMT_connect_copper" )
			{
				m_bSMT_copper_connect = my_atoi( &p[0] );
				m_nlist->SetSMTconnect( m_bSMT_copper_connect );
			}
			else if( np && key_str == "default_glue_width" )
			{
				m_default_glue_w = my_atoi( &p[0] );
			}
			else if( np && key_str == "n_copper_layers" )
			{
				m_num_copper_layers = my_atoi( &p[0] );
				m_plist->SetNumCopperLayers( m_num_copper_layers );
				m_nlist->SetNumCopperLayers( m_num_copper_layers );
				m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
			}
			else if( np && key_str == "autosave_interval" )
			{
				m_auto_interval = my_atoi( &p[0] );
			}
			else if( np && key_str == "auto_ratline_disable" )
			{
				m_auto_ratline_disable = my_atoi( &p[0] );
			}

			else if( np && key_str == "auto_ratline_disable_min_pins" )
			{
				m_auto_ratline_min_pins = my_atoi( &p[0] );
			}
			else if( np && key_str == "netlist_import_flags" )
			{
				m_import_flags = my_atoi( &p[0] );
			}
			else if( np && key_str == "units" )
			{
				if( p[0] == "MM" )
					m_units = MM;
				else
					m_units = MIL;
			}
			else if( np && key_str == "visible_grid_spacing" )
			{
				m_visual_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "visible_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_visible_grid.InsertAt( 0, -value );				// CPT: add items in the reverse of the old order
				else
					m_visible_grid.InsertAt( 0, value );
			}
			else if( np && key_str == "placement_grid_spacing" )
			{
				m_part_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "placement_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_part_grid.InsertAt( 0, -value );					// CPT: add items in the reverse of the old order
				else
					m_part_grid.InsertAt( 0, value );
			}
			else if( np && key_str == "routing_grid_spacing" )
			{
				m_routing_grid_spacing = my_atof( &p[0] );
			}
/* CPT:  New system involving prefs dialog and global values stored in registry 
			else if( np && key_str == "routing_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_routing_grid.Add( -value );
				else
					m_routing_grid.Add( value );
			}
*/
			else if( np && key_str == "snap_angle" )
			{
				m_snap_angle = my_atof( &p[0] );
			}
			else if( np && key_str == "fp_visible_grid_spacing" )
			{
				m_fp_visual_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "fp_visible_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_fp_visible_grid.InsertAt( 0, -value );			// CPT:  add items in the reverse of the old order
				else
					m_fp_visible_grid.InsertAt( 0, value );
			}
			else if( np && key_str == "fp_placement_grid_spacing" )
			{
				m_fp_part_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "fp_placement_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_fp_part_grid.InsertAt( 0, -value );				// CPT
				else
					m_fp_part_grid.InsertAt( 0, value );
			}
			else if( np && key_str == "fp_snap_angle" )
			{
				m_fp_snap_angle = my_atof( &p[0] );
			}
			// CAM stuff
			else if( np && key_str == "fill_clearance" )
			{
				m_fill_clearance = my_atoi( &p[0] );
			}
			else if( np && key_str == "mask_clearance" )
			{
				m_mask_clearance = my_atoi( &p[0] );
			}
			else if( np && key_str == "thermal_width" )
			{
				m_thermal_width = my_atoi( &p[0] );
			}
			else if( np && key_str == "min_silkscreen_width" )
			{
				m_min_silkscreen_stroke_wid = my_atoi( &p[0] );
			}
			else if( np && key_str == "pilot_diameter" )
			{
				m_pilot_diameter = my_atoi( &p[0] );
			}
			else if( np && key_str == "board_outline_width" )
			{
				m_outline_width = my_atoi( &p[0] );
			}
			else if( np && key_str == "hole_clearance" )
			{
				m_hole_clearance = my_atoi( &p[0] );
			}
			else if( np && key_str == "annular_ring_for_pins" )
			{
				m_annular_ring_pins = my_atoi( &p[0] );
			}
			else if( np && key_str == "annular_ring_for_vias" )
			{
				m_annular_ring_vias = my_atoi( &p[0] );
			}
			else if( np && key_str == "shrink_paste_mask" )
			{
				m_paste_shrink = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_flags" )
			{
				m_cam_flags = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_layers" )
			{
				m_cam_layers = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_drill_file" )
			{
				m_cam_drill_file = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_units" )
			{
				m_cam_units = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_n_x" )
			{
				m_n_x = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_n_y" )
			{
				m_n_y = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_space_x" )
			{
				m_space_x = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_space_y" )
			{
				m_space_y = my_atoi( &p[0] );
			}
			else if( np && key_str == "report_options" )
			{
				m_report_flags = my_atoi( &p[0] );
			}
			// DRC stuff
			else if( np && key_str == "drc_check_unrouted" )
			{
				m_dr.bCheckUnrouted = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_trace_width" )
			{
				m_dr.trace_width = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_pad_pad" )
			{
				m_dr.pad_pad = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_pad_trace" )
			{
				m_dr.pad_trace = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_trace_trace" )
			{
				m_dr.trace_trace = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_hole_copper" )
			{
				m_dr.hole_copper = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_annular_ring_pins" )
			{
				m_dr.annular_ring_pins = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_annular_ring_vias" )
			{
				m_dr.annular_ring_vias = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_board_edge_copper" )
			{
				m_dr.board_edge_copper = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_board_edge_hole" )
			{
				m_dr.board_edge_hole = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_hole_hole" )
			{
				m_dr.hole_hole = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_copper_copper" )
			{
				m_dr.copper_copper = my_atoi( &p[0] );
			}
			else if( np && key_str == "default_trace_width" )
			{
				m_trace_w = my_atoi( &p[0] );
				m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
			}
			else if( np && key_str == "default_via_pad_width" )
			{
				m_via_w = my_atoi( &p[0] );
				m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
			}
			else if( np && key_str == "default_via_hole_width" )
			{
				m_via_hole_w = my_atoi( &p[0] );
				m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
			}
			else if( np && key_str == "n_width_menu" )
			{
				int n = my_atoi( &p[0] );
				m_w.SetSize( n );
				m_v_w.SetSize( n );
				m_v_h_w.SetSize( n );
				for( int i=0; i<n; i++ )
				{
					pos = pcb_file->GetPosition();
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( np < 5 || key_str != "width_menu_item" )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingOptionsSectionOfProjectFile );
						throw err_str;
					}
					int ig = my_atoi( &p[0] ) - 1;
					if( ig != i )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingOptionsSectionOfProjectFile );
						throw err_str;
					}
					m_w[i] = my_atoi( &p[1] );
					m_v_w[i] = my_atoi( &p[2] );
					m_v_h_w[i] = my_atoi( &p[3] );
				}
			}
			else if( np && key_str == "layer_info" )
			{
				CString file_layer_name = p[0];
				int i = my_atoi( &p[1] );
				int layer = -1;
				for( int il=0; il<MAX_LAYERS; il++ )
				{
					CString layer_string = &layer_str[il][0];
					if( file_layer_name == layer_string )
					{
						SetFileLayerMap( i, il );
						layer = il;
						break;
					}
				}
				if( layer < 0 )
				{
					CString s ((LPCSTR) IDS_WarningLayerNotSupported), mess;
					mess.Format(s, file_layer_name);
					AfxMessageBox( mess );
				}
				else
				{
					m_rgb[layer][0] = my_atoi( &p[2] );
					m_rgb[layer][1] = my_atoi( &p[3] );
					m_rgb[layer][2] = my_atoi( &p[4] );
					m_vis[layer] = my_atoi( &p[5] );
				}
			}
		}
		if( m_fp_visible_grid.GetSize() == 0 )
		{
			m_fp_visual_grid_spacing = m_visual_grid_spacing;
			for( int i=0; i<m_visible_grid.GetSize(); i++ )
				m_fp_visible_grid.Add( m_visible_grid[i] );
		}
		if( m_fp_part_grid.GetSize() == 0 )
		{
			m_fp_part_grid_spacing = m_part_grid_spacing;
			for( int i=0; i<m_part_grid.GetSize(); i++ )
				m_fp_part_grid.Add( m_part_grid[i] );
		}
		if( m_fp_snap_angle != 0 && m_fp_snap_angle != 45 && m_fp_snap_angle != 90 )
			m_fp_snap_angle = m_snap_angle;
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
			m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, m_units );
		m_dlist->SetVisibleGrid( TRUE, m_visual_grid_spacing );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteOptions()\n" + *err_str;
		throw err_str;
	}

}

// write project options to file
//
// throws CString * exception on error
//
void CFreePcbDoc::WriteOptions( CStdioFile * file )
{
	CString line;

	try
	{
		CString str;
		CFreePcbView * view = (CFreePcbView*)m_view;
		line.Format( "[options]\n\n" );
		file->WriteString( line );
		line.Format( "version: %5.3f\n", m_version );
		file->WriteString( line );
		line.Format( "file_version: %5.3f\n", m_file_version );
		file->WriteString( line );
		line.Format( "project_name: \"%s\"\n", m_name );
		file->WriteString( line );
		line.Format( "full_library_folder: \"%s\"\n", m_full_lib_dir );
		file->WriteString( line );
		line.Format( "CAM_folder: \"%s\"\n", m_cam_full_path );
		file->WriteString( line );
		line.Format( "ses_file_path: \"%s\"\n", m_ses_full_path );
		file->WriteString( line );
		line.Format( "netlist_file_path: \"%s\"\n", m_netlist_full_path );
		file->WriteString( line );
		line.Format( "SMT_connect_copper: \"%d\"\n", m_bSMT_copper_connect );
		file->WriteString( line );
		line.Format( "default_glue_width: \"%d\"\n", m_default_glue_w );
		file->WriteString( line );
		line.Format( "dsn_flags: \"%d\"\n", m_dsn_flags );
		file->WriteString( line );
		line.Format( "dsn_bounds_poly: \"%d\"\n", m_dsn_bounds_poly );
		file->WriteString( line );
		line.Format( "dsn_signals_poly: \"%d\"\n", m_dsn_signals_poly );
		file->WriteString( line );
		line.Format( "autosave_interval: %d\n", m_auto_interval );
		file->WriteString( line );
		line.Format( "auto_ratline_disable: \"%d\"\n", m_auto_ratline_disable );
		file->WriteString( line );
		line.Format( "auto_ratline_disable_min_pins: \"%d\"\n", m_auto_ratline_min_pins );
		file->WriteString( line );
		line.Format( "netlist_import_flags: %d\n", m_import_flags );
		file->WriteString( line );
		if( m_units == MIL )
			file->WriteString( "units: MIL\n\n" );
		else
			file->WriteString( "units: MM\n\n" );
		line.Format( "visible_grid_spacing: %f\n", m_visual_grid_spacing );
		file->WriteString( line );
		// CPT: to maintain compatibility with the old file format, output the visible grid vals in reverse (ascending) order.
		for( int i=m_visible_grid.GetSize()-1; i>=0; i-- )
		{
			if( m_visible_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_visible_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_visible_grid[i], MM );
			file->WriteString( "  visible_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "placement_grid_spacing: %f\n", m_part_grid_spacing );
		file->WriteString( line );
		// CPT: output placement grid vals in reverse
		for( int i=m_part_grid.GetSize()-1; i>=0; i-- )
		{
			if( m_part_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_part_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_part_grid[i], MM );
			file->WriteString( "  placement_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "routing_grid_spacing: %f\n", m_routing_grid_spacing );
		file->WriteString( line );
		for( int i=0; i<m_routing_grid.GetSize(); i++ )
		{
			if( m_routing_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_routing_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_routing_grid[i], MM );
			file->WriteString( "  routing_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "snap_angle: %d\n", m_snap_angle );
		file->WriteString( line );
		file->WriteString( "\n" );
		line.Format( "fp_visible_grid_spacing: %f\n", m_fp_visual_grid_spacing );
		file->WriteString( line );
		// CPT: output in reverse order...
		for( int i=m_fp_visible_grid.GetSize()-1; i>=0; i-- )
		{
			if( m_fp_visible_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_fp_visible_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_fp_visible_grid[i], MM );
			file->WriteString( "  fp_visible_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "fp_placement_grid_spacing: %f\n", m_fp_part_grid_spacing );
		file->WriteString( line );
		// CPT: output in reverse order
		for( int i=m_fp_part_grid.GetSize()-1; i>=0; i-- )
		{
			if( m_fp_part_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_fp_part_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_fp_part_grid[i], MM );
			file->WriteString( "  fp_placement_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "fp_snap_angle: %d\n", m_fp_snap_angle );
		file->WriteString( line );
		file->WriteString( "\n" );
		line.Format( "fill_clearance: %d\n", m_fill_clearance );
		file->WriteString( line );
		line.Format( "mask_clearance: %d\n", m_mask_clearance );
		file->WriteString( line );
		line.Format( "thermal_width: %d\n", m_thermal_width );
		file->WriteString( line );
		line.Format( "min_silkscreen_width: %d\n", m_min_silkscreen_stroke_wid );
		file->WriteString( line );
		line.Format( "board_outline_width: %d\n", m_outline_width );
		file->WriteString( line );
		line.Format( "hole_clearance: %d\n", m_hole_clearance );
		file->WriteString( line );
		line.Format( "pilot_diameter: %d\n", m_pilot_diameter );
		file->WriteString( line );
		line.Format( "annular_ring_for_pins: %d\n", m_annular_ring_pins );
		file->WriteString( line );
		line.Format( "annular_ring_for_vias: %d\n", m_annular_ring_vias );
		file->WriteString( line );
		line.Format( "shrink_paste_mask: %d\n", m_paste_shrink );
		file->WriteString( line );
		line.Format( "cam_flags: %d\n", m_cam_flags );
		file->WriteString( line );
		line.Format( "cam_layers: %d\n", m_cam_layers );
		file->WriteString( line );
		line.Format( "cam_drill_file: %d\n", m_cam_drill_file );
		file->WriteString( line );
		line.Format( "cam_units: %d\n", m_cam_units );
		file->WriteString( line );
		line.Format( "cam_n_x: %d\n", m_n_x );
		file->WriteString( line );
		line.Format( "cam_n_y: %d\n", m_n_y );
		file->WriteString( line );
		line.Format( "cam_space_x: %d\n", m_space_x );
		file->WriteString( line );
		line.Format( "cam_space_y: %d\n", m_space_y );
		file->WriteString( line );
		file->WriteString( "\n" );

		line.Format( "report_options: %d\n", m_report_flags );
		file->WriteString( line );

		line.Format( "drc_check_unrouted: %d\n", m_dr.bCheckUnrouted );
		file->WriteString( line );
		line.Format( "drc_trace_width: %d\n", m_dr.trace_width );
		file->WriteString( line );
		line.Format( "drc_pad_pad: %d\n", m_dr.pad_pad );
		file->WriteString( line );
		line.Format( "drc_pad_trace: %d\n", m_dr.pad_trace );
		file->WriteString( line );
		line.Format( "drc_trace_trace: %d\n", m_dr.trace_trace );
		file->WriteString( line );
		line.Format( "drc_hole_copper: %d\n", m_dr.hole_copper );
		file->WriteString( line );
		line.Format( "drc_annular_ring_pins: %d\n", m_dr.annular_ring_pins );
		file->WriteString( line );
		line.Format( "drc_annular_ring_vias: %d\n", m_dr.annular_ring_vias );
		file->WriteString( line );
		line.Format( "drc_board_edge_copper: %d\n", m_dr.board_edge_copper );
		file->WriteString( line );
		line.Format( "drc_board_edge_hole: %d\n", m_dr.board_edge_hole );
		file->WriteString( line );
		line.Format( "drc_hole_hole: %d\n", m_dr.hole_hole );
		file->WriteString( line );
		line.Format( "drc_copper_copper: %d\n", m_dr.copper_copper );
		file->WriteString( line );
		file->WriteString( "\n" );

		line.Format( "default_trace_width: %d\n", m_trace_w );
		file->WriteString( line );
		line.Format( "default_via_pad_width: %d\n", m_via_w );
		file->WriteString( line );
		line.Format( "default_via_hole_width: %d\n", m_via_hole_w );
		file->WriteString( line );
		line.Format( "n_width_menu: %d\n", m_w.GetSize() );
		file->WriteString( line );
		for( int i=0; i<m_w.GetSize(); i++ )
		{
			line.Format( "  width_menu_item: %d %d %d %d\n", i+1, m_w[i], m_v_w[i], m_v_h_w[i]  );
			file->WriteString( line );
		}
		file->WriteString( "\n" );
		line.Format( "n_copper_layers: %d\n", m_num_copper_layers );
		file->WriteString( line );
		for( int i=0; i<(LAY_TOP_COPPER+m_num_copper_layers); i++ )
		{
			line.Format( "  layer_info: \"%s\" %d %d %d %d %d\n",
				&layer_str[i][0], i,
				m_rgb[i][0], m_rgb[i][1], m_rgb[i][2], m_vis[i] );
			file->WriteString( line );
		}
		file->WriteString( "\n" );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			err_str->Format( s, e->m_cause );
		else
			err_str->Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteBoardOutline()\n" + *err_str;
		throw err_str;
	}
}

// set defaults for a new project
//
void CFreePcbDoc::InitializeNewProject()
{
	m_bShowMessageForClearance = TRUE;

	// these are the embedded defaults
	m_name = "";
	m_path_to_folder = "..\\projects\\";
	m_lib_dir = "..\\lib\\" ;
	m_pcb_filename = "";
	m_pcb_full_path = "";
	m_board_outline.RemoveAll();
	m_units = MIL;
	m_num_copper_layers = 4;
	m_plist->SetNumCopperLayers( m_num_copper_layers );
	m_nlist->SetNumCopperLayers( m_num_copper_layers );
	m_nlist->SetSMTconnect( m_bSMT_copper_connect );
	m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
	m_auto_ratline_disable = FALSE;
	m_auto_ratline_min_pins = 100;
	m_auto_interval = 0;
	m_sm_cutout.RemoveAll();

	// set main screen layer colors & visibility
	for( int i=0; i<m_num_layers; i++ )
	{
		m_vis[i] = 1;
		m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
		m_dlist->SetLayerVisible( i, m_vis[i] );
	}
	// now set footprint editor layer colors and visibility
	m_fp_num_layers = NUM_FP_LAYERS;
	for( int i=0; i<m_fp_num_layers; i++ )
	{
		m_fp_vis[i] = 1;
		m_dlist_fp->SetLayerRGB( i, m_fp_rgb[i][0], m_fp_rgb[i][1], m_fp_rgb[i][2] );
		m_dlist_fp->SetLayerVisible( i, 1 );
	}

	/* CPT supplanted by ReadPrefs() below
	// colors for layers
	for( int i=0; i<MAX_LAYERS; i++ )
	{
		m_vis[i] = 0;
		m_rgb[i][0] = 127; 
		m_rgb[i][1] = 127; 
		m_rgb[i][2] = 127;			// default grey
	}
	m_rgb[LAY_SELECTION][0] = 255; 
	m_rgb[LAY_SELECTION][1] = 255; 
	m_rgb[LAY_SELECTION][2] = 255;		//selection WHITE
	m_rgb[LAY_BACKGND][0] = 0; 
	m_rgb[LAY_BACKGND][1] = 0; 
	m_rgb[LAY_BACKGND][2] = 0;			// background BLACK
	m_rgb[LAY_VISIBLE_GRID][0] = 255; 
	m_rgb[LAY_VISIBLE_GRID][1] = 255; 
	m_rgb[LAY_VISIBLE_GRID][2] = 255;	// visible grid WHITE 
	m_rgb[LAY_HILITE][0] = 255; 
	m_rgb[LAY_HILITE][1] = 255; 
	m_rgb[LAY_HILITE][2] = 255;			//highlight WHITE
	m_rgb[LAY_DRC_ERROR][0] = 255; 
	m_rgb[LAY_DRC_ERROR][1] = 128; 
	m_rgb[LAY_DRC_ERROR][2] = 64;		// DRC error ORANGE
	m_rgb[LAY_BOARD_OUTLINE][0] = 0; 
	m_rgb[LAY_BOARD_OUTLINE][1] = 0; 
	m_rgb[LAY_BOARD_OUTLINE][2] = 255;	//board outline BLUE
	m_rgb[LAY_RAT_LINE][0] = 255; 
	m_rgb[LAY_RAT_LINE][1] = 0; 
	m_rgb[LAY_RAT_LINE][2] = 255;		//ratlines VIOLET
	m_rgb[LAY_SILK_TOP][0] = 255; 
	m_rgb[LAY_SILK_TOP][1] = 255; 
	m_rgb[LAY_SILK_TOP][2] =   0;		//top silk YELLOW
	m_rgb[LAY_SILK_BOTTOM][0] = 255; 
	m_rgb[LAY_SILK_BOTTOM][1] = 192; 
	m_rgb[LAY_SILK_BOTTOM][2] = 192;	//bottom silk PINK
	m_rgb[LAY_SM_TOP][0] =   160; 
	m_rgb[LAY_SM_TOP][1] =   160; 
	m_rgb[LAY_SM_TOP][2] =   160;		//top solder mask cutouts LIGHT GREY
	m_rgb[LAY_SM_BOTTOM][0] = 95; 
	m_rgb[LAY_SM_BOTTOM][1] = 95; 
	m_rgb[LAY_SM_BOTTOM][2] = 95;	//bottom solder mask cutouts DARK GREY
	m_rgb[LAY_PAD_THRU][0] =   0; 
	m_rgb[LAY_PAD_THRU][1] =   0; 
	m_rgb[LAY_PAD_THRU][2] = 255;		//thru-hole pads BLUE
	m_rgb[LAY_TOP_COPPER][0] =   0; 
	m_rgb[LAY_TOP_COPPER][1] = 255; 
	m_rgb[LAY_TOP_COPPER][2] =   0;		//top copper GREEN
	m_rgb[LAY_BOTTOM_COPPER][0] = 255; 
	m_rgb[LAY_BOTTOM_COPPER][1] =   0; 
	m_rgb[LAY_BOTTOM_COPPER][2] =   0;	//bottom copper RED
	m_rgb[LAY_BOTTOM_COPPER+1][0] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+1][1] = 128; 
	m_rgb[LAY_BOTTOM_COPPER+1][2] = 64;	
	m_rgb[LAY_BOTTOM_COPPER+2][0] = 128; // inner 1 
	m_rgb[LAY_BOTTOM_COPPER+2][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+2][2] = 64;	
	m_rgb[LAY_BOTTOM_COPPER+3][0] = 64; // inner 2
	m_rgb[LAY_BOTTOM_COPPER+3][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+3][2] = 128;	
	m_rgb[LAY_BOTTOM_COPPER+4][0] = 64; // inner 3
	m_rgb[LAY_BOTTOM_COPPER+4][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+4][2] = 64;	
	m_rgb[LAY_BOTTOM_COPPER+5][0] = 64; // inner 5
	m_rgb[LAY_BOTTOM_COPPER+5][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+5][2] = 64;	
	m_rgb[LAY_BOTTOM_COPPER+6][0] = 64; // inner 6 
	m_rgb[LAY_BOTTOM_COPPER+6][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+6][2] = 64;	

	
/*
	CPT:  new system involving the registry.  See ReadPrefs() below.
	// colors for footprint editor layers
	m_fp_rgb[LAY_FP_SELECTION][0] = 255; 
	m_fp_rgb[LAY_FP_SELECTION][1] = 255; 
	m_fp_rgb[LAY_FP_SELECTION][2] = 255;		//selection WHITE
	m_fp_rgb[LAY_FP_BACKGND][0] = 0; 
	m_fp_rgb[LAY_FP_BACKGND][1] = 0; 
	m_fp_rgb[LAY_FP_BACKGND][2] = 0;			// background BLACK
	m_fp_rgb[LAY_FP_VISIBLE_GRID][0] = 255; 
	m_fp_rgb[LAY_FP_VISIBLE_GRID][1] = 255; 
	m_fp_rgb[LAY_FP_VISIBLE_GRID][2] = 255;	// visible grid WHITE 
	m_fp_rgb[LAY_FP_HILITE][0] = 255; 
	m_fp_rgb[LAY_FP_HILITE][1] = 255; 
	m_fp_rgb[LAY_FP_HILITE][2] = 255;		//highlight WHITE
	m_fp_rgb[LAY_FP_SILK_TOP][0] = 255; 
	m_fp_rgb[LAY_FP_SILK_TOP][1] = 255; 
	m_fp_rgb[LAY_FP_SILK_TOP][2] =   0;		//top silk YELLOW
	m_fp_rgb[LAY_FP_SILK_BOTTOM][0] = 255; 
	m_fp_rgb[LAY_FP_SILK_BOTTOM][1] = 192; 
	m_fp_rgb[LAY_FP_SILK_BOTTOM][2] = 192;	//bottom silk PINK
	m_fp_rgb[LAY_FP_CENTROID][0] = 255; 
	m_fp_rgb[LAY_FP_CENTROID][1] = 255; 
	m_fp_rgb[LAY_FP_CENTROID][2] = 255;		//centroid WHITE
	m_fp_rgb[LAY_FP_DOT][0] = 255; 
	m_fp_rgb[LAY_FP_DOT][1] = 128; 
	m_fp_rgb[LAY_FP_DOT][2] =  64;			//adhesive dot ORANGE
	m_fp_rgb[LAY_FP_PAD_THRU][0] =   0; 
	m_fp_rgb[LAY_FP_PAD_THRU][1] =   0; 
	m_fp_rgb[LAY_FP_PAD_THRU][2] = 255;		//thru-hole pads BLUE
	m_fp_rgb[LAY_FP_TOP_COPPER][0] =   0; 
	m_fp_rgb[LAY_FP_TOP_COPPER][1] = 255; 
	m_fp_rgb[LAY_FP_TOP_COPPER][2] =   0;		//top copper GREEN
	m_fp_rgb[LAY_FP_INNER_COPPER][0] =  128; 
	m_fp_rgb[LAY_FP_INNER_COPPER][1] = 128; 
	m_fp_rgb[LAY_FP_INNER_COPPER][2] =  128;		//inner copper GREY
	m_fp_rgb[LAY_FP_BOTTOM_COPPER][0] = 255; 
	m_fp_rgb[LAY_FP_BOTTOM_COPPER][1] = 0; 
	m_fp_rgb[LAY_FP_BOTTOM_COPPER][2] = 0;		//bottom copper RED
	m_fp_rgb[LAY_FP_TOP_MASK][0] = 0; 
	m_fp_rgb[LAY_FP_TOP_MASK][1] = 127; 
	m_fp_rgb[LAY_FP_TOP_MASK][2] = 0;		//top mask DARK GREEN
	m_fp_rgb[LAY_FP_TOP_PASTE][0] = 0; 
	m_fp_rgb[LAY_FP_TOP_PASTE][1] = 127; 
	m_fp_rgb[LAY_FP_TOP_PASTE][2] = 0;		//top paste DARK GREEN
	m_fp_rgb[LAY_FP_BOTTOM_MASK][0] = 127; 
	m_fp_rgb[LAY_FP_BOTTOM_MASK][1] = 0; 
	m_fp_rgb[LAY_FP_BOTTOM_MASK][2] = 0;		//bottom mask DARK RED
	m_fp_rgb[LAY_FP_BOTTOM_PASTE][0] = 127; 
	m_fp_rgb[LAY_FP_BOTTOM_PASTE][1] = 0; 
	m_fp_rgb[LAY_FP_BOTTOM_PASTE][2] = 0;		//bottom paste DARK RED

	*/

	// default visible grid spacing menu values (in NM)
	// CPT:  streamlined and reversed the orders of these lists
	static const double vis_grid_vals[] =
		{ 1000*NM_PER_MIL, 500*NM_PER_MIL, 400*NM_PER_MIL, 250*NM_PER_MIL, 200*NM_PER_MIL, 125*NM_PER_MIL, 100*NM_PER_MIL };
	m_visible_grid.RemoveAll();
	for (int i=0; i<7; i++)
		m_visible_grid.Add( vis_grid_vals[i] );
//	int visible_index = 2;
	m_visual_grid_spacing = 200*NM_PER_MIL;
	m_dlist->SetVisibleGrid( TRUE, m_visual_grid_spacing );

	// default placement grid spacing menu values (in NM)
	static const double part_grid_vals[] = 
		{ 1000*NM_PER_MIL, 500*NM_PER_MIL, 400*NM_PER_MIL, 250*NM_PER_MIL, 200*NM_PER_MIL, 100*NM_PER_MIL, 
		  50*NM_PER_MIL, 40*NM_PER_MIL, 25*NM_PER_MIL, 20*NM_PER_MIL, 10*NM_PER_MIL };
	m_part_grid.RemoveAll();
	for (int i=0; i<11; i++)
		m_part_grid.Add( part_grid_vals[i] );
	m_part_grid_spacing = 50*NM_PER_MIL;

	// default routing grid spacing menu values (in NM)
/* CPT.  New system involving reading from the registry...
	m_routing_grid.RemoveAll();
	m_routing_grid.Add( 1*NM_PER_MIL );
	m_routing_grid.Add( 2*NM_PER_MIL );
	m_routing_grid.Add( 2.5*NM_PER_MIL );
	m_routing_grid.Add( 3.333333333333*NM_PER_MIL );
	m_routing_grid.Add( 4*NM_PER_MIL );
	m_routing_grid.Add( 5*NM_PER_MIL );
	m_routing_grid.Add( 8.333333333333*NM_PER_MIL );
	m_routing_grid.Add( 10*NM_PER_MIL );	// default
	m_routing_grid.Add( 16.66666666666*NM_PER_MIL );
	m_routing_grid.Add( 20*NM_PER_MIL );
	m_routing_grid.Add( 25*NM_PER_MIL );
	m_routing_grid.Add( 40*NM_PER_MIL );
	m_routing_grid.Add( 50*NM_PER_MIL );
	m_routing_grid.Add( 100*NM_PER_MIL );
	*/

	m_routing_grid_spacing = 10*NM_PER_MIL;

	// footprint editor parameters 
	m_fp_units = MIL;

	// default footprint editor visible grid spacing menu values (in NM)
	static const double fp_vis_grid_vals[] =
		{ 400*NM_PER_MIL, 250*NM_PER_MIL, 200*NM_PER_MIL, 125*NM_PER_MIL, 100*NM_PER_MIL };
	m_fp_visible_grid.RemoveAll();
	for (int i=0; i<5; i++)
		m_fp_visible_grid.Add( fp_vis_grid_vals[i] );
	m_fp_visual_grid_spacing = 200*NM_PER_MIL;

	// default footprint editor placement grid spacing menu values (in NM)  (CPT: same as regular placement grid)
	m_fp_part_grid.RemoveAll();
	for (int i=0; i<11; i++)
		m_fp_part_grid.Add( part_grid_vals[i] );
	m_fp_part_grid_spacing = 50*NM_PER_MIL;

	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
		m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, MIL );

	// default PCB parameters
	m_bSMT_copper_connect = FALSE;
	m_default_glue_w = 25*NM_PER_MIL;
	m_trace_w = 10*NM_PER_MIL;
	m_via_w = 28*NM_PER_MIL;
	m_via_hole_w = 14*NM_PER_MIL;
	m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );

	// default cam parameters
	m_dsn_flags = 0;
	m_dsn_bounds_poly = 0;
	m_dsn_signals_poly = 0;
	m_cam_full_path = "";
	m_ses_full_path = "";
	m_fill_clearance = 10*NM_PER_MIL;
	m_mask_clearance = 8*NM_PER_MIL;
	m_thermal_width = 10*NM_PER_MIL;
	m_min_silkscreen_stroke_wid = 5*NM_PER_MIL;
	m_pilot_diameter = 10*NM_PER_MIL;
	m_cam_flags = GERBER_BOARD_OUTLINE | GERBER_NO_CLEARANCE_SMCUTOUTS;
	m_cam_layers = 0xf00fff;	// default layers
	m_cam_units = MIL;
	m_cam_drill_file = 1;
	m_outline_width = 5*NM_PER_MIL;
	m_hole_clearance = 15*NM_PER_MIL;
	m_annular_ring_pins = 7*NM_PER_MIL;
	m_annular_ring_vias = 5*NM_PER_MIL;
	m_paste_shrink = 0;
	m_n_x = 1;
	m_n_y = 1;
	m_space_x = 0;
	m_space_y = 0;

	// default DRC limits
	m_dr.bCheckUnrouted = FALSE;
	m_dr.trace_width = 10*NM_PER_MIL; 
	m_dr.pad_pad = 10*NM_PER_MIL; 
	m_dr.pad_trace = 10*NM_PER_MIL;
	m_dr.trace_trace = 10*NM_PER_MIL; 
	m_dr.hole_copper = 15*NM_PER_MIL; 
	m_dr.annular_ring_pins = 7*NM_PER_MIL;
	m_dr.annular_ring_vias = 5*NM_PER_MIL;
	m_dr.board_edge_copper = 25*NM_PER_MIL;
	m_dr.board_edge_hole = 25*NM_PER_MIL;
	m_dr.hole_hole = 25*NM_PER_MIL;
	m_dr.copper_copper = 10*NM_PER_MIL;

	// default trace widths (must be in ascending order)
	m_w.SetAtGrow( 0, 6*NM_PER_MIL );
	m_w.SetAtGrow( 1, 8*NM_PER_MIL );
	m_w.SetAtGrow( 2, 10*NM_PER_MIL );
	m_w.SetAtGrow( 3, 12*NM_PER_MIL );
	m_w.SetAtGrow( 4, 15*NM_PER_MIL );
	m_w.SetAtGrow( 5, 20*NM_PER_MIL );
	m_w.SetAtGrow( 6, 25*NM_PER_MIL );

	// default via widths
	m_v_w.SetAtGrow( 0, 24*NM_PER_MIL );
	m_v_w.SetAtGrow( 1, 24*NM_PER_MIL );
	m_v_w.SetAtGrow( 2, 24*NM_PER_MIL );
	m_v_w.SetAtGrow( 3, 24*NM_PER_MIL );
	m_v_w.SetAtGrow( 4, 30*NM_PER_MIL );
	m_v_w.SetAtGrow( 5, 30*NM_PER_MIL );
	m_v_w.SetAtGrow( 6, 40*NM_PER_MIL );

	// default via hole widths
	m_v_h_w.SetAtGrow( 0, 15*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 1, 15*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 2, 15*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 3, 15*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 4, 18*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 5, 18*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 6, 20*NM_PER_MIL );

	// netlist import options
	m_netlist_full_path = "";
	m_import_flags = IMPORT_PARTS | IMPORT_NETS | KEEP_TRACES | KEEP_STUBS | KEEP_AREAS;

	CFreePcbView * view = (CFreePcbView*)m_view;
	view->InitializeView();

	// now try to find global options file
	CString fn = m_app_dir + "\\" + "default.cfg";
	CStdioFile file;
	if( !file.Open( fn, CFile::modeRead | CFile::typeText ) )
	{
		CString s ((LPCSTR) IDS_UnableToOpenGlobalConfigurationFile);
		AfxMessageBox( s );
	}
	else
	{
		try
		{
			// read global default file options
			ReadOptions( &file );
			// make path to library folder and index libraries
			char full[_MAX_PATH];
			CString fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
			if( fullpath[fullpath.GetLength()-1] == '\\' )	
				fullpath = fullpath.Left(fullpath.GetLength()-1);
			m_full_lib_dir = fullpath;
		}
		catch( CString * err_str )
		{
			*err_str = "CFreePcbDoc::InitializeNewProject()\n" + *err_str;
			throw err_str;
		}
	}
	m_plist->SetPinAnnularRing( m_annular_ring_pins );
	m_nlist->SetViaAnnularRing( m_annular_ring_vias );
}

// Call this function when the project is modified,
// or to clear the modified flags
//
void CFreePcbDoc::ProjectModified( BOOL flag, BOOL b_clear_redo )
{
	if( flag )
	{
		// set modified flag
		if( b_clear_redo && m_redo_list->m_num_items > 0 )
		{
			// can't redo after a new operation
			m_redo_list->Clear();
		}
		if( m_project_modified )
		{
			// flag already set
			m_project_modified_since_autosave = TRUE;
		}
		else
		{
			// set flags
			CWnd* pMain = AfxGetMainWnd();
			m_project_modified = TRUE;
			m_project_modified_since_autosave = TRUE;
			m_window_title = m_window_title + "*";
			pMain->SetWindowText( m_window_title );
		}
	}
	else
	{
		// clear flags
		m_project_modified = FALSE;
		m_project_modified_since_autosave = FALSE;
		int len = m_window_title.GetLength();
		if( len > 1 && m_window_title[len-1] == '*' )
		{
			CWnd* pMain = AfxGetMainWnd();
			m_window_title = m_window_title.Left(len-1);
			pMain->SetWindowText( m_window_title );
		}
		m_undo_list->Clear();
		m_redo_list->Clear();
	}
	// enable/disable menu items
	CWnd* pMain = AfxGetMainWnd();
	pMain->SetWindowText( m_window_title );
	CMenu* pMenu = pMain->GetMenu();
	CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
	if( m_undo_list->m_num_items == 0 )
		submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	else
		submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_ENABLED );	
	if( m_redo_list->m_num_items == 0 )
		submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	else
		submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_ENABLED );	
	pMain->DrawMenuBar();
}

// CPT:  reworked the Layers dialog
void CFreePcbDoc::OnViewLayers()
{
	CDlgLayers dlg;
	CFreePcbView * view = (CFreePcbView*)m_view;
	dlg.Initialize( m_num_layers, m_vis, m_fp_vis, m_rgb, m_fp_rgb );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// CPT: For simplicity moved the following out of the DlgLayers::DoDataExchange() code
		for( int i=0; i<NUM_MAIN_LAYERS; i++ )
		{
			m_rgb[i][0] = dlg.m_rgb[i][0];
			m_rgb[i][1] = dlg.m_rgb[i][1];
			m_rgb[i][2] = dlg.m_rgb[i][2];
		}
		// Set footprint layer colors & visibility.  Some of these are borrowed from the main screen layer values; others are based on the last
		// four rows of the layer-dialog
		static int fpLayerMap[] = {	LAY_SELECTION, LAY_BACKGND, LAY_VISIBLE_GRID, LAY_HILITE, LAY_SILK_TOP, -1, -1, LAY_PAD_THRU, 
			-1, -1, -1, -1, LAY_TOP_COPPER, LAY_BOTTOM_COPPER+1, LAY_BOTTOM_COPPER };
		for (int i=0; i<NUM_FP_LAYERS; i++) {
			int mainLayer = fpLayerMap[i];
			if (mainLayer!=-1)
				m_fp_rgb[i][0] = m_rgb[mainLayer][0],
				m_fp_rgb[i][1] = m_rgb[mainLayer][1],
				m_fp_rgb[i][2] = m_rgb[mainLayer][2],
				m_fp_vis[i] = m_vis[mainLayer];
			}
		static int fpLayers[] = { LAY_FP_CENTROID, LAY_FP_DOT, LAY_FP_TOP_MASK, LAY_FP_BOTTOM_MASK };
		for (int i=0; i<4; i++) {
			int layer = fpLayers[i];
			m_fp_rgb[layer][0] = dlg.m_rgb[i+NUM_MAIN_LAYERS][0];
			m_fp_rgb[layer][1] = dlg.m_rgb[i+NUM_MAIN_LAYERS][1];
			m_fp_rgb[layer][2] = dlg.m_rgb[i+NUM_MAIN_LAYERS][2];
			}

		// The footprint top-paste and top-mask colors are always the same on exit.  Likewise for visibility:
		m_fp_rgb[LAY_FP_TOP_PASTE][0] = m_fp_rgb[LAY_FP_TOP_MASK][0];
		m_fp_rgb[LAY_FP_TOP_PASTE][1] = m_fp_rgb[LAY_FP_TOP_MASK][1];
		m_fp_rgb[LAY_FP_TOP_PASTE][2] = m_fp_rgb[LAY_FP_TOP_MASK][2];
		m_fp_rgb[LAY_FP_BOTTOM_PASTE][0] = m_fp_rgb[LAY_FP_BOTTOM_MASK][0];
		m_fp_rgb[LAY_FP_BOTTOM_PASTE][1] = m_fp_rgb[LAY_FP_BOTTOM_MASK][1];
		m_fp_rgb[LAY_FP_BOTTOM_PASTE][2] = m_fp_rgb[LAY_FP_BOTTOM_MASK][2];
		m_fp_vis[LAY_FP_TOP_PASTE] = m_fp_vis[LAY_FP_TOP_MASK];
		m_fp_vis[LAY_FP_BOTTOM_PASTE] = m_fp_vis[LAY_FP_BOTTOM_MASK];

		if (dlg.fColorsDefault) {		
			// User wants to apply settings to future new projects
			HKEY hkey;
			char buf[16], buf2[16];
			if (RegCreateKey(HKEY_CURRENT_USER, "Software\\FreePCB", &hkey) != ERROR_SUCCESS) return;
			for( int i=0; i<MAX_LAYERS; i++ ) {
				sprintf(buf, "layerColor%d", i);
				sprintf(buf2, "%d", (m_rgb[i][0]<<16) | (m_rgb[i][1]<<8) | m_rgb[i][2]);
				RegSetValue(hkey, buf, REG_SZ, buf2, strlen(buf2));
				}
			for( int i=0; i<NUM_FP_LAYERS; i++ ) {
				sprintf(buf, "fpLayerColor%d", i);
				sprintf(buf2, "%d", (m_fp_rgb[i][0]<<16) | (m_fp_rgb[i][1]<<8) | m_fp_rgb[i][2]);
				RegSetValue(hkey, buf, REG_SZ, buf2, strlen(buf2));
				}
			RegCloseKey(hkey);
			}

		for( int i=0; i<m_num_layers; i++ )
		{
			m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
			m_dlist->SetLayerVisible( i, m_vis[i] );
		}
		view->m_left_pane_invalid = TRUE;	// force erase of left pane
		view->CancelSelection();
		ProjectModified( TRUE );
		view->Invalidate( FALSE );
	}
}

void CFreePcbDoc::OnProjectPartlist()
{
	CDlgPartlist dlg;
	dlg.Initialize( m_plist, &m_footprint_cache_map, &m_footlibfoldermap, 
		m_units, m_dlg_log );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		ResetUndoState();
		CFreePcbView * view = (CFreePcbView*)m_view;
		view->CancelSelection();
		if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
			m_nlist->OptimizeConnections();
		ProjectModified( TRUE );
		view->Invalidate( FALSE );
	}
}

void CFreePcbDoc::OnPartProperties()
{
	partlist_info pl;
	int ip = m_plist->ExportPartListInfo( &pl, m_view->m_sel_part );
	CDlgAddPart dlg;
	dlg.Initialize( &pl, ip, TRUE, FALSE, FALSE, 0, &m_footprint_cache_map, 
		&m_footlibfoldermap, m_units, m_dlg_log );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// note: part must be selected in view
		cpart * part = m_view->m_sel_part;
		CShape * old_shape = part->shape;
		CString old_ref_des = part->ref_des;
		// see if ref_des has changed
		CString new_ref_des = pl[ip].ref_des;
		m_view->SaveUndoInfoForPartAndNets( part, 
			CPartList::UNDO_PART_MODIFY, &new_ref_des, TRUE, m_undo_list );
		m_plist->ImportPartListInfo( &pl, 0 );
		m_view->SelectPart( part );
		if( dlg.GetDragFlag() )
			ASSERT(0);	// not allowed
		else
		{
			if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
				m_nlist->OptimizeConnections();
			m_view->Invalidate( FALSE );
			ProjectModified( TRUE );
		}
	}
}

void CFreePcbDoc::OnFileExport()
{
	CString s ((LPCSTR) IDS_AllFiles);
	// CPT:  since MS broke CFileDialog::SetTemplate(), I had to rewrite this (as in OnFileImport(), qv)
	CFileDialog dlg( FALSE, NULL, NULL, 
		OFN_HIDEREADONLY | OFN_EXPLORER | OFN_OVERWRITEPROMPT, 
		s, NULL, 0 /* CPT... OPENFILENAME_SIZE_VERSION_400 */ );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString str = dlg.GetPathName();
		CStdioFile file;
		if( !file.Open( str, CFile::modeWrite | CFile::modeCreate ) )
		{
			CString s ((LPCSTR) IDS_UnableToOpenFile2);
			AfxMessageBox( s );
		}
		else
		{
			CDlgExportOptions dlg2;
			dlg2.Initialize(EXPORT_PARTS | EXPORT_NETS); 
			ret = dlg2.DoModal();
			if (ret==IDCANCEL) return;
			partlist_info pl;
			netlist_info nl;
			m_plist->ExportPartListInfo( &pl, NULL );
			m_nlist->ExportNetListInfo( &nl );
			if( dlg2.m_format == CMyFileDialog::PADSPCB )
				ExportPADSPCBNetlist( &file, dlg2.m_select, &pl, &nl );
			else
				ASSERT(0);
			file.Close();
		}
	}
}
void CFreePcbDoc::OnFileImport()
{
	CString s ((LPCSTR) IDS_AllFiles), s2 ((LPCSTR) IDS_ImportNetListFile);
	// CPT:  Under Vista, good old MS has broken the CFileDialog::SetTemplate function, which this code formerly relied on.
	// It's not at all clear to me how one's supposed to write code that
	// works under XP and Vista/Win 7 simultaneously...  So screw it, I've rewritten this to put the options that were formerly in the
	// open-file dialog into the import-options dialog
	CFileDialog dlg( TRUE, NULL, (LPCTSTR)m_netlist_full_path, OFN_HIDEREADONLY | OFN_EXPLORER, 
		s, NULL, 0 /* CPT eliminated (doesn't work under VS10.0): OPENFILENAME_SIZE_VERSION_400 */ );
	dlg.m_ofn.lpstrTitle = s2;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{ 
		CString str = dlg.GetPathName(); 
		CStdioFile file;
		if( !file.Open( str, CFile::modeRead ) )
		{
			CString s ((LPCSTR) IDS_UnableToOpenFile2);
			AfxMessageBox( s );
		}
		else
		{
			ResetUndoState();	
			partlist_info pl;
			netlist_info nl;
			m_netlist_full_path = str;	// save path for next time
			// CPT: Do the option dlg even if there are no parts and/or nets in project 
			CDlgImportOptions dlg_options;
			dlg_options.Initialize( m_import_flags );
			int ret = dlg_options.DoModal();
			if( ret == IDCANCEL )
				return;
			else
				m_import_flags = IMPORT_FROM_NETLIST_FILE | dlg_options.m_flags;
			if( m_import_flags & SAVE_BEFORE_IMPORT )
			{
				// save project
				OnFileSave();
			}
			// show log dialog
			m_dlg_log->ShowWindow( SW_SHOW );
			m_dlg_log->UpdateWindow();
			m_dlg_log->BringWindowToTop();
			m_dlg_log->Clear();
			m_dlg_log->UpdateWindow();

			// import the netlist file
			CString line;
			if( dlg_options.m_format == CMyFileDialog::PADSPCB )
			{
				CString s ((LPCSTR) IDS_ReadingNetlistFile);
				line.Format( s, str ); 
				m_dlg_log->AddLine( line );
				int err = ImportPADSPCBNetlist( &file, m_import_flags, &pl, &nl );
				if( err == NOT_PADSPCB_FILE )
				{
					m_dlg_log->ShowWindow( SW_HIDE );
					CString mess ((LPCSTR) IDS_WarningThisDoesNotAppearToBeALegalPADSPCB);
					int ret = AfxMessageBox( mess, MB_OK );
					return;
				}
			}
			else
				ASSERT(0);
			if( m_import_flags & IMPORT_PARTS )
			{
				line.LoadStringA(IDS_ImportingPartsIntoProject);
				m_dlg_log->AddLine( line );
				m_plist->ImportPartListInfo( &pl, m_import_flags, m_dlg_log );
			}
			if( m_import_flags & IMPORT_NETS )
			{
				line.LoadStringA(IDS_ImportingNetsIntoProject);
				m_dlg_log->AddLine( line );
				CNetList * old_nlist = new CNetList( NULL, m_plist ); 
				old_nlist->Copy( m_nlist );
				m_nlist->ImportNetListInfo( &nl, m_import_flags, m_dlg_log, 0, 0, 0 );
				line.LoadStringA(IDS_MovingTracesAndCopperAreas);
				m_dlg_log->AddLine( line );
				m_nlist->RestoreConnectionsAndAreas( old_nlist, m_import_flags, m_dlg_log );
				delete old_nlist;
				// rehook all parts to nets after destroying old_nlist
				CIterator_cnet iter_net(m_nlist);
				cnet * net = iter_net.GetFirst();
				for( cnet * net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
					m_nlist->RehookPartsToNet( net );
			}
			// clean up
			CString str = "\r\n";
			m_nlist->CleanUpAllConnections( &str );
			m_dlg_log->AddLine( str );
			line.LoadStringA(IDS_Done);
			m_dlg_log->AddLine( line );
			// finish up
			m_nlist->OptimizeConnections( FALSE );
			m_view->OnViewAllElements();
			ProjectModified( TRUE );
			m_view->Invalidate( FALSE );
			// make sure log is visible
			m_dlg_log->ShowWindow( SW_SHOW );
			m_dlg_log->UpdateWindow();
			m_dlg_log->BringWindowToTop();
		}
	}
}

int GetSessionLayer( CString * ses_str )
{
	if( *ses_str == "Top" )
		return LAY_TOP_COPPER;
	else if( *ses_str == "Bottom" )
		return LAY_BOTTOM_COPPER;
	else if( ses_str->Left(6) == "Inner_" )
	{
		return( LAY_BOTTOM_COPPER + my_atoi( &(ses_str->Right(1)) ) );
	}
	return -1;
}

// import session file from autorouter
//
void CFreePcbDoc::ImportSessionFile( CString * filepath, CDlgLog * log, BOOL bVerbose )
{
	// process session file
	enum STATE {	// state machine, sub-states indented
	IDLE,
	  PLACEMENT,
	    COMPONENT,
	  ROUTES,
	    PARSER,
	    LIBRARY_OUT,
	      PADSTACK,
	        SHAPE,
	    NETWORK_OUT,
	      NET,
	        VIA,
	        WIRE,
	          PATH
	};
	#define ENDSTATE (field[0] == ")")

	CStdioFile file;
	if( !file.Open( *filepath, CFile::modeRead ) )
	{
		CString mess, s ((LPCSTR) IDS_UnableToOpenSessionFile);
		mess.Format( s, filepath );
		if( log )
			log->AddLine( mess + "\r\n" );
		else
			AfxMessageBox( mess );
		return;
	}
	CArray<CString> field;
	CString instr, units_str, mult_str, footprint_name;
	int mult = 254; // default = 0.01 mil in nm.
	CString net_name, layer_str, width_str, via_name, via_x_str, via_y_str;
	BOOL bNewViaName = FALSE;
	CArray<cnode> nodes;	// array of nodes in net
	CArray<cpath> paths;	// array of paths in net
	CMapStringToPtr via_map;
	STATE state = IDLE;
	while( file.ReadString( instr ) )
	{
		instr.Trim();
		int nf = ParseStringFields( &instr, &field );
		if( nf )
		{
			// IDLE
			if( state == IDLE )
			{
				if( field[0] == "(placement" )
					state = PLACEMENT;
				else if( field[0] == "(routes" )
					state = ROUTES;
			}
			// IDLE -> PLACEMENT
			else if( state == PLACEMENT )
			{
				if( ENDSTATE )
					state = IDLE;
				else if( field[0] == "(component" )
				{
					state = COMPONENT;
					footprint_name = field[1];
				}
				else if( field[0] == "(resolution" )
				{
					units_str = field[1];
					mult_str = field[2];
					if( units_str == "mil" )
					{
						mult = my_atoi( &mult_str );
						mult = 25400/mult;
					}
					else
						ASSERT(0);
				}
			}
			// IDLE -> PLACEMENT -> COMPONENT
			else if( state == COMPONENT )
			{
				if( ENDSTATE )
					state = PLACEMENT;
				else if( field[0] == "(place" )
				{
				}
			}
			// IDLE -> ROUTES
			else if( state == ROUTES )
			{
				if( ENDSTATE )
					state = IDLE;
				else if( field[0] == "(resolution" )
				{
					units_str = field[1];
					mult_str = field[2];
					if( units_str == "mil" )
					{
						mult = my_atoi( &mult_str );
						mult = 25400/mult;
					}
					else
						ASSERT(0);
				}
				else if( field[0] == "(parser" )
					state = PARSER;
				else if( field[0] == "(library_out" )
					state = LIBRARY_OUT;
				else if( field[0] == "(network_out" )
					state = NETWORK_OUT;
			}
			// IDLE -> ROUTES -> PARSER
			else if( state == PARSER )
			{
				if( ENDSTATE )
					state = ROUTES;
			}
			// IDLE -> ROUTES -> LIBRARY_OUT
			else if( state == LIBRARY_OUT )
			{
				if( ENDSTATE )
					state = ROUTES;
				else if( field[0] == "(padstack" )
				{
					state = PADSTACK;
					via_name = field[1];
					bNewViaName = TRUE;
				}
			}
			// IDLE -> ROUTES -> LIBRARY_OUT -> PADSTACK
			else if( state == PADSTACK )
			{
				if( ENDSTATE )
					state = LIBRARY_OUT;
				else if( field[0] == "(shape" )
					state = SHAPE;
			}
			// IDLE -> ROUTES -> LIBRARY_OUT -> PADSTACK -> SHAPE
			else if( state == SHAPE )
			{
				if( ENDSTATE )
					state = PADSTACK;
				else if( field[0] == "(circle" && bNewViaName )
				{
					// add via definition to via_map
					CString via_w_str = field[2];
					int via_w = mult * my_atoi( &via_w_str );
					via_map.SetAt( via_name, (void*)via_w );
					bNewViaName = FALSE;
				}
			}
			// IDLE -> ROUTES -> NETWORK_OUT
			else if( state == NETWORK_OUT )
			{
				if( ENDSTATE )
					state = ROUTES;
				else if( field[0] == "(net" )
				{
					state = NET;
					net_name = field[1];
					nodes.SetSize(0);
					paths.SetSize(0);
				}
			}
			// IDLE -> ROUTES -> NETWORK_OUT -> NET
			else if( state == NET )
			{
				if( ENDSTATE )
				{
					// end of data for this net, route project
					m_nlist->ImportNetRouting( &net_name, &nodes, &paths, mult, log, bVerbose );
					state = NETWORK_OUT;
				}
				else if( field[0] == "(via" )
				{
					// data for a via
					state = VIA;
					via_name = field[1];
					void * ptr;
					BOOL bOK = via_map.Lookup( via_name, ptr );
					if( bOK )
					{
						via_x_str = field[2];
						via_y_str = field[3];
						int inode = nodes.GetSize();
						nodes.SetSize(inode+1);
						nodes[inode].type = NVIA;
						nodes[inode].x = mult * my_atoi( &via_x_str );
						nodes[inode].y = mult * my_atoi( &via_y_str );
						nodes[inode].layer = LAY_PAD_THRU;
						nodes[inode].via_w = (int)ptr;
						nodes[inode].bUsed = FALSE;
					}
					else
						ASSERT(0);
				}
				else if( field[0] == "(wire" )
				{
					state = WIRE;
				}
			}
			// IDLE -> ROUTES -> NETWORK_OUT -> NET -> VIA
			else if( state == VIA )
			{
				if( ENDSTATE )
					state = NET;
			}
			// IDLE -> ROUTES -> NETWORK_OUT -> NET -> WIRE
			else if( state == WIRE )
			{
				if( ENDSTATE )
					state = NET;
				else if( field[0] == "(path" )
				{
					// path data
					state = PATH;
					layer_str = field[1];
					width_str = field[2];
					int ipath = paths.GetSize();
					paths.SetSize( ipath+1 );
					paths[ipath].layer = GetSessionLayer( &layer_str );
					paths[ipath].width = mult * my_atoi( &width_str );
					paths[ipath].n_used = 0;
				}
			}
			// IDLE -> ROUTES -> NETWORK_OUT -> NET -> WIRE -> PATH
			else if( state == PATH )
			{
				if( ENDSTATE )
					state = WIRE;
				else
				{
					// path point data
					CString x_str = field[0];
					CString y_str = field[1];
					int ipath = paths.GetSize()-1;
					int ipt = paths[ipath].pt.GetSize();
					paths[ipath].pt.SetSize( ipt+1 );
					paths[ipath].pt[ipt].x = mult * my_atoi( &x_str );;
					paths[ipath].pt[ipt].y = mult * my_atoi( &y_str );;
					paths[ipath].pt[ipt].inode = -1;
				}
			}
		}
	}
	file.Close();
}

// import netlist 
// enter with file already open
//
int CFreePcbDoc::ImportNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pl, netlist_info * nl )
{
	CString instr;
	int err_flags = 0;
	int line = 0;
	BOOL not_eof;

	// find next section
	not_eof = file->ReadString( instr );
	line++;
	instr.Trim();
	while( 1 )
	{
		if( instr == "[parts]" && pl && (flags & IMPORT_PARTS) )
		{
			// read parts
			int ipart = 0;
			BOOL parts_section = FALSE;
			while( 1 )
			{
				not_eof = file->ReadString( instr );
				if( !not_eof )
				{
					// end of file
					file->Close();
					return err_flags;
				}
				line++;
				instr.Trim();
				if( instr[0] == '[' )
				{
					// next section
					break;
				}
				else if( instr.GetLength() && instr[0] != '/' )
				{
					// get ref prefix, ref number and shape
					pl->SetSize( ipart+1 );
					CString ref_str( mystrtok( instr, " \t" ) );
					CString shape_str( mystrtok( NULL, "\n\r" ) );
					shape_str.Trim();
					// find footprint, get from library if necessary
					CShape * s = GetFootprintPtr( shape_str );
					// add part to partlist_info
					(*pl)[ipart].part = NULL;
					(*pl)[ipart].ref_des = ref_str;
					if( s )
					{
						(*pl)[ipart].ref_size = s->m_ref_size;
						(*pl)[ipart].ref_width = s->m_ref_w;
					}
					else
					{
						(*pl)[ipart].ref_size = 0;
						(*pl)[ipart].ref_width = 0;
					}
					(*pl)[ipart].package = shape_str;
					(*pl)[ipart].shape = s;
					(*pl)[ipart].x = 0;
					(*pl)[ipart].y = 0;
					(*pl)[ipart].angle = 0;
					(*pl)[ipart].side = 0;
					if( !s )
						err_flags |= FOOTPRINTS_NOT_FOUND;
					ipart++;
				}
			}
		}
		else if( instr == "[nets]" && nl && (flags & IMPORT_NETS) )
		{
			// read nets
			cnet * net = 0;
			int num_pins = 0;
			while( 1 )
			{
				not_eof = file->ReadString( instr );
				if( !not_eof )
				{
					// end of file
					file->Close();
					return err_flags;
				}
				line++;
				instr.Trim();
				if( instr[0] == '[' )
				{
					// next section
					break;
				}
				else if( instr.GetLength() && instr[0] != '/' )
				{
					int delim_pos;
					if( (delim_pos = instr.Find( ":", 0 )) != -1 )
					{
						// new net, get net name
						int inet = nl->GetSize();
						nl->SetSize( inet+1 );
						CString net_name( mystrtok( instr, ":" ) );
						net_name.Trim();
						if( net_name.GetLength() )
						{
							// add new net
							(*nl)[inet].name = net_name;
							(*nl)[inet].net = NULL;
							(*nl)[inet].modified = TRUE;
							(*nl)[inet].deleted = FALSE;
							(*nl)[inet].visible = TRUE;
							(*nl)[inet].w = 0;
							(*nl)[inet].v_w = 0;
							(*nl)[inet].v_h_w = 0;
							instr = instr.Right( instr.GetLength()-delim_pos-1 );
							num_pins = 0;
						}
						// add pins to net
						char * pin = mystrtok( instr, " \t\n\r" );
						while( pin )
						{
							CString pin_cstr( pin );
							if( pin_cstr.GetLength() > 3 )
							{
								int dot = pin_cstr.Find( ".", 0 );
								if( dot )
								{
									CString ref_des = pin_cstr.Left( dot );
									CString pin_num_cstr = pin_cstr.Right( pin_cstr.GetLength()-dot-1 );
									(*nl)[inet].ref_des.Add( ref_des );
									(*nl)[inet].pin_name.Add( pin_num_cstr );
#if 0	// TODO: check for illegal pin names
									}
									else
									{
										// illegal pin number for part
										CString mess;
										mess.Format( "Error in line %d of netlist file\nIllegal pin number \"%s\"", 
											line, pin_cstr );
										AfxMessageBox( mess );
										break;
									}
#endif
								}
								else
								{
									// illegal string
									break;
								}
							}
							else if( pin_cstr != "" )
							{
								// illegal pin identifier
								CString mess, s ((LPCSTR) IDS_ErrorInLineOfNetlistFile);
								mess.Format( s,	line, pin_cstr );
								AfxMessageBox( mess );
							}
							pin = mystrtok( NULL, " \t\n\r" );
						} // end while( pin )
					}
				}
			}
		}
		else if( instr == "[end]" )
		{
			// normal return
			file->Close();
			return err_flags;
		}
		else
		{
			not_eof = file->ReadString( instr );
			line++;
			instr.Trim();
			if( !not_eof)
			{
				// end of file
				file->Close();
				return err_flags;
			}
		}
	}
}

// export netlist in PADS-PCB format
// enter with file already open
// flags:
//	EXPORT_PARTS = include parts in file						// CPT:  fixed comment text
//	EXPORT_NETS = include nets in file
//	EXPORT_VALUES = use "value@footprint" format for parts
//
int CFreePcbDoc::ExportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pl, netlist_info * nl )
{
	CString str, str2;

	file->WriteString( "*PADS-PCB*\n" );
	if( flags & EXPORT_PARTS )
	{
		file->WriteString( "*PART*\n" );
		for( int i=0; i<pl->GetSize(); i++ )
		{
			part_info * pi = &(*pl)[i];
			str2 = "";
			if( flags & EXPORT_VALUES && pi->value != "" )
				str2 = pi->value + "@";
			if( pi->shape )
				str2 += pi->shape->m_name;
			else
				str2 += pi->package;
			str.Format( "%s %s\n", pi->ref_des, str2 );
			file->WriteString( str );
		}
	}

	if( flags & EXPORT_NETS )
	{
		if( flags & IMPORT_PARTS )
			file->WriteString( "\n" );
		file->WriteString( "*NET*\n" );
		for( int i=0; i<nl->GetSize(); i++ )
		{
			net_info * ni = &(*nl)[i];
			str.Format( "*SIGNAL* %s\n", ni->name );
			file->WriteString( str );
			str = "";
			int np = ni->pin_name.GetSize();
			for( int ip=0; ip<np; ip++ )
			{
				CString pin_str;
				pin_str.Format( "%s.%s ", ni->ref_des[ip], ni->pin_name[ip] );
				str += pin_str;
				if( !((ip+1)%8) || ip==(np-1) )
				{
					str += "\n";
					file->WriteString( str );
					str = "";
				}
			}
		}
	}
	file->WriteString( "*END*\n" );
	return 0;
}

// import netlist in PADS-PCB format
// enter with file already open
//
int CFreePcbDoc::ImportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pl, netlist_info * nl )
{
	CString instr, net_name, mess;
	CMapStringToPtr part_map, net_map, pin_map;
	void * ptr;
	int npins, inet;
	int err_flags = 0;
	int line = 0;
	BOOL not_eof;
	int ipart;
	if( pl )
		ipart = pl->GetSize();

	// state machine
	enum { IDLE, PARTS, NETS, SIGNAL };
	int state = IDLE;
	BOOL bLegal = FALSE;

	while( 1 )
	{
		not_eof = file->ReadString( instr );
		line++;
		instr.Trim();
		if( line > 2 && !bLegal )
		{
			file->Close();
			return NOT_PADSPCB_FILE;
		}
		if( instr.Left(5) == "*END*" || !not_eof )
		{
			// normal return
			file->Close();
			return err_flags;
		}
		else if( instr.Left(10) == "*PADS-PCB*" || instr.Left(10) == "*PADS2000*" )
			bLegal = TRUE;
		else if( instr.Left(6) == "*PART*" )
			state = PARTS;
		else if( instr.Left(5) == "*NET*" )
			state = NETS;
		else if( state == PARTS && pl && (flags & IMPORT_PARTS) )
		{
			// read parts
			if( instr.GetLength() && instr[0] != '/' )
			{
				// get ref_des and footprint
				CString ref_str( mystrtok( instr, " \t" ) );
				if( ref_str.GetLength() > MAX_REF_DES_SIZE )
				{
					CString mess, s ((LPCSTR) IDS_LineReferenceDesignatorTooLong);
					mess.Format( s, line, ref_str );
					m_dlg_log->AddLine( mess );
					ref_str = ref_str.Left(MAX_REF_DES_SIZE);
				}
				// check for legal ref_designator
				if( ref_str.FindOneOf( ". " ) != -1 )
				{
					CString mess, s ((LPCSTR) IDS_LinePartIllegalReferenceDesignator);
					mess.Format( s,	line, ref_str );
					m_dlg_log->AddLine( mess );
					continue;
				}
				// check for duplicate part
				if( part_map.Lookup( ref_str, ptr ) )
				{
					CString mess, s ((LPCSTR) IDS_LinePartIsDuplicateIgnored);
					mess.Format( s,	line, ref_str );
					m_dlg_log->AddLine( mess );
					continue;
				}
				// new part
				pl->SetSize( ipart+1 );
				CString shape_str( mystrtok( NULL, "\n\r" ) );
				shape_str.Trim();
				// check for "ssss@ffff" format
				int pos = shape_str.Find( "@" );
				if( pos != -1 )
				{
					CString value_str;
					SplitString( &shape_str, &value_str, &shape_str, '@' );
					(*pl)[ipart].value = value_str;
				}
				if( shape_str.GetLength() > CShape::MAX_NAME_SIZE )
				{
					CString mess, s ((LPCSTR) IDS_LinePackageNameTooLong);
					mess.Format( s,	line, shape_str );
					m_dlg_log->AddLine( mess );
					shape_str = shape_str.Left(CShape::MAX_NAME_SIZE);
				}
				// find footprint, get from library if necessary
				CShape * s = GetFootprintPtr( shape_str );
				if( s == NULL )
				{
					CString mess, s ((LPCSTR) IDS_LinePartFootprintNotFound);
					mess.Format( s,	line, ref_str, shape_str );
					m_dlg_log->AddLine( mess );
				}
				// add part to partlist_info
				(*pl)[ipart].part = NULL;
				(*pl)[ipart].ref_des = ref_str;
				part_map.SetAt( ref_str, NULL );
				if( s )
				{
					(*pl)[ipart].ref_size = s->m_ref_size;
					(*pl)[ipart].ref_width = s->m_ref_w;
				}
				else
				{
					(*pl)[ipart].ref_size = 0;
					(*pl)[ipart].ref_width = 0;
				}
				(*pl)[ipart].package = shape_str;
				(*pl)[ipart].bOffBoard = TRUE;
				(*pl)[ipart].shape = s;
				(*pl)[ipart].angle = 0;
				(*pl)[ipart].side = 0;
				(*pl)[ipart].x = 0;
				(*pl)[ipart].y = 0;
				ipart++;
			}
		}
		else if( instr.Left(8) == "*SIGNAL*" && nl && (flags & IMPORT_NETS) )
		{
			state = NETS;
			net_name = instr.Right(instr.GetLength()-8);
			net_name.Trim();
			int pos = net_name.Find( " " );
			if( pos != -1 )
			{
				net_name = net_name.Left( pos );
			}
			if( net_name.GetLength() )
			{
				if( net_name.GetLength() > MAX_NET_NAME_SIZE )
				{
					CString mess, s ((LPCSTR) IDS_LineNetNameTooLong);
					mess.Format( s,	line, net_name, net_name.Left(MAX_NET_NAME_SIZE) );
					m_dlg_log->AddLine( mess );
					net_name = net_name.Left(MAX_NET_NAME_SIZE);
				}
				if( net_name.FindOneOf( " \"" ) != -1 )
				{
					CString mess, s ((LPCSTR) IDS_LineNetNameIllegal);
					mess.Format( s, line, net_name );
					m_dlg_log->AddLine( mess );
				}
				else
				{
					if( net_map.Lookup( net_name, ptr ) )
					{
						CString mess, s ((LPCSTR) IDS_LineNetNameIsDuplicate);
						mess.Format( s,	line, net_name );
						m_dlg_log->AddLine( mess );
					}
					else
					{
						// add new net
						net_map.SetAt( net_name, NULL );
						inet = nl->GetSize();
						nl->SetSize( inet+1 );
						(*nl)[inet].name = net_name;
						(*nl)[inet].net = NULL;
						(*nl)[inet].apply_trace_width = FALSE;
						(*nl)[inet].apply_via_width = FALSE;
						(*nl)[inet].modified = TRUE;
						(*nl)[inet].deleted = FALSE;
						(*nl)[inet].visible = TRUE;
						// mark widths as undefined
						(*nl)[inet].w = -1;
						(*nl)[inet].v_w = -1;
						(*nl)[inet].v_h_w = -1;
						npins = 0;
						state = SIGNAL;
					}
				}
			}
		}
		else if( state == SIGNAL  && nl && (flags & IMPORT_NETS) )
		{
			// add pins to net
			char * pin = mystrtok( instr, " \t\n\r" );
			while( pin )
			{
				CString pin_cstr( pin );
				if( pin_cstr.GetLength() > 3 )
				{
					int dot = pin_cstr.Find( ".", 0 );
					if( dot )
					{
						if( pin_map.Lookup( pin_cstr, ptr ) )
						{
							CString mess, s ((LPCSTR) IDS_LineNetPinIsDuplicate);
							mess.Format( s,	line, net_name, pin_cstr );
							m_dlg_log->AddLine( mess );
						}
						else
						{
							pin_map.SetAt( pin_cstr, NULL );
							CString ref_des = pin_cstr.Left( dot );
							CString pin_num_cstr = pin_cstr.Right( pin_cstr.GetLength()-dot-1 );
							(*nl)[inet].ref_des.Add( ref_des );
							if( pin_num_cstr.GetLength() > CShape::MAX_PIN_NAME_SIZE )
							{
								CString mess, s ((LPCSTR) IDS_LinePinNameTooLong);
								mess.Format( s,	line, pin_num_cstr );
								m_dlg_log->AddLine( mess );
								pin_num_cstr = pin_num_cstr.Left(CShape::MAX_PIN_NAME_SIZE);
							}
							(*nl)[inet].pin_name.Add( pin_num_cstr );
						}
					}
					else
					{
						// illegal pin identifier
						CString mess, s ((LPCSTR) IDS_LinePinIdentifierIllegal);
						mess.Format( s,	line, pin_cstr );
						m_dlg_log->AddLine( mess );
					}
				}
				else
				{
					// illegal pin identifier
					CString mess, s ((LPCSTR) IDS_LinePinIdentifierIllegal);
					mess.Format( s, line, pin_cstr );
					m_dlg_log->AddLine( mess );
				}
				pin = mystrtok( NULL, " \t\n\r" );
			} // end while( pin )
		}
	} // end while
}

void CFreePcbDoc::OnAppExit()
{
	if( FileClose() != IDCANCEL )
	{
//		m_view->SetHandleCmdMsgFlag( FALSE );
		AfxGetMainWnd()->SendMessage( WM_CLOSE, 0, 0 );
	}
}

void CFreePcbDoc::OnFileConvert()
{
	CivexDlg dlg;
	dlg.DoModal();
}

// create undo record for moving origin
//
undo_move_origin * CFreePcbDoc::CreateMoveOriginUndoRecord( int x_off, int y_off )
{
	// create undo record 
	undo_move_origin * undo = new undo_move_origin;
	undo->x_off = x_off;
	undo->y_off = y_off;
	return undo;
}

// undo operation on move origin
//
void CFreePcbDoc::MoveOriginUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		// restore previous origin
		undo_move_origin * un_mo = (undo_move_origin*)ptr;
		int x_off = un_mo->x_off;
		int y_off = un_mo->y_off;
		this_Doc->m_view->MoveOrigin( -x_off, -y_off );
		this_Doc->m_view->Invalidate( FALSE );
	}
	delete ptr;
}

// create undo record for board outline
//
undo_board_outline * CFreePcbDoc::CreateBoardOutlineUndoRecord( CPolyLine * poly )
{
	// create undo record for board outline
	undo_board_outline * undo;
	int ncorners = poly->GetNumCorners();
	undo = (undo_board_outline*)malloc( sizeof(undo_board_outline)+ncorners*sizeof(undo_corner));
	undo->ncorners = poly->GetNumCorners();
	undo_corner * corner = (undo_corner*)((UINT)undo + sizeof(undo_board_outline));
	for( int ic=0; ic<ncorners; ic++ )
	{
		corner[ic].x = poly->GetX( ic );
		corner[ic].y = poly->GetY( ic );
		corner[ic].style = poly->GetSideStyle( ic );
	}
	return undo;
}

// undo operation on board outline
//
void CFreePcbDoc::BoardOutlineUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo ) 
	{
		if( type == CFreePcbView::UNDO_BOARD_OUTLINE_CLEAR_ALL ) 
		{
			// remove all cutouts
			this_Doc->m_board_outline.RemoveAll();
		}
		else
		{
			// restore cutout from undo record
			undo_board_outline * un_bd = (undo_board_outline*)ptr;
			undo_corner * corner = (undo_corner*)((UINT)un_bd + sizeof(undo_board_outline));
			int i = this_Doc->m_board_outline.GetSize();
			this_Doc->m_board_outline.SetSize(i+1);
			CPolyLine * poly = &this_Doc->m_board_outline[i];
			poly->SetDisplayList( this_Doc->m_dlist );
			id bd_id( ID_BOARD, ID_BOARD, i );
			poly->Start( LAY_BOARD_OUTLINE, 1, 10*NM_PER_MIL, 
				corner[0].x, corner[0].y, 0, &bd_id, NULL );
			for( int ic=1; ic<un_bd->ncorners; ic++ )
				poly->AppendCorner( corner[ic].x, corner[ic].y, corner[ic-1].style );
			poly->Close( corner[un_bd->ncorners-1].style );
		}
	}
	delete ptr;
}

// create undo record for SM cutout
// only include closed polys
//
undo_sm_cutout * CFreePcbDoc::CreateSMCutoutUndoRecord( CPolyLine * poly )
{
	// create undo record for sm cutout
	undo_sm_cutout * undo;
	int ncorners = poly->GetNumCorners();
	undo = (undo_sm_cutout*)malloc( sizeof(undo_sm_cutout)+ncorners*sizeof(undo_corner));
	undo->layer = poly->GetLayer();
	undo->hatch_style = poly->GetHatch();
	undo->ncorners = poly->GetNumCorners();
	undo_corner * corner = (undo_corner*)((UINT)undo + sizeof(undo_sm_cutout));
	for( int ic=0; ic<ncorners; ic++ )
	{
		corner[ic].x = poly->GetX( ic );
		corner[ic].y = poly->GetY( ic );
		corner[ic].style = poly->GetSideStyle( ic );
	}
	return undo;
}

// undo operation on solder mask cutout
//
void CFreePcbDoc::SMCutoutUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo ) 
	{
		if( type == CFreePcbView::UNDO_SM_CUTOUT_CLEAR_ALL ) 
		{
			// remove all cutouts
			this_Doc->m_sm_cutout.RemoveAll();
		}
		else
		{
			// restore cutout from undo record
			undo_sm_cutout * un_sm = (undo_sm_cutout*)ptr;
			undo_corner * corner = (undo_corner*)((UINT)un_sm + sizeof(undo_sm_cutout));
			int i = this_Doc->m_sm_cutout.GetSize();
			this_Doc->m_sm_cutout.SetSize(i+1);
			CPolyLine * poly = &this_Doc->m_sm_cutout[i];
			poly->SetDisplayList( this_Doc->m_dlist );
			id sm_id( ID_SM_CUTOUT, ID_SM_CUTOUT, i );
			poly->Start( un_sm->layer, 1, 10*NM_PER_MIL, 
				corner[0].x, corner[0].y, un_sm->hatch_style, &sm_id, NULL );
			for( int ic=1; ic<un_sm->ncorners; ic++ )
				poly->AppendCorner( corner[ic].x, corner[ic].y, corner[ic-1].style );
			poly->Close( corner[un_sm->ncorners-1].style );
		}
	}
	delete ptr;
}

// call dialog to create Gerber and drill files
void CFreePcbDoc::OnFileGenerateCadFiles()
{
	if( m_board_outline.GetSize() == 0 )
	{
		CString s ((LPCSTR) IDS_ABoardOutlineMustBePresentForCAM);
		AfxMessageBox( s );
		return;
	}
	CDlgCAD dlg;
	if( m_cam_full_path == "" )
		m_cam_full_path = m_path_to_folder + "\\CAM";
	dlg.Initialize( m_version,
		&m_cam_full_path, 
		&m_path_to_folder,
		&m_app_dir,
		m_num_copper_layers, 
		m_cam_units,
		m_bSMT_copper_connect,
		m_fill_clearance, 
		m_mask_clearance,
		m_thermal_width,
		m_pilot_diameter,
		m_min_silkscreen_stroke_wid,
		m_outline_width,
		m_hole_clearance,
		m_annular_ring_pins,
		m_annular_ring_vias,
		m_paste_shrink,
		m_n_x, m_n_y, m_space_x, m_space_y,
		m_cam_flags,
		m_cam_layers,
		m_cam_drill_file,
		&m_board_outline, 
		&m_sm_cutout,
		&m_bShowMessageForClearance,
		m_plist, 
		m_nlist, 
		m_tlist, 
		m_dlist,
		m_dlg_log );
	m_nlist->OptimizeConnections( FALSE );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// update parameters
		if( m_cam_full_path != dlg.m_folder
			|| m_cam_units != dlg.m_units
			|| m_fill_clearance != dlg.m_fill_clearance
			|| m_mask_clearance != dlg.m_mask_clearance
			|| m_thermal_width != dlg.m_thermal_width
			|| m_min_silkscreen_stroke_wid != dlg.m_min_silkscreen_width
			|| m_pilot_diameter != dlg.m_pilot_diameter
			|| m_outline_width != dlg.m_outline_width
			|| m_hole_clearance != dlg.m_hole_clearance
			|| m_annular_ring_pins != dlg.m_annular_ring_pins
			|| m_annular_ring_vias != dlg.m_annular_ring_vias
			|| m_cam_flags != dlg.m_flags
			|| m_cam_layers != dlg.m_layers
			|| m_cam_drill_file != dlg.m_drill_file )
		{
			ProjectModified( TRUE );
		}
		m_cam_full_path = dlg.m_folder;
		m_cam_units = dlg.m_units;
		m_fill_clearance = dlg.m_fill_clearance;
		m_mask_clearance = dlg.m_mask_clearance;
		m_thermal_width = dlg.m_thermal_width;
		m_min_silkscreen_stroke_wid = dlg.m_min_silkscreen_width;
		m_pilot_diameter = dlg.m_pilot_diameter;
		m_outline_width = dlg.m_outline_width;
		m_hole_clearance = dlg.m_hole_clearance;
		m_annular_ring_pins = dlg.m_annular_ring_pins;
		m_annular_ring_vias = dlg.m_annular_ring_vias;
		m_plist->SetPinAnnularRing( m_annular_ring_pins );
		m_nlist->SetViaAnnularRing( m_annular_ring_vias );
		m_paste_shrink = dlg.m_paste_shrink;
		m_cam_flags = dlg.m_flags;
		m_cam_layers = dlg.m_layers;
		m_cam_drill_file = dlg.m_drill_file;
		m_n_x = dlg.m_n_x;
		m_n_y = dlg.m_n_y;
		m_space_x = dlg.m_space_x;
		m_space_y = dlg.m_space_y;
		m_bShowMessageForClearance = dlg.m_bShowMessageForClearance;
	}
}

void CFreePcbDoc::OnToolsFootprintwizard()
{
	CDlgWizQuad dlg;
	dlg.Initialize( &m_footprint_cache_map, &m_footlibfoldermap, TRUE, m_dlg_log );
	dlg.DoModal();
}

void CFreePcbDoc::MakeLibraryMaps( CString * fullpath )
{
	m_footlibfoldermap.SetDefaultFolder( fullpath );
	m_footlibfoldermap.AddFolder( fullpath, NULL );
}

void CFreePcbDoc::OnProjectOptions()
{
	CDlgProjectOptions dlg;
	if( m_name == "" )
	{
		m_name = m_pcb_filename;
		if( m_name.Right(4) == ".fpc" )
			m_name = m_name.Left( m_name.GetLength()-4 );
	}
	dlg.Init( FALSE, &m_name, &m_path_to_folder, &m_full_lib_dir,
		m_num_copper_layers, m_bSMT_copper_connect, m_default_glue_w,
		m_trace_w, m_via_w, m_via_hole_w,
		m_auto_interval, m_auto_ratline_disable, m_auto_ratline_min_pins,
		&m_w, &m_v_w, &m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )  
	{
		// set options from dialog
		m_auto_ratline_disable = dlg.GetAutoRatlineDisable();
		m_auto_ratline_min_pins = dlg.GetAutoRatlineMinPins();
		BOOL bResetAreaConnections = m_bSMT_copper_connect != dlg.m_bSMT_connect_copper;
		m_bSMT_copper_connect = dlg.m_bSMT_connect_copper;
		m_nlist->SetSMTconnect( m_bSMT_copper_connect );
		m_default_glue_w = dlg.GetGlueWidth();
		// deal with decreased number of layers
		if( m_num_copper_layers > dlg.GetNumCopperLayers() )
		{
			// decreasing number of layers, offer to reassign them
			CDlgReassignLayers rel_dlg;
			rel_dlg.Initialize( m_num_copper_layers, dlg.GetNumCopperLayers() );
			int ret = rel_dlg.DoModal();
			if( ret == IDOK )
			{
				// reassign copper layers
				m_nlist->ReassignCopperLayers( dlg.GetNumCopperLayers(), rel_dlg.new_layer );
				m_tlist->ReassignCopperLayers( dlg.GetNumCopperLayers(), rel_dlg.new_layer );
				m_num_copper_layers = dlg.GetNumCopperLayers();
				m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
			}
			// clear clipboard
			clip_sm_cutout.SetSize(0);
			clip_board_outline.SetSize(0);
			clip_tlist->RemoveAllTexts();
			clip_nlist->RemoveAllNets();
			clip_plist->RemoveAllParts();
		}
		else if( m_num_copper_layers < dlg.GetNumCopperLayers() )
		{
			// increasing number of layers, don't reassign
			for( int il=m_num_copper_layers; il<dlg.GetNumCopperLayers(); il++ )
				m_vis[LAY_TOP_COPPER+il] = 1;
			m_num_copper_layers = dlg.GetNumCopperLayers();
			m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
		}
		m_nlist->SetNumCopperLayers( m_num_copper_layers );
		m_plist->SetNumCopperLayers( m_num_copper_layers );

		m_name = dlg.GetName();
		if( m_full_lib_dir != dlg.GetLibFolder() )
		{
			m_full_lib_dir = dlg.GetLibFolder();
			m_footlibfoldermap.SetDefaultFolder( &m_full_lib_dir );		
			m_footlibfoldermap.SetLastFolder( &m_full_lib_dir );		
		}
		m_trace_w = dlg.GetTraceWidth();
		m_via_w = dlg.GetViaWidth();
		m_via_hole_w = dlg.GetViaHoleWidth();
		m_nlist->SetWidths( m_trace_w, m_via_w, m_via_hole_w );
		m_auto_interval = dlg.GetAutoInterval();
		m_auto_ratline_disable = dlg.GetAutoRatlineDisable();

		if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
			m_nlist->OptimizeConnections();
		m_view->InvalidateLeftPane();
		m_view->Invalidate( FALSE );
		m_project_open = TRUE;

		// force redraw of function key text
		m_view->m_cursor_mode = 999;
		m_view->CancelSelection();
		ProjectModified( TRUE );
		ResetUndoState();
	}
}

// come here from MainFrm on timer event
//
void CFreePcbDoc::OnTimer()
{
	if( m_project_modified_since_autosave )
	{
		m_auto_elapsed += TIMER_PERIOD;
		if( m_view && m_auto_interval && m_auto_elapsed > m_auto_interval )
		{
			if( !m_view->CurDragging() )
				AutoSave();
		}
	}
}



void CFreePcbDoc::OnToolsCheckPartsAndNets()
{
	// open log
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	CString str;
	int nerrors = m_plist->CheckPartlist( &str );
	str += "\r\n";
	nerrors += m_nlist->CheckNetlist( &str );
	m_dlg_log->AddLine( str );
}

void CFreePcbDoc::OnToolsDrc()
{
	DlgDRC dlg;
	if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
		m_nlist->OptimizeConnections();
	m_drelist->Clear();
	dlg.Initialize( m_units, 
					&m_dr,
					m_plist,
					m_nlist,
					m_drelist,
					m_num_copper_layers,
					&m_board_outline,
					m_annular_ring_pins,
					m_annular_ring_vias,
					m_dlg_log );
	int ret = dlg.DoModal();
	m_annular_ring_pins = dlg.m_CAM_annular_ring_pins;
	m_annular_ring_vias = dlg.m_CAM_annular_ring_vias;
	ProjectModified( TRUE );
	m_view->BringWindowToTop();
	m_view->Invalidate( FALSE );
}

void CFreePcbDoc::OnToolsClearDrc()
{
	if( m_view->m_cursor_mode == CUR_DRE_SELECTED )
	{
		m_view->CancelSelection();
		m_view->SetCursorMode( CUR_NONE_SELECTED );
	}
	m_drelist->Clear();
	m_view->Invalidate( FALSE );
}

void CFreePcbDoc::OnToolsShowDRCErrorlist()
{
	// TODO: Add your command handler code here
}

void CFreePcbDoc::SetFileLayerMap( int file_layer, int layer )
{
	m_layer_by_file_layer[file_layer] = layer;
}


void CFreePcbDoc::OnToolsCheckConnectivity()
{
	// open log
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	CString str;
	int nerrors = m_nlist->CheckConnectivity( &str );
	m_dlg_log->AddLine( str );
	if( !nerrors )
	{
		str.LoadStringA(IDS_NoUnroutedConnections);
		m_dlg_log->AddLine( str );
	}
}

void CFreePcbDoc::OnViewLog()
{
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
}

void CFreePcbDoc::OnToolsCheckCopperAreas()
{
	CString str;
 
	m_dlg_log->ShowWindow( SW_SHOW );   
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	m_view->CancelSelection();
	CIterator_cnet iter_net(m_nlist);
	cnet * net = iter_net.GetFirst(); 
	BOOL new_event = TRUE; 
	while( net ) 
	{
		if( net->NumAreas() > 0 )
		{
			CString s ((LPCSTR) IDS_NetAreas);
			str.Format( s, net->name, net->NumAreas() ); 
			m_dlg_log->AddLine( str );
			m_view->SaveUndoInfoForAllAreasInNet( net, new_event, m_undo_list ); 
			new_event = FALSE;
			// check for minimum number of corners and closed contours
			for( int ia=0; ia<net->NumAreas(); ia++ )
			{
				int nc = net->area[ia].poly->GetNumCorners();
				if( nc < 3 )
				{
					s.LoadStringA(IDS_AreaHasOnlyCorners);
					str.Format( s, ia+1, nc );
					m_dlg_log->AddLine( str );
				}
				else
				{
					if( !net->area[ia].poly->GetClosed() )
					{
						s.LoadStringA(IDS_AreaIsNotClosed);
						str.Format( s, ia+1 );
						m_dlg_log->AddLine( str );
					}
				}
			}
			// check all areas in net for self-intersection
			for( int ia=0; ia<net->NumAreas(); ia++ )
			{
				int ret = m_nlist->ClipAreaPolygon( net, ia, FALSE, FALSE );
				if( ret == -1 )
				{
					s.LoadStringA(IDS_AreaIsSelfIntersecting);
					str.Format( s, ia+1 );
					m_dlg_log->AddLine( str );
				}
				if( ret == 0 )
				{
					s.LoadStringA(IDS_AreaIsOK);
					str.Format( s, ia+1 );
					m_dlg_log->AddLine( str );
				}
				else if( ret == 1 )
				{
					s.LoadStringA(IDS_AreaIsSelfIntersecting2);
					str.Format( s, ia+1 );
					m_dlg_log->AddLine( str );
				}
			}
			// check all areas in net for intersection
			if( net->NumAreas() > 1 )
			{
				for( int ia1=0; ia1<net->NumAreas()-1; ia1++ ) 
				{
					BOOL mod_ia1 = FALSE;
					for( int ia2=net->NumAreas()-1; ia2 > ia1; ia2-- )
					{
						if( net->area[ia1].poly->GetLayer() == net->area[ia2].poly->GetLayer() )
						{
							// check ia2 against 1a1 
							int ret = m_nlist->TestAreaIntersection( net, ia1, ia2 );
							if( ret == 2 ) 
							{
								s.LoadStringA(IDS_AreasHaveAnIntersectingArc);
								str.Format( s, ia1+1, ia2+1 );
								m_dlg_log->AddLine( str );
							}
							else if( ret == 1 && net->area[ia1].utility2 == -1 )
							{
								s.LoadStringA(IDS_AreasIntersectButCantBeCombined);
								str.Format( s, ia1+1, ia2+1, ia1+1 );
								m_dlg_log->AddLine( str );
							}
							else if( ret == 1 && net->area[ia2].utility2 == -1 )
							{
								s.LoadStringA(IDS_AreasIntersectButCantBeCombined);
								str.Format( s, ia1+1, ia2+1, ia1+1 );
								m_dlg_log->AddLine( str );
							}
							else if( ret == 1 )
							{
								ret = m_nlist->CombineAreas( net, ia1, ia2 );
								if( ret == 1 )
								{
									s.LoadStringA(IDS_AreasIntersectAndHaveBeenCombined);
									str.Format( s, ia1+1, ia2+1 );
									m_dlg_log->AddLine( str );
									mod_ia1 = TRUE;
								}
							}
						}
					}
					if( mod_ia1 )
						ia1--;		// if modified, we need to check it again
				}
			}
		}
		net = iter_net.GetNext();
	}
	str.LoadStringA(IDS_Done);
	m_dlg_log->AddLine( str );
	if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
		m_nlist->OptimizeConnections();
	ProjectModified( TRUE );
	m_view->Invalidate( FALSE );
}

void CFreePcbDoc::OnToolsCheckTraces()
{
	CString str;
	ResetUndoState();
	m_view->CancelSelection();
	m_dlg_log->ShowWindow( SW_SHOW );   
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	CString s ((LPCSTR) IDS_CheckingTracesForZeroLengthOrColinearSegments);
	m_dlg_log->AddLine( s );
	m_nlist->CleanUpAllConnections( &str );
	m_dlg_log->AddLine( str );
	m_dlg_log->AddLine( "\r\n");
	s.LoadStringA(IDS_Done);
	m_dlg_log->AddLine(s);
}

void CFreePcbDoc::OnEditPasteFromFile()
{
	// force old-style file dialog by setting size of OPENFILENAME struct
	CString s ((LPCSTR) IDS_AllFiles);
	CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_EXPLORER, 
		s, NULL, 0 /* CPT OPENFILENAME_SIZE_VERSION_400 */ );
	s.LoadStringA(IDS_PasteGroupFromFile);
	dlg.m_ofn.lpstrTitle = s; 
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{ 
		// read project file
		ResetUndoState();
		CString pathname = dlg.GetPathName();
		CString filename = dlg.GetFileName();
		CStdioFile pcb_file;
		int err = pcb_file.Open( pathname, CFile::modeRead, NULL );
		if( !err )
		{
			// error opening project file
			CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
			mess.Format( s, pathname );
			AfxMessageBox( mess );
			return;
		}
		// clear clipboard objects to hold group
		clip_nlist->RemoveAllNets();
		clip_plist->RemoveAllParts();
		clip_tlist->RemoveAllTexts();
		clip_sm_cutout.RemoveAll();
		clip_board_outline.RemoveAll();
		CMapStringToPtr cache_map;		// incoming footprints
		try
		{
			// get layers
			int fpos = 0;
			CString in_str = "";
			while( in_str.Left(16) != "n_copper_layers:" )
			{
				fpos = pcb_file.GetPosition();
				pcb_file.ReadString( in_str );
			}
			int n_copper_layers = atoi( in_str.Right( in_str.GetLength()-16 ) );
			if( n_copper_layers > m_num_copper_layers )
			{
				CString mess ((LPCSTR) IDS_TheGroupFileThatYouArePastingHasMoreLayers);
				AfxMessageBox( mess, MB_OK );
				pcb_file.Close();
				return;
			}

			// read footprints
			while( in_str.Left(12) != "[footprints]" )
			{
				fpos = pcb_file.GetPosition();
				pcb_file.ReadString( in_str );
			}
			pcb_file.Seek( fpos, CFile::begin );
			ReadFootprints( &pcb_file, &cache_map );
			// copy footprints to project cache if necessary
			void * ptr;
			CShape * s;
			POSITION pos;
			CString key;
			for( pos = cache_map.GetStartPosition(); pos != NULL; )
			{
				cache_map.GetNextAssoc( pos, key, ptr );
				s = (CShape*)ptr;
				if( !m_footprint_cache_map.Lookup( s->m_name, ptr ) )
				{
					// copy shape to project cache
					m_footprint_cache_map.SetAt( s->m_name, s );
				}
				else
				{
					// delete duplicate shape
					delete s;
				}
			}

			// read board outline
			ReadBoardOutline( &pcb_file, &clip_board_outline );

			// read sm_cutouts
			ReadSolderMaskCutouts( &pcb_file, &clip_sm_cutout );

			// read parts, nets and texts
			clip_plist->ReadParts( &pcb_file );
			clip_nlist->ReadNets( &pcb_file, m_read_version );
			clip_tlist->ReadTexts( &pcb_file );
			pcb_file.Close();
		}
		catch( CString * err_str )
		{
			// parsing error
			AfxMessageBox( *err_str );
			delete err_str;
			pcb_file.Close();
			return;
		}
		m_view->OnGroupPaste();
	}
}

// Purge footprunts from local cache unless they are used in
// partlist or clipboard
//
void CFreePcbDoc::PurgeFootprintCache()
{
	POSITION pos;
	CString key;
	void * ptr;

	for( pos = m_footprint_cache_map.GetStartPosition(); pos != NULL; )
	{
		m_footprint_cache_map.GetNextAssoc( pos, key, ptr );
		CShape * shape = (CShape*)ptr;
		if( m_plist->GetNumFootprintInstances( shape ) == 0
			&& clip_plist->GetNumFootprintInstances( shape ) == 0 )
		{
			// purge this footprint
			delete shape;
			m_footprint_cache_map.RemoveKey( key );
		}
	}
}


void CFreePcbDoc::OnFilePrint()
{
	// TODO: Add your command handler code here
}

void CFreePcbDoc::OnFileExportDsn()
{
	if( m_project_modified )
	{
		CString s ((LPCSTR) IDS_ThisFunctionCreatesADsnFile);
		int ret = AfxMessageBox( s, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return;
		else if( ret == IDYES )
			OnFileSave();
	}
	CDlgExportDsn dlg;
	CString dsn_filepath = m_pcb_full_path;
	int dot_pos = dsn_filepath.ReverseFind( '.' );
	if( dot_pos != -1 )
		dsn_filepath = dsn_filepath.Left( dot_pos );
	dsn_filepath += ".dsn";
	int num_polys = m_board_outline.GetSize();
	if( m_dsn_signals_poly >= num_polys )
		m_dsn_signals_poly = 0;
	if( m_dsn_bounds_poly >= num_polys )
		m_dsn_bounds_poly = 0;
	dlg.Initialize( &dsn_filepath, m_board_outline.GetSize(), 
						m_dsn_bounds_poly, m_dsn_signals_poly, m_dsn_flags );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_dlg_log->ShowWindow( SW_SHOW );   
		m_dlg_log->UpdateWindow();
		m_dlg_log->BringWindowToTop();
		m_dlg_log->Clear();
		m_dlg_log->UpdateWindow(); 

		m_dsn_flags = dlg.m_flags;
		m_dsn_bounds_poly = dlg.m_bounds_poly;
		m_dsn_signals_poly = dlg.m_signals_poly;
		OnFileSave();
		CString s ((LPCSTR) IDS_SavingProjectFile), mess;
		mess.Format(s, m_pcb_full_path);
		m_dlg_log->AddLine( mess );
		s.LoadStringA(IDS_CreatingDsnFile);
		mess.Format(s, dsn_filepath);
		m_dlg_log->AddLine( mess );  
		CString commandLine = "\"" + m_app_dir + "\\fpcroute.exe\"";
		int from_to = m_dsn_flags & CDlgExportDsn::DSN_FROM_TO_MASK;
		if( from_to == CDlgExportDsn::DSN_FROM_TO_ALL )
			commandLine += " -F3";
		else if( from_to == CDlgExportDsn::DSN_FROM_TO_LOCKED )
			commandLine += " -F1";
		else if( from_to == CDlgExportDsn::DSN_FROM_TO_NET_LOCKED )
			commandLine += " -F2";
		if( m_dsn_flags & CDlgExportDsn::DSN_VERBOSE )
			commandLine += " -V"; 
		if( m_dsn_flags & CDlgExportDsn::DSN_INFO_ONLY )
			commandLine += " -I";
		if( m_dsn_bounds_poly != 0 || m_dsn_signals_poly != 0 ) 
		{
			CString str;
			str.Format( " -U%d,%d", m_dsn_bounds_poly+1, m_dsn_signals_poly+1 );
			commandLine += str;
		}
		commandLine += " \"" + m_pcb_full_path + "\"";
//		CString commandLine = "C:/freepcb/bin/RTconsole.exe  C:/freepcb/bin/fpcroute.exe -V C:/freepcb/bin/test";
		s.LoadStringA(IDS_Run);
		mess.Format(s, commandLine);
		m_dlg_log->AddLine( mess );  
		RunConsoleProcess( commandLine, m_dlg_log );
#if 0
		HANDLE hOutput, hProcess;
		hProcess = SpawnAndRedirect(commandLine, &hOutput, NULL); 
		if (!hProcess) 
		{
			m_dlg_log->AddLine( "Failed!\r\n" );
			return;
		}

		// if necessary, this could be put in a separate thread so the GUI thread is not blocked
		BeginWaitCursor();
		CHAR buffer[65];
		DWORD read;
		while (ReadFile(hOutput, buffer, 64, &read, NULL))
		{
			buffer[read] = '\0';
			m_dlg_log->AddLine( buffer );
		}
		CloseHandle(hOutput);
		CloseHandle(hProcess);
		EndWaitCursor();
#endif
	}
}

void CFreePcbDoc::OnFileImportSes()
{
	CDlgImportSes dlg;
	dlg.Initialize( &m_ses_full_path, &m_pcb_full_path );
	int ret = dlg.DoModal(); 
	if( ret == IDOK )
	{
		m_dlg_log->ShowWindow( SW_SHOW );   
		m_dlg_log->UpdateWindow();
		m_dlg_log->BringWindowToTop(); 
		m_dlg_log->Clear();
		m_dlg_log->UpdateWindow(); 
		// save current project if modified (including dialog parameters)
		if( dlg.m_ses_filepath != m_ses_full_path )
		{
			m_ses_full_path = dlg.m_ses_filepath;
			ProjectModified( TRUE );
		}
		if( m_project_modified )
		{
			CString s ((LPCSTR) IDS_ProjectModifiedSaveBeforeImport);
			int ret = AfxMessageBox( s, MB_YESNO );
			if( ret = IDYES )
			{
				OnFileSave();
			}
			else
				ProjectModified( FALSE );
		}
		CString temp_file_name = "~temp$$$.fpc";   
		CString temp_routed_file_name = "~temp$$$_routed.fpc";
		CString temp_file_path = m_path_to_folder + "\\" + temp_file_name;
		CString temp_routed_file_path = m_path_to_folder + "\\" + temp_routed_file_name;
		struct _stat buf;
		int err = _stat( temp_file_path, &buf );
		if( !err )
		{
			CString s ((LPCSTR) IDS_Delete2), mess;
			mess.Format(s, temp_file_path);
			m_dlg_log->AddLine( mess );  
			remove( temp_file_path );
		}
		err = _stat( dlg.m_routed_pcb_filepath, &buf );
		if( !err )
		{
			CString s ((LPCSTR) IDS_Delete2), mess;
			mess.Format(s, dlg.m_routed_pcb_filepath);
			m_dlg_log->AddLine( mess );  
			remove( dlg.m_routed_pcb_filepath );
		}
		m_ses_full_path = dlg.m_ses_filepath;

		// save project as temporary file
		CString s ((LPCSTR) IDS_Save), mess;
		mess.Format(s, temp_file_path);
		m_dlg_log->AddLine( mess );
		CString old_pcb_filename = m_pcb_filename;
		CString old_pcb_full_path = m_pcb_full_path;
		m_pcb_filename = temp_file_name;
		m_pcb_full_path = temp_file_path;
		OnFileSave();

		// import session file
		CString verbose = "";
		if( dlg.m_bVerbose )
			verbose = "-V ";
		CString commandLine = "\"" + m_app_dir + "\\fpcroute.exe\" -B " + verbose + "\"" +
			temp_file_path + "\" \"" + m_ses_full_path + "\""; 
		s.LoadStringA(IDS_Run);
		mess.Format(s, commandLine);
		m_dlg_log->AddLine( mess );  
		RunConsoleProcess( commandLine, m_dlg_log );
		err = _stat( temp_routed_file_path, &buf );
		if( err )
		{
			s.LoadStringA(IDS_FpcRouteFailedToCreate);
			mess.Format(s, temp_routed_file_path);
			m_dlg_log->AddLine( mess );  
			return;
		}
		s.LoadStringA(IDS_Rename);
		mess.Format(s, temp_routed_file_path, dlg.m_routed_pcb_filepath);
		m_dlg_log->AddLine( mess );  
		err = rename( temp_routed_file_path, dlg.m_routed_pcb_filepath ); 
		if( err )
		{
			s.LoadStringA(IDS_RenamingProjectFileFailed);
			mess.Format(s, temp_routed_file_path, dlg.m_routed_pcb_filepath); 
			m_dlg_log->AddLine( mess );
		}
		CString old_ses_full_path = m_ses_full_path;
		s.LoadStringA(IDS_Load);
		mess.Format(s, dlg.m_routed_pcb_filepath);
		m_dlg_log->AddLine( mess );  
		OnFileAutoOpen( dlg.m_routed_pcb_filepath );
		m_ses_full_path = old_ses_full_path;
		s.LoadStringA(IDS_Reimport);
		mess.Format(s, m_ses_full_path);
		m_dlg_log->AddLine( mess );  
		ImportSessionFile( &m_ses_full_path, m_dlg_log, dlg.m_bVerbose );
		m_dlg_log->AddLine("\r\n");
		s.LoadStringA(IDS_Done);
		m_dlg_log->AddLine( s );  
		ProjectModified( TRUE );
	}
}

void CFreePcbDoc::OnEditUndo()
{
	if( m_undo_list->m_num_items > 0 )
	{
		// undo last operation unless dragging something
		if( !m_view->CurDragging() )
		{
			while( m_undo_list->Pop() )
			{
			}
			m_view->CancelSelection();
			m_nlist->SetAreaConnections();
			m_view->Invalidate();
		}
		m_bLastPopRedo = FALSE;
		ProjectModified( TRUE, FALSE );
	}
}

void CFreePcbDoc::OnEditRedo()
{
	if( m_redo_list->m_num_items > 0 )
	{
		// redo last operation unless dragging something
		m_bLastPopRedo = TRUE;
		if( !m_view->CurDragging() )
		{
			while( m_redo_list->Pop() )
			{
			}
			m_view->CancelSelection();
			m_nlist->SetAreaConnections();
			m_view->Invalidate();
		}
		m_bLastPopRedo = TRUE;
		ProjectModified( TRUE, FALSE );
	}
}

void CFreePcbDoc::ResetUndoState()
{
	m_undo_list->Clear();
	m_redo_list->Clear();
	m_bLastPopRedo = FALSE;
}


void CFreePcbDoc::OnRepeatDrc()
{
	if( m_vis[LAY_RAT_LINE] && !m_auto_ratline_disable )
		m_nlist->OptimizeConnections();
	m_drelist->Clear();
	m_dlg_log->ShowWindow( SW_SHOW );   
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop(); 
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow(); 
	m_plist->DRC( m_dlg_log, m_num_copper_layers, m_units, 
		m_dr.bCheckUnrouted, &m_board_outline, &m_dr, m_drelist );
	m_view->Invalidate( FALSE );
}

void CFreePcbDoc::OnFileGenerateReportFile()
{
	CDlgReport dlg;
	dlg.Initialize( this ); 
	int ret = dlg.DoModal();
	if( ret = IDOK )
	{
		m_report_flags = dlg.m_flags;	// update flags
	}
}

void CFreePcbDoc::OnProjectCombineNets()
{
	CDlgNetCombine dlg;
	dlg.Initialize( m_nlist, m_plist );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// copy existing netlist
		CNetList * old_nlist = new CNetList( NULL, NULL );	// save to fix traces
		old_nlist->Copy( m_nlist );
		// combine nets under new name
		CString c_name = dlg.m_new_name;
		if( c_name == "" )
			ASSERT(0);
		CArray<CString> * names = &dlg.m_names;
		netlist_info nl_info;
		m_nlist->ExportNetListInfo( &nl_info );
		// now combine the nets
		// first, find the net and index for the combined net
		int c_index = -1;
		cnet * c_net = NULL;
		for( int in=0; in<nl_info.GetSize(); in++ )
		{
			CString nl_name = nl_info[in].name;
			if( nl_name == c_name )
			{
				c_index = in;
				c_net = nl_info[in].net;
				break;
			}
		}
		if( c_index == -1 )
			ASSERT(0);
		// now, combine the other nets
		for( int i=0; i<names->GetSize(); i++ )
		{
			CString name = (*names)[i];		// net name to combine
			int nl_index = -1;
			if( name != c_name )
			{
				// combine this net
				for( int in=0; in<nl_info.GetSize(); in++ )
				{
					CString nl_name = nl_info[in].name;
					if( nl_name == name )
					{
						nl_index = in;
						break;
					}
				}
				if( nl_index == -1 )
					ASSERT(0);
				for( int ip=0; ip<nl_info[nl_index].pin_name.GetSize(); ip++ )
				{
					CString * pname = &nl_info[nl_index].pin_name[ip];
					CString * pref = &nl_info[nl_index].ref_des[ip];
					int ipp = nl_info[c_index].pin_name.GetSize();
					nl_info[c_index].pin_name.SetAtGrow( ipp, *pname );
					nl_info[c_index].ref_des.SetAtGrow( ipp, *pref );
					nl_info[c_index].modified = TRUE;
				}
				nl_info[nl_index].deleted = TRUE;
			}
		}
		// show log dialog
		m_dlg_log->ShowWindow( SW_SHOW );
		m_dlg_log->UpdateWindow();
		m_dlg_log->BringWindowToTop();
		m_dlg_log->Clear();
		m_dlg_log->UpdateWindow();

		m_nlist->ImportNetListInfo( &nl_info, 0, m_dlg_log, 0, 0, 0 );
		m_import_flags = KEEP_TRACES | KEEP_STUBS | KEEP_AREAS	| KEEP_PARTS_AND_CON;
		m_nlist->RestoreConnectionsAndAreas( old_nlist, m_import_flags, NULL );
		delete old_nlist;
	}
}

void CFreePcbDoc::OnFileLoadLibrary()
{
	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();
	InitializeNewProject();		// set defaults

	// get project file name
	// force old-style file dialog by setting size of OPENFILENAME struct (for Win98)
	CString s ((LPCSTR) IDS_LibraryFiles);
	CFileDialog dlg( 1, "fpl", LPCTSTR(m_pcb_filename), 0, 
		s, NULL, 0 /* OPENFILENAME_SIZE_VERSION_400 */ );
	dlg.AssertValid();

	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_parent_folder;

	if( m_full_lib_dir != "" )
		dlg.m_ofn.lpstrInitialDir = m_lib_dir;

	// now show dialog
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		// read project file
		CString pathname = dlg.GetPathName();
		FileLoadLibrary( pathname );
	}
	else
	{
		// CANCEL or error
		DWORD dwError = ::CommDlgExtendedError();
		if( dwError )
		{
			CString str, s ((LPCSTR) IDS_FileOpenDialogErrorCode);
			str.Format( s, (unsigned long)dwError );
			AfxMessageBox( str );
		}
	}
}
void CFreePcbDoc::FileLoadLibrary( LPCTSTR pathname )
{
	BOOL bOK = FileOpen( pathname, TRUE );
	if( bOK )
	{
		// now add one instance of each footprint in cache
		POSITION pos;
		CString key;
		void * ptr;
		LPCSTR p;
		CShape *shape;
		CString ref;
		int i=1, x=0, y=0, max_height=0;
		for( pos = m_footprint_cache_map.GetStartPosition(); pos != NULL; )
		{
			m_footprint_cache_map.GetNextAssoc( pos, key, ptr );
			p = (LPCSTR)key;
			shape = (CShape*)ptr;
			ref.Format( "LIB_%d", i );
			cpart * part = m_plist->Add( shape, &ref, NULL, x, y, 0, 0, 1, 0 );
			// get bounding rectangle of pads and outline
			CRect shape_r = part->shape->GetBounds();
			int height = shape_r.top - shape_r.bottom;
			int width = shape_r.right - shape_r.left;
			// get dimensions of bounding rectangle for value text
			m_plist->SetValue( part, &shape->m_name, 
				shape_r.left, shape_r.top + part->m_ref_w, 0, 
				part->m_ref_size, part->m_ref_w, 1, LAY_SILK_TOP );
			CRect vr;
			vr = m_plist->GetValueRect( part );
			int value_width = vr.right - vr.left;
			// see if we can fit part between x and 8 inches
			int max_width = max( width, value_width );
			BOOL bFits = (x + max_width) < (8000*NM_PER_MIL);
			if( !bFits ) 
			{
				// start new row
				x = 0;
				y -= max_height;
				max_height = 0;
			}
			// move part so upper-left corner is at x,y
			m_plist->Move( part, x - shape_r.left, 
				y - shape_r.top - 2*part->m_ref_size, 0, 0 );
			// make ref invisible
			m_plist->ResizeRefText( part, part->m_ref_size, part->m_ref_w, 0 );
			// move value to top of part
			m_plist->SetValue( part, &shape->m_name, 
				shape_r.left, shape_r.top + part->m_ref_w, 0, 
				part->m_ref_size, part->m_ref_w, 1, part->m_ref_layer );
			m_plist->DrawPart( part );
			i++;
			x += max_width + 200*NM_PER_MIL;	// step right .2 inches
			max_height = max( max_height, height + 2*part->m_ref_size );
		}
		if( m_pcb_filename.Right(4) == ".fpl" )
		{
			int len = m_pcb_filename.GetLength();
			m_pcb_filename.SetAt(len-1, 'c');
			len = m_pcb_full_path.GetLength();
			m_pcb_full_path.SetAt(len-1, 'c');
		}
		else
		{
			m_pcb_filename = m_pcb_filename + ".fpc";
			m_pcb_full_path = *pathname + ".fpc";
		}
		CString s ((LPCSTR) IDS_FreePCBLibraryProject);
		m_window_title = s + m_pcb_filename;
		CWnd* pMain = AfxGetMainWnd();
		pMain->SetWindowText( m_window_title );
		m_view->OnViewAllElements();
		ProjectModified( FALSE );
	}
}

void CFreePcbDoc::OnFileSaveLibrary()
{
	CDlgSaveLib dlg;
	CArray<CString> names;
	cpart * part = m_plist->GetFirstPart();
	int i = 0;
	while( part )
	{
		names.SetAtGrow( i, part->value );
		part = m_plist->GetNextPart( part );
		i++;
	}
	dlg.Initialize( &names );
	int ret = dlg.DoModal();
}


// CPT

static double routingGridVals[] = { 
		1*NM_PER_MIL,  2*NM_PER_MIL, 2.5*NM_PER_MIL, 3.333333333333*NM_PER_MIL, 4*NM_PER_MIL, 
		5*NM_PER_MIL,  6.6666666667*NM_PER_MIL, 10*NM_PER_MIL, 12.5*NM_PER_MIL, 16.66666666666*NM_PER_MIL, 
		20*NM_PER_MIL, 25*NM_PER_MIL, 40*NM_PER_MIL, 50*NM_PER_MIL, 100*NM_PER_MIL, 
		-10000, -20000, -40000, -50000, -100000,
		-200000, -250000, -400000, -500000,	-1000000, 
		-2000000, -2500000, -4000000, -5000000, -10000000
		};

#define cDefaultLayerColors 17
static unsigned int defaultLayerColors[] = {
	0xffffff, 0x000000, 0xffffff, 0xffffff, // selection WHITE, backgrnd BLACK, vis grid WHITE, highlight WHITE
	0xff8040, 0x0000ff, 0xff00ff, 0xffff00, // DRE orange, board outline BLUE, ratlines VIOLET, top silk YELLOW
	0xffc0c0, 0xa0a0a0, 0x5f5f5f, 0x0000ff, // bottom silk PINK, top sm cutout LT GRAY, bottom sm cutout DK GRAY, thru-hole pads BLUE
	0x00ff00, 0xff0000, 0x408040, 0x804040, // top copper GREEN, bottom copper RED, inner 1, inner 2
	0x404080
};

static unsigned int defaultFpLayerColors[] = {
	0xffffff, 0x000000, 0xffffff, 0xffffff, // selection WHITE, backgrnd BLACK, vis grid WHITE, highlight WHITE
	0xffff00, 0xffffff, 0xff8040, 0x0000ff, // silk top YELLOW, centroid WHITE, dot ORANGE, pad-thru BLUE
	0x007f00, 0x007f00, 0x7f0000, 0x7f0000, // Top mask+paste DK GREEN, bottom mask+paste DK RED
	0x00ff00, 0x5f5f5f, 0xff0000            // top copper GREEN, inner copper GRAY, bottom copper RED
};

void CFreePcbDoc::ReadPrefs() {
	// Read preferences from the registry (fReversePgupPgDn for reversing page-up/page-down;  preferred routing grid values).
	// This supersedes the routing grid values in default.cfg, a file I wasn't really aware of when writing this.  Well, perhaps
	// the ultimate goal should be moving everything into the registry?  Hard to decide.
	// Newer:  deal also with default layer colors
	HKEY hkey;
	char buf[16], buf2[16];
	LONG cb = 10, val;

	fReversePgupPgdn = false;
	memset(fGridFlags, 1, 30);

	if (RegCreateKey(HKEY_CURRENT_USER, "Software\\FreePCB", &hkey) == ERROR_SUCCESS) {
		bool fOK = RegQueryValue(hkey, "fReversePgupPgdn", buf, &cb) == ERROR_SUCCESS;
		if (fOK) fReversePgupPgdn = atoi(buf);
		for (int i=0; i<30; i++) {
			sprintf(buf2, "fGridFlags%d", i);
			cb = 10;
			fOK = RegQueryValue(hkey, buf2, buf, &cb) == ERROR_SUCCESS;
			if (fOK) fGridFlags[i] = atoi(buf);
			}
		for( int i=0; i<MAX_LAYERS; i++ ) {
			sprintf(buf2, "layerColor%d", i);
			cb = 10;
			fOK = RegQueryValue(hkey, buf2, buf, &cb) == ERROR_SUCCESS;
			if (fOK)
				val = atoi(buf);
			else if (i<cDefaultLayerColors)
				val = defaultLayerColors[i];
			else
				val = 0x808080;
			m_rgb[i][0] = (val>>16) & 0xff;
			m_rgb[i][1] = (val>>8) & 0xff;
			m_rgb[i][2] = val & 0xff;
			}
		for( int i=0; i<NUM_FP_LAYERS; i++ ) {
			sprintf(buf2, "fpLayerColor%d", i);
			cb = 10;
			fOK = RegQueryValue(hkey, buf2, buf, &cb) == ERROR_SUCCESS;
			if (fOK)
				val = atoi(buf);
			else
				val = defaultFpLayerColors[i];
			m_fp_rgb[i][0] = (val>>16) & 0xff;
			m_fp_rgb[i][1] = (val>>8) & 0xff;
			m_fp_rgb[i][2] = val & 0xff;
			}

		RegCloseKey(hkey);
		}

	m_routing_grid.RemoveAll();
	for (int i=0; i<30; i++)
		if (fGridFlags[i]) 
			m_routing_grid.InsertAt(0, routingGridVals[i]);		// Reverse the list compared to the old order...
	}

void CFreePcbDoc::SavePrefs() {
	// Save preferences to the registry.  Runs after OK is hit in the Prefs Dlg.
	HKEY hkey;
	char buf[16], buf2[16];

	if (RegCreateKey(HKEY_CURRENT_USER, "Software\\FreePCB", &hkey) != ERROR_SUCCESS) return;
	sprintf(buf, "%d", fReversePgupPgdn);
	RegSetValue(hkey, "fReversePgupPgdn", REG_SZ, buf, strlen(buf));
	for (int i=0; i<30; i++) {
		sprintf(buf, "fGridFlags%d", i);
		sprintf(buf2, "%d", fGridFlags[i]);
		RegSetValue(hkey, buf, REG_SZ, buf2, strlen(buf2));
		}
	RegCloseKey(hkey);
	}

void CFreePcbDoc::OnToolsPreferences() {
	CDlgPrefs dlg;
	dlg.doc = this;
	dlg.fReverse = fReversePgupPgdn;
	memmove(dlg.fGridFlags, fGridFlags, 30);
	int ret = dlg.DoModal();
	if( ret == IDOK ) {
		fReversePgupPgdn = dlg.fReverse;
		memmove(&fGridFlags, &dlg.fGridFlags, 30); 
		m_routing_grid.RemoveAll();
		for (int i=0; i<30; i++) 
			if (fGridFlags[i]) 
				m_routing_grid.InsertAt(0, routingGridVals[i]);
		SavePrefs();
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
			m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, m_units );
		}
	}
