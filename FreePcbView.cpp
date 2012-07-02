// FreePcbView.cpp : implementation of the CFreePcbView class
//

#include "stdafx.h"
#include "DlgAddText.h"
#include "DlgAssignNet.h"
#include "DlgSetSegmentWidth.h"
#include "DlgEditBoardCorner.h"
#include "DlgAddArea.h"
#include "DlgRefText.h"
#include "DlgAddPart.h"
#include "DlgSetAreaHatch.h"
#include "DlgDupFootprintName.h" 
#include "DlgFindPart.h"
#include "DlgAddMaskCutout.h"
#include "DlgChangeLayer.h"
#include "DlgEditNet.h"
#include "DlgMoveOrigin.h"
#include "DlgMyMessageBox.h"
#include "DlgVia.h"
#include "DlgAreaLayer.h"
#include "DlgGroupPaste.h"
#include "DlgSideStyle.h"
#include "DlgValueText.h"
#include "PartListNew.h"


// globals
extern CFreePcbApp theApp;
BOOL t_pressed = FALSE;
BOOL n_pressed = FALSE;
// CPT:  removed gShiftKeyDown global. Other globals moved into class CCommonView
// BOOL gShiftKeyDown = FALSE;
// int gTotalArrowMoveX = 0;
// int gTotalArrowMoveY = 0;
// BOOL gLastKeyWasArrow = FALSE;
// BOOL gLastKeyWasGroupRotate = FALSE;
// end CPT
long long groupAverageX=0, groupAverageY=0;
int groupNumberItems=0;

HCURSOR my_cursor = LoadCursor( NULL, IDC_CROSS );

int CFreePcbView::sel_mask_btn_bits[16] = { 0 };

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_HITS   500		// max number of items selected

// macro for approximating angles to 1 degree accuracy
#define APPROX(angle,ref) ((angle > ref-M_PI/360) && (angle < ref+M_PI/360))

//** these must be changed if context menu is edited
enum {
	CONTEXT_NONE = 0,
	CONTEXT_PART,
	CONTEXT_REF_TEXT,
	CONTEXT_PAD,
	CONTEXT_SEGMENT,
	CONTEXT_RATLINE,
	CONTEXT_VERTEX,
	CONTEXT_TEXT,
	CONTEXT_AREA_CORNER,
	CONTEXT_AREA_EDGE,
	CONTEXT_BOARD_CORNER,
	CONTEXT_BOARD_SIDE,
	CONTEXT_END_VERTEX,
	CONTEXT_FP_PAD,
	CONTEXT_SM_CORNER,
	CONTEXT_SM_SIDE,
	CONTEXT_CONNECT,
	CONTEXT_NET,
	CONTEXT_GROUP,
	CONTEXT_VALUE_TEXT
};

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView

IMPLEMENT_DYNCREATE(CFreePcbView, CView)

BEGIN_MESSAGE_MAP(CFreePcbView, CView)
	//{{AFX_MSG_MAP(CFreePcbView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	// Standard printing commands
//	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
//	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
//	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
//	ON_WM_SYSCHAR()
//  ON_WM_SYSCOMMAND()
ON_WM_CONTEXTMENU()
ON_COMMAND(ID_PART_MOVE, OnPartMove)
ON_COMMAND(ID_NONE_ADDTEXT, OnTextAdd)
ON_COMMAND(ID_TEXT_DELETE, OnTextDelete)
ON_COMMAND(ID_TEXT_MOVE, OnTextMove)
ON_COMMAND(ID_PART_GLUE, OnPartGlue)
ON_COMMAND(ID_PART_UNGLUE, OnPartUnglue)
ON_COMMAND(ID_PART_DELETE, OnPartDelete)
ON_COMMAND(ID_PART_OPTIMIZE, OnPartOptimize)
ON_COMMAND(ID_REF_MOVE, OnRefMove)
ON_COMMAND(ID_PAD_OPTIMIZERATLINES, OnPadOptimize)
ON_COMMAND(ID_PAD_ADDTONET, OnPadAddToNet)
ON_COMMAND(ID_PAD_DETACHFROMNET, OnPadDetachFromNet)
ON_COMMAND(ID_PAD_CONNECTTOPIN, OnPadStartRatline)
ON_COMMAND(ID_SEGMENT_SETWIDTH, OnSegmentSetWidth)
ON_COMMAND(ID_SEGMENT_UNROUTE, OnSegmentUnroute)
ON_COMMAND(ID_RATLINE_ROUTE, OnRatlineRoute)
ON_COMMAND(ID_RATLINE_OPTIMIZE, OnRatlineOptimize)
ON_COMMAND(ID_VERTEX_MOVE, OnVertexMove)
ON_COMMAND(ID_VERTEX_DELETE, OnVertexDelete)
ON_COMMAND(ID_VERTEX_SETSIZE, OnVertexProperties)
ON_COMMAND(ID_RATLINE_COMPLETE, OnRatlineComplete)
ON_COMMAND(ID_RATLINE_SETWIDTH, OnRatlineSetWidth)
ON_COMMAND(ID_RATLINE_DELETECONNECTION, OnRatlineDeleteConnection)
ON_COMMAND(ID_RATLINE_LOCKCONNECTION, OnRatlineLockConnection)
ON_COMMAND(ID_RATLINE_UNLOCKCONNECTION, OnRatlineUnlockConnection)
ON_COMMAND(ID_TEXT_EDIT, OnTextEdit)
ON_COMMAND(ID_ADD_BOARDOUTLINE, OnAddBoardOutline)
ON_COMMAND(ID_NONE_ADDBOARDOUTLINE, OnAddBoardOutline)
ON_COMMAND(ID_BOARDCORNER_MOVE, OnBoardCornerMove)
ON_COMMAND(ID_BOARDCORNER_EDIT, OnBoardCornerEdit)
ON_COMMAND(ID_BOARDCORNER_DELETECORNER, OnBoardCornerDelete)
ON_COMMAND(ID_BOARDCORNER_DELETEOUTLINE, OnBoardDeleteOutline)
ON_COMMAND(ID_BOARDSIDE_INSERTCORNER, OnBoardSideAddCorner)
ON_COMMAND(ID_BOARDSIDE_DELETEOUTLINE, OnBoardDeleteOutline)
ON_COMMAND(ID_PAD_STARTSTUBTRACE, OnPadStartTrace)
ON_COMMAND(ID_SEGMENT_DELETE, OnSegmentDelete)
ON_COMMAND(ID_ENDVERTEX_MOVE, OnEndVertexMove)
ON_COMMAND(ID_ENDVERTEX_ADDSEGMENTS, OnVertexStartTrace)
ON_COMMAND(ID_ENDVERTEX_ADDCONNECTION, OnVertexStartRatline)
ON_COMMAND(ID_ENDVERTEX_DELETE, OnEndVertexDelete)
ON_COMMAND(ID_ENDVERTEX_EDIT, OnEndVertexEdit)
ON_COMMAND(ID_AREACORNER_MOVE, OnAreaCornerMove)
ON_COMMAND(ID_AREACORNER_DELETE, OnAreaCornerDelete)
ON_COMMAND(ID_AREACORNER_DELETEAREA, OnAreaCornerDeleteArea)
ON_COMMAND(ID_AREAEDGE_ADDCORNER, OnAreaSideAddCorner)
ON_COMMAND(ID_AREAEDGE_DELETE, OnAreaSideDeleteArea)
ON_COMMAND(ID_AREAEDGE_DELETECUTOUT, OnAreaDeleteCutout)
ON_COMMAND(ID_AREACORNER_DELETECUTOUT, OnAreaDeleteCutout)
ON_COMMAND(ID_ADD_AREA, OnAddArea)
ON_COMMAND(ID_NONE_ADDCOPPERAREA, OnAddArea)
ON_COMMAND(ID_ENDVERTEX_ADDVIA, OnEndVertexAddVia)
ON_COMMAND(ID_ENDVERTEX_REMOVEVIA, OnEndVertexRemoveVia)
ON_COMMAND(ID_ENDVERTEX_SETSIZE, OnVertexProperties)
ON_MESSAGE( WM_USER_VISIBLE_GRID, OnChangeVisibleGrid )
ON_COMMAND(ID_ADD_TEXT, OnTextAdd)
ON_COMMAND(ID_SEGMENT_DELETETRACE, OnSegmentDeleteTrace)
ON_COMMAND(ID_AREACORNER_PROPERTIES, OnAreaCornerProperties)
ON_COMMAND(ID_REF_PROPERTIES, OnRefProperties)
ON_COMMAND(ID_VERTEX_PROPERITES, OnVertexProperties)
ON_WM_ERASEBKGND()
ON_COMMAND(ID_BOARDSIDE_CONVERTTOSTRAIGHTLINE, OnBoardSideConvertToStraightLine)
ON_COMMAND(ID_BOARDSIDE_CONVERTTOARC_CW, OnBoardSideConvertToArcCw)
ON_COMMAND(ID_BOARDSIDE_CONVERTTOARC_CCW, OnBoardSideConvertToArcCcw)
ON_COMMAND(ID_SEGMENT_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_VERTEX_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_VIEW_ENTIREBOARD, OnViewEntireBoard)
ON_COMMAND(ID_VIEW_ALLELEMENTS, OnViewAllElements)
ON_COMMAND(ID_AREAEDGE_HATCHSTYLE, OnAreaEdgeHatchStyle)
ON_COMMAND(ID_PART_EDITFOOTPRINT, OnPartEditThisFootprint)
ON_COMMAND(ID_PART_SET_REF, OnRefProperties)
ON_COMMAND(ID_RATLINE_CHANGEPIN, OnRatlineChangeEndPin)
ON_WM_KEYUP()
ON_COMMAND(ID_VIEW_FINDPART, OnViewFindpart)
ON_COMMAND(ID_NONE_FOOTPRINTWIZARD, OnFootprintWizard)
ON_COMMAND(ID_NONE_FOOTPRINTEDITOR, OnFootprintEditor)
ON_COMMAND(ID_NONE_CHECKPARTSANDNETS, OnCheckPartsAndNets)
ON_COMMAND(ID_NONE_DRC, OnDrc)
ON_COMMAND(ID_NONE_CLEARDRCERRORS, OnClearDRC)
ON_COMMAND(ID_NONE_VIEWALL, OnViewAll)
ON_COMMAND(ID_ADD_SOLDERMASKCUTOUT, OnAddSoldermaskCutout)
ON_COMMAND(ID_SMCORNER_MOVE, OnSmCornerMove)
ON_COMMAND(ID_SMCORNER_SETPOSITION, OnSmCornerSetPosition)
ON_COMMAND(ID_SMCORNER_DELETECORNER, OnSmCornerDeleteCorner)
ON_COMMAND(ID_SMCORNER_DELETECUTOUT, OnSmCornerDeleteCutout)
ON_COMMAND(ID_SMSIDE_INSERTCORNER, OnSmSideInsertCorner)
ON_COMMAND(ID_SMSIDE_HATCHSTYLE, OnSmSideHatchStyle)
ON_COMMAND(ID_SMSIDE_DELETECUTOUT, OnSmSideDeleteCutout)
ON_COMMAND(ID_PART_CHANGESIDE, OnPartChangeSide)
ON_COMMAND(ID_PART_ROTATE, OnPartRotate)
ON_COMMAND(ID_AREAEDGE_ADDCUTOUT, OnAreaAddCutout)
ON_COMMAND(ID_AREACORNER_ADDCUTOUT, OnAreaAddCutout)
ON_COMMAND(ID_NET_SETWIDTH, OnNetSetWidth)
ON_COMMAND(ID_CONNECT_SETWIDTH, OnConnectSetWidth)
ON_COMMAND(ID_CONNECT_UNROUTETRACE, OnConnectUnroutetrace)
ON_COMMAND(ID_CONNECT_DELETETRACE, OnConnectDeletetrace)
ON_COMMAND(ID_SEGMENT_CHANGELAYER, OnSegmentChangeLayer)
ON_COMMAND(ID_SEGMENT_ADDVERTEX, OnSegmentAddVertex)
ON_COMMAND(ID_CONNECT_CHANGELAYER, OnConnectChangeLayer)
ON_COMMAND(ID_NET_CHANGELAYER, OnNetChangeLayer)
ON_COMMAND(ID_NET_EDITNET, OnNetEditnet)
ON_COMMAND(ID_TOOLS_MOVEORIGIN, OnToolsMoveOrigin)
ON_WM_LBUTTONUP()
ON_COMMAND(ID_GROUP_MOVE, OnGroupMove)
ON_COMMAND(ID_AREACORNER_ADDNEWAREA, OnAddSimilarArea)
ON_COMMAND(ID_AREAEDGE_ADDNEWAREA, OnAddSimilarArea)
ON_COMMAND(ID_AREAEDGE_CHANGELAYER, OnAreaEdit)
ON_COMMAND(ID_AREACORNER_CHANGELAYER, OnAreaEdit)
ON_COMMAND(ID_AREAEDGE_APPLYCLEARANCES, OnAreaEdgeApplyClearances)
ON_COMMAND(ID_GROUP_SAVETOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_GROUP_COPY, OnGroupCopy)
ON_COMMAND(ID_GROUP_CUT, OnGroupCut)
ON_COMMAND(ID_GROUP_DELETE, OnGroupDelete)
ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
ON_COMMAND(ID_VERTEX_CONNECTTOPIN, OnVertexStartRatline)
ON_COMMAND(ID_EDIT_CUT, OnEditCut)
ON_COMMAND(ID_EDIT_SAVEGROUPTOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_GROUP_ROTATE, OnGroupRotate)
ON_WM_SETCURSOR()
ON_WM_MOVE()
ON_COMMAND(ID_REF_SHOWPART, OnRefShowPart)
ON_COMMAND(ID_AREA_SIDESTYLE, OnAreaSideStyle)
ON_COMMAND(ID_VALUE_MOVE, OnValueMove)
ON_COMMAND(ID_VALUE_CHANGESIZE, OnValueProperties)
ON_COMMAND(ID_VALUE_SHOWPART, OnValueShowPart)
ON_COMMAND(ID_PART_EDITVALUE, OnPartEditValue)
ON_COMMAND(ID_PART_ROTATECOUNTERCLOCKWISE, OnPartRotateCCW)
ON_COMMAND(ID_REF_ROTATECW, OnRefRotateCW)
ON_COMMAND(ID_REF_ROTATECCW, OnRefRotateCCW)
ON_COMMAND(ID_VALUE_ROTATECW, OnValueRotateCW)
ON_COMMAND(ID_VALUE_ROTATECCW, OnValueRotateCCW)
ON_COMMAND(ID_SEGMENT_MOVE, OnSegmentMove)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView construction/destruction

CFreePcbView::CFreePcbView()
{
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	CalibrateTimer();
	m_lastKeyWasArrow = m_lastKeyWasGroupRotate = FALSE;		// CPT
	m_highlight_net = NULL;

	// CPT:  put the following into the constructor (was in InitInstance()).
	// set up arrays of mask ids
	// CPT2.  Adapted for new system with type bits.
	sel_mask_btn_bits[SEL_MASK_PARTS] = bitPart;
	sel_mask_btn_bits[SEL_MASK_REF] = bitRefText;
	sel_mask_btn_bits[SEL_MASK_VALUE] = bitValueText;
	sel_mask_btn_bits[SEL_MASK_PINS] = bitPin;
	sel_mask_btn_bits[SEL_MASK_CON] = bitSeg;
	sel_mask_btn_bits[SEL_MASK_VIA] = bitVia | bitTraceVtx | bitTee;
	sel_mask_btn_bits[SEL_MASK_AREAS] = bitAreaCorner | bitAreaSide;
	sel_mask_btn_bits[SEL_MASK_TEXT] = bitText;
	sel_mask_btn_bits[SEL_MASK_SM] = bitSmCorner | bitSmSide;
	sel_mask_btn_bits[SEL_MASK_BOARD] = bitBoardCorner | bitBoardSide;
	sel_mask_btn_bits[SEL_MASK_DRC] = bitDRE;
	m_sel_mask = 0xffffffff;
	m_sel_mask_bits = 0xffffffff;
	// end CPT
}

// initialize the view object
// this code can't be placed in the constructor, because it depends on document
// don't try to draw window until this function has been called
// need only be called once
//
void CFreePcbView::InitInstance()
{
	// this should be called from InitInstance function of CApp,
	// after the document is created.  CPT:  factored out, preparatory to the CCommonView reorganization
	OnNewProject();
	ShowSelectStatus();
	ShowActiveLayer();
	m_doc->m_view = this;
	// end CPT
}

// initialize view with defaults for a new project
// should be called each time a new project is created
// CPT:  reorganized & renamed as part of the CCommonView reorganization (used to be called InitializeView())
void CFreePcbView::OnNewProject()
{
	BaseInit();
	// CFreePcbView specific defaults
	m_sel_layer = 0;
	m_dir = 0;
	m_active_layer = LAY_TOP_COPPER;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	m_inflection_mode = IM_90_45;
	m_snap_mode = SM_GRID_POINTS;
	m_units = m_doc->m_units;
	m_highlight_net = NULL;		// AMW
}


// destructor
CFreePcbView::~CFreePcbView()
{
}


/////////////////////////////////////////////////////////////////////////////
// CFreePcbView drawing

void CFreePcbView::OnDraw(CDC* pDC)
{
	// get client rectangle
	GetClientRect( &m_client_r );

	// clear screen to black if no project open
	if( !m_doc )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}
	if( !m_doc->m_project_open )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}

	// CPT - moved code to draw left pane into DrawLeftPane()
	// draw stuff on left pane
	DrawLeftPane(pDC);
	// draw function keys on bottom pane
	DrawBottomPane(pDC);
	// end CPT

	//** this is for testing only, needs to be converted to PCB coords
#if 0
	if( b_update_rect )
	{
		// clip to update rectangle
		CRgn rgn;
		rgn.CreateRectRgn( m_left_pane_w + update_rect.left,
			update_rect.bottom,
			m_left_pane_w + update_rect.right,
			update_rect.top  );
		pDC->SelectClipRgn( &rgn );
	}
	else
#endif
	{
		// clip to pcb drawing region
		pDC->SelectClipRgn( &m_pcb_rgn );
	}

	// now draw the display list
	SetDCToWorldCoords( pDC );
	m_dlist->Draw( pDC );

	// CPT After an autoscroll, this routine is called, and at the end we have to redraw the drag rectangle:
	if (m_bDraggingRect) {
		SIZE s1;
		s1.cx = s1.cy = 1;
		pDC->IntersectClipRect(m_left_pane_w, 0, m_client_r.right, m_client_r.bottom - m_bottom_pane_h);
		pDC->DrawDragRect(m_drag_rect, s1, NULL, s1);
		m_last_drag_rect = m_drag_rect;
		m_bDontDrawDragRect = false;
	}
	// end CPT
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView printing

BOOL CFreePcbView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFreePcbView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFreePcbView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


/////////////////////////////////////////////////////////////////////////////
// CFreePcbView message handlers

// select item from id sid
// sets m_sel_id and pointer to top level item 
// returns TRUE if successful
//

#ifndef CPT2
// CPT2 PHASED OUT.
BOOL CFreePcbView::SelectItem( id sid )
{

	if( m_bNetHighlighted )
		CancelHighlightNet();

	if( sid.IsDRC() )
	{
#if 0
		CancelSelection();
		DRError * dre = (DRError*)ptr;
		m_sel_id = sid;
		m_sel_dre = dre;
		m_doc->m_drelist->Highlight( m_sel_dre );
		SetCursorMode( CUR_DRE_SELECTED );
		Invalidate( FALSE );
#endif
	}
	else if( sid.IsBoardCorner() )
	{
		CancelSelection();
		m_doc->m_board_outline[sid.I2()].HighlightCorner( sid.I3() );
		m_sel_id = sid;
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
		Invalidate( FALSE );
	}
	else if( sid.IsBoardSide() )
	{
		CancelSelection();
		m_doc->m_board_outline[sid.I2()].HighlightSide( sid.I3() );
		m_sel_id = sid;
		SetCursorMode( CUR_BOARD_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( sid.IsMaskCorner() )
	{
		CancelSelection();
		m_doc->m_sm_cutout[sid.I2()].HighlightCorner( sid.I3() );
		m_sel_id = sid;
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
		Invalidate( FALSE );
	}
	else if( sid.IsMaskSide() )
	{
		CancelSelection();
		m_doc->m_sm_cutout[sid.I2()].HighlightSide( sid.I3() );
		m_sel_id = sid;
		SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( sid.T1() == ID_PART )
	{
		CancelSelection();
		m_sel_part = sid.Part();
		m_sel_id = sid;
#if 0	// AMW r272: can't select net, just highlight it
		if( (GetKeyState('N') & 0x8000) && sid.T2()  == ID_SEL_PAD )
		{
			// pad selected and if "n" held down, select net
			cnet * net = m_doc->m_plist->GetPinNet( m_sel_part, sid.I2() );
			if( net )
			{
				m_sel_net = sid.Net();
				m_sel_id = net->m_id;
				m_sel_id.SetT2( ID_ENTIRE_NET );
				m_doc->m_nlist->HighlightNet( m_sel_net );
				m_doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
				SetCursorMode( CUR_NET_SELECTED );
			}
			else
			{
				m_doc->m_plist->HighlightPad( m_sel_part, sid.I2() );
				SetCursorMode( CUR_PAD_SELECTED );
				Invalidate( FALSE );
			}
		}
		else if( sid.IsPart() )
#endif
		if( sid.IsPart() )		// AMW r272
		{
			SelectPart( m_sel_part );
			m_doc->m_plist->SelectRefText( m_sel_part );
			m_doc->m_plist->SelectValueText( m_sel_part );
		}
		else if( sid.IsRefText() )
		{
			m_doc->m_plist->SelectRefText( m_sel_part );
			SetCursorMode( CUR_REF_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.IsValueText() )
		{
			m_doc->m_plist->SelectValueText( m_sel_part );
			SetCursorMode( CUR_VALUE_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.IsPin() )
		{
			m_doc->m_plist->HighlightPad( m_sel_part, sid.I2() );
			SetCursorMode( CUR_PAD_SELECTED );
			Invalidate( FALSE );
		}
	}
	else if( sid.T1() == ID_NET )
	{
		CancelSelection();
		m_sel_net = sid.Net();
		m_sel_id = sid;
		if( sid.T2()  == ID_CONNECT && sid.T3() == ID_SEL_SEG )
		{
			// select segment
			m_doc->m_nlist->HighlightSegment( m_sel_net, sid.I2(), sid.I3() );
			if( m_sel_seg->m_layer != LAY_RAT_LINE )
				SetCursorMode( CUR_SEG_SELECTED );
			else
				SetCursorMode( CUR_RAT_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.T2()  == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
		{
			// select vertex
			cvertex * v = m_sel_id.Vtx();
			if( v->GetType() == cvertex::V_END )
				SetCursorMode( CUR_END_VTX_SELECTED );
			else
				SetCursorMode( CUR_VTX_SELECTED );
			m_doc->m_nlist->HighlightVertex( m_sel_net, sid.I2(), sid.I3() );
			Invalidate( FALSE );
		}
		else if( sid.T2()  == ID_AREA && sid.T3() == ID_SEL_SIDE )
		{
			// select copper area side
			m_doc->m_nlist->SelectAreaSide( m_sel_net, sid.I2(), sid.I3() );
			SetCursorMode( CUR_AREA_SIDE_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.T2()  == ID_AREA && sid.T3() == ID_SEL_CORNER )
		{
			// select copper area corner
			m_doc->m_nlist->SelectAreaCorner( m_sel_net, sid.I2(), sid.I3() );
			SetCursorMode( CUR_AREA_CORNER_SELECTED );
			Invalidate( FALSE );
		}
		else if( sid.T2()  == ID_CONNECT && sid.T3() == ID_ENTIRE_CONNECT )
		{
			// CPT:  account for the possibility of whole-trace selection.
			m_doc->m_nlist->HighlightConnection( m_sel_net, sid.I2() );
			SetCursorMode( CUR_CONNECT_SELECTED );
			Invalidate( FALSE );
			// end CPT
		}
		else
			ASSERT(0);
	}
	else if( sid.T1() == ID_TEXT )
	{
		CancelSelection();
		m_sel_text = sid.Text();
		m_sel_id = sid;
		m_doc->m_tlist->HighlightText( m_sel_text );
		SetCursorMode( CUR_TEXT_SELECTED );
		Invalidate( FALSE );
	}
	else
	{
		// nothing selected
		return FALSE;
		CancelSelection();
		m_sel_id.Clear();
		Invalidate( FALSE );
	}
	return TRUE;
}
#endif

void CFreePcbView::SelectItem(cpcb_item *item) 
{
	// CPT2.  Convenience method that clears the selection array, adds "item" as its sole new member, then calls HighlightSelection()
	m_sel.RemoveAll();
	m_sel.Add(item);
	HighlightSelection();
}

// Displays a popup menu for the mouse hits in hit_info
//
// Param:
//	point    - current mouse position (relative to client window)
// CPT r294: removed hit-info args (using m_hit_info instead).  Reorganized and tidied up.
int CFreePcbView::SelectObjPopup( CPoint const &point )
{
	CDC *winDC = GetDC();

	CDC dc;
	dc.CreateCompatibleDC(winDC);
	dc.SetMapMode(MM_TEXT);
	dc.SetWindowExt( 1,1 );
	dc.SetWindowOrg( 0,0 );
	dc.SetViewportExt( 1,1 );
	dc.SetViewportOrg( 0,0 );

	int sel = 0;
	int num_hits = m_hit_info.GetCount();
	if (num_hits>25) num_hits = 25;					// m_hit_info has unlimited size...

	// Create bitmap array
	CArray<CBitmap> bitmaps;
	bitmaps.SetSize(num_hits);
	CString str;
	CMenu file_menu;
	file_menu.CreatePopupMenu();

	for( int idx = 0; idx < num_hits; idx++ )
	{
		CHitInfo *pInfo = &m_hit_info[idx];

		// Don't display masked items.  CPT r294:  obsolete, removed
		// if( info.priority < 0 )
		//    break;

		CRect r(0,0, 139,23);
		CBitmap *pBitmap = &bitmaps[idx];
		pBitmap->CreateCompatibleBitmap(winDC, r.Width()+1, r.Height()+1);
		CBitmap *pOldBitmap = dc.SelectObject(pBitmap);
		COLORREF layer_color = C_RGB( m_doc->m_rgb[ pInfo->layer ][0],
										m_doc->m_rgb[ pInfo->layer ][1],
										m_doc->m_rgb[ pInfo->layer ][2] );
		COLORREF text_color  = C_RGB(m_doc->m_rgb[ LAY_BACKGND ][0],
										m_doc->m_rgb[ LAY_BACKGND ][1],
										m_doc->m_rgb[ LAY_BACKGND ][2] );

		dc.FillSolidRect(r, layer_color);
		dc.SetTextColor(text_color);

		if( pInfo->ID.T1() == ID_BOARD )
		{
			str.LoadStringA(IDS_Board);
			if( pInfo->ID.T3() == ID_SEL_SIDE )
			{
				CString s ((LPCSTR) IDS_Side3);
				str += s;
			}
			else if( pInfo->ID.T3() == ID_SEL_CORNER )
			{
				CString s ((LPCSTR) IDS_Corner3);
				str += s;
			}
		}
		else if( pInfo->ID.T1() == ID_PART )
		{
			cpart *part = (cpart*)pInfo->ptr;

			if( pInfo->ID.T2() == ID_SEL_PAD )
			{
				CString s ((LPCSTR) IDS_Pin3);
				str.Format( s, part->ref_des, part->shape->GetPinNameByIndex(pInfo->ID.I2()) );
			}
			else if( pInfo->ID.T2() == ID_SEL_RECT )
			{
				str = "";
				CShape *shape = part->shape;
				if( shape )
				{
					CMetaFileDC m_mfDC;

					CRect shape_bounds = shape->GetBounds();
					int dx = -shape_bounds.Height() / NM_PER_MIL;

					// Scale part bitmap height between 40 and 128 for better readability
					r.bottom = 32 + dx / 11;
					if( r.bottom > 128 ) r.bottom = 128;

					// Trade in the default bitmap for the new one
					dc.SelectObject(pOldBitmap);
					pBitmap->DeleteObject();
					pBitmap->CreateCompatibleBitmap(winDC, r.Width()+1, r.Height()+1);
					dc.SelectObject(pBitmap);

					// Draw the shape with actual ref_des & no selection rectangle
					HENHMETAFILE hMF = shape->CreateMetafile( &m_mfDC, winDC, r, part->ref_des, FALSE );
					dc.PlayMetaFile( hMF, r );
					DeleteEnhMetaFile( hMF );
				}
			}
			else if( pInfo->ID.T2() == ID_REF_TXT )
			{
				CString s ((LPCSTR) IDS_Ref3);
				str.Format(s, part->ref_des);
			}
			else if( pInfo->ID.T2() == ID_VALUE_TXT )
			{
				CString s ((LPCSTR) IDS_Value3);
				str.Format(s, part->value);
			}
		}
		else if( pInfo->ID.T1() == ID_NET )
		{
			cnet *net = (cnet*)pInfo->ptr;

			if( pInfo->ID.T2() == ID_CONNECT )
			{
				if( pInfo->ID.T3() == ID_SEL_SEG )
					str.LoadStringA(IDS_Segment3);
				else if( pInfo->ID.T3() == ID_SEL_VERTEX )
				{
					if( net->ConByIndex(pInfo->ID.I2())->VtxByIndex(pInfo->ID.I3()).via_w )
						str.LoadStringA(IDS_Via3);
					else
						str.LoadStringA(IDS_Vertex3);
				}
			}
			else if( pInfo->ID.T2() == ID_AREA )
			{
				str.LoadStringA(IDS_Copper3);
				if( pInfo->ID.T3() == ID_SEL_SIDE )
				{
					CString s ((LPCSTR) IDS_Side3);
					str += s;
				}
				else if( pInfo->ID.T3() == ID_SEL_CORNER )
				{
					CString s ((LPCSTR) IDS_Corner3);
					str += s;
				}
			}
		}
		else if( pInfo->ID.T1() == ID_TEXT )
		{
			CText *text = (CText*)pInfo->ptr;
			CString s ((LPCSTR) IDS_Text3);
			str.Format(s, text->m_str);
		}
		else if( pInfo->ID.T1() == ID_DRC )
			str.LoadStringA(IDS_DRC3);
		else if( pInfo->ID.T1() == ID_MASK )
		{
			str.LoadStringA(IDS_Cutout3);
			if( pInfo->ID.T3() == ID_SEL_SIDE )
			{
				CString s ((LPCSTR) IDS_Side3);
				str += s;
			}
			else if( pInfo->ID.T3() == ID_SEL_CORNER )
			{
				CString s ((LPCSTR) IDS_Corner3);
				str += s;
			}
		}
		else if( pInfo->ID.T1() == ID_CENTROID )
			str.LoadStringA(IDS_Centroid3);
		else if( pInfo->ID.T1() == ID_GLUE )
			str.LoadStringA(IDS_GlueSpot3);
		else
			str = "Unknown";

		if( str.GetLength() > 0 )
			dc.TextOut( 10,3, str );

		// Draw bounding box around the bitmap
		dc.MoveTo(r.left,r.top);
		dc.LineTo(r.right,r.top);
		dc.LineTo(r.right,r.bottom);
		dc.LineTo(r.left,r.bottom);
		dc.LineTo(r.left,r.top);
		dc.SelectObject(pOldBitmap);

		file_menu.AppendMenu( MF_STRING, idx + 1, pBitmap );
	}

	if (num_hits > 0)
	{
		CRect r;
		GetWindowRect(r);
		sel = file_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x + r.left + 5, point.y + r.top + 5, this);
	}

	// Release GDI objects
	bitmaps.RemoveAll();
	ReleaseDC(&dc);
	ReleaseDC(winDC);

	return (sel - 1);
}


// Left mouse button released, we should probably do something
//
void CFreePcbView::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();									// CPT
	bool bCtrlKeyDown = (nFlags & MK_CONTROL) != 0;		// CPT
	bool bShiftKeyDown = (nFlags & MK_SHIFT) != 0;		// CPT r294
	m_last_click = point;								// CPT

	if( !m_bLButtonDown )
	{
		// this avoids problems with opening a project with the button held down
		CView::OnLButtonUp(nFlags, point);
		return;
	}

	CDC * pDC = NULL;
	CPoint tp = m_dlist->WindowToPCB( point );

	m_bLButtonDown = FALSE;
	m_lastKeyWasArrow = FALSE;		// cancel series of arrow keys
	m_lastKeyWasGroupRotate=false;	// cancel series of group rotations

	// CPT Begin:
	if( m_bDraggingRect )
	{
		// we were dragging selection rect, handle it.  CPT modified:  formerly used m_last_drag_rect, which is no longer a 
		//  reliable gauge of user's actual (auto-scrolled) rectangle 
		m_drag_rect.TopLeft() = m_start_pt;
		m_drag_rect.BottomRight() = point;
		m_drag_rect.NormalizeRect();
		CPoint tl = m_dlist->WindowToPCB( m_drag_rect.TopLeft() );
		CPoint br = m_dlist->WindowToPCB( m_drag_rect.BottomRight() );
		m_sel_rect = CRect( tl, br );
		if( bCtrlKeyDown )
			SelectItemsInRect( m_sel_rect, TRUE );
		else
			SelectItemsInRect( m_sel_rect, FALSE );
		m_bDraggingRect = FALSE;
		Invalidate( FALSE );
		CView::OnLButtonUp(nFlags, point);
		return;
	}

	if (CheckBottomPaneClick(point) || CheckLeftPaneClick(point)) 
	{
		CView::OnLButtonUp(nFlags, point); 
		return;
	}
	//end CPT

	// clicked in PCB pane
	if(	CurNone() || CurSelected() )
	{
		if (!bCtrlKeyDown)
			m_sel.RemoveAll();
		else if (m_sel_offset>=0) 
			// User is doing multiple ctrl-clicks.  Reverse the selection state of the item that was affected by the previous click
			ToggleSelectionState(m_sel_prev);
		else if (m_sel.GetSize()==1 && !m_sel.First()->IsSelectableForGroup())
			// E.g. if user clicks a vertex, then ctrl-clicks something else, the vertex can't be part of a group-select
			m_sel.RemoveAll();

		/* CPT2.  The following superfluous (with ctrl down, pins and ref text are always masked anyway).
		int old_mask = m_sel_mask_bits;
		if(bCtrlKeyDown && m_mask_id[SEL_MASK_PARTS].T1() == ID_NONE )
		{
			// if control key pressed and parts masked, also mask pins and ref
			m_mask_id[SEL_MASK_PINS].SetT1( ID_NONE );
			m_mask_id[SEL_MASK_REF].SetT1( ID_NONE );
		}
		*/

		// Search for selectors overlapping the click point, and choose among them depending on user's number of multiple clicks
		CPoint p = m_dlist->WindowToPCB( point );
		int nHits = m_hit_info.GetCount();										// Might reuse the previous contents of m_hit_info...
		if( bShiftKeyDown )
			nHits = m_dlist->TestSelect(p.x, p.y, &m_hit_info, 0xffffffff, 0);	// NB: No masking of results
		else if (m_sel_offset==-1)
			// Series of clicks is just beginning: calculate m_hit_info, and select the zero-th of those (highest priority)
			nHits = m_dlist->TestSelect(p.x, p.y, &m_hit_info, m_sel_mask_bits, bCtrlKeyDown),
			m_sel_offset = nHits==0? -1: 0;
		else if (nHits==1)
			m_sel_offset = -1;					// Unselect if there's just one hit nearby, already selected.
		else if (m_sel_offset == nHits-1)
			m_sel_offset = 0;					// Cycle round...
		else
			m_sel_offset++;						// Try next member of m_hit_info

#ifndef CPT2
		// restore mask
		m_mask_id[SEL_MASK_PINS] = old_mask_pins;
		m_mask_id[SEL_MASK_REF] = old_mask_ref;
#endif

		if( bShiftKeyDown )
			if( nHits>0 )
				m_sel_offset = SelectObjPopup( point );
			else
				m_sel_offset = -1;

		if( m_sel_offset >= 0 )
		{
			// Something to select!
			cpcb_item *item = m_hit_info[m_sel_offset].item;
			m_sel_layer = m_hit_info[m_sel_offset].layer;
			m_sel_prev = item;								// CPT

#ifndef CPT2
			// check for second pad selected while holding down 's'
			SHORT kc = GetKeyState( 'S' );
			if( kc & 0x8000 && m_cursor_mode == CUR_PAD_SELECTED )
			{
				if( sid.T1() == ID_PART && sid.T2()  == ID_SEL_PAD )
				{
					CString mess;
					// OK, now swap pads
					cpart * part1 = m_sel_part;
					CString pin_name1 = part1->shape->GetPinNameByIndex( m_sel_id.I2() );
					cnet * net1 = m_doc->m_plist->GetPinNet(part1, &pin_name1);
					CString net_name1 = "unconnected";
					if( net1 )
						net_name1 = net1->name;
					cpart * part2 = (cpart*)ptr;
					CString pin_name2 = part2->shape->GetPinNameByIndex( sid.I2() );
					cnet * net2 = m_doc->m_plist->GetPinNet(part2, &pin_name2);
					CString net_name2 = "unconnected";
					if( net2 )
						net_name2 = net2->name;
					if( net1 == NULL && net2 == NULL )
					{
						CString s ((LPCSTR) IDS_NoConnectionsToSwap);
						AfxMessageBox( s );
						return;
					}
					CString s ((LPCSTR) IDS_SwapPins);
					mess.Format( s, part1->ref_des, pin_name1, net_name1,
						part2->ref_des, pin_name2, net_name2 );
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret == IDOK )
					{
						SaveUndoInfoFor2PartsAndNets( part1, part2, TRUE, m_doc->m_undo_list );
						m_doc->m_nlist->SwapPins( part1, &pin_name1, part2, &pin_name2 );
						m_doc->ProjectModified( TRUE );
						ShowSelectStatus();
						Invalidate( FALSE );
					}
					return;
				}
			}
#endif

			// Do it!
			ToggleSelectionState(item);
		}
		else if (bCtrlKeyDown)
			HighlightSelection();				// Apparently user ctrl-clicked an object twice; it's been unselected already, so no further changes...
		else
		{
			// nothing selected
			CancelHighlightNet();
			CancelSelection();
			m_sel.RemoveAll();
		}
		Invalidate( FALSE );
	}

#ifndef CPT2
	else if( m_cursor_mode == CUR_DRAG_PART )
	{
		// complete move
		SetCursorMode( CUR_PART_SELECTED );
		CPoint p = m_dlist->WindowToPCB( point );
		m_doc->m_plist->StopDragging();
		int old_angle = m_doc->m_plist->GetAngle( m_sel_part );
		int angle = old_angle + m_dlist->GetDragAngle();
		angle = angle % 360;
		int old_side = m_sel_part->side;
		int side = old_side + m_dlist->GetDragSide();
		if( side > 1 )
			side = side - 2;

		// save undo info for part and attached nets
		if( !m_dragging_new_item )
			SaveUndoInfoForPartAndNets( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
		m_dragging_new_item = FALSE;

		// now move it
		m_sel_part->glued = 0;
		int dx = m_last_cursor_point.x - m_from_pt.x, dy = m_last_cursor_point.y - m_from_pt.y;		// CPT bug fix #29
		m_doc->m_plist->Move( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
			angle, side );
		m_doc->m_plist->HighlightPart( m_sel_part );
		m_doc->m_nlist->PartMoved( m_sel_part, dx, dy );											// CPT bug fix #29
		if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections( m_sel_part, m_doc->m_auto_ratline_disable, 
									m_doc->m_auto_ratline_min_pins );
		SetFKText( m_cursor_mode );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP || m_cursor_mode == CUR_DRAG_GROUP_ADD )
	{
		// complete move
		m_doc->m_dlist->StopDragging();
		if( m_cursor_mode == CUR_DRAG_GROUP )
			SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel_ptrs, &m_sel_ids, m_doc->m_undo_list );
		MoveGroup( m_last_cursor_point.x - m_from_pt.x, m_last_cursor_point.y - m_from_pt.y );
		m_dlist->SetLayerVisible( LAY_HILITE, TRUE );
		HighlightSelection();
		if(m_cursor_mode == CUR_DRAG_GROUP_ADD)
			FindGroupCenter();
		SetCursorMode( CUR_GROUP_SELECTED );
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_doc->m_vis[LAY_RAT_LINE] );
		if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections( m_doc->m_auto_ratline_disable, 
									m_doc->m_auto_ratline_min_pins );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_MOVE_ORIGIN )
	{
		// complete move
		SetCursorMode( CUR_NONE_SELECTED );
		CPoint p = m_dlist->WindowToPCB( point );
		m_doc->m_dlist->StopDragging();
		SaveUndoInfoForMoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y, m_doc->m_undo_list );
		MoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y );
		OnViewAllElements();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_REF )
	{
		// complete move
		SetCursorMode( CUR_REF_SELECTED );
		CPoint p = m_dlist->WindowToPCB( point );
		m_doc->m_plist->StopDragging();
		int drag_angle = m_dlist->GetDragAngle();
		// if part on bottom of board, drag angle is CCW instead of CW
		if( m_doc->m_plist->GetSide( m_sel_part ) && drag_angle )
			drag_angle = 360 - drag_angle;
		int angle = m_doc->m_plist->GetRefAngle( m_sel_part ) + drag_angle;
		if( angle>270 )
			angle = angle - 360;
		// save undo info
		SaveUndoInfoForPart( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
		// now move it
		m_doc->m_plist->MoveRefText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
			angle, m_sel_part->m_ref_size, m_sel_part->m_ref_w );
		m_doc->m_plist->SelectRefText( m_sel_part );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_VALUE )
	{
		// complete move
		SetCursorMode( CUR_VALUE_SELECTED );
		CPoint p = m_dlist->WindowToPCB( point );
		m_doc->m_plist->StopDragging();
		int drag_angle = m_dlist->GetDragAngle();
		// if part on bottom of board, drag angle is CCW instead of CW
		if( m_doc->m_plist->GetSide( m_sel_part ) && drag_angle )
			drag_angle = 360 - drag_angle;
		int angle = m_doc->m_plist->GetValueAngle( m_sel_part ) + drag_angle;
		if( angle>270 )
			angle = angle - 360;
		// save undo info
		SaveUndoInfoForPart( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
		// now move it
		m_doc->m_plist->MoveValueText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
			angle, m_sel_part->m_value_size, m_sel_part->m_value_w );
		m_doc->m_plist->SelectValueText( m_sel_part );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
#endif
	else if( m_cursor_mode == CUR_DRAG_RAT )
	{
		// routing a ratline, add segment(s)
		cnet2 *highlight_net0 = m_highlight_net;        // AMW r274 save previous state
		CancelHighlight();
		cseg2 *rat = m_sel.First()->ToSeg();
		cconnect2 *c = rat->m_con;
		c->Undraw();
		cnet2 *net = rat->m_net;
		CPoint p = m_last_cursor_point;

		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
//			m_dlist->StopDragging();

		// test for reaching destination of ratline (which might be a pin-vtx, a tee-vtx, or any other vtx)
		cvertex2 *start = m_dir==0? rat->preVtx: rat->postVtx;
		cvertex2 *dest = m_dir==0? rat->postVtx: rat->preVtx;
		// If destination is a pin, we have to take layers into account when deciding if we've hit it.  OW, don't care about layers:
		int test_layer = dest->pin? m_active_layer: 0;
		bool bHit = dest->TestHit( p.x, p.y, test_layer ), bRatlineFinished = false;
		if (bHit)
		{
			// Hit ratline's endpoint, so finish routing
			FinishRouting(rat);
			bRatlineFinished = true;
		}
		else if (!dest->preSeg || !dest->postSeg)
		{
			// routing ratline at end of trace, test for hit on any pad in net
			CArray<CHitInfo> hit_info;
			int num_hits = m_dlist->TestSelect(p.x, p.y, &hit_info,	bitPin);
			cpin2 *pin = num_hits? hit_info[0].item->ToPin(): NULL;
			if (pin && pin->net != net)
			{
				// pin assigned to different net, can't connect it
				CString s ((LPCSTR) IDS_YouAreTryingToConnectATraceToAPin);
				AfxMessageBox( s );
				c->Draw();
				goto cancel_selection_and_goodbye;
			}
			if (pin && pin->TestHit(p.x, p.y, m_active_layer))					// Double check that layer is OK for hooking onto (SMT) pins
			{
				if (start->pin == pin && c->NumSegs() < 3)
					// Hit on starting pin with too few segments, don't route to pin
					;
				else 
				{
					// route to pin
					if (dest->pin != pin)
					{
						// not our destination pin:  so move dest over [unless it is the starting pin --- CPT2 removed restriction]
						dest->pin = pin;
						dest->x = pin->x, dest->y = pin->y;
					}
					// now route to destination pin
					FinishRouting(rat);
					bRatlineFinished = true;
				}
			}
		}

		if (!bRatlineFinished)
		{
			// trace was not terminated, insert segment and continue routing
			SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
			CPoint pi = m_snap_angle_ref;
			CPoint pf = m_last_cursor_point;
			CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
			if( pp != pi )
			{
				bool bInserted = rat->InsertSegment( pp.x, pp.y, m_active_layer, m_active_width, m_dir );
				if( !bInserted )
					// Hit end of ratline, terminate routing (not likely, given our tests above)
					bRatlineFinished = true;
			}
			if (!bRatlineFinished)
			{
				bool bInserted = rat->InsertSegment( pf.x, pf.y, m_active_layer, m_active_width, m_dir );
				if( !bInserted )
					bRatlineFinished = true;
			}
		}

		c->Draw();
		if (bRatlineFinished)
			goto cancel_selection_and_goodbye;
		rat->StartDragging( pDC, m_last_cursor_point.x, m_last_cursor_point.y, 
							m_active_layer,	LAY_SELECTION, m_active_width, m_active_layer, m_dir, 2 );
		if( highlight_net0 )							// AMW r274
			m_highlight_net = highlight_net0,
			m_highlight_net->Highlight( rat );
		m_snap_angle_ref = m_last_cursor_point;
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}

#ifndef CPT2
	else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
	{
		// add trace segment and vertex
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		m_dlist->StopDragging();

		// make undo record
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		int layer = m_sel_seg->m_layer;
		int w = m_sel_seg->m_width;
		int insert_flag = m_doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is,
			m_last_cursor_point.x, m_last_cursor_point.y,
			layer, w, m_dir );
		CancelSelection();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_BOARD )
	{
		// place first corner of board outline
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		// make new board outline
		int ib = m_doc->m_board_outline.GetSize();
		m_doc->m_board_outline.SetSize( ib+1 );
		m_doc->m_board_outline[ib].SetDisplayList( m_dlist );
		m_sel_id.SetI2( ib );
		m_doc->m_board_outline[ib].Start( LAY_BOARD_OUTLINE, 1, 20*NM_PER_MIL, p.x, p.y,
			0, &m_sel_id, NULL );
		m_sel_id.SetT3( ID_SEL_CORNER );
		m_sel_id.SetI3( 0 );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		SetCursorMode( CUR_DRAG_BOARD_1 );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_1 )
	{
		// place second corner of board outline
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_doc->m_board_outline[m_sel_id.I2()].AppendCorner( p.x, p.y, m_polyline_style );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		m_sel_id.SetI3( m_sel_id.I3() + 1 );
		SetCursorMode( CUR_DRAG_BOARD );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD )
	{
		// place subsequent corners of board outline
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		if( p.x == m_doc->m_board_outline[m_sel_id.I2()].X(0)
			&& p.y == m_doc->m_board_outline[m_sel_id.I2()].Y(0) )
		{
			// this point is the start point, close the polyline and quit
			SaveUndoInfoForBoardOutlines( TRUE, m_doc->m_undo_list );
			m_doc->m_board_outline[m_sel_id.I2()].Close( m_polyline_style );
			SetCursorMode( CUR_NONE_SELECTED );
			m_doc->m_dlist->StopDragging();
		}
		else
		{
			// add corner to polyline
			m_doc->m_board_outline[m_sel_id.I2()].AppendCorner( p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( m_sel_id.I3() + 1 );
			m_snap_angle_ref = m_last_cursor_point;
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_INSERT )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_dlist->StopDragging();
		SaveUndoInfoForBoardOutlines( TRUE, m_doc->m_undo_list );
		m_doc->m_board_outline[m_sel_id.I2()].InsertCorner( m_sel_id.I3()+1, p.x, p.y );
		m_doc->m_board_outline[m_sel_id.I2()].HighlightCorner( m_sel_id.I3()+1 );
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
		m_sel_id.Set( ID_BOARD, -1, ID_OUTLINE, -1, m_sel_id.I2(), ID_SEL_CORNER, -1, m_sel_id.I3()+1 );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_MOVE )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_dlist->StopDragging();
		SaveUndoInfoForBoardOutlines( TRUE, m_doc->m_undo_list );
		m_doc->m_board_outline[m_sel_id.I2()].MoveCorner( m_sel_id.I3(), p.x, p.y );
		m_doc->m_board_outline[m_sel_id.I2()].HighlightCorner( m_sel_id.I3() );
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		int iarea = m_doc->m_nlist->AddArea( m_sel_net, m_active_layer, p.x, p.y, m_polyline_hatch );
		m_sel_id.Set( m_sel_net->m_id.T1(), -1, ID_AREA, -1, iarea, ID_SEL_CORNER, -1, 1 );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		SetCursorMode( CUR_DRAG_AREA_1 );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_1 )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		m_sel_id.SetI3( 2 );
		SetCursorMode( CUR_DRAG_AREA );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_AREA )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		if( p.x == m_sel_net->area[m_sel_id.I2()].X(0)
			&& p.y == m_sel_net->area[m_sel_id.I2()].Y(0) )
		{
			// cursor point is first point, close area
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_doc->m_undo_list );
			m_doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
			m_doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
			m_doc->m_dlist->StopDragging();
			if( m_doc->m_vis[LAY_RAT_LINE] )
				m_doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_doc->m_auto_ratline_disable,
													m_doc->m_auto_ratline_min_pins, TRUE );
			CancelSelection();
		}
		else
		{
			// add cursor point
			m_doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( m_sel_id.I3() + 1 );
			SetCursorMode( CUR_DRAG_AREA );
			m_snap_angle_ref = m_last_cursor_point;
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
	{
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_doc->m_undo_list );
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_dlist->StopDragging();
		m_doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is, p.x, p.y );
		int ret = m_doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
		if( ret == -1 )
		{
			// error
			CString s ((LPCSTR) IDS_ErrorUnableToClipPolygon);
			AfxMessageBox( s );
			CancelSelection();
			m_doc->OnEditUndo();
		}
		else
		{
			TryToReselectAreaCorner( p.x, p.y );
			if( m_doc->m_vis[LAY_RAT_LINE] )
				m_doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_doc->m_auto_ratline_disable,
													m_doc->m_auto_ratline_min_pins, TRUE );
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
	{
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_doc->m_undo_list );
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_dlist->StopDragging();
		m_doc->m_nlist->InsertAreaCorner( m_sel_net, m_sel_ia, m_sel_is+1, p.x, p.y, CPolyLine::STRAIGHT );
		int ret = m_doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
		if( ret == -1 )
		{
			// error
			CString s ((LPCSTR) IDS_ErrorUnableToClipPolygon);
			AfxMessageBox( s );
			CancelSelection();
			m_doc->OnEditUndo();
		}
		else
		{
			TryToReselectAreaCorner( p.x, p.y );
			if( m_doc->m_vis[LAY_RAT_LINE] )
				m_doc->m_nlist->OptimizeConnections( m_sel_net, -1, m_doc->m_auto_ratline_disable,
													m_doc->m_auto_ratline_min_pins, TRUE );
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		int ia = m_sel_id.I2();
		carea * a = &m_sel_net->area[ia];
		m_doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
		m_sel_id.Set( m_sel_net->m_id.T1(), -1, ID_AREA, -1, ia, ID_SEL_CORNER, -1, a->NumCorners()-1 );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		SetCursorMode( CUR_DRAG_AREA_CUTOUT_1 );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1 )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		m_sel_id.SetI3( 2 );
		SetCursorMode( CUR_DRAG_AREA_CUTOUT );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		CPolyLine * poly = &m_sel_net->area[m_sel_id.I2()];
		int icontour = poly->Contour( poly->NumCorners()-1 );
		int istart = poly->ContourStart( icontour );
		if( p.x == poly->X(istart)
			&& p.y == poly->Y(istart) )
		{
			// cursor point is first point, close area
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_doc->m_undo_list );
			m_doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
			m_doc->m_dlist->StopDragging();
			int n_old_areas = m_sel_net->area.GetSize();
			int ret = m_doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, FALSE );
			if( ret == -1 )
			{
				CString s ((LPCSTR) IDS_ErrorUnableToClipPolygon);
				AfxMessageBox( s );
				m_doc->OnEditUndo();
			}
			else
			{
				if( m_doc->m_vis[LAY_RAT_LINE] )
					m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
													m_doc->m_auto_ratline_min_pins, TRUE  );
			}
			CancelSelection();
		}
		else
		{
			// add cursor point
			m_doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( m_sel_id.I3() + 1 );
			SetCursorMode( CUR_DRAG_AREA_CUTOUT );
			m_snap_angle_ref = m_last_cursor_point;
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_SMCUTOUT )
	{
		// add poly for new cutout
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		int ism = m_doc->m_sm_cutout.GetSize();
		m_doc->m_sm_cutout.SetSize( ism + 1 );
		CPolyLine * p_sm = &m_doc->m_sm_cutout[ism];
		p_sm->SetDisplayList( m_doc->m_dlist );
		id id_sm( ID_MASK, -1, ID_MASK, -1, ism );
		m_sel_id = id_sm;
		p_sm->Start( m_polyline_layer, 0, 10*NM_PER_MIL, p.x, p.y, m_polyline_hatch, &m_sel_id, NULL );
		m_sel_id.SetT3( ID_SEL_CORNER );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		m_sel_id.SetI3( 1 );
		SetCursorMode( CUR_DRAG_SMCUTOUT_1 );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_1 )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		CPolyLine * p_sm = &m_doc->m_sm_cutout[m_sel_id.I2()];
		p_sm->AppendCorner( p.x, p.y, m_polyline_style );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		m_sel_id.SetI3( 2 );
		SetCursorMode( CUR_DRAG_SMCUTOUT );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		CPolyLine * p_sm = &m_doc->m_sm_cutout[m_sel_id.I2()];
		if( p.x == p_sm->X(0)
			&& p.y == p_sm->Y(0) )
		{
			// cursor point is first point, close area
			SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
			p_sm->Close( m_polyline_style );
			SetCursorMode( CUR_NONE_SELECTED );
			m_doc->m_dlist->StopDragging();
		}
		else
		{
			// add cursor point
			p_sm->AppendCorner( p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.SetI3( m_sel_id.I3() + 1 );
			SetCursorMode( CUR_DRAG_SMCUTOUT );
			m_snap_angle_ref = m_last_cursor_point;
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE )
	{
		SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
		CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_dlist->StopDragging();
		poly->MoveCorner( m_sel_id.I3(), p.x, p.y );
		poly->HighlightCorner( m_sel_id.I3() );
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT )
	{
		SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
		CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		m_dlist->StopDragging();
		poly->InsertCorner( m_sel_id.I3()+1, p.x, p.y );
		poly->HighlightCorner( m_sel_id.I3()+1 );
		m_sel_id.Set( ID_MASK, -1, ID_MASK, -1, m_sel_id.I2(), ID_SEL_CORNER, -1, m_sel_id.I3()+1 );
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p;
		p = m_last_cursor_point;
		int ia = m_sel_id.I2();
		carea * a = &m_sel_net->area[ia];
		m_doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
		m_sel_id.Set( m_sel_net->m_id.T1(), -1, ID_AREA, -1, ia, ID_SEL_CORNER, -1, a->NumCorners()-1 );
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
		SetCursorMode( CUR_DRAG_AREA_1 );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		m_snap_angle_ref = m_last_cursor_point;
	}
	else if( m_cursor_mode == CUR_DRAG_VTX )
	{
		// move vertex by modifying adjacent segments and reconciling via
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		CPoint p;
		p = m_last_cursor_point;
		int ic = m_sel_id.I2();
		int ivtx = m_sel_id.I3();
		m_doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is, p.x, p.y );
		m_doc->m_nlist->HighlightVertex( m_sel_net, ic, ivtx );
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			m_doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_doc->m_auto_ratline_disable,
													m_doc->m_auto_ratline_min_pins, TRUE  );
			if( m_sel_id.Resolve() )
				SelectItem( m_sel_id );
			else
				CancelSelection();
		}
		else
			SetCursorMode( CUR_VTX_SELECTED );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_MOVE_SEGMENT )
	{
		// move vertex by modifying adjacent segments and reconciling via
		m_doc->m_dlist->StopDragging();
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		CPoint cpi;
		CPoint cpf;
		m_doc->m_dlist->Get_Endpoints(&cpi, &cpf);
		int ic = m_sel_id.I2();
		int ivtx = m_sel_id.I3();
		m_doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,   cpi.x, cpi.y );
		ASSERT(cpi != cpf);			// Should be at least one grid snap apart.
		m_doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is+1, cpf.x, cpf.y );
		SetCursorMode( CUR_NONE_SELECTED );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_END_VTX )
	{
		// move end-vertex of stub trace
		m_doc->m_dlist->StopDragging();
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		CPoint p;
		p = m_last_cursor_point;
		int ic = m_sel_id.I2();
		int ivtx = m_sel_id.I3();
		m_doc->m_nlist->MoveVertex( m_sel_net, ic, ivtx, p.x, p.y );
		SetCursorMode( CUR_END_VTX_SELECTED );
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			m_doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_doc->m_auto_ratline_disable,
													m_doc->m_auto_ratline_min_pins, TRUE  );
			if( m_sel_id.Resolve() )
				SelectItem( m_sel_id );
			else
				CancelSelection();
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
#endif

	else if( m_cursor_mode == CUR_DRAG_CONNECT )
	{
		// dragging ratline to make a new connection from vertex or pin
		// test for hit on pin
		// CPT2 not thrilled by keeping the net highlight after a new ratline is made:
		// cnet2 *highlight_net0 = m_highlight_net;	// AMW r275 save previous state
		CancelHighlight();
		CPoint p = m_dlist->WindowToPCB( point );
		CArray<CHitInfo> hit_info;
		int num_hits = m_dlist->TestSelect(p.x, p.y, &hit_info,	bitPin|bitVia|bitTraceVtx|bitTee);
		if( num_hits )
		{
			// we have a hit
			cpcb_item *hit = hit_info[0].item;
			cpcb_item *sel0 = m_sel.First();
			if (hit->IsTee())
				hit = hit->ToTee()->vtxs.First();
			if (sel0->IsTee())
				sel0 = sel0->ToTee()->vtxs.First();
			
			if( hit->IsVertex() && sel0->IsPin() || hit->IsPin() && sel0->IsVertex() )
			{
				// connecting from vertex to pin, or from pin to vertex, sort it out
				cpin2 *pin = hit->IsPin()? hit->ToPin(): sel0->ToPin();
				cvertex2 *v = hit->IsVertex()? hit->ToVertex(): sel0->ToVertex();
				// now do it
				if( pin->net && pin->net != v->m_net )
				{
					// pin assigned to different net, can't connect it
					CString s ((LPCSTR) IDS_YouAreTryingToConnectATraceToAPin);
					AfxMessageBox( s );
					goto cancel_selection_and_goodbye;
				}
				// add ratline from vertex to pin
				SaveUndoInfoForNetAndConnections( v->m_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				SaveUndoInfoForPartAndNets( pin->part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_doc->m_undo_list );
				cnet2 *net = v->m_net;
				net->Undraw();
				if( !pin->net )
					net->AddPin( pin );
				cseg2 *rat_seg = v->AddRatlineToPin( pin );
				m_doc->m_dlist->StopDragging();
				net->Draw();
				SelectItem(rat_seg);
				/* CPT2 seems undesirable to me
				if( highlight_net0 )
					m_highlight_net = highlight_net0,
					highlight_net0->Highlight();
				*/
				m_doc->ProjectModified( TRUE );
			}

			else if( hit->IsPin() && sel0->IsPin() )
			{
				// connecting pin to pin
				cpin2 *p0 = hit->ToPin(), *p1 = sel0->ToPin();
				if( p0->net && p1->net && p0->net != p1->net) 
				{
					// pins assigned to different nets, can't connect them
					CString s ((LPCSTR) IDS_YouAreTryingToConnectPinsOnDifferentNets);
					AfxMessageBox( s );
					goto cancel_selection_and_goodbye;
				}
				// see if we are trying to connect a pin to itself
				if( p0==p1 )
					goto goodbye;
				// we can connect these pins
				SaveUndoInfoForPart( p0->part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
				SaveUndoInfoForPart( p1->part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_doc->m_undo_list );
				if( p0->net != p1->net )
				{
					// one pin is unassigned, assign it to the other pin's net
					if( !p0->net )
					{
						SaveUndoInfoForNetAndConnections( p1->net, CNetList::UNDO_NET_MODIFY, FALSE, m_doc->m_undo_list );
						p1->net->AddPin( p0 );
					}
					else if( !p1->net )
					{
						SaveUndoInfoForNetAndConnections( p0->net, CNetList::UNDO_NET_MODIFY, FALSE, m_doc->m_undo_list );
						p0->net->AddPin( p1 );
					}
					else
						ASSERT(0);
				}
				else if( !p0->net && !p1->net )
				{
					// connecting 2 unassigned pins, select net
					DlgAssignNet assign_net_dlg;
					assign_net_dlg.m_nlist = m_doc->m_nlist;
					int ret = assign_net_dlg.DoModal();
					if( ret != IDOK )
					{
						m_doc->m_dlist->StopDragging();
						m_doc->m_dlist->StopDragging();
						SetCursorMode( CUR_PAD_SELECTED );
						goto goodbye;
					}
					CString name = assign_net_dlg.m_net_str;
					cnet2 *net = m_doc->m_nlist->GetNetPtrByName(&name);
					if( net )
					{
						// assign pins to existing net
						SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE, m_doc->m_undo_list );
						net->AddPin(p0);
						net->AddPin(p1);
					}
					else
					{
						// make new net
						net = new cnet2(m_doc, name, 0, 0, 0);
						SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_ADD, FALSE, m_doc->m_undo_list );
						net->AddPin(p0);
						net->AddPin(p1);
					}
				}
				cseg2 *rat_seg = p0->AddRatlineToPin( p1 );
				m_doc->m_dlist->StopDragging();
				rat_seg->Draw();
				SelectItem(rat_seg);
				/* CPT2 seems undesirable to me
				if( highlight_net0 )
					m_highlight_net = highlight_net0,
					highlight_net0->Highlight();
				*/
				m_doc->ProjectModified( TRUE );
				Invalidate( FALSE );
			}

			else if( hit->IsVertex() && sel0->IsVertex() )
			{
				cvertex2 *v1 = hit->ToVertex(), *v2 = sel0->ToVertex();
				// see if we are trying to connect a pin to itself
				if( v1->m_net != v2->m_net )
				{
					CString s ((LPCSTR) IDS_YouAreTryingToConnectTracesOnDifferentNets);
					AfxMessageBox( s );
					goto cancel_selection_and_goodbye;
				}
				if( v1==v2 )
					goto goodbye;
				
				// OK, we should be able to do this, save undo info
				SaveUndoInfoForNetAndConnections( v1->m_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				cconnect2 *c1 = v1->m_con;
				cnet2 *net = v1->m_net;
				net->Undraw();
				ctee *adjust_at_end = NULL;
				if (c1 == v2->m_con && c1->head->tee && c1->tail->tee==c1->head->tee)
					// Slight subtlety if v1 and v2 are on the same connect, and the connect is a closed loop.  In that case, the tee that joins
					// the head and tail of the connect may need adjustment at the end
					adjust_at_end = c1->head->tee;
				// First see if v1 and v2 are mid-trace vertices;  if so, split their respective connects.
				if (v1->preSeg && v1->postSeg)
					v1->SplitConnect();
				if (v2->preSeg && v2->postSeg)
					v2->SplitConnect();
				// v1 and v2 are now both end vertices, possibly with tee!=NULL.  
				// Create the new ratline as its own connect, join it to v1 and v2 with tees, then Adjust() the tees.
				cconnect2 *new_c = new cconnect2 (net);
				cvertex2 *new_v = new cvertex2 (new_c, v1->x, v1->y);
				new_c->Start(new_v);
				new_c->AppendSegment(v2->x, v2->y, LAY_RAT_LINE, 0);
				cseg2 *rat_seg = new_v->postSeg;
				if (!v1->tee)
					v1->tee = new ctee(net),
					v1->tee->Add(v1);
				v1->tee->Add(new_v);
				if (!v2->tee)
					v2->tee = new ctee(net),
					v2->tee->Add(v2);
				v2->tee->Add(new_c->tail);
				if (adjust_at_end && adjust_at_end != v1->tee && adjust_at_end != v2->tee)
					adjust_at_end->Adjust();
				v1->tee->Adjust();
				v2->tee->Adjust();
				m_doc->m_dlist->StopDragging();
				net->Draw();
				SelectItem(rat_seg);
				/* CPT2 seems undesirable to me
				if( highlight_net0 )
					m_highlight_net = highlight_net0,
					highlight_net0->Highlight(); 
				*/
				m_doc->ProjectModified( TRUE );
				Invalidate( FALSE );
			}
		}
	}

#ifndef CPT2
	else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
	{
		// dragging ratline to change end pin of trace
		// see if pad selected
		CPoint p = m_dlist->WindowToPCB( point );
		m_sel_offset = -1;						// CPT r294
		id sel_id;								// id of selected item
		id pad_id( ID_PART, -1, ID_SEL_PAD );	// force selection of pad
		CArray<CHitInfo> hit_info;
		int num_hits = m_dlist->TestSelect(p.x, p.y, &hit_info, &pad_id, 1);
		sel_id.Clear();
		if( num_hits )
		{
			sel_id = hit_info[0].ID;		
			sel_id.Resolve();
			if( sel_id.IsPin() )
			{
				// see if we can connect to this pin
				cpart * new_sel_part = sel_id.Part();
				cnet * new_sel_net = sel_id.Net();
				CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.I2() );

				if( new_sel_net && (new_sel_net != m_sel_net) )
				{
					// pin assigned to different net, can't connect it
					CString s ((LPCSTR) IDS_YouAreTryingToConnectToAPinOnADifferentNet);
					AfxMessageBox( s );
					return;
				}
				else if( new_sel_net == 0 )
				{
					// unassigned pin, assign it
					SaveUndoInfoForPart( new_sel_part,
						CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
					SaveUndoInfoForNetAndConnections( m_sel_net,
						CNetList::UNDO_NET_MODIFY, FALSE, m_doc->m_undo_list );
					new_sel_net->AddPin( &new_sel_part->ref_des, &pin_name );
				}
				else
				{
					// pin already assigned to this net
					SaveUndoInfoForNetAndConnections( m_sel_net,
						CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				}
				m_doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, m_sel_is,
					new_sel_part, &pin_name );
				m_dlist->Set_visible( m_sel_seg->dl_el, TRUE );
				m_dlist->StopDragging();
				CancelHighlight();
				SetCursorMode( CUR_RAT_SELECTED );
				m_doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
				m_doc->m_nlist->SetAreaConnections( m_sel_net );
				m_doc->ProjectModified( TRUE );
				Invalidate( FALSE );
			}
		}
	}
#endif
	else if( m_cursor_mode == CUR_DRAG_STUB )			// CPT2 used to be called CUR_DRAG_TRACE, renamed (less confusing I think)
	{
		cvertex2 *vtx0 = m_sel.First()->ToVertex();		// (The vertex where user first started the stub)
		cconnect2 *con0 = vtx0->m_con;
		cvertex2 *tail0 = con0->tail;					// (The vertex that will be the start of the new seg)
		cnet2 *net = vtx0->m_net;
 		if( vtx0->preSeg )
			// if first vertex, we have already saved undo info,
			// otherwise save it here
			SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );

		// test for hit on vertex or pin
		CDisplayList *dl = m_doc->m_dlist;
		CPoint p = m_last_cursor_point;
		CArray<CHitInfo> hit_info;
		int num_hits = dl->TestSelect(p.x, p.y, &hit_info, 
										bitVia | bitTraceVtx | bitTee | bitPin);
		if( num_hits )
		{
			// hit pin or vertex
			cpcb_item *hit = hit_info[0].item;
			if (hit->IsTee())
				hit = hit->ToTee()->vtxs.First();
			if( cpin2 *pin = hit->ToPin() )
			{
				// hit on pin, see if we can connect to it
				if (pin->TestHit(p.x, p.y, m_active_layer))								// Have to make sure layer is OK, in case pin is SMT.
				{
					// check net
					if( pin->net && pin->net!=net )
					{
						// pin assigned to different net, can't connect it
						CString s ((LPCSTR) IDS_YouAreTryingToConnectToAPinOnADifferentNet);
						AfxMessageBox( s );
						goto cancel_selection_and_goodbye;
					}
					if( !pin->net )
					{
						// unassigned pin, assign it
						SaveUndoInfoForPart( pin->part,	CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_doc->m_undo_list );
						net->AddPin( pin );
					}
					// Go for it
					con0->Undraw();
					CPoint pi = m_snap_angle_ref;
					CPoint pf = m_last_cursor_point;
					CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
					if( pp != pi )
						con0->AppendSegment( pp.x, pp.y, m_active_layer, m_active_width ) ;
					con0->AppendSegment( pf.x, pf.y, m_active_layer, m_active_width );
					CPoint pin_point (pin->x, pin->y);
					if( pf != pin_point )
						con0->AppendSegment( pin_point.x, pin_point.y, m_active_layer, m_active_width );
					con0->tail->pin = pin;
					tail0->ReconcileVia();
					// Cleanup
					m_dlist->StopDragging();
					con0->Draw();						// AMW r267 added
					CancelSelection();
					net->SetAreaConnections();
					m_doc->ProjectModified( TRUE );
					Invalidate( FALSE );
					goto goodbye;
				}
			}

			else if( cvertex2 *vtx1 = hit->ToVertex() )
			{
				/* CPT2 TODO don't understand this second test... there's no issue with layers as there might be with pins.
				// hit on vertex, see if we can connect to it
				cnet * hit_net = NULL;
				int hit_ic, hit_iv;
				BOOL bHit = m_doc->m_nlist->TestHitOnVertex( m_sel_net, 0,
					m_last_cursor_point.x, m_last_cursor_point.y,
					&hit_net, &hit_ic, &hit_iv );
				if( bHit )
				{
				*/
				/* CPT2: Removed restriction
				if( vtx1->m_con == con0 )
				{
					CString s ((LPCSTR) IDS_CantConnectTraceToItself);
					AfxMessageBox( s );
					goto goodbye;
				}
				*/
				// CPT2 TODO dump message box?  Or give a turn-it-off option.
				CString s ((LPCSTR) IDS_ConnectingTraceToVertex);
				AfxMessageBox( s );

				// Ready to connect to vertex! We undraw the whole net, then redraw after this job's finished.  TODO optimize?
				net->Undraw();
				CPoint pi = m_snap_angle_ref;
				CPoint pf = m_last_cursor_point;
				CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
				if( pp != pi )
					con0->AppendSegment( pp.x, pp.y, m_active_layer, m_active_width ) ;
				con0->AppendSegment( pf.x, pf.y, m_active_layer, m_active_width );
				CPoint vtx1_point (vtx1->x, vtx1->y);
				if( pf != vtx1_point )
					con0->AppendSegment( vtx1->x, vtx1->y, m_active_layer, m_active_width );
				cvertex2 *tail1 = con0->tail;
				tail1->pin = pin;
				// CPT2 attach tail1 to vtx1 with a (possibly new) tee.  Afterwards Adjust() the tee (in case vtx1 was an end-vtx)
				if (vtx1->preSeg && vtx1->postSeg)
					// Mid-trace vertex:  split trace (SplitConnect() will set vtx1->tee)
					vtx1->SplitConnect();
				else if (!vtx1->tee)
					vtx1->tee = new ctee (net),
					vtx1->tee->Add(vtx1);
				vtx1->tee->Add(tail1);
				vtx1->tee->Adjust();					// Also reconciles the via
				tail0->ReconcileVia();
				// Cleanup
				m_dlist->StopDragging();
				if( m_doc->m_vis[LAY_RAT_LINE] )
					net->OptimizeConnections(  m_doc->m_auto_ratline_disable, m_doc->m_auto_ratline_min_pins, TRUE  );
				net->Draw();
				CancelSelection();
				net->SetAreaConnections();
				m_doc->ProjectModified( TRUE );
				Invalidate( FALSE );
				goto goodbye;
			}
		}

		// come here if not connecting to anything
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		con0->Undraw();
		CPoint pi = m_snap_angle_ref;
		CPoint pf = m_last_cursor_point;
		CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
		if( pp != pi )
			con0->AppendSegment( pp.x, pp.y, m_active_layer, m_active_width ) ;
		con0->AppendSegment( pf.x, pf.y, m_active_layer, m_active_width );
		tail0->ReconcileVia();
		// Cleanup
		m_dlist->StopDragging();
		con0->Draw();
		CPoint p0 = m_last_cursor_point;
		// AMW r275 The following statement calls m_dlist->CancelHighlight()
		con0->tail->StartDraggingStub( pDC, p0.x, p0.y, m_active_layer, m_active_width, m_active_layer, 2, m_inflection_mode );
		m_snap_angle_ref = m_last_cursor_point;
		if( m_highlight_net )					// AMW r275 re-highlight net
			m_highlight_net->Highlight();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}

#ifndef CPT2
	else if( m_cursor_mode == CUR_DRAG_TEXT )
	{
		CPoint p;
		p = m_last_cursor_point;
		m_dlist->StopDragging();
		if( !m_dragging_new_item )
			SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_doc->m_undo_list );
		int old_angle = m_sel_text->m_angle;
		int angle = old_angle + m_dlist->GetDragAngle();
		if( angle>270 )
			angle = angle - 360;
		int old_mirror = m_sel_text->m_mirror;
		int mirror = (old_mirror + m_dlist->GetDragSide())%2;
		BOOL negative = m_sel_text->m_bNegative;;
		int layer = m_sel_text->m_layer;
		m_doc->m_tlist->MoveText( m_sel_text, m_last_cursor_point.x, m_last_cursor_point.y,
			angle, mirror, negative, layer );
		if( m_dragging_new_item )
		{
			SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD, TRUE, m_doc->m_undo_list );
			m_dragging_new_item = FALSE;
		}
		SetCursorMode( CUR_TEXT_SELECTED );
		m_doc->m_tlist->HighlightText( m_sel_text );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_MEASURE_1 )
	{
		m_from_pt = m_last_cursor_point;
		m_dlist->MakeDragRatlineArray( 1, 1 );
		m_dlist->AddDragRatline( m_from_pt, zero ); 
		SetCursorMode( CUR_DRAG_MEASURE_2 );
	}
	else if( m_cursor_mode == CUR_DRAG_MEASURE_2 )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
	}
#endif
	goto goodbye;

cancel_selection_and_goodbye:
	m_dlist->StopDragging();
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );

goodbye:
	ShowSelectStatus();
	if( pDC )
		ReleaseDC( pDC );
	CView::OnLButtonUp(nFlags, point);
}

// left double-click
//
void CFreePcbView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
#if 0
	if( m_cursor_mode == CUR_PART_SELECTED )
	{
		SetCursorMode( CUR_DRAG_PART );
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->StartDraggingSelection( pDC, p.x, p.y );
	}
	if( m_cursor_mode == CUR_REF_SELECTED )
	{
		SetCursorMode( CUR_DRAG_REF );
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->StartDraggingSelection( pDC, p.x, p.y );
	}
#endif
	m_bLButtonDown = true;				// CPT r294. Basically double-clicking is a moribund concept in this program, and should be treated like 2 rapid
										// single clicks.  This statement ensures that we get the usual results when the OnLButtonUp() gets called 
										// momentarily
	CView::OnLButtonDblClk(nFlags, point);
}

// right mouse button
//
void CFreePcbView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_sel_offset = -1;							// CPT r294:  the current series of left-clicks has been interrupted...
	m_disable_context_menu = 1;
	cpcb_item *sel0 = m_sel.First();

	if( m_cursor_mode == CUR_DRAG_PART )
	{
#ifndef CPT2
		m_doc->m_plist->CancelDraggingPart( m_sel_part );
		if( m_dragging_new_item )
		{
			CancelSelection();
			m_doc->OnEditUndo();	// remove the part
		}
		else
		{
			m_doc->m_plist->HighlightPart( m_sel_part );
		}
		m_dragging_new_item = FALSE;
#endif
	}
#ifndef CPT2
	else if( m_cursor_mode == CUR_DRAG_REF )
	{
		m_doc->m_plist->CancelDraggingRefText( m_sel_part );
		m_doc->m_plist->SelectRefText( m_sel_part );
	}
	else if( m_cursor_mode == CUR_DRAG_VALUE )
	{
		m_doc->m_plist->CancelDraggingValue( m_sel_part );
		m_doc->m_plist->SelectValueText( m_sel_part );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
	{
		m_dlist->StopDragging();
		m_dlist->Set_visible( m_sel_seg->dl_el, TRUE );
		m_doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX )
	{
		m_doc->m_nlist->CancelDraggingVertex( m_sel_net, m_sel_ic, m_sel_is );
		m_doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
	else if( m_cursor_mode == CUR_MOVE_SEGMENT )
	{
		m_doc->m_nlist->CancelMovingSegment( m_sel_net, m_sel_ic, m_sel_is );
		m_doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
	{
		m_doc->m_nlist->CancelDraggingSegmentNewVertex( m_sel_net, m_sel_ic, m_sel_is );
		m_doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	}
	else if( m_cursor_mode == CUR_DRAG_END_VTX )
	{
		m_doc->m_nlist->CancelDraggingVertex( m_sel_net, m_sel_ic, m_sel_iv );
		m_doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
#endif
	else if( m_cursor_mode == CUR_DRAG_CONNECT )
		m_dlist->StopDragging();

	else if( m_cursor_mode == CUR_DRAG_STUB )
	{
		m_dlist->StopDragging();
		CancelHighlight();
		cvertex2 *sel = m_sel.First()->ToVertex();
		cnet2 *net = sel->m_net;
		if (sel->m_con->NumSegs() > 0)
		{
			cvertex2 *tail = sel->m_con->tail;
			if ( net->NetAreaFromPoint(tail->x, tail->y, LAY_PAD_THRU) )		// "LAY_PAD_THRU" so that we find areas on any layer at (tail->x,tail->y)
				tail->ForceVia(),
				tail->Draw();
			net->SetAreaConnections();
			SelectItem(tail);
			// optimize
			if( m_doc->m_vis[LAY_RAT_LINE] )
			{
				net->OptimizeConnections( m_doc->m_auto_ratline_disable, m_doc->m_auto_ratline_min_pins, TRUE  );
				if( !tail->IsValid() )
					CancelSelection();				// Mighty unlikely...
			}
		}
		else
		{
			sel->m_con->Remove();
			CancelSelection();
		}
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT )
	{
		m_dlist->CancelHighlight();
		cseg2 *rat = m_sel.First()->ToSeg();
		rat->CancelDragging();
		rat->Highlight();
	}

#ifndef CPT2
	else if( m_cursor_mode == CUR_DRAG_TEXT )
	{
		m_doc->m_tlist->CancelDraggingText( m_sel_text );
		if( m_dragging_new_item )
		{
			m_doc->m_tlist->RemoveText( m_sel_text );
			CancelSelection();
			m_dragging_new_item = 0;
		}
	}
	else if( m_cursor_mode == CUR_ADD_SMCUTOUT
		  || m_cursor_mode == CUR_DRAG_SMCUTOUT_1
		  || (m_cursor_mode == CUR_DRAG_SMCUTOUT && m_sel_id.I3()<3) )
	{
		// dragging first, second or third corner of solder mask cutout
		// delete it and cancel dragging
		m_dlist->StopDragging();
		if( m_cursor_mode != CUR_ADD_SMCUTOUT )
			m_doc->m_sm_cutout.RemoveAt( m_sel_id.I2() );
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT )
	{
		// dragging fourth or higher corner of solder mask cutout, close it
		m_dlist->StopDragging();
		SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
		m_doc->m_sm_cutout[m_sel_id.I2()].Close( m_polyline_style );
		m_doc->ProjectModified( TRUE );
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
		poly->MakeVisible();
		poly->HighlightSide( m_sel_id.I3() );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
		poly->MakeVisible();
		poly->HighlightCorner( m_sel_id.I3() );
	}
	else if( m_cursor_mode == CUR_ADD_BOARD
		  || m_cursor_mode == CUR_DRAG_BOARD_1
		  || (m_cursor_mode == CUR_DRAG_BOARD && m_doc->m_board_outline[m_sel_id.I2()].NumCorners()<3) )
	{
		// dragging first, second or third corner of board outline
		// just delete it (if necessary) and cancel
		m_dlist->StopDragging();
		if( m_cursor_mode != CUR_ADD_BOARD )
			m_doc->m_board_outline.RemoveAt( m_sel_id.I2() );	
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD )
	{
		// dragging fourth or higher corner of board outline, close it
		m_dlist->StopDragging();
		SaveUndoInfoForBoardOutlines( TRUE, m_doc->m_undo_list );
		m_doc->m_board_outline[m_sel_id.I2()].Close( m_polyline_style );
		m_doc->ProjectModified( TRUE );
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_INSERT )
	{
		m_dlist->StopDragging();
		m_doc->m_board_outline[m_sel_id.I2()].MakeVisible();
		m_doc->m_board_outline[m_sel_id.I2()].HighlightSide( m_sel_id.I2() );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_MOVE )
	{
		// get indexes for preceding and following corners
		m_dlist->StopDragging();
		m_doc->m_board_outline[m_sel_id.I2()].MakeVisible();
		m_doc->m_board_outline[m_sel_id.I2()].HighlightCorner( m_sel_id.I2() );
	}
	else if( m_cursor_mode == CUR_ADD_AREA )
	{
		m_dlist->StopDragging();
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_1
		  || (m_cursor_mode == CUR_DRAG_AREA && m_sel_id.I3()<3) )
	{
		m_dlist->StopDragging();
		m_doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_AREA)
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_doc->m_undo_list );
		m_doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
		m_doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
		CancelSelection();
		if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
	{
		m_dlist->StopDragging();
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
		  || (m_cursor_mode == CUR_DRAG_AREA_CUTOUT && m_sel_id.I3()<3) )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_sel_net->area[m_sel_id.I2()];
		int ncont = poly->NumContours();
		poly->RemoveContour(ncont-1);
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_doc->m_undo_list );
		m_doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
		int icont = m_sel_net->area[m_sel_ia].NumContours() - 1;
		int ic = m_sel_net->area[m_sel_ia].ContourStart(icont);
		CPoint p;
		p.x = m_sel_net->area[m_sel_ia].X(ic);
		p.y = m_sel_net->area[m_sel_ia].Y(ic);
		int ret = m_doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, FALSE );
		if( ret == -1 )
		{
			// error
			CString s ((LPCSTR) IDS_ErrorUnableToClipPolygonDueToIntersectingArc);
			AfxMessageBox( s );
			m_doc->OnEditUndo();
		}
		TryToReselectAreaCorner( p.x, p.y );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
	{
		m_doc->m_nlist->CancelDraggingInsertedAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_is );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
	{
		m_doc->m_nlist->CancelDraggingAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_doc->m_nlist->SelectAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_doc->m_vis[LAY_RAT_LINE] );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP_ADD )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_doc->m_vis[LAY_RAT_LINE] );
		m_doc->OnEditUndo();
	}
	else if( m_cursor_mode == CUR_DRAG_MEASURE_1 || m_cursor_mode == CUR_DRAG_MEASURE_2 )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
	}
	else
	{
		m_disable_context_menu = 0;
	}
#endif
	HighlightSelection();
	Invalidate(false);
	ShowSelectStatus();
	CView::OnRButtonDown(nFlags, point);
}


// Key pressed up
//
void CFreePcbView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	m_sel_offset = -1;							// CPT r294
	// CPT:  eliminated gShiftKeyDown stuff...
	if( nChar == 'D' )
	{
		// 'd'
		m_doc->m_drelist->MakeHollowCircles();
		Invalidate( FALSE );
	}
	else if( nChar == 16 || nChar == 17 )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set mode
			if( nChar == 17 )
				m_snap_mode = SM_GRID_POINTS;
			if( nChar == 16 && m_doc->m_snap_angle == 45 )
				m_inflection_mode = IM_90_45;
			m_dlist->SetInflectionMode( m_inflection_mode );
			Invalidate( FALSE );
		}
	}
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

// Key pressed down
//
void CFreePcbView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 'D' )
	{
		// 'd'
		m_doc->m_drelist->MakeSolidCircles();
		Invalidate( FALSE );
	}
	else if( nChar == 16 || nChar == 17 )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set mode
			if( nChar == 17 )	// ctrl
				m_snap_mode = SM_GRID_LINES;
			if( nChar == 16 && m_doc->m_snap_angle == 45 )	// shift
				m_inflection_mode = IM_45_90;
			m_dlist->SetInflectionMode( m_inflection_mode );
			Invalidate( FALSE );
		}
	}
	else
	{
		HandleKeyPress( nChar, nRepCnt, nFlags );
	}

	// don't pass through SysKey F10
	if( nChar != 121 )
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

// Key on keyboard pressed down
// CPT2 converted.  But notice that undo functions are still in the old format and are therefore disabled

void CFreePcbView::HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( m_bDraggingRect )
		return;

	// CPT: different way of dealing with gShiftKeyDown
	bool bShiftKeyDown = (GetKeyState(VK_SHIFT)&0x8000) != 0;
	bool bCtrlKeyDown = (GetKeyState(VK_CONTROL)&0x8000) != 0;
	// end CPT

#ifdef ALLOW_CURVED_SEGMENTS
	if( nChar == 'C' && m_cursor_mode == CUR_SEG_SELECTED )
	{
		// toggle segment through straight and curved shapes
		cconnect * c = &m_sel_net->connect[m_sel_ic];
		cseg * s = &c->seg[m_sel_is];
		cvertex * pre_v = &m_sel_net->connect[m_sel_ic].vtx[m_sel_iv];
		cvertex * post_v = &m_sel_net->connect[m_sel_ic].vtx[m_sel_iv+1];
		int dx = post_v->x - pre_v->x;
		int dy = post_v->y - pre_v->y;
		if( dx == 0 || dy == 0 || s->layer == LAY_RAT_LINE )
		{
			// ratline or vertical or horizontal segment, must be straight
			if( s->curve != cseg::STRAIGHT )
				ASSERT(0);
		}
		else
		{
			// toggle through straight or curved options
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
			m_doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
			s->curve++;
			if( s->curve > cseg::CW )
				s->curve = cseg::STRAIGHT;
			m_doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
			ShowSelectStatus();
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		return;
	}
#endif

	if( nChar == 'F' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_END_VTX_SELECTED ) )
	{
		// force via at a vertex/tee
		cpcb_item *sel = m_sel.First();
		SaveUndoInfoForNetAndConnections( sel->GetNet(), CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		sel->ForceVia( true );																							// CPT2 changed arg to true
		sel->Draw();
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			sel->GetNet()->OptimizeConnections( m_doc->m_auto_ratline_disable,	m_doc->m_auto_ratline_min_pins, TRUE  );	// CPT2 TODO still disabled
			// Check if OptimizeConnections() clobbered vtx:
			if( !sel->IsValid() )
				CancelSelection();
		}
		ShowSelectStatus();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		return;
	}

	if( nChar == 'U' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_END_VTX_SELECTED ) )
	{
		// unforce via
		cpcb_item *sel = m_sel.First();
		SaveUndoInfoForNetAndConnections( sel->GetNet(), CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		sel->UnforceVia( true );																							// CPT2 changed arg to true
		sel->Draw();
		/* CPT2.  The following (translated from the old code) doesn't strike me as necessary:
		if( m_cursor_mode == CUR_END_VTX_SELECTED && !vtx->tee )
		{
			cseg2 * s = vtx->preSeg? vtx->preSeg: vtx->postSeg;
			s->RemoveBreak();
			CancelSelection();
		}
		else
		*/
		if (!sel->IsTee())
			sel->GetConnect()->MergeUnroutedSegments();
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			sel->GetNet()->OptimizeConnections( m_doc->m_auto_ratline_disable,	m_doc->m_auto_ratline_min_pins, TRUE  );
			if( !sel->IsValid() )
				CancelSelection();
		}
		ShowSelectStatus();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		return;
	}

	if( nChar == 'T' )
	{ 
		// CPT:  'T' key now toggles back and forth between selected item and trace.
		if (m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_SEG_SELECTED || m_cursor_mode == CUR_RAT_SELECTED )
		{
			m_sel_prev = m_sel.First();
			m_cursor_mode_prev = m_cursor_mode;
			m_sel.RemoveAll();
			if (m_cursor_mode == CUR_VTX_SELECTED)
				m_sel.Add(m_sel_prev->ToVertex()->m_con);
			else
				m_sel.Add(m_sel_prev->ToSeg()->m_con);
			HighlightSelection();
			SetCursorMode( CUR_CONNECT_SELECTED );
			Invalidate( FALSE );
		}
		else if (m_cursor_mode==CUR_CONNECT_SELECTED)
			m_sel.RemoveAll(),
			m_sel.Add(m_sel_prev),
			HighlightSelection(),
			SetCursorMode(m_cursor_mode_prev);
		// end CPT
	}

	if( nChar == 'N' )
	{
		// "n" pressed
		// AMW r272: changed from "select net" to "toggle net highlighted state"
		if( m_highlight_net )
			CancelHighlightNet();
		else if(   m_cursor_mode == CUR_VTX_SELECTED
				|| m_cursor_mode == CUR_END_VTX_SELECTED
				|| m_cursor_mode == CUR_SEG_SELECTED
				|| m_cursor_mode == CUR_CONNECT_SELECTED 
				|| m_cursor_mode == CUR_RAT_SELECTED 
				|| m_cursor_mode == CUR_PAD_SELECTED 
				|| m_cursor_mode == CUR_AREA_CORNER_SELECTED 
				|| m_cursor_mode == CUR_AREA_SIDE_SELECTED
				|| m_cursor_mode == CUR_DRAG_RAT 
				|| m_cursor_mode == CUR_DRAG_STUB 
				|| m_cursor_mode == CUR_DRAG_CONNECT 
				|| m_cursor_mode == CUR_DRAG_VTX 
				|| m_cursor_mode == CUR_DRAG_END_VTX 
				|| m_cursor_mode == CUR_DRAG_VTX_INSERT )
		{
			m_highlight_net = m_sel.First()->GetNet();
			if (!m_highlight_net)
				;
			else if( CurDragging() )
				// highlight selected net, except the element being dragged
				m_highlight_net->Highlight( m_sel.First() );
			else
				// highlight selected net, no exceptions
				m_highlight_net->Highlight();
		}
		Invalidate( FALSE );
	}

	// CPT
	if (nChar==VK_OEM_2 || nChar==VK_DIVIDE) 
	{
		// CPT. Slash key => toggle units
		UnitToggle(bShiftKeyDown);
		return;
	}
	// end CPT

	if( nChar == 27 )
	{
		// ESC key, if something selected, cancel it
		// otherwise, fake a right-click
		if( m_highlight_net )														// CPT r293
			CancelHighlightNet();
		if( CurSelected() )
			CancelSelection();
		else
			OnRButtonDown( nFlags, CPoint(0,0) );
		return;
	}

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );

	if( nChar == 'M' )
	{
		if( !CurDragging() )
		{
			CancelSelection();
			SetCursorMode( CUR_DRAG_MEASURE_1 );
			m_dlist->StartDraggingArray( pDC, m_last_mouse_point.x, m_last_mouse_point.y, 0, LAY_SELECTION, 1 ); 
		}
		else if( m_cursor_mode == CUR_DRAG_MEASURE_1 || m_cursor_mode == CUR_DRAG_MEASURE_2 )
		{
			m_dlist->StopDragging();
			SetCursorMode( CUR_NONE_SELECTED );
		}
	}

	if( nChar == 8 )
	{
		// backspace, see if we are routing
		if( m_cursor_mode == CUR_DRAG_RAT )
		{
			// backup, if possible, by removing preceding segment (merging it into the ratline).  Also change active layer
			cseg2 *seg = m_sel.First()->ToSeg();
			cconnect2 *c = seg->m_con;
			cnet2 *net = seg->m_net;
			cseg2 *remove = m_dir==0? seg->preVtx->preSeg: seg->postVtx->postSeg;
			if (remove)
			{
				SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				m_active_layer = remove->m_layer;
				CancelSelection();
				c->Undraw();
				remove->RemoveMerge(!m_dir);
				c->Draw();
				SelectItem(seg);
				cvertex2 *start = m_dir==0? seg->preVtx: seg->postVtx;
				m_last_mouse_point.x = start->x;
				m_last_mouse_point.y = start->y;
				CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
				SetCursorPos( p.x, p.y );
				m_dlist->ChangeRoutingLayer( pDC, m_active_layer, LAY_SELECTION, 0 );
				OnRatlineRoute(false);										// Set the dlist dragging mode, highlight the net, show the active layer, etc.
			}
		}

		else if( m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing stub trace
			cvertex2 *sel = m_sel.First()->ToVertex();
			cconnect2 *c = sel->m_con;
			cnet2 *net = sel->m_net;
			CancelHighlight();
			m_dlist->StopDragging();
			bool bConnectRemoved = false;
			if (c->NumSegs() < 2) 
				c->Remove(),
				bConnectRemoved = true;
			else
			{
				SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				c->Undraw();
				bConnectRemoved = c->tail->preSeg->RemoveBreak();
			}
			if (bConnectRemoved)
				CancelSelection();
			else 
			{
				c->Draw();
				m_last_mouse_point.x = c->tail->x;
				m_last_mouse_point.y = c->tail->y;
				CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
				SetCursorPos( p.x, p.y );
				SelectItem(c->tail);
				m_active_layer = c->tail->preSeg->m_layer;
				m_dlist->ChangeRoutingLayer( pDC, m_active_layer, LAY_SELECTION, 0 );
				OnVertexStartTrace(false);							// Set the dlist dragging mode, highlight the net, show the active layer, etc.
			}
		}

		return;
	}

	int fk = FK_NONE;
	int dx = 0;
	int dy = 0;

	// get cursor position and convert to PCB coords
	CPoint p;
	GetCursorPos( &p );		// cursor pos in screen coords
	p = m_dlist->ScreenToPCB( p );	// convert to PCB coords

	// CPT
	if (HandleLayerKey(nChar, bShiftKeyDown, bCtrlKeyDown, pDC)) 
		{ ReleaseDC(pDC); return; }
	// end CPT

	if( nChar >= 112 && nChar <= 123 )
	{
		// function key pressed
		// CPT
		if (bCtrlKeyDown) {
			HandleCtrlFKey(nChar);
			ReleaseDC(pDC);
			return;
		}
		// end CPT

		fk = m_fkey_option[nChar-112];
	}
	if( nChar >= 37 && nChar <= 40 )
	{
		// CPT: arrow key (streamlined code)
		fk = FK_ARROW;
		bool bRoutingGrid = m_sel.GetSize()<2;															// CPT2 updated
		bRoutingGrid &= m_cursor_mode==CUR_ADD_AREA || m_sel.First() && m_sel.First()->IsNetItem();
		int d;
		if( bShiftKeyDown && m_units == MM )
			d = 10000;		// 0.01 mm
		else if( bShiftKeyDown && m_units == MIL )
			d = 25400;		// 1 mil
		else if (bCtrlKeyDown && nChar==VK_UP)	
		{
			if (bRoutingGrid)
				RoutingGridUp();
			else
				PlacementGridUp(); 
			return;
		}
		else if (bCtrlKeyDown && nChar==VK_DOWN) 
		{
			if (bRoutingGrid)
				RoutingGridDown();
			else
				PlacementGridDown(); 
			return;
		}
		else if( m_sel.First() && m_sel.First()->IsNetItem() )
			d = m_doc->m_routing_grid_spacing;
		else
			d = m_doc->m_part_grid_spacing;
		if( nChar == 37 )
			dx -= d;
		else if( nChar == 39 )
			dx += d;
		else if( nChar == 38 )
			dy += d;
		else if( nChar == 40 )
			dy -= d;
		// end CPT
	}
	else
		m_lastKeyWasArrow = FALSE;

	switch( m_cursor_mode )
	{
	case  CUR_NONE_SELECTED:
		if( fk == FK_ADD_AREA )
			OnAddArea();
		else if( fk == FK_ADD_TEXT )
			OnTextAdd();
		else if( fk == FK_ADD_PART )
			m_doc->OnAddPart();
		else if( fk == FK_REDO_RATLINES )
		{
			SaveUndoInfoForAllNets( TRUE, m_doc->m_undo_list );
			//			StartTimer();
			m_doc->m_nlist->OptimizeConnections();
			//			double time = GetElapsedTime();
			Invalidate( FALSE );
		}
		break;

	case CUR_PART_SELECTED:
		if( fk == FK_ARROW )
		{
			cpart2 *part = m_sel.First()->ToPart();
			if( !m_lastKeyWasArrow )
			{
				if( part->glued )
				{
					CString s ((LPCSTR) IDS_ThisPartIsGluedDoYouWantToUnglueIt);
					int ret = AfxMessageBox( s, MB_YESNO );
					if( ret == IDYES )
						part->glued = 0;
					else
						return;
				}
				SaveUndoInfoForPartAndNets( part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();
			part->Move( part->x+dx, part->y+dy, part->angle, part->side );
			part->PartMoved( dx, dy );	// CPT
			if( m_doc->m_vis[LAY_RAT_LINE] )
				part->OptimizeConnections( m_doc->m_auto_ratline_disable, m_doc->m_auto_ratline_min_pins );
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			part->Highlight();
			ShowRelativeDistance( part->x, part->y, m_totalArrowMoveX, m_totalArrowMoveY );
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_DELETE_PART || nChar == 46 )
			OnPartDelete();
		else if( fk == FK_EDIT_PART )
			m_doc->OnPartProperties();
		else if( fk == FK_EDIT_FOOTPRINT )
		{
			m_doc->m_edit_footprint = TRUE;
			OnPartEditFootprint();
		}
		else if( fk == FK_GLUE_PART )
			OnPartGlue();
		else if( fk == FK_UNGLUE_PART )
			OnPartUnglue();
		else if( fk == FK_MOVE_PART )
			OnPartMove();
		else if( fk == FK_REDO_RATLINES )
			OnPartOptimize();
		break;

	case CUR_REF_SELECTED:
		if( fk == FK_ARROW )
		{
			cpart2 *part = m_sel.First()->ToRefText()->part;
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForPart( part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();
			CPoint ref_pt = part->GetRefPoint();
			part->MoveRefText( ref_pt.x + dx, ref_pt.y + dy );
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			// m_doc->m_plist->SelectRefText( m_sel_part );
			ShowRelativeDistance( part->GetRefPoint().x, part->GetRefPoint().y,
				m_totalArrowMoveX, m_totalArrowMoveY );
			part->m_ref->Highlight();
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_PARAMS )
			OnRefProperties();
		else if( fk == FK_MOVE_REF )
			OnRefMove();
		else if( fk == FK_ROTATE_REF )
			OnRefRotateCW();
		else if( fk == FK_ROTATE_REF_CCW )
			OnRefRotateCCW();
		break;

	case CUR_VALUE_SELECTED:
		if( fk == FK_ARROW )
		{
			cpart2 *part = m_sel.First()->ToValueText()->part;
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForPart( part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();
			CPoint val_pt = part->GetValuePoint();
			part->MoveValueText( val_pt.x + dx,	val_pt.y + dy );
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			// m_doc->m_plist->SelectValueText( m_sel_part );
			ShowRelativeDistance( part->GetValuePoint().x, part->GetValuePoint().y,
				m_totalArrowMoveX, m_totalArrowMoveY );
			part->m_value->Highlight();
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_PARAMS )
			OnValueProperties();
		else if( fk == FK_MOVE_VALUE )
			OnValueMove();
		else if( fk == FK_ROTATE_VALUE )
			OnValueRotateCW();
		else if( fk == FK_ROTATE_VALUE_CCW )
			OnValueRotateCCW();
		break;

	case CUR_RAT_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnRatlineSetWidth();
		else if( fk == FK_LOCK_CONNECT )
			OnRatlineLockConnection();
		else if( fk == FK_UNLOCK_CONNECT )
			OnRatlineUnlockConnection();
		else if( fk == FK_ROUTE )
			OnRatlineRoute();
		else if( fk == FK_CHANGE_PIN )
			OnRatlineChangeEndPin();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_SEGMENT )
			OnSegmentDelete();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_SEG_SELECTED:
		if( fk == FK_ARROW )
		{
			cseg2 *seg = m_sel.First()->ToSeg();
			// CPT2 TODO.  The old code was:
			// if (!SegmentMovable())
			//	 { PlaySound( TEXT("CriticalStop"), 0, 0 ); break; }
			// I'm hoping the following will suffice, but time will tell
			if (seg->preVtx->pin || seg->postVtx->pin || seg->preVtx->tee || seg->postVtx->tee)
			{
				PlaySound( TEXT("CriticalStop"), 0, 0 );
				break;
			}

			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( seg->m_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();

			// 1. Move the line defined by the segment.
			m_from_pt.x = seg->preVtx->x;
			m_from_pt.y = seg->preVtx->y;
			m_to_pt.x = seg->postVtx->x;
			m_to_pt.y = seg->postVtx->y;

			// CPT2.  The old code was I think problematic for the case that this seg is at the beginning of a trace.
			// Let's try:
			cseg2 *prev = seg->preVtx->preSeg;
			cseg2 *next = seg->postVtx->postSeg;
			if (prev)
				m_last_pt.x = prev->preVtx->x,
				m_last_pt.y = prev->preVtx->y;
			if (next)
				m_next_pt.x = next->postVtx->x,
				m_next_pt.y = next->postVtx->y;

			// 1. Move the endpoints of (xi, yi), (xf, yf) of the line by the mouse movement. This
			//		is just temporary, since the final ending position is determined by the intercept
			//		points with the leading and trailing segments:
			int new_xi = m_from_pt.x + dx;			
			int new_yi = m_from_pt.y + dy;

			int new_xf = m_to_pt.x + dx;			
			int new_yf = m_to_pt.y + dy;

			int old_x0_dir = sign(m_from_pt.x - m_last_pt.x);
			int old_y0_dir = sign(m_from_pt.y - m_last_pt.y);

			int old_x1_dir = sign(m_to_pt.x - m_from_pt.x);
			int old_y1_dir = sign(m_to_pt.y - m_from_pt.y);

			int old_x2_dir = sign(m_next_pt.x - m_to_pt.x);
			int old_y2_dir = sign(m_next_pt.y - m_to_pt.y);

			int i_nudge_xi = new_xi, i_nudge_yi = new_yi;
			int i_nudge_xf = new_xf, i_nudge_yf = new_yf;

			// 2. Find the intercept between the extended segment in motion and the leading segment IF ANY.
			if (prev)
			{
				double d_new_xi, d_new_yi;
				FindLineIntersection(m_last_pt.x, m_last_pt.y, m_from_pt.x, m_from_pt.y,
									 new_xi, new_yi, new_xf, new_yf,
									 &d_new_xi, &d_new_yi);
				i_nudge_xi = floor(d_new_xi + .5);
				i_nudge_yi = floor(d_new_yi + .5);
			}

			// 3. Find the intercept between the extended segment in motion and the trailing segment:
			if(next)
			{
				double d_new_xf;
				double d_new_yf;
				FindLineIntersection(new_xi, new_yi, new_xf, new_yf,
									 m_to_pt.x,	m_to_pt.y, m_next_pt.x, m_next_pt.y,
									 &d_new_xf, &d_new_yf);
				i_nudge_xf = floor(d_new_xf + .5);
				i_nudge_yf = floor(d_new_yf + .5);
			}
			
			// If we drag too far, the line segment can reverse itself causing a little triangle to form.
			//   That's a bad thing.
			bool bOK = sign(i_nudge_xf - i_nudge_xi) == old_x1_dir && sign(i_nudge_yf - i_nudge_yi) == old_y1_dir;
			if (prev)
				bOK &= sign(i_nudge_xi - m_last_pt.x) == old_x0_dir && sign(i_nudge_yi - m_last_pt.y) == old_y0_dir;
			if (next)
				bOK &= sign(m_next_pt.x - i_nudge_xf) == old_x2_dir && sign(m_next_pt.y - i_nudge_yf) == old_y2_dir;
			if (bOK)
			{
				//	Move both vetices to the new position:
				seg->m_con->Undraw();
				seg->preVtx->Move( i_nudge_xi, i_nudge_yi );
				seg->postVtx->Move( i_nudge_xf, i_nudge_yf );
				seg->m_con->Draw();
			}
			else
				{ seg->Highlight(); break; }

			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			ShowRelativeDistance( seg->preVtx->x, seg->preVtx->y, m_totalArrowMoveX, m_totalArrowMoveY );
			seg->Highlight();
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		if( fk == FK_SET_WIDTH )
			OnSegmentSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnSegmentChangeLayer();
		else if( fk == FK_ADD_VERTEX )
			OnSegmentAddVertex();
		else if( fk == FK_MOVE_SEGMENT)
			OnSegmentMove();
		else if( fk == FK_UNROUTE )
			OnSegmentUnroute();
		else if( fk == FK_DELETE_SEGMENT )
			OnSegmentDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();	//**
		break;

	case  CUR_VTX_SELECTED:
		if( fk == FK_ARROW )
		{
			cvertex2 *vtx = m_sel.First()->ToVertex();
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( vtx->m_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();
			vtx->Move( vtx->x + dx, vtx->y + dy );
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			ShowRelativeDistance( vtx->x, vtx->y, m_totalArrowMoveX, m_totalArrowMoveY );
			vtx->Highlight();	
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_START_TRACE )
			OnVertexStartTrace();
		else if( fk == FK_VERTEX_PROPERTIES )
			OnVertexProperties();
		else if( fk == FK_MOVE_VERTEX )
			OnVertexMove();
		else if( fk == FK_ADD_CONNECT )
			OnVertexStartRatline();
		else if( fk == FK_DELETE_VERTEX || nChar == 46 )
			OnVertexDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_END_VTX_SELECTED:
		// CPT r295:  added arrow key support
		if( fk == FK_ARROW )
		{
			cvertex2 *vtx = m_sel.First()->ToVertex();
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( vtx->m_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();
			vtx->Move( vtx->x + dx, vtx->y + dy );
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			ShowRelativeDistance( vtx->x, vtx->y, m_totalArrowMoveX, m_totalArrowMoveY );
			vtx->Highlight();	
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		// end CPT
		else if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_VERTEX_PROPERTIES )						// CPT r295
			OnVertexProperties();
		else if( fk == FK_ADD_CONNECT )
			OnVertexStartRatline();
		else if( fk == FK_ADD_SEGMENT )
			OnVertexStartTrace();
		else if( fk == FK_MOVE_VERTEX )
			OnEndVertexMove();
		else if( fk == FK_DELETE_VERTEX || nChar == 46 )
			OnVertexDelete();
		else if( fk == FK_ADD_VIA )
			OnEndVertexAddVia();
		else if( fk == FK_DELETE_VIA )
			OnEndVertexRemoveVia();
		else if( fk == FK_DELETE_CONNECT )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case CUR_TEE_SELECTED:
		if( fk == FK_ARROW )
		{
			ctee *tee = m_sel.First()->ToTee();
			cvertex2 *first = tee->vtxs.First();
			ASSERT(first);
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( first->m_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();
			tee->Move( first->x + dx, first->y + dy );
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			ShowRelativeDistance( first->x, first->y, m_totalArrowMoveX, m_totalArrowMoveY );
			tee->Highlight();	
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}

		else if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_VERTEX_PROPERTIES )
			OnVertexProperties();
		else if( fk == FK_START_TRACE )
			OnVertexStartTrace();
		else if( fk == FK_ADD_CONNECT )
			OnVertexStartRatline();
		else if( fk == FK_MOVE_VERTEX )
			OnVertexMove();
		else if( fk == FK_DELETE_VERTEX || nChar == 46 )
			OnVertexDelete();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_CONNECT_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnConnectSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnConnectChangeLayer();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		break;

	case  CUR_NET_SELECTED:					// CPT2 TODO is this defunct?
		if( fk == FK_SET_WIDTH )
			OnNetSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnNetChangeLayer();
		else if( fk == FK_EDIT_NET )
			OnNetEditnet();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_PAD_SELECTED:
		if( fk == FK_ATTACH_NET )
			OnPadAddToNet();
		else if( fk == FK_START_TRACE )
			OnPadStartTrace();
		else if( fk == FK_ADD_CONNECT )
			OnPadStartRatline();
		else if( fk == FK_DETACH_NET )
			OnPadDetachFromNet();
		else if( fk == FK_REDO_RATLINES )
			OnPadOptimize();
		break;

	case CUR_TEXT_SELECTED:
		if( fk == FK_ARROW )
		{
			ctext *text = m_sel.First()->ToText();
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForText( text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();
			text->Move( text->m_x + dx, text->m_y + dy,
						text->m_angle, text->m_bMirror,	text->m_bNegative, text->m_layer );
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			ShowRelativeDistance( text->m_x, text->m_y, 
				m_totalArrowMoveX, m_totalArrowMoveY );
			text->Highlight();
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_EDIT_TEXT )
			OnTextEdit();
		else if( fk == FK_MOVE_TEXT )
			OnTextMove();
		else if( fk == FK_DELETE_TEXT || nChar == 46 )
			OnTextDelete();
		break;

#ifndef CPT2							// TODO
	case CUR_BOARD_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForBoardOutlines( TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_doc->m_board_outline[m_sel_id.I2()];
			poly->MoveCorner( m_sel_is,
				poly->X( m_sel_is ) + dx,
				poly->Y( m_sel_is ) + dy );
			CancelHighlight();
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			ShowRelativeDistance( poly->X( m_sel_is ), poly->Y( m_sel_is ),
				m_totalArrowMoveX, m_totalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnBoardCornerEdit();
		else if( fk == FK_MOVE_CORNER )
			OnBoardCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnBoardCornerDelete();
		else if( fk == FK_DELETE_OUTLINE || nChar == 46 )
			OnBoardDeleteOutline();
		break;

	case CUR_BOARD_SIDE_SELECTED:
		// CPT
		if( fk == FK_ARROW )
			MoveGroup(dx, dy);
		// end CPT
		else if( fk == FK_POLY_STRAIGHT )
			OnBoardSideConvertToStraightLine();
		else if( fk == FK_POLY_ARC_CW )
			OnBoardSideConvertToArcCw();
		else if( fk == FK_POLY_ARC_CCW )
			OnBoardSideConvertToArcCcw();
		else if( fk == FK_ADD_CORNER )
			OnBoardSideAddCorner();
		else if( fk == FK_DELETE_OUTLINE || nChar == 46 )
			OnBoardDeleteOutline();
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_ic];
			poly->MoveCorner( m_sel_is,
				poly->X( m_sel_is ) + dx,
				poly->Y( m_sel_is ) + dy );
			CancelHighlight();
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			ShowRelativeDistance( poly->X( m_sel_is ), poly->Y( m_sel_is ),
				m_totalArrowMoveX, m_totalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnSmCornerSetPosition();
		else if( fk == FK_MOVE_CORNER )
			OnSmCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnSmCornerDeleteCorner();
		else if( fk == FK_DELETE_CUTOUT || nChar == 46 )
			OnSmCornerDeleteCutout();
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
			// CPT: enable arrow keys
			if( fk == FK_ARROW )
				MoveGroup(dx, dy);
			// end CPT
			else if( fk == FK_POLY_STRAIGHT )
			{
				CancelHighlight();
				m_polyline_style = CPolyLine::STRAIGHT;
				poly->SetSideStyle( m_sel_id.I3(), m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.I3() );
				Invalidate( FALSE );
				m_doc->ProjectModified( TRUE );
			}
			else if( fk == FK_POLY_ARC_CW )
			{
				CancelHighlight();
				m_polyline_style = CPolyLine::ARC_CW;
				poly->SetSideStyle( m_sel_id.I3(), m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.I3() );
				Invalidate( FALSE );
				m_doc->ProjectModified( TRUE );
			}
			else if( fk == FK_POLY_ARC_CCW )
			{
				CancelHighlight();
				m_polyline_style = CPolyLine::ARC_CCW;
				poly->SetSideStyle( m_sel_id.I3(), m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.I3() );
				Invalidate( FALSE );
				m_doc->ProjectModified( TRUE );
			}
			else if( fk == FK_ADD_CORNER )
				OnSmSideInsertCorner();
			else if( fk == FK_DELETE_CUTOUT || nChar == 46 )
				OnSmSideDeleteCutout();
		}
		break;
#endif

	case CUR_AREA_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			ccorner *c = m_sel.First()->ToCorner();
			cnet2 *net = c->GetNet();
			carea2 *area = c->contour->poly->ToArea();
			if( !m_lastKeyWasArrow )
			{
				SaveUndoInfoForAllAreasInNet( net, TRUE, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			CancelHighlight();
			c->Move( c->x+dx, c->y+dy );
			c->Highlight();
			int ret = area->PolygonModified( FALSE, TRUE );
			if( ret == -1 )
			{
				// error
				CString s ((LPCSTR) IDS_ErrorUnableToClipPolygon);
				AfxMessageBox( s );
				CancelSelection();
				m_doc->OnEditUndo();
			}
			else
			{
				m_totalArrowMoveX += dx;
				m_totalArrowMoveY += dy;
				ShowRelativeDistance( p.x, p.y, m_totalArrowMoveX, m_totalArrowMoveY );
				TryToReselectAreaCorner( p.x, p.y );
			}
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if( fk == FK_SET_POSITION )
			OnAreaCornerProperties();
		else if( fk == FK_MOVE_CORNER )
			OnAreaCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnAreaCornerDelete();
		else if( fk == FK_DELETE_AREA )
			OnAreaCornerDeleteArea();
		else if( fk == FK_AREA_CUTOUT )
			OnAreaAddCutout();
		else if( fk == FK_DELETE_CUTOUT )
			OnAreaDeleteCutout();
		else if( nChar == 46 )
			OnAreaCornerDelete();
		break;

	case CUR_AREA_SIDE_SELECTED:
		// CPT
		if( fk == FK_ARROW )
			MoveGroup(dx, dy);
		// end CPT
		else if( fk == FK_SIDE_STYLE )
			OnAreaSideStyle();
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if( fk == FK_POLY_ARC_CCW )
		{
			cside *s = m_sel.First()->ToSide();
			cnet2 *net = s->GetNet();
			carea2 *area = s->contour->poly->ToArea();
			SaveUndoInfoForArea( area, CNetList::UNDO_AREA_MODIFY, TRUE, m_doc->m_undo_list );
//			SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_doc->m_undo_list );
			m_polyline_style = cpolyline::ARC_CCW;
			s->m_style = m_polyline_style;
			area->SetConnections();
			if( m_doc->m_vis[LAY_RAT_LINE] )
				net->OptimizeConnections( m_doc->m_auto_ratline_disable, m_doc->m_auto_ratline_min_pins, TRUE  );
			SetFKText( m_cursor_mode );
			Invalidate( FALSE );
			m_doc->ProjectModified( TRUE );
		}
		else if( fk == FK_ADD_CORNER )
			OnAreaSideAddCorner();
		else if( fk == FK_DELETE_AREA )
			OnAreaSideDeleteArea();
		else if( fk == FK_AREA_CUTOUT )
			OnAreaAddCutout();
		else if( fk == FK_DELETE_CUTOUT )
			OnAreaDeleteCutout();
		else if( nChar == 46 )
		{
			if (m_sel.First()->ToSide()->IsOnCutout())
				OnAreaDeleteCutout();
			else
				OnAreaSideDeleteArea();
		}
		break;

	case CUR_DRE_SELECTED:
		if( nChar == 46 )
		{
			CancelSelection();
#ifndef CPT2
			m_doc->m_drelist->Remove( m_sel.First()->ToDRE() );
#endif
			Invalidate( FALSE );
		}
		break;

	case CUR_GROUP_SELECTED:
		if( fk == FK_ARROW )
		{
			CancelHighlight();
			if( !m_lastKeyWasArrow && !m_lastKeyWasGroupRotate)
			{
				if( GluedPartsInGroup() )
				{
					CString s ((LPCSTR) IDS_ThisGroupContainsGluedPartsDoYouWantToUnglueThem);
					int ret = AfxMessageBox( s, MB_YESNO );
					if( ret != IDYES )
						return;
				}
				SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel, m_doc->m_undo_list );
				m_totalArrowMoveX = 0;
				m_totalArrowMoveY = 0;
				m_lastKeyWasArrow = TRUE;
			}
			MoveGroup( dx, dy );
			m_totalArrowMoveX += dx;
			m_totalArrowMoveY += dy;
			HighlightSelection();
			ShowRelativeDistance( m_totalArrowMoveX, m_totalArrowMoveY );
			m_doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_MOVE_GROUP )
			OnGroupMove();
		else if( fk == FK_DELETE_GROUP || nChar == 46 )
			OnGroupDelete();
		else if(fk == FK_ROTATE_GROUP)
			OnGroupRotate();
		break;

	case CUR_DRAG_RAT:
	case CUR_DRAG_STUB:
		if( fk == FK_COMPLETE )
		{
			// CUR_DRAG_RAT only
			cseg2 *seg = m_sel.First()->ToSeg();
			cvertex2 *start = m_dir==0? seg->preVtx: seg->postVtx;
			cvertex2 *end = m_dir==0? seg->postVtx: seg->preVtx;
			cconnect2 *c = seg->m_con;
			cnet2 *net = seg->m_net;
			SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
			// CPT2 TODO.  I'm trying using FinishRouting() instead of cseg2::Route().  This gives us inflection points rather than just a 
			// straight line.  Desirable?
			bool bValid = true;
			if (start->pin)
			{
				int pad_layer = start->pin->pad_layer;
				if( pad_layer != LAY_PAD_THRU && m_active_layer != pad_layer )
					bValid = false;
			}
			if (end->pin)
			{
				int pad_layer = end->pin->pad_layer;
				if( pad_layer != LAY_PAD_THRU && m_active_layer != pad_layer )
					bValid = false;
			}

			if (bValid)
			{
				m_snap_angle_ref = CPoint(start->x, start->y);
				m_last_cursor_point = CPoint(end->x, end->y);
				net->Undraw();
				FinishRouting(seg);
				net->Draw();
				seg->CancelDragging();
				CancelSelection();
			}
			else
				PlaySound( TEXT("CriticalStop"), 0, 0 );
			Invalidate( FALSE );
			m_doc->ProjectModified( TRUE );
		}

        // CPT
        else if (fk==FK_ACTIVE_WIDTH_UP || fk==FK_ARROW && dy>0)     // F2 or up-arrow
            ActiveWidthUp(pDC);
        else if (fk==FK_ACTIVE_WIDTH_DOWN || fk==FK_ARROW && dy<0)   // F1 or down-arrow
            ActiveWidthDown(pDC);
		else if (fk==FK_RGRID_UP)
			RoutingGridUp();
		else if (fk==FK_RGRID_DOWN)
			RoutingGridDown();
		// end CPT

		break;

	// CPT:
	case CUR_DRAG_VTX:
	case CUR_DRAG_END_VTX:
	case CUR_DRAG_VTX_INSERT:
		if (fk==FK_RGRID_UP)
			RoutingGridUp();
		else if (fk==FK_RGRID_DOWN)
			RoutingGridDown();
		break;
	// end CPT

	case  CUR_DRAG_PART:
		if( fk == FK_ROTATE_PART )
			m_dlist->IncrementDragAngle( pDC );
		if( fk == FK_ROTATE_PART_CCW )
		{
			m_dlist->IncrementDragAngle( pDC );
			m_dlist->IncrementDragAngle( pDC );
			m_dlist->IncrementDragAngle( pDC );
		}
		else if( fk == FK_SIDE )
			m_dlist->FlipDragSide( pDC );
		break;

	case  CUR_DRAG_REF:
		if( fk == FK_ROTATE_REF )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_VALUE:
		if( fk == FK_ROTATE_VALUE )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_TEXT:
		if( fk == FK_ROTATE_TEXT )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_BOARD:
	case  CUR_DRAG_BOARD_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	case  CUR_DRAG_AREA:
	case  CUR_DRAG_AREA_1:
	case  CUR_DRAG_AREA_CUTOUT:
	case  CUR_DRAG_AREA_CUTOUT_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	case  CUR_DRAG_SMCUTOUT:
	case  CUR_DRAG_SMCUTOUT_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	default:
		break;
	}	// end switch

	// CPT
	if( nChar == VK_HOME )
		// home key pressed, ViewAllElements
		OnViewAllElements();
	else
		HandlePanAndZoom(nChar, p);
	// end CPT

	ReleaseDC( pDC );
	if( m_lastKeyWasArrow ==FALSE && m_lastKeyWasGroupRotate==false )
		ShowSelectStatus();
}

// Mouse moved
//
void CFreePcbView::OnMouseMove(UINT nFlags, CPoint point)
{
	// CPT r294:  determine whether a series of mouse clicks by the user is truly over (see if we've moved away significantly from m_last_click).
	// If so m_sel_offset is reset to -1.
	if (m_sel_offset!=-1 && (abs(point.x-m_last_click.x) > 1 || abs(point.y-m_last_click.y) > 1))
		m_sel_offset = -1;

	static BOOL bCursorOn = TRUE;

	if( (nFlags & MK_LBUTTON) && m_bLButtonDown )
	{
		double d = abs(point.x-m_start_pt.x) + abs(point.y-m_start_pt.y);
		if( m_bDraggingRect
			|| (d > 10 && !CurDragging() ) )
		{
			// we are dragging a selection rect
			// CPT: autoscrolling implemented
			int w = m_client_r.right - m_left_pane_w;
			int h = m_client_r.bottom - m_bottom_pane_h;
			int dx = 0, dy = 0;
			if (point.x < m_left_pane_w) dx = -w/4;
			else if (point.x > m_client_r.right) dx = w/4;
			if (point.y < 0) dy = h/4;
			else if (point.y > h) dy = -h/4;
			if (dx || dy) {
				DWORD tick = GetTickCount();
				if (tick-m_last_autoscroll > 500) {
					// It's been a half-second plus since the last autoscroll.  Do another:
					m_bDontDrawDragRect = true;						// Won't draw drag-rect again until after repaint finishes
					m_last_autoscroll = tick;
					m_org_x += dx*m_pcbu_per_pixel;
					m_org_y += dy*m_pcbu_per_pixel;
					m_start_pt.x -= dx;
					m_start_pt.y += dy;
					CRect screen_r;
					GetWindowRect( &screen_r );
					m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
						m_org_x, m_org_y );
					Invalidate( FALSE );
				}
			}
			
			SIZE s1;
			s1.cx = s1.cy = 1;
			m_drag_rect.TopLeft() = m_start_pt;
			m_drag_rect.BottomRight() = point;
			m_drag_rect.NormalizeRect();
			// CPT Modify drag-rect if necessary to prevent messing up the bottom or left panes
			if (m_drag_rect.top >= h) m_drag_rect.bottom = m_drag_rect.top = h-1;
			else if (m_drag_rect.bottom >= h) m_drag_rect.bottom = h-1;
			if (m_drag_rect.right <= m_left_pane_w) m_drag_rect.right = m_drag_rect.left = m_left_pane_w+1;
			else if (m_drag_rect.left <= m_left_pane_w) m_drag_rect.left = m_left_pane_w+1;
			// end CPT
			
			CDC * pDC = GetDC();
			if (m_bDontDrawDragRect) 
				;
			else if( !m_bDraggingRect )
				//start dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, NULL, s1 );
			else
				// continue dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, &m_last_drag_rect, s1 );
			m_bDraggingRect  = TRUE;
			m_last_drag_rect = m_drag_rect;
			ReleaseDC( pDC );
		}
	}
	m_last_mouse_point = m_dlist->WindowToPCB( point );
	SnapCursorPoint( m_last_mouse_point, nFlags );
	// check for cursor hiding
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	if( !CurDragging() )
		frm->m_bHideCursor = FALSE;		// disable cursor hiding
	else if( !frm->m_bHideCursor )
	{
		// enable cursor hiding and set rect
		CRect r = frm->m_client_rect;
		r.left += m_left_pane_w;
		r.bottom -= m_bottom_pane_h;
		frm->SetHideCursor( TRUE, &r );
	}

	// CPT:  when in connect-to-pin mode, see if we're on top of a pin and display its name in the status bar if so
	if (m_cursor_mode != CUR_DRAG_CONNECT) 
		return;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain ) 
		return;
	CString pin_name = "", connect_to ((LPCSTR) IDS_ConnectTo);
	CPoint p = m_dlist->WindowToPCB( point );
	// CPT r294:  new args for TestSelect()...
	CArray<CHitInfo> hit_info;
	int num_hits = m_dlist->TestSelect( p.x, p.y, &hit_info, bitPin );
	if( num_hits > 0 )
	{
		void * ptr = hit_info[0].ptr;			// hit_info[0] is the highest-priority hit
		id hit_id = hit_info[0].ID;
		if (ptr && hit_id.IsPin() )
		{ 
			// hit on pin
			cpart * hit_part = (cpart*)ptr;
			pin_name = connect_to + hit_part->ref_des + "." + hit_part->shape->GetPinNameByIndex( hit_id.I2() ) + "?";
			pMain->DrawStatus( 3, &pin_name );
			return;
		}
	}
	// no hit on pin
	pMain->DrawStatus(3, &pin_name); 
}


/////////////////////////////////////////////////////////////////////////
// Utility functions
//

// Set the device context to world coords
//

// Set cursor mode, update function key menu if necessary
void CFreePcbView::SetCursorMode( int mode )
{
	// CPT2.  Removed:
	// if( mode != m_cursor_mode )
	// FK buttons can sometimes vary even when the mode doesn't change.
		SetFKText( mode );
		m_cursor_mode = mode;
		ShowSelectStatus();
		/*  CPT removed.  Want to allow copy/paste when single item is selected
		//** AMW r284: reinserted since edit commands never enabled */
		/* CPT fixed problem by removing other cases where edit-copy and edit-paste were disabled.  These should now always be enabled, and if
		   users try to use them in a bogus way, they get an error msg.
		if( mode == CUR_GROUP_SELECTED )
		{
			CWnd* pMain = AfxGetMainWnd();
			if (pMain != NULL)
			{
				CMenu* pMenu = pMain->GetMenu();
				CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
				submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_ENABLED );
				submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_ENABLED );
				submenu->EnableMenuItem( ID_EDIT_SAVEGROUPTOFILE, MF_BYCOMMAND | MF_ENABLED );
				pMain->DrawMenuBar();
			}
		}
		else
		{
			CWnd* pMain = AfxGetMainWnd();
			if (pMain != NULL)
			{
				CMenu* pMenu = pMain->GetMenu();
				CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
				submenu->EnableMenuItem( ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
				submenu->EnableMenuItem( ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
				submenu->EnableMenuItem( ID_EDIT_SAVEGROUPTOFILE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
				pMain->DrawMenuBar();
			}
		}
		*/
		if( CurDragging() )
			SetMainMenu( FALSE );
		else if( m_doc->m_project_open )
			SetMainMenu( TRUE );
}


// Set function key shortcut text
//
void CFreePcbView::SetFKText( int mode )
{
	// CPT: text for routing/placement grid controls in the toolbar changes depending on what's selected
	CString placement, routing;
	bool bRoutingGrid = m_sel.GetSize()<2;
	bRoutingGrid &= mode==CUR_ADD_AREA || m_sel.First() && m_sel.First()->IsNetItem();			// CPT2 updated
	if (bRoutingGrid) 
		placement.LoadStringA(IDS_ToolbarPlacement),
		routing.LoadStringA(IDS_ToolbarRoutingCtrlUpDn);
	else
		placement.LoadStringA(IDS_ToolbarPlacementCtrlUpDn),
		routing.LoadStringA(IDS_ToolbarRouting);
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.m_ctlStaticPlacementGrid.SetWindowTextA(placement);
	frm->m_wndMyToolBar.m_ctlStaticRoutingGrid.SetWindowTextA(routing);
	// End CPT

	for( int i=0; i<12; i++ )
	{
		m_fkey_option[i] = 0;
		m_fkey_command[i] = 0;
	}

	switch( mode )
	{
	case CUR_NONE_SELECTED:
		if( m_doc->m_project_open )
		{
			m_fkey_option[1] = FK_ADD_AREA;
			m_fkey_option[2] = FK_ADD_TEXT;
			m_fkey_option[3] = FK_ADD_PART;
			m_fkey_option[8] = FK_REDO_RATLINES;
		}
		break;

	case CUR_PART_SELECTED:
		m_fkey_option[0] = FK_EDIT_PART;
		m_fkey_option[1] = FK_EDIT_FOOTPRINT;
		if( m_sel.First()->ToPart()->glued )
			m_fkey_option[2] = FK_UNGLUE_PART;
		else
			m_fkey_option[2] = FK_GLUE_PART;
		m_fkey_option[3] = FK_MOVE_PART;
		m_fkey_option[7] = FK_DELETE_PART;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_REF_SELECTED:
		m_fkey_option[0] = FK_SET_PARAMS;
		m_fkey_option[1] = FK_ROTATE_REF;
		m_fkey_option[2] = FK_ROTATE_REF_CCW;
		m_fkey_option[3] = FK_MOVE_REF;
		break;

	case CUR_VALUE_SELECTED:
		m_fkey_option[0] = FK_SET_PARAMS;
		m_fkey_option[1] = FK_ROTATE_VALUE;
		m_fkey_option[2] = FK_ROTATE_VALUE_CCW;
		m_fkey_option[3] = FK_MOVE_VALUE;
		break;

	case CUR_PAD_SELECTED:
		if( m_sel.First()->ToPin()->net )
			m_fkey_option[0] = FK_DETACH_NET;
		else
			m_fkey_option[0] = FK_ATTACH_NET;
		m_fkey_option[1] = FK_START_TRACE;
		m_fkey_option[2] = FK_ADD_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_TEXT_SELECTED:
		m_fkey_option[0] = FK_EDIT_TEXT;
		m_fkey_option[3] = FK_MOVE_TEXT;
		m_fkey_option[7] = FK_DELETE_TEXT;
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		m_fkey_option[7] = FK_DELETE_CUTOUT;
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		{
			int style = m_sel.First()->ToSide()->m_style;
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		m_fkey_option[7] = FK_DELETE_CUTOUT;
		break;

	case CUR_BOARD_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		m_fkey_option[7] = FK_DELETE_OUTLINE;
		break;

	case CUR_BOARD_SIDE_SELECTED:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		{
			int style = m_sel.First()->ToSide()->m_style;
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		m_fkey_option[7] = FK_DELETE_OUTLINE;
		break;

	case CUR_AREA_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[1] = FK_EDIT_AREA;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		if (m_sel.First()->ToCorner()->IsOnCutout())
			m_fkey_option[6] = FK_DELETE_CUTOUT;
		else
			m_fkey_option[6] = FK_AREA_CUTOUT;
		m_fkey_option[7] = FK_DELETE_AREA;
		break;

	case CUR_AREA_SIDE_SELECTED:
		{
			m_fkey_option[0] = FK_SIDE_STYLE;
			m_fkey_option[1] = FK_EDIT_AREA;
			int style = m_sel.First()->ToSide()->m_style;
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
			if( m_sel.First()->ToSide()->IsOnCutout() )
				m_fkey_option[6] = FK_DELETE_CUTOUT;
			else
				m_fkey_option[6] = FK_AREA_CUTOUT;
			m_fkey_option[7] = FK_DELETE_AREA;
			break;
		}

	case CUR_SEG_SELECTED:
		{
			cseg2 *seg = m_sel.First()->ToSeg();
			cconnect2 *con = seg->m_con;
			m_fkey_option[0] = FK_SET_WIDTH;
			m_fkey_option[1] = FK_CHANGE_LAYER;
			m_fkey_option[2] = FK_ADD_VERTEX;
			// CPT2 TODO.  The old code was:
			// if (SegmentMovable())
				// m_fkey_option[3] = FK_MOVE_SEGMENT;
			// I'm hoping the following will suffice, but time will tell:
			if (!seg->preVtx->pin && !seg->postVtx->pin && !seg->preVtx->tee && !seg->postVtx->tee)
				m_fkey_option[3] = FK_MOVE_SEGMENT;
			if (!seg->preVtx->IsLooseEnd() && !seg->postVtx->IsLooseEnd())
				m_fkey_option[4] = FK_UNROUTE;
			if (!con->head->IsLooseEnd() && !con->tail->IsLooseEnd())
				m_fkey_option[5] = FK_UNROUTE_TRACE;
			m_fkey_option[6] = FK_DELETE_SEGMENT;
			m_fkey_option[7] = FK_DELETE_CONNECT;
			m_fkey_option[8] = FK_REDO_RATLINES;
			break;
		}

	case CUR_RAT_SELECTED:
		{
			cseg2 *seg = m_sel.First()->ToSeg();
			m_fkey_option[0] = FK_SET_WIDTH;
			if( seg->m_con->locked )
				m_fkey_option[2] = FK_UNLOCK_CONNECT;
			else
				m_fkey_option[2] = FK_LOCK_CONNECT;
			m_fkey_option[3] = FK_ROUTE;
			/* TODO Before it was:
			if( m_sel_con->NumSegs() > 1
				&& ( m_sel_id.I3() == 0 || m_sel_id.I3() == (m_sel_con->NumSegs()-1) ) )
			{
				// pin-pin connection
				m_fkey_option[4] = FK_CHANGE_PIN;
			}
			How about:
			*/
			if (seg->preVtx->pin && !seg->postVtx->pin || seg->postVtx->pin && !seg->preVtx->pin)
				m_fkey_option[4] = FK_CHANGE_PIN;
			m_fkey_option[5] = FK_UNROUTE_TRACE;
			m_fkey_option[7] = FK_DELETE_CONNECT;
			m_fkey_option[8] = FK_REDO_RATLINES;
			break;
		}

	case CUR_VTX_SELECTED:
		{
			cvertex2 *vtx = m_sel.First()->ToVertex();
			cconnect2 *con = vtx->m_con;
			m_fkey_option[0] = FK_VERTEX_PROPERTIES;
			m_fkey_option[1] = FK_START_TRACE;
			m_fkey_option[2] = FK_ADD_CONNECT;
			m_fkey_option[3] = FK_MOVE_VERTEX;
			if (!con->head->IsLooseEnd() && !con->tail->IsLooseEnd())
				m_fkey_option[5] = FK_UNROUTE_TRACE;
			m_fkey_option[6] = FK_DELETE_VERTEX;
			m_fkey_option[7] = FK_DELETE_CONNECT;
			m_fkey_option[8] = FK_REDO_RATLINES;
			break;
		}

	case CUR_END_VTX_SELECTED:
		m_fkey_option[0] = FK_VERTEX_PROPERTIES;
		m_fkey_option[1] = FK_ADD_SEGMENT;
		m_fkey_option[2] = FK_ADD_CONNECT;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		if( m_sel.First()->IsVia() )
			m_fkey_option[4] = FK_DELETE_VIA;
		else
			m_fkey_option[4] = FK_ADD_VIA;
		if (m_sel.First()->IsVia() || m_sel.First()->IsSlaveVtx())		// CPT2 added the slave-vtx option
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_DELETE_VERTEX;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_TEE_SELECTED:
		// CPT2, case is new.
		m_fkey_option[0] = FK_VERTEX_PROPERTIES;
		m_fkey_option[1] = FK_START_TRACE;
		m_fkey_option[2] = FK_ADD_CONNECT;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		m_fkey_option[6] = FK_DELETE_VERTEX;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_CONNECT_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_NET_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[2] = FK_EDIT_NET;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_GROUP_SELECTED:
		m_fkey_option[2] = FK_ROTATE_GROUP;
		m_fkey_option[3] = FK_MOVE_GROUP;
		m_fkey_option[7] = FK_DELETE_GROUP;
		break;

	case CUR_DRAG_PART:
		m_fkey_option[1] = FK_SIDE;
		m_fkey_option[2] = FK_ROTATE_PART;
		m_fkey_option[3] = FK_ROTATE_PART_CCW;
		break;

	case CUR_DRAG_REF:
		m_fkey_option[2] = FK_ROTATE_REF;
		break;

	case CUR_DRAG_VALUE:
		m_fkey_option[2] = FK_ROTATE_VALUE;
		break;

	case CUR_DRAG_TEXT:
		m_fkey_option[2] = FK_ROTATE_TEXT;
		break;

	case CUR_DRAG_VTX:
	case CUR_DRAG_END_VTX:
	case CUR_DRAG_VTX_INSERT:
		// CPT
        m_fkey_option[4] = FK_RGRID_DOWN;
        m_fkey_option[5] = FK_RGRID_UP;
		// END cpt
		break;

	case CUR_DRAG_RAT:
        // CPT
        m_fkey_option[0] = FK_ACTIVE_WIDTH_DOWN;
        m_fkey_option[1] = FK_ACTIVE_WIDTH_UP;
        m_fkey_option[4] = FK_RGRID_DOWN;
        m_fkey_option[5] = FK_RGRID_UP;
		// END cpt
		m_fkey_option[3] = FK_COMPLETE;
		break;

	case CUR_DRAG_STUB:
        // CPT
        m_fkey_option[0] = FK_ACTIVE_WIDTH_DOWN;
        m_fkey_option[1] = FK_ACTIVE_WIDTH_UP;
        m_fkey_option[4] = FK_RGRID_DOWN;
        m_fkey_option[5] = FK_RGRID_UP;
		// END cpt
		break;

	case CUR_DRAG_SMCUTOUT_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_SMCUTOUT:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_CUTOUT:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_CUTOUT_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_BOARD:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_BOARD_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;
	}


	// CPT: Lefthanded mode support:  if set, reverse key meanings
	if (m_doc->m_bLefthanded) 
		for (int lo=0, hi=8, tmp; lo<hi; lo++, hi--)
			tmp = m_fkey_option[lo], 
			m_fkey_option[lo] = m_fkey_option[hi],
			m_fkey_option[hi] = tmp;

	for( int i=0; i<12; i++ )
	{
		// CPT: now we store resource string id's rather than do a strcpy() as before
		int index = 2*m_fkey_option[i];
		m_fkey_rsrc[2*i] = IDS_FkStr+index;
		m_fkey_rsrc[2*i+1] = IDS_FkStr+index+1;
	}
	// end CPT

	InvalidateLeftPane();
	Invalidate( FALSE );
}

int CFreePcbView::SegmentMovable(void)
{
#ifdef CPT2
	return TRUE;
#else
	// see if this is the end of the road, if so, can't move it:
	{
		int x = m_sel_vtx->x;
		int y = m_sel_vtx->y;
		int layer = m_sel_seg->m_layer;
		BOOL test = m_doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
			m_sel_id.I2(), layer, 1 );
		if( test || m_sel_vtx->tee_ID )
		{
			return FALSE;
		}
	}

	// see if end vertex of this segment is in end pad of connection
	{
		int x = m_sel_next_vtx->x;
		int y = m_sel_next_vtx->y;
		int layer = m_sel_seg->m_layer;
		BOOL test = m_doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
			m_sel_id.I2(), layer, 0 );
		if( test || m_sel_next_vtx->tee_ID)
		{
			return FALSE;
		}
	}
	return TRUE;

#endif
}


// display selected item in status bar
//
int CFreePcbView::ShowSelectStatus()
{
// #define SHOW_UIDS	// show UIDs for selected element, mainly for debugging
	CString uid_str;
#ifdef SHOW_UIDS
	if( m_sel_id.U3() != -1 )
		uid_str.Format( ",uid %d %d %d", m_sel_id.U1() , m_sel_id.U2(), m_sel_id.U3() );
	else if( m_sel_id.U2() != -1 && m_sel_id.U2() != m_sel_id.U1() )
		uid_str.Format( ",uid %d %d", m_sel_id.U1() , m_sel_id.U2() );
	else
		uid_str.Format( ",uid %d", m_sel_id.U1() );
#endif

	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	int u = m_doc->m_units;
	CString x_str, y_str, w_str, hole_str, via_w_str, via_hole_str;
	CString str, s;
	cpcb_item *sel = m_sel.First();

	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		str.LoadStringA(IDS_NoSelection);
		break;

#ifndef CPT2
	case CUR_DRE_SELECTED:
		str.Format( "DRE %s", m_sel.First()->ToDRE()->str );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		{
			CString lay_str;
			CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
			if( poly->Layer() == LAY_SM_TOP )
				lay_str.LoadStringA(IDS_Top3);
			else
				lay_str.LoadStringA(IDS_Bottom2);
			::MakeCStringFromDimension( &x_str, poly->X(m_sel_id.I3()), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, poly->Y(m_sel_id.I3()), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			CString s ((LPCSTR) IDS_SolderMaskCutoutCorner);
			str.Format( s, m_sel_id.I2()+1, lay_str, m_sel_id.I3()+1, x_str, y_str, uid_str );
		}
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		{
			CString style_str;
			CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
			if( poly->SideStyle( m_sel_id.I3() ) == CPolyLine::STRAIGHT )
				style_str.LoadStringA(IDS_Straight);
			else if( poly->SideStyle( m_sel_id.I3() ) == CPolyLine::ARC_CW )
				style_str.LoadStringA(IDS_ArcCw);
			else if( poly->SideStyle( m_sel_id.I3() ) == CPolyLine::ARC_CCW )
				style_str.LoadStringA(IDS_ArcCcw);
			CString lay_str;
			if( poly->Layer() == LAY_SM_TOP )
				lay_str.LoadStringA(IDS_Top3);
			else
				lay_str.LoadStringA(IDS_Bottom2);
			CString s ((LPCSTR) IDS_SolderMaskCutoutSide);
			str.Format( s, m_sel_id.I2()+1, lay_str, m_sel_id.I3()+1,
				poly->NumCorners(), style_str, uid_str );
		}
		break;

	case CUR_BOARD_CORNER_SELECTED: 
		::MakeCStringFromDimension( &x_str, m_doc->m_board_outline[m_sel_id.I2()].X(m_sel_id.I3()), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &y_str, m_doc->m_board_outline[m_sel_id.I2()].Y(m_sel_id.I3()), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		s.LoadStringA(IDS_BoardOutlineCorner);
		str.Format( s, m_sel_id.I2()+1, m_sel_id.I3()+1, x_str, y_str, uid_str );
		break;

	case CUR_BOARD_SIDE_SELECTED:
		{
			CString style_str;
			if( m_doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() ) == CPolyLine::STRAIGHT )
				style_str.LoadStringA(IDS_Straight);
			else if( m_doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() ) == CPolyLine::ARC_CW )
				style_str.LoadStringA(IDS_ArcCw);
			else if( m_doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() ) == CPolyLine::ARC_CCW )
				style_str.LoadStringA(IDS_ArcCcw);
			s.LoadStringA(IDS_BoardOutlineSide);
			str.Format( s, m_sel_id.I2()+1, m_sel_id.I3()+1,
				m_doc->m_board_outline[m_sel_id.I2()].NumCorners(), style_str, uid_str );
		}
		break;
#endif

	case CUR_PART_SELECTED:
		{
			cpart2 *part = sel->ToPart();
			CString side ((LPCSTR) IDS_Top);
			if( part->side )
				side.LoadStringA(IDS_Bottom);
			::MakeCStringFromDimension( &x_str, part->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, part->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			int rep_angle = ::GetReportedAngleForPart( part->angle, part->shape->m_centroid_angle, part->side );
			s.LoadStringA(IDS_PartXYAngle);
			str.Format( s, part->ref_des, part->shape->m_name, x_str, y_str, rep_angle, side );
		}
		break;

	case CUR_REF_SELECTED:
		{
			creftext *rt = sel->ToRefText();
			s.LoadStringA(IDS_RefText);
			str.Format( s, rt->m_str );
			break;
		}

	case CUR_VALUE_SELECTED:
		{
			cvaluetext *vt = sel->ToValueText();
			s.LoadStringA(IDS_Value2);
			str.Format( s, vt->m_str );
			break;
		}

	case CUR_PAD_SELECTED:
		{
			cpin2 *pin = sel->ToPin();
			::MakeCStringFromDimension( &x_str, pin->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, pin->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			if( pin->net )
			{
				// pad attached to net
				CString s ((LPCSTR) IDS_PinOnNet);
				str.Format( s, pin->part->ref_des,
					pin->pin_name, pin->net->name, x_str, y_str, uid_str );
			}
			else
			{
				// pad not attached to a net
				CString s ((LPCSTR) IDS_PinUnconnected);
				str.Format( s, pin->part->ref_des,
					pin->pin_name, x_str, y_str, uid_str );
			}
			break;
		}

	case CUR_SEG_SELECTED:
	case CUR_RAT_SELECTED:
	case CUR_DRAG_RAT:
		{
            // CPT.  If dragging a ratline, we want to display the active width
            int width = 0;
            if (m_cursor_mode==CUR_DRAG_RAT)
                width = m_active_width;
			// End CPT.

			cseg2 *seg = sel->ToSeg();
			CString con_str, seg_str;
			seg->m_con->GetStatusStr( &con_str );
			seg->GetStatusStr( &seg_str, width );			// CPT added arg
			str = con_str + ", " + seg_str;
			str = str + uid_str;
			break;
		}

	case CUR_DRAG_STUB:
		{
			// CPT2.  In this mode, the selected object is always the starting vertex.  Just show connection info, plus active width
			CString con_str, w_str, s;
			cconnect2 *c = sel->ToVertex()->m_con;
			c->GetStatusStr( &con_str );
			::MakeCStringFromDimension( &w_str, m_active_width, m_doc->m_units, FALSE, FALSE, FALSE, u==MIL?1:3 );
			s.LoadStringA(IDS_W);
			str.Format(s, con_str, w_str);
			break;
		}

	case CUR_VTX_SELECTED:
	case CUR_END_VTX_SELECTED:
		{
			CString con_str, vtx_str;
			cvertex2 *v = sel->ToVertex();
			v->m_con->GetStatusStr( &con_str );
			v->GetStatusStr( &vtx_str );
			str = con_str + "; " + vtx_str;
			str = str + uid_str;
			break;
		}

	case CUR_TEE_SELECTED:
		{
			CString con_str, vtx_str;
			sel->GetNet()->GetStatusStr( &str );
			sel->GetStatusStr( &vtx_str );
			str = str + "; " + vtx_str;
			break;
		}

	case CUR_CONNECT_SELECTED:
		{
			cconnect2 *c = sel->ToConnect();
			CString con_str;
			c->GetStatusStr( &con_str );
			// get length of trace
			CString len_str;
			double len = 0;
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
			{
				double dx = s->preVtx->x - s->postVtx->x;
				double dy = s->preVtx->y - s->postVtx->y;
				len += sqrt( dx*dx + dy*dy );
			}
			::MakeCStringFromDimension( &len_str, (int)len, u, TRUE, TRUE, FALSE, u==MIL?1:3 );
			CString s ((LPCSTR) IDS_Length);
			str.Format(s, con_str, len_str);
		}
		break;

	/* CPT2 TODO confirm that this is obsolete
	case CUR_NET_SELECTED:
		s.LoadStringA(IDS_Net);
		str.Format( s, m_sel_net->name );
		break;
	*/

	case CUR_TEXT_SELECTED:
		{
			ctext *t = sel->ToText();
			CString neg_str = "";
			if( t->m_bNegative )
				neg_str.LoadStringA(IDS_Neg);
			CString s ((LPCSTR) IDS_Text);
			str.Format( s, t->m_str, neg_str );
			break;
		}

	case CUR_AREA_CORNER_SELECTED:
		{
			ccorner *c = sel->ToCorner();
			cpolyline *p = c->contour->poly;
			cnet2 *net = c->GetNet();
			::MakeCStringFromDimension( &x_str, c->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, c->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			CString s ((LPCSTR) IDS_CopperAreaCorner);
			str.Format( s, net->name, p->UID(), c->UID(), x_str, y_str, uid_str );
		}
		break;

	case CUR_AREA_SIDE_SELECTED:
		{
			cside *side = sel->ToSide();
			cpolyline *p = side->contour->poly;
			cnet2 *net = side->GetNet();
			if (!side->IsOnCutout()) 
				s.LoadStringA(IDS_CopperAreaEdge),
				str.Format( s, net->name, p->UID(), side->UID() );
			else
				s.LoadStringA(IDS_CopperAreaCutoutEdge),
				str.Format( s, net->name, p->UID(), side->contour->UID(), side->UID() );
		}
		break;

	case CUR_GROUP_SELECTED:
		str.LoadStringA(IDS_GroupSelected);
		break;

	case CUR_ADD_BOARD:
		str.LoadStringA(IDS_PlacingFirstCornerOfBoardOutline);
		break;

	case CUR_DRAG_BOARD_1:
		str.LoadStringA(IDS_PlacingSecondCornerOfBoardOutline);
		break;

#ifndef CPT2
	case CUR_DRAG_BOARD:
		s.LoadStringA(IDS_PlacingCornerOfBoardOutline);
		str.Format( s, m_sel_id.I3()+2 );
		break;

	case CUR_DRAG_BOARD_INSERT:
		s.LoadStringA(IDS_InsertingCornerOfBoardOutline);
		str.Format( s, m_sel_id.I3()+2 );
		break;

	case CUR_DRAG_BOARD_MOVE:
		s.LoadStringA(IDS_MovingCornerOfBoardOutline);
		str.Format( s, m_sel_id.I3()+1 );
		break;
#endif

	case CUR_DRAG_PART:
		s.LoadStringA(IDS_MovingPart);
		str.Format( s, sel->ToPart()->ref_des );
		break;

	case CUR_DRAG_REF:
		s.LoadStringA(IDS_MovingRefTextForPart);
		str.Format( s, sel->ToRefText()->m_str );
		break;

	case CUR_DRAG_VTX:
	case CUR_DRAG_END_VTX:
		s.LoadStringA(IDS_RoutingNet);
		str.Format( s, sel->GetNet()->name );
		break;

	case CUR_DRAG_TEXT:
		s.LoadStringA(IDS_MovingText);
		str.Format( s, sel->ToText()->m_str );
		break;

	case CUR_ADD_AREA:
		str.LoadStringA(IDS_PlacingFirstCornerOfCopperArea);
		break;

	case CUR_DRAG_AREA_1:
		str.LoadStringA(IDS_PlacingSecondCornerOfCopperArea);
		break;

	case CUR_DRAG_AREA:
		s.LoadStringA(IDS_PlacingCornerOfCopperArea);
		str.Format( s, sel->UID() );
		break;

	case CUR_DRAG_AREA_INSERT:
		s.LoadStringA(IDS_InsertingCornerOfCopperArea);
		str.Format( s, sel->UID() );
		break;

	case CUR_DRAG_AREA_MOVE:
		s.LoadStringA(IDS_MovingCornerOfCopperArea);
		str.Format( s, sel->UID() );
		break;

	case CUR_DRAG_CONNECT:
		if( cpin2 *p = sel->ToPin() )
			s.LoadStringA(IDS_AddingConnectionToPin),
			str.Format( s, p->part->ref_des, p->pin_name );
		else if( sel->IsVertex() )
			s.LoadStringA(IDS_AddingBranchToTrace),
			str.Format( s, sel->GetNet()->name,	sel->UID() );
		break;

	case CUR_DRAG_MEASURE_1:
		str.LoadStringA(IDS_MeasurementModeLeftClickToStart);
		break;

	}
	pMain->DrawStatus( 3, &str );
	return 0;
}

// display cursor coords in status bar
//
// display active layer in status bar and change layer order for DisplayList
//
int CFreePcbView::ShowActiveLayer()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str, s;
	if( m_active_layer == LAY_TOP_COPPER )
		str.LoadStringA( IDS_Top3 );
	else if( m_active_layer == LAY_BOTTOM_COPPER )
		str.LoadStringA( IDS_Bottom2 );
	else if( m_active_layer > LAY_BOTTOM_COPPER )
		s.LoadStringA(IDS_Inner2),
		str.Format( s, m_active_layer - LAY_BOTTOM_COPPER );
	pMain->DrawStatus( 4, &str );
	for( int order=LAY_TOP_COPPER; order<LAY_TOP_COPPER+m_doc->m_num_copper_layers; order++ )
	{
		if( order == LAY_TOP_COPPER )
			m_dlist->SetLayerDrawOrder( m_active_layer, order );
		else if( order <= m_active_layer )
			m_dlist->SetLayerDrawOrder( order-1, order );
		else
			m_dlist->SetLayerDrawOrder( order, order );
	}
	Invalidate( FALSE );
	return 0;
}

// handle mouse scroll wheel
//
BOOL CFreePcbView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_sel_offset = -1;							// CPT:  the current series of left-clicks has been interrupted...
	// ignore if cursor not in window
	CRect wr;
	GetWindowRect( wr );
	if( pt.x < wr.left || pt.x > wr.right || pt.y < wr.top || pt.y > wr.bottom )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	// ignore if we are dragging a selection rect
	if( m_bDraggingRect )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	return CCommonView::OnMouseWheel(nFlags, zDelta, pt);
}

/* Old version, pre-CCommonView reorganization
BOOL CFreePcbView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
#define MIN_WHEEL_DELAY 1.0

	static struct _timeb current_time;
	static struct _timeb last_time;
	static int first_time = 1;
	double diff;

	// ignore if cursor not in window
	CRect wr;
	GetWindowRect( wr );
	if( pt.x < wr.left || pt.x > wr.right || pt.y < wr.top || pt.y > wr.bottom )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	// ignore if we are dragging a selection rect
	if( m_bDraggingRect )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	// get current time
	_ftime( &current_time );

	if( first_time )
	{
		diff = 999.0;
		first_time = 0;
	}
	else
	{
		// get elapsed time since last wheel event
		diff = difftime( current_time.time, last_time.time );
		double diff_mil = (double)(current_time.millitm - last_time.millitm)*0.001;
		diff = diff + diff_mil;
	}

	if( diff > MIN_WHEEL_DELAY )
	{
		// first wheel movement in a while
		// center window on cursor then center cursor
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = m_dlist->ScreenToPCB( p );
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
		Invalidate( FALSE );
		p = m_dlist->PCBToScreen( p );
		SetCursorPos( p.x, p.y - 4 );
	}
	else
	{
		// serial movements, zoom in or out
		if( zDelta > 0 && m_pcbu_per_pixel > NM_PER_MIL/1000 )
		{
			// wheel pushed, zoom in then center world coords and cursor
			CPoint p;
			GetCursorPos( &p );		// cursor pos in screen coords
			p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
			CRect screen_r;
			GetWindowRect( &screen_r );
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
			Invalidate( FALSE );
			p = m_dlist->PCBToScreen( p );
			SetCursorPos( p.x, p.y - 4 );
		}
		else if( zDelta < 0 )
		{
			// wheel pulled, zoom out then center
			// first, make sure that window boundaries will be OK
			CPoint p;
			GetCursorPos( &p );		// cursor pos in screen coords
			p = m_dlist->ScreenToPCB( p );
			int org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
			int org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
			int max_x = org_x + (m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO;
			int max_y = org_y + (m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO;
			if( org_x > -PCB_BOUND && org_x < PCB_BOUND && max_x > -PCB_BOUND && max_x < PCB_BOUND
				&& org_y > -PCB_BOUND && org_y < PCB_BOUND && max_y > -PCB_BOUND && max_y < PCB_BOUND )
			{
				// OK, do it
				m_org_x = org_x;
				m_org_y = org_y;
				m_pcbu_per_pixel = m_pcbu_per_pixel*ZOOM_RATIO;
				CRect screen_r;
				GetWindowRect( &screen_r );
				m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
				Invalidate( FALSE );
				p = m_dlist->PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
			}
		}
	}
	last_time = current_time;

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}
*/

// SelectPart...this is called from FreePcbDoc when a new part is added
// selects the new part as long as the cursor is not dragging something
// CPT2.  TODO eliminate
//
int CFreePcbView::SelectPart( cpart * part )
{
#ifndef CPT2
	if(	!CurDragging() )
	{
		// deselect previously selected item
		if( m_sel_id.T1() )
			CancelSelection();

		// select part
		m_sel_part = part;
		m_sel_id = part->m_id;
		m_sel_id.SetT2( ID_SEL_RECT );
		m_doc->m_plist->SelectPart( m_sel_part );
		SetCursorMode( CUR_PART_SELECTED );
	}
	m_lastKeyWasArrow = FALSE;
	m_lastKeyWasGroupRotate=false;
	Invalidate( FALSE );
#endif
	return 0;
}

// cancel any highlights
//
void CFreePcbView::CancelHighlight()
{
	m_dlist->CancelHighlight();
	m_highlight_net = NULL;
	Invalidate(false);					// CPT r293
}

// cancel selection.  CPT2 updated
//
void CFreePcbView::CancelSelection()
{
	cnet2 *highlight_net0 = m_highlight_net;
	CancelHighlight();

	/* CPT2 see proposal in notes.txt
	// AMW r274
	if( highlight_net0 )
		// rehighlight but don't select
		m_highlight_net = highlight_net0,
		highlight_net0->Highlight();
	// end AMW
	*/
	m_sel.RemoveAll();

	// CPT
	m_active_width = 0;
	m_dragging_new_item = FALSE;
	// end CPT

	SetCursorMode( CUR_NONE_SELECTED );
}

void CFreePcbView::HighlightSelection()
{
	// CPT2 Was named HighlightGroup(), but renamed since it works regardless of how many items are in m_sel.
	// Now also sets the cursor mode based on what's in m_sel.
	CancelHighlight();
	citer<cpcb_item> ii (&m_sel);
	for (cpcb_item *i = ii.First(); i; i = ii.Next())
		i->Highlight();

	cpcb_item *first = m_sel.First();
	if (m_sel.GetSize()==0)
		SetCursorMode( CUR_NONE_SELECTED );
	else if (m_sel.GetSize()>=2)
		SetCursorMode( CUR_GROUP_SELECTED );
	else if (first->IsDRE())
		SetCursorMode( CUR_DRE_SELECTED );
	else if( first->IsBoardCorner() )
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
	else if( first->IsBoardSide() )
		SetCursorMode( CUR_BOARD_SIDE_SELECTED );
	else if( first->IsSmCorner() )
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
	else if( first->IsSmSide() )
		SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
	else if( first->IsPart() )
		SetCursorMode( CUR_PART_SELECTED );				// CPT2 TODO Make sure cpart2::Highlight has dealt with ref text and value text
	else if( first->IsRefText() )
		SetCursorMode( CUR_REF_SELECTED );				// CPT2 NB ref and value-text selection is independent of part selection (except that 
														// selecting a part does highlight the ref and value)
	else if( first->IsValueText() )
		SetCursorMode( CUR_VALUE_SELECTED );
	else if( first->IsPin() )
		SetCursorMode( CUR_PAD_SELECTED );
	else if( cseg2 *s = first->ToSeg() )
		if (s->m_layer == LAY_RAT_LINE)
			SetCursorMode( CUR_RAT_SELECTED );
		else
			SetCursorMode( CUR_SEG_SELECTED );
	else if( cvertex2 *v = first->ToVertex() )
		if (!v->preSeg || !v->postSeg)
			SetCursorMode( CUR_END_VTX_SELECTED );		// CPT2 TODO Is the distinction of CUR_END_VTX_SELECTED necessary?
		else
			SetCursorMode( CUR_VTX_SELECTED );
	else if ( first->IsTee())
		SetCursorMode( CUR_TEE_SELECTED );
	else if( first->IsAreaSide() )
		SetCursorMode( CUR_AREA_SIDE_SELECTED );
	else if( first->IsAreaCorner() )
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
	else if( first->IsConnect() )
		// CPT:  account for the possibility of whole-trace selection.
		// CPT2:  TODO figure out if this is salvageable
		SetCursorMode( CUR_CONNECT_SELECTED );
	else if( first->IsText() )
		SetCursorMode( CUR_TEXT_SELECTED );
	else
		// nothing selected (CPT2: UNLIKELY I HOPE)
		SetCursorMode( CUR_NONE_SELECTED );

	m_lastKeyWasArrow = FALSE;
	m_lastKeyWasGroupRotate=false;
	Invalidate(false);
}

// highlight all segments, vertices and pads in net, except for excluded_id.
// CPT2 supplanted with cnet2::Highlight(cpcb_item *except)
//
/*
void CFreePcbView::HighlightNet( cnet * net, id * exclude_id ) 
{
#ifndef CPT2
	if( net == NULL )
		return;
	m_doc->m_nlist->HighlightNet( net, exclude_id ); 
	m_doc->m_plist->HighlightAllPadsOnNet( net ); 
	m_bNetHighlighted = TRUE;
#endif
}
*/

// cancel net highlight, reselect selected item if not dragging
// CPT2 updated.  TODO check that this is right
void CFreePcbView::CancelHighlightNet()
{
	CancelHighlight();
	if( !CurDragging() )
		HighlightSelection();
	m_highlight_net = NULL;
}

// attempt to reselect area corner based on position
// should be used after areas are modified
void CFreePcbView::TryToReselectAreaCorner( int x, int y )
{
#ifndef CPT2
	CancelHighlight();
	for( int ia=0; ia<m_sel_net->NumAreas(); ia++ )
	{
		for( int ic=0; ic<m_sel_net->area[ia].NumCorners(); ic++ )
		{
			carea * sel_a = & m_sel_net->area[ia];
			if( x == sel_a->X(ic)
				&& y == sel_a->Y(ic) )
			{
				// found matching corner
				m_sel_id = id(ID_NET, m_sel_net->UID(), ID_AREA, sel_a->UID(), ia, 
					ID_SEL_CORNER, sel_a->CornerUID(ic), ic);
				SetCursorMode( CUR_AREA_CORNER_SELECTED );
				m_doc->m_nlist->HighlightAreaCorner( m_sel_net, ia, ic );
				return;
			}
		}
	}
	CancelSelection();
#endif
}


// set trace width using dialog
// enter with:
//	mode = 0 if called with segment selected
//	mode = 1 if called with connection selected
//	mode = 2 if called with net selected
//
int CFreePcbView::SetWidth( int mode )
{
#ifndef CPT2
	// set parameters for dialog
	DlgSetSegmentWidth dlg;
	dlg.m_w = &m_doc->m_w;
	dlg.m_v_w = &m_doc->m_v_w;
	dlg.m_v_h_w = &m_doc->m_v_h_w;
	dlg.m_init_w = m_doc->m_trace_w;
	dlg.m_init_via_w = m_doc->m_via_w;
	dlg.m_init_via_hole_w = m_doc->m_via_hole_w;
	if( mode == 0 )
	{
		cseg * seg = m_sel_seg;
		cconnect * con = m_sel_con;
		int seg_w = seg->m_width;
		if( seg_w )
			dlg.m_init_w = seg_w;
		else if( m_sel_net->def_w )
			dlg.m_init_w = m_sel_net->def_w;
	}
	else
	{
		if( m_sel_net->def_w )
			dlg.m_init_w = m_sel_net->def_w;
	}

	// launch dialog
	dlg.m_mode = mode;
	int ret = dlg.DoModal();
	int w = 0;
	int via_w = 0;
	int via_hole_w = 0;
	if( ret == IDOK )
	{
		// returned with "OK"
		w = dlg.m_width;
		via_w = dlg.m_via_width;
		via_hole_w = dlg.m_hole_width;
		if( dlg.m_tv == 3 )
			w = 0;
		else if( dlg.m_tv == 2 )
			via_w = 0;
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );

		// set default values for net or connection
		if( dlg.m_def == 2 )
		{
			// set default for net
			if( w )
				m_sel_net->def_w = w;
			if( via_w )
			{
				m_sel_net->def_via_w = via_w;
				m_sel_net->def_via_hole_w = via_hole_w;
			}
		}
		// apply new widths to net, connection or segment
		if( dlg.m_apply == 3 )
		{
			// apply width to net
			m_doc->m_nlist->SetNetWidth( m_sel_net, w, via_w, via_hole_w );
		}
		else if( dlg.m_apply == 2 )
		{
			// apply width to connection
			m_doc->m_nlist->SetConnectionWidth( m_sel_net, m_sel_ic, w, via_w, via_hole_w );
		}
		else if( dlg.m_apply == 1 )
		{
			// apply width to segment
			m_doc->m_nlist->SetSegmentWidth( m_sel_net, m_sel_ic,
				m_sel_id.I3(), w, via_w, via_hole_w );
		}
	}
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
	return 0;
}

// context-sensitive menu invoked by right-click
//
void CFreePcbView::OnContextMenu(CWnd* pWnd, CPoint point )
{
	if( m_disable_context_menu )
	{
		// right-click already handled, don't pop up menu
		m_disable_context_menu = 0;
		return;
	}
	if( !m_doc->m_project_open )	// no project open
		return;

	// OK, pop-up context menu
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_CONTEXT));
	CMenu* pPopup;
	int style;
	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NONE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_BOARD_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_BOARD_CORNER);
		ASSERT(pPopup != NULL);
		if( m_doc->m_board_outline[m_sel_id.I2()].NumCorners() < 4 )
				pPopup->EnableMenuItem( ID_BOARDCORNER_DELETECORNER, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_BOARD_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_BOARD_SIDE);
		ASSERT(pPopup != NULL);
		style = m_doc->m_board_outline[m_sel_id.I2()].SideStyle( m_sel_id.I3() );
		if( style == CPolyLine::STRAIGHT )
		{
			int xi = m_doc->m_board_outline[m_sel_id.I2()].X( m_sel_id.I3() );
			int yi = m_doc->m_board_outline[m_sel_id.I2()].Y( m_sel_id.I3() );
			int xf, yf;
			if( m_sel_id.I3() != (m_doc->m_board_outline[m_sel_id.I2()].NumCorners()-1) )
			{
				xf = m_doc->m_board_outline[m_sel_id.I2()].X( m_sel_id.I3()+1 );
				yf = m_doc->m_board_outline[m_sel_id.I2()].Y( m_sel_id.I3()+1 );
			}
			else
			{
				xf = m_doc->m_board_outline[m_sel_id.I2()].X( 0 );
				yf = m_doc->m_board_outline[m_sel_id.I2()].Y( 0 );
			}
			if( xi == xf || yi == yf )
			{
				pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CW, MF_GRAYED );
				pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CCW, MF_GRAYED );
			}
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOSTRAIGHTLINE, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CW )
		{
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CW, MF_GRAYED );
			pPopup->EnableMenuItem( ID_BOARDSIDE_INSERTCORNER, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CCW )
		{
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CCW, MF_GRAYED );
			pPopup->EnableMenuItem( ID_BOARDSIDE_INSERTCORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_PART_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PART);
		ASSERT(pPopup != NULL);
		if( m_sel_part->glued )
			pPopup->EnableMenuItem( ID_PART_GLUE, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_PART_UNGLUE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_REF_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_REF_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_VALUE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VALUE_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_PAD_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PAD);
		ASSERT(pPopup != NULL);
		if( m_sel_part->pin[m_sel_id.I2()].net )
			pPopup->EnableMenuItem( ID_PAD_ADDTONET, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_PAD_DETACHFROMNET, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SEG_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SEGMENT);
		ASSERT(pPopup != NULL);

		if(!SegmentMovable())
				pPopup->EnableMenuItem( ID_SEGMENT_MOVE, MF_GRAYED );

		if( m_sel_con->end_pin == cconnect::NO_END
			&& m_sel_con->VtxByIndex(m_sel_con->NumSegs()).tee_ID == 0
			&& m_sel_con->VtxByIndex(m_sel_con->NumSegs()).force_via_flag == 0
			)
		{
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		}
		if( m_sel_con->end_pin == cconnect::NO_END
			&& m_sel_con->NumSegs() == (m_sel_id.I3()+1)
			&& m_sel_con_last_vtx->tee_ID == 0
			&& m_sel_con_last_vtx->force_via_flag == 0
			)
		{
			// last segment of stub trace unless a tee or via
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTE, MF_GRAYED );
		}
		if( m_sel_con->end_pin != cconnect::NO_END
			|| m_sel_con->NumSegs() > (m_sel_id.I3()+1)
			)
		{
			pPopup->EnableMenuItem( ID_SEGMENT_DELETE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_RAT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_RATLINE);
		ASSERT(pPopup != NULL);
		if( m_sel_con->locked )
			pPopup->EnableMenuItem( ID_RATLINE_LOCKCONNECTION, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_RATLINE_UNLOCKCONNECTION, MF_GRAYED );
		if( m_sel_con->end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		if( m_sel_con->NumSegs() == 1
			|| !(m_sel_id.I3() == 0 || m_sel_id.I3() == (m_sel_con->NumSegs()-1) ) )
			pPopup->EnableMenuItem( ID_RATLINE_CHANGEPIN, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VERTEX);
		ASSERT(pPopup != NULL);
		if( m_sel_vtx->via_w == 0 )
			pPopup->EnableMenuItem( ID_VERTEX_SETSIZE, MF_GRAYED );
		if( m_sel_con->end_pin == cconnect::NO_END
			&& m_sel_con_last_vtx->tee_ID == 0
			&& m_sel_con_last_vtx->force_via_flag == 0
			)
		{
			pPopup->EnableMenuItem( ID_VERTEX_UNROUTETRACE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_END_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_END_VERTEX);
		ASSERT(pPopup != NULL);
		if( m_sel_vtx->via_w )
			pPopup->EnableMenuItem( ID_ENDVERTEX_ADDVIA, MF_GRAYED );
		else
		{
			pPopup->EnableMenuItem( ID_ENDVERTEX_SETSIZE, MF_GRAYED );
			pPopup->EnableMenuItem( ID_ENDVERTEX_REMOVEVIA, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_CONNECT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_CONNECT);
		ASSERT(pPopup != NULL);
		if( m_sel_con->end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_CONNECT_UNROUTETRACE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_NET_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NET);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_TEXT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_AREA_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_CORNER);
		ASSERT(pPopup != NULL);
		{
			carea * area = &m_sel_net->area[m_sel_ia];
			if( area->Contour( m_sel_id.I3() ) == 0 )
				pPopup->EnableMenuItem( ID_AREACORNER_DELETECUTOUT, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_AREACORNER_ADDCUTOUT, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_AREA_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_EDGE);
		ASSERT(pPopup != NULL);
		{
			carea * area = &m_sel_net->area[m_sel_ia];
			if( area->Contour( m_sel_id.I3() ) == 0 )
				pPopup->EnableMenuItem( ID_AREAEDGE_DELETECUTOUT, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_AREAEDGE_ADDCUTOUT, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SM_SIDE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SM_CORNER);
		ASSERT(pPopup != NULL);
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
			if( poly->NumCorners() < 4 )
				pPopup->EnableMenuItem( ID_SMCORNER_DELETECORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_GROUP_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_GROUP);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;
	}
}

// add copper area
//
void CFreePcbView::OnAddArea()
{
#ifndef CPT2
	CDlgAddArea dlg;
	dlg.Initialize( m_doc->m_nlist, m_doc->m_num_layers, NULL, m_active_layer, -1 );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( !dlg.m_net )
		{
			CString str, s ((LPCSTR) IDS_NetNotFound);
			str.Format( s, dlg.m_net_name );
			AfxMessageBox( str, MB_OK );
		}
		else
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			CancelHighlight();
			SetCursorMode( CUR_ADD_AREA );
			// make layer visible
			m_active_layer = dlg.m_layer;
			m_doc->m_vis[m_active_layer] = TRUE;
			m_dlist->SetLayerVisible( m_active_layer, TRUE );
			ShowActiveLayer();
			m_sel_net = dlg.m_net;
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
				m_last_cursor_point.y, 0, m_active_layer, 2 );
			m_polyline_style = CPolyLine::STRAIGHT;
			m_polyline_hatch = dlg.m_hatch;
			Invalidate( FALSE );
			ReleaseDC( pDC );
		}
	}
#endif
}

// add copper area cutout
//
void CFreePcbView::OnAreaAddCutout()
{
	// check if any non-straight sides
	BOOL bArcs = FALSE;
	CPolyLine * poly = &m_sel_net->area[m_sel_ia];
	int ns = poly->NumCorners();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CancelHighlight();
	SetCursorMode( CUR_ADD_AREA_CUTOUT );
	// make layer visible
	m_active_layer = m_sel_net->area[m_sel_ia].Layer();
	m_doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPolyLine::STRAIGHT;
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnAreaDeleteCutout()
{
#ifndef CPT2
	CPolyLine * poly = &m_sel_net->area[m_sel_ia];
	int icont = poly->Contour( m_sel_id.I3() );
	if( icont < 1 )
		ASSERT(0);
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_doc->m_undo_list );
	poly->RemoveContour( icont );
	CancelSelection();
	m_doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
	if( m_doc->m_vis[LAY_RAT_LINE] )
		m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
	Invalidate( FALSE );
#endif
}

// move part
//
void CFreePcbView::OnPartMove()
{
#ifndef CPT2
	// check for glue
	if( m_sel_part->glued )
	{
		CString s ((LPCSTR) IDS_ThisPartIsGluedDoYouWantToUnglueIt);
		int ret = AfxMessageBox( s, MB_YESNO );
		if( ret != IDYES )
			return;
	}
	// drag part
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint p;
	p.x  = m_sel_part->x;
	p.y  = m_sel_part->y;
	m_from_pt = p;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	BOOL bRatlines = m_doc->m_vis[LAY_RAT_LINE];
	m_doc->m_plist->StartDraggingPart( pDC, m_sel_part, bRatlines, 
					m_doc->m_auto_ratline_disable, m_doc->m_auto_ratline_min_pins );
	SetCursorMode( CUR_DRAG_PART );
	Invalidate( FALSE );
	ReleaseDC( pDC );
#endif
}

// add text string
//
void CFreePcbView::OnTextAdd()
{
#ifndef CPT2
	// create, initialize and show dialog
	CDlgAddText add_text_dlg;
	CString str = "";
	add_text_dlg.Initialize( 0, m_doc->m_num_layers, 1, &str, m_doc->m_units,
			LAY_SILK_TOP, 0, 0, 0, 0, 0, 0, 0 );
	add_text_dlg.m_num_layers = m_doc->m_num_layers;
	add_text_dlg.m_bDrag = 1;
	// defaults for dialog
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;
	else
	{
		int x = add_text_dlg.m_x;
		int y = add_text_dlg.m_y;
		int mirror = add_text_dlg.m_bMirror;
		BOOL bNegative = add_text_dlg.m_bNegative;
		int angle = add_text_dlg.m_angle;
		int font_size = add_text_dlg.m_height;
		int stroke_width = add_text_dlg.m_width;
		int layer = add_text_dlg.m_layer;
		CString str = add_text_dlg.m_str;
		m_doc->m_vis[layer] = TRUE;
		m_dlist->SetLayerVisible( layer, TRUE );

		// get cursor position and convert to PCB coords
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
		// set pDC to PCB coords
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		if( add_text_dlg.m_bDrag )
		{
			m_sel_text = m_doc->m_tlist->AddText( p.x, p.y, angle, mirror, bNegative,
				layer, font_size, stroke_width, &str );
			m_dragging_new_item = 1;
			m_doc->m_tlist->StartDraggingText( pDC, m_sel_text );
			SetCursorMode( CUR_DRAG_TEXT );
		}
		else
		{
			m_sel_text = m_doc->m_tlist->AddText( x, y, angle, mirror, bNegative,
				layer, font_size,  stroke_width, &str );
			SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD, TRUE, m_doc->m_undo_list );
			m_doc->m_tlist->HighlightText( m_sel_text );
		}
		ReleaseDC( pDC );
		Invalidate( FALSE );
	}
#endif
}

// delete text ... enter with text selected
//
void CFreePcbView::OnTextDelete()
{
#ifndef CPT2
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_DELETE, TRUE, m_doc->m_undo_list );
	m_doc->m_tlist->RemoveText( m_sel_text );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// move text, enter with text selected
//
void CFreePcbView::OnTextMove()
{
#ifndef CPT2
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to text origin
	CPoint p;
	p.x  = m_sel_text->m_x;
	p.y  = m_sel_text->m_y;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start moving
	CancelHighlight();
	m_dragging_new_item = 0;
	m_doc->m_tlist->StartDraggingText( pDC, m_sel_text );
	SetCursorMode( CUR_DRAG_TEXT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
#endif
}

// glue part.  CPT2 updated (it was easy)
//
void CFreePcbView::OnPartGlue()
{
	SaveUndoInfoForPart( m_sel.First()->ToPart(),
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
	m_sel_part->glued = 1;
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// unglue part
//
void CFreePcbView::OnPartUnglue()
{
	SaveUndoInfoForPart( m_sel.First()->ToPart(),
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
	m_sel_part->glued = 0;
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// delete part
//
void CFreePcbView::OnPartDelete()
{
#ifndef CPT2
	// delete part
	CString mess, s ((LPCSTR) IDS_DeletingPartDoYouWishToRemoveAllReferences);
	mess.Format( s, m_sel_part->ref_des );
	int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
	if( ret == IDCANCEL )
		return;
	// save undo info
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_DELETE, NULL, TRUE, m_doc->m_undo_list );
	// now do it
	if( ret == IDYES )
		m_doc->m_nlist->PartDeleted( m_sel_part, TRUE );
	else if( ret == IDNO )
		m_doc->m_nlist->PartDisconnected( m_sel_part, TRUE );
	m_doc->m_plist->Remove( m_sel_part );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// optimize all nets to part
//
void CFreePcbView::OnPartOptimize()
{
#ifndef CPT2
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
	m_doc->m_nlist->OptimizeConnections( m_sel_part, FALSE, -1 );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// move ref. designator text for part
//
void CFreePcbView::OnRefMove()
{
#ifndef CPT2
	// move reference ID
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint cur_p = m_dlist->PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	m_doc->m_plist->StartDraggingRefText( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_REF );
	ReleaseDC( pDC );
	Invalidate( FALSE );
#endif
}

// optimize net for this pad
//
void CFreePcbView::OnPadOptimize()
{
#ifndef CPT2
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.I2()].net;
	if( pin_net )
	{
		m_doc->m_nlist->OptimizeConnections( pin_net, -1, FALSE, -1, FALSE );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
#endif
}

// start stub trace from this pad
// CPT2 converted.  Quite similar to OnVertexStartTrace().
void CFreePcbView::OnPadStartTrace()
{
	cpin2 *pin = m_sel.First()->ToPin();
	cnet2 *net = pin->net;
	if( !net )
	{
		CString s ((LPCSTR) IDS_PadMustBeAssignedToANetBeforeAddingTrace);
		AfxMessageBox( s, MB_OK );
		return;
	}

	SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	m_snap_angle_ref.x = pin->x;
	m_snap_angle_ref.y = pin->y;
	// now add new connection and first vertex.  The 1st vertex becomes the selection, which we refer to when we get to OnLButtonUp again
	// NB it's unusual to have a vertex attached to a pin as the selection.
	cconnect2 *new_c = new cconnect2 (net);
	cvertex2 *new_v = new cvertex2 (new_c, pin->x, pin->y);
	new_c->Start(new_v);
	m_sel.RemoveAll();
	m_sel.Add(new_v);
	net->GetWidth( &m_active_width );								// AMW r267 added
	CPoint p = m_last_cursor_point;
	new_v->StartDraggingStub( pDC, p.x, p.y, m_active_layer, m_active_width, m_active_layer, 2, m_inflection_mode );
	if( m_doc->m_bHighlightNet )
		m_highlight_net = net,
		net->Highlight();
	SetCursorMode( CUR_DRAG_STUB );
	ShowActiveLayer();
	ShowSelectStatus();
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// attach this pad to a net
//
void CFreePcbView::OnPadAddToNet()
{
#ifndef CPT2
	DlgAssignNet assign_net_dlg;
	assign_net_dlg.m_map = &m_doc->m_nlist->m_map;
	int ret = assign_net_dlg.DoModal();
	if( ret == IDOK )
	{
		CString name = assign_net_dlg.m_net_str;
		void * ptr;
		cnet * new_net = 0;
		int test = m_doc->m_nlist->m_map.Lookup( name, ptr );
		if( !test )
		{
			// create new net if legal string
			name.Trim();
			if( name.GetLength() )
			{
				new_net = m_doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 10, 0, 0, 0 );
				SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_ADD, TRUE, m_doc->m_undo_list );
			}
			else
			{
				// blank net name
				CString s ((LPCSTR) IDS_IllegalNetName);
				AfxMessageBox( s );
				return;
			}
		}
		else
		{
			// use selected net
			new_net = (cnet*)ptr;
			SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		}
		// assign pin to net
		if( new_net )
		{
			SaveUndoInfoForPart( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_doc->m_undo_list );
			CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.I2() );
			new_net->AddPin( &m_sel_part->ref_des, &pin_name );
			if( m_doc->m_vis[LAY_RAT_LINE] )
				m_doc->m_nlist->OptimizeConnections(  new_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
			SetFKText( m_cursor_mode );
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
#endif
}

// remove this pad from net
//
void CFreePcbView::OnPadDetachFromNet()
{
#ifndef CPT2
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.I2()].net;
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
	CString pin_name = m_sel_part->shape->GetPinNameByIndex(m_sel_id.I2());
	cnet * net = m_doc->m_plist->GetPinNet( m_sel_part, m_sel_id.I2() );
	m_doc->m_nlist->RemoveNetPin( m_sel_part, &pin_name );
	m_doc->m_nlist->RemoveOrphanBranches( net, 0 );
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// connect this pad to another pad
// CPT2 converted
//
void CFreePcbView::OnPadStartRatline()
{
	cpin2 *pin = m_sel.First()->ToPin();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, pin->x, pin->y, LAY_RAT_LINE, 1, 1 );
	if( m_doc->m_bHighlightNet && pin->net )
		m_highlight_net = pin->net,
		pin->net->Highlight();
	SetCursorMode( CUR_DRAG_CONNECT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// connect this vertex to another pad with a tee connection
// CPT2 converted
//
void CFreePcbView::OnVertexStartRatline()
{
	cpcb_item *sel0 = m_sel.First();
	cvertex2 *v = sel0->ToVertex();
	if (sel0->IsTee())
		v = sel0->ToTee()->vtxs.First();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, v->x, v->y, LAY_RAT_LINE, 1, 1 );
	if( m_doc->m_bHighlightNet )
		m_highlight_net = v->m_net,
		v->m_net->Highlight();
	SetCursorMode( CUR_DRAG_CONNECT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// start stub trace from vertex of an existing trace
// CPT2 converted.  Also now works if selected vtx is an end-vertex or a tee.  Also added an optional new bResetActiveWidth param

void CFreePcbView::OnVertexStartTrace() { OnVertexStartTrace(true); }

void CFreePcbView::OnVertexStartTrace(bool bResetActiveWidth)
{
	cpcb_item *sel0 = m_sel.First();
	cvertex2 *v = sel0->ToVertex();
	if (sel0->IsTee())
		v = sel0->ToTee()->vtxs.First();
	cnet2 *net = v->m_net;
	SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	m_snap_angle_ref.x = v->x;
	m_snap_angle_ref.y = v->y;
	// now add new connection and first vertex.  The 1st vertex becomes the selection, which we refer to when we get to OnLButtonUp again
	if (v->preSeg && v->postSeg)
		v->SplitConnect();
	cconnect2 *new_c;
	cvertex2 *new_v;
	if (v->tee) 
	{
		new_c = new cconnect2 (net);
		new_v = new cvertex2 (new_c, v->x, v->y);
		new_c->Start(new_v);
		v->tee->Add(new_v);
		m_sel.RemoveAll();
		m_sel.Add(new_v);
	}
	else
	{
		// Selected vertex must have been a loose end-vertex.  Just extend its connect
		new_c = v->m_con;
		new_v = v;
		if (!v->preSeg)
			v->m_con->ReverseDirection();			// Ensure we're extending from the tail end of connect
	}
    // CPT.  In drag mode, the width used will be m_active_width.  Initialize this value to the net's default value, unless bResetActiveWidth is false
	if (bResetActiveWidth)
		net->GetWidth(&m_active_width);
	// end CPT
	CPoint p = m_last_cursor_point;
	new_v->StartDraggingStub( pDC, p.x, p.y, m_active_layer, m_active_width, m_active_layer, 2, m_inflection_mode );
	if( m_doc->m_bHighlightNet )
		m_highlight_net = net,
		net->Highlight();
	SetCursorMode( CUR_DRAG_STUB );
	ShowActiveLayer();
	ShowSelectStatus();
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// set width for this segment (not a ratline)
//
void CFreePcbView::OnSegmentSetWidth()
{
#ifndef CPT2
	SetWidth( 0 );
	CancelHighlight();
	m_doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	Invalidate( FALSE );
#endif
}

// unroute this segment, convert to a ratline
//
void CFreePcbView::OnSegmentUnroute()
{
#ifndef CPT2
	// save undo info for connection
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );

	// edit connection segment
//	m_doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	// see if segments to pin also need to be unrouted
	// see if start vertex of this segment is in start pad of connection
	int x = m_sel_vtx->x;
	int y = m_sel_vtx->y;
	int layer = m_sel_seg->m_layer;
	BOOL test = m_doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
		m_sel_id.I2(), layer, 1 );
	if( test )
	{
		// unroute preceding segments
		for( int is=m_sel_id.I3()-1; is>=0; is-- )
			m_doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );	}
	// see if end vertex of this segment is in end pad of connection
	x = m_sel_next_vtx->x;
	y = m_sel_next_vtx->y;
	test = m_doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
		m_sel_id.I2(), layer, 0 );
	if( test )
	{
		// unroute following segments
		for( int is=m_sel_con->NumSegs()-1; is>m_sel_id.I3(); is-- )
			m_doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );
	}

	// unroute segment
	id id = m_doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is );
	m_doc->m_nlist->SetAreaConnections( m_sel_net );

	// reselect it if possible
	if( m_sel_id.Resolve() )
	{
		SelectItem( m_sel_id );
		SetCursorMode( CUR_RAT_SELECTED );
	}
	else
	{
		CancelSelection();
	}
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// delete this segment
//
void CFreePcbView::OnSegmentDelete()
{
#ifndef CPT2
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	m_sel_net->RemoveSegmentAdjustTees( m_sel_id.Seg() );
	m_doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	ShowSelectStatus();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// route this ratline
//
void CFreePcbView::OnRatlineRoute() { OnRatlineRoute(true); }			// CPT2.  Need a version with the new bResetActiveWidth param

void CFreePcbView::OnRatlineRoute(bool bResetActiveWidth)
{
	cseg2 *seg = m_sel.First()->ToSeg();
	cnet2 *net = seg->m_net;
	cvertex2 *preVtx = seg->preVtx, *postVtx = seg->postVtx;
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	// int last_seg_layer = 0;			// CPT2 TODO eliminated this old local (which was only passed down, semi-uselessly, to StartDragging()).  Desirable?

	// get direction for routing, based on closest end of selected segment to cursor.  Then determine initial routing layer
	double d1x = p.x - preVtx->x;
	double d1y = p.y - preVtx->y;
	double d2x = p.x - postVtx->x;
	double d2y = p.y - postVtx->y;
	double d1 = d1x*d1x + d1y*d1y;
	double d2 = d2x*d2x + d2y*d2y;
	if( d1<d2 )
	{
		// route forward
		m_dir = 0;
		m_snap_angle_ref.x = preVtx->x;
		m_snap_angle_ref.y = preVtx->y;
	}
	else
	{
		// route backward
		m_dir = 1;
		m_snap_angle_ref.x = postVtx->x;
		m_snap_angle_ref.y = postVtx->y;
	}
	if (m_dir==0 && preVtx->pin && preVtx->pin->pad_layer!=LAY_PAD_THRU)
		// Routing forward from an SMT pin. Force routing on pin's layer
		m_active_layer = preVtx->pin->pad_layer;
	else if (m_dir==1 && postVtx->pin && postVtx->pin->pad_layer!=LAY_PAD_THRU)
		// Routing backward from an SMT:
		m_active_layer = postVtx->pin->pad_layer;

    // CPT.  In drag mode, the width used will be m_active_width.  Initialize this value to the net's default value, unless bResetActiveWidth is false
	if (bResetActiveWidth)
		net->GetWidth(&m_active_width);
	// end CPT

	// now start dragging segment!
	m_dragging_new_item = 0;
	seg->StartDragging( pDC, p.x, p.y, 
		m_active_layer, LAY_SELECTION, m_active_width, m_active_layer, m_dir, 2 );
	SetCursorMode( CUR_DRAG_RAT );

	// AMW r269: highlight net while routing, except for ratline being routed
	if( m_doc->m_bHighlightNet )
		m_highlight_net = net,
		net->Highlight(seg);
	// end AMW

	ShowActiveLayer();
	ShowSelectStatus();
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// optimize this connection
//
void CFreePcbView::OnRatlineOptimize()
{
#ifndef CPT2
	int new_ic = m_doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, FALSE, -1, FALSE  );
	ReselectNetItemIfConnectionsChanged( new_ic );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// change end-pin for ratline
//
void CFreePcbView::OnRatlineChangeEndPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CancelHighlight();
	cconnect * c = m_sel_con;
	m_dlist->Set_visible( m_sel_seg->dl_el, FALSE );
	int x, y;
	if( m_sel_id.I3() == 0 )
	{
		// ratline is first segment of connection
		x = c->VtxByIndex(1).x;
		y = c->VtxByIndex(1).y;
	}
	else
	{
		// ratline is last segment of connection
		x = m_sel_vtx->x;
		y = m_sel_vtx->y;
	}
	m_dlist->StartDraggingRatLine( pDC, 0, 0, x, y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_RAT_PIN );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// change via properties
//
void CFreePcbView::OnVertexProperties()
{
#ifndef CPT2
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	CPoint v_pt( m_sel_vtx->x, m_sel_vtx->y );
	CDlgVia dlg = new CDlgVia;
	dlg.Initialize( m_sel_vtx->via_w, m_sel_vtx->via_hole_w, v_pt, m_doc->m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_sel_vtx->via_w = dlg.m_via_w;
		m_sel_vtx->via_hole_w = dlg.m_via_hole_w;
		CancelHighlight();
		m_doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.pt().x, dlg.pt().y );
		m_doc->ProjectModified( TRUE );
		m_doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
	Invalidate( FALSE );
#endif
}

// move this vertex
//
void CFreePcbView::OnVertexMove()
{
#ifndef CPT2
//	m_doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	id id = m_sel_id;
	int ic = m_sel_id.I2();
	int ivtx = m_sel_id.I3();
	m_dragging_new_item = 0;
	m_from_pt.x = m_sel_vtx->x;
	m_from_pt.y = m_sel_vtx->y;
	m_doc->m_nlist->StartDraggingVertex( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_VTX );
	ReleaseDC( pDC );
	Invalidate( FALSE );
#endif
}

// delete this vertex
// i.e. unroute adjacent segments and reroute if on same layer
// stub trace needs some special handling
//
void CFreePcbView::OnVertexDelete()
{
#ifndef CPT2
	int ic = m_sel_id.I2();
	int iv = m_sel_id.I3();
	cvertex * v = m_sel_id.Vtx();
	cconnect * c = m_sel_id.Con();

	if( v->GetType() == cvertex::V_TEE )
	{
		// ask about tee-connection
		CString s ((LPCSTR) IDS_YouAreDeletingATeeVertexContinue);
		int ret = AfxMessageBox( s,	MB_OKCANCEL );
		if( ret == IDCANCEL )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	c->Undraw();
	m_sel_net->RemoveVertex( v );
	// convert id from vertex to connection, and redraw if it still exists
	m_sel_id.SetSubSubType( ID_NONE );
	if( m_sel_id.Resolve() )
	{
		m_sel_id.Con()->Draw();
	}
	if( m_doc->m_vis[LAY_RAT_LINE] )
		m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// move the end vertex of a stub trace
//
void CFreePcbView::OnEndVertexMove()
{
#ifndef CPT2
//	m_doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	SetCursorMode( CUR_DRAG_END_VTX );
	m_doc->m_nlist->StartDraggingVertex( pDC, m_sel_net, m_sel_ic, m_sel_iv, p.x, p.y, 2 );
	ReleaseDC( pDC );
	Invalidate( FALSE );
#endif
}


// force a via on end vertex
//
void CFreePcbView::OnEndVertexAddVia()
{
#ifndef CPT2
//	m_doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	m_doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_is );
	SetFKText( m_cursor_mode );
#if 0 //** debugging 
	if( m_doc->m_vis[LAY_RAT_LINE] ) 
	{
		m_doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
		if( m_sel_id.Resolve() )
			SelectItem( m_sel_id );
		else
			CancelSelection();
	}
#endif //** debugging
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// remove forced via on end vertex
//
void CFreePcbView::OnEndVertexRemoveVia()
{
//	m_doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
#ifndef CPT2
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	m_doc->m_nlist->UnforceVia( m_sel_net, m_sel_ic, m_sel_is, FALSE );
	if( m_sel_prev_seg->m_layer == LAY_RAT_LINE )
	{
		m_doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1, TRUE );
		CancelSelection();
	}
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
	if( m_doc->m_vis[LAY_RAT_LINE] )
	{
		m_doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
		if( m_sel_id.Resolve() )
			SelectItem( m_sel_id );
		else
			CancelSelection();
	}
	Invalidate( FALSE );
#endif
}

// append more segments to this stub trace
//
/* CPT2 Supplanted by OnVertexStartTrace
void CFreePcbView::OnEndVertexAddSegments()
{
//	m_doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	// if this is the first vertex in a trace, reverse the trace 
	// and reselect the last vertex
	if( m_sel_id.IsVtx() && m_sel_id.I3() == 0 )
	{
		m_sel_id.Con()->ReverseDirection();
		m_sel_id = m_sel_id.Con()->LastVtx()->Id();
	}
	CPoint p;
	p = m_last_cursor_point;
	m_sel_id.SetT3( ID_SEL_SEG );

	// CPT.  Set m_active_width to the net default value:
	int w;
    w = m_doc->m_trace_w;
    if (m_sel_net->def_w)
        w = m_sel_net->def_w;
    m_active_width = w;
	// end CPT

	m_snap_angle_ref.x = m_sel_vtx->x;
	m_snap_angle_ref.y = m_sel_vtx->y;
	m_doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_active_layer, w, m_active_layer, 2, m_inflection_mode );
	SetCursorMode( CUR_DRAG_STUB );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}
*/

// convert stub trace to regular connection by adding ratline to a pad
/* CPT2 obsolete.  Repalced all references with OnVertexStartRatline.
void CFreePcbView::OnEndVertexAddConnection()
{
	OnVertexStartRatline();
}
*/

// end vertex selected, delete it and the adjacent segment
//
void CFreePcbView::OnEndVertexDelete()
{
#ifndef CPT2
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
//	m_doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	m_doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1, TRUE );
	if( m_doc->m_vis[LAY_RAT_LINE] )
		m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// edit the position of an end vertex
//
void CFreePcbView::OnEndVertexEdit()
{
#ifndef CPT2
	DlgEditBoardCorner dlg;
	CString str ((LPCSTR) IDS_EditEndVertexPosition);
	int x = m_sel_vtx->x;
	int y = m_sel_vtx->y;
	dlg.Init( &str, m_doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		m_doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.X(), dlg.Y() );
		m_doc->ProjectModified( TRUE );
		m_doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
#endif
}

// finish routing a connection by making a segment to the destination pad
//
void CFreePcbView::OnRatlineComplete()
{
#ifndef CPT2
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );

	// complete routing to pin

	// CPT:
    int w, via_w, via_hole_w;				// CPT r295.  The latter two are basically unused (just passed to GetWidthsForSegment() as dummies).
	if (m_active_width!=0)
		w = m_active_width;
	else
		GetWidthsForSegment(&w, &via_w, &via_hole_w);
	// end CPT

	int test = m_doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is, m_active_layer, w );
	if( !test )
	{
		CancelSelection();
		Invalidate( FALSE );
	}
	else
	{
		// didn't work
		PlaySound( TEXT("CriticalStop"), 0, 0 );
		// TODO: eliminate undo
	}
	m_doc->ProjectModified( TRUE );
#endif
}

// set width of a connection
//
void CFreePcbView::OnRatlineSetWidth()
{
	if( m_sel_con->NumSegs() == 1 )
		SetWidth( 2 );
	else
		SetWidth( 1 );
	Invalidate( FALSE );
}

// delete a connection
//
void CFreePcbView::OnRatlineDeleteConnection()
{
#ifndef CPT2
	if( m_sel_con->locked )
	{
		CString s ((LPCSTR) IDS_YouAreTryingToDeleteALockedConnection);
		int ret = AfxMessageBox( s,	MB_YESNO );
		if( ret == IDNO )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	m_doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic, FALSE );
	m_sel_id.Resolve();
	m_sel_net = m_sel_id.Net();
	m_doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// lock a connection CPT2 converted
//
void CFreePcbView::OnRatlineLockConnection()
{
	SaveUndoInfoForNetAndConnections( m_sel.First()->GetNet(), CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	m_sel_con->locked = 1;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// unlock a connection CPT2 converted
//
void CFreePcbView::OnRatlineUnlockConnection()
{
	SaveUndoInfoForNetAndConnections( m_sel.First()->GetNet(), CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	m_sel_con->locked = 0;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	Invalidate( FALSE );
	m_doc->ProjectModified( TRUE );
}

// edit a text string
//
void CFreePcbView::OnTextEdit()
{
#ifndef CPT2
	// create dialog and pass parameters
	CDlgAddText add_text_dlg;
	add_text_dlg.Initialize( 0, m_doc->m_num_layers, 0, &m_sel_text->m_str,
		m_doc->m_units, m_sel_text->m_layer, m_sel_text->m_mirror,
			m_sel_text->m_bNegative, m_sel_text->m_angle, m_sel_text->m_font_size,
			m_sel_text->m_stroke_width, m_sel_text->m_x, m_sel_text->m_y );
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;

	// now replace old text with new one
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_doc->m_undo_list );
	int x = add_text_dlg.m_x;
	int y = add_text_dlg.m_y;
	int mirror = add_text_dlg.m_bMirror;
	BOOL bNegative = add_text_dlg.m_bNegative;
	int angle = add_text_dlg.m_angle;
	int font_size = add_text_dlg.m_height;
	int stroke_width = add_text_dlg.m_width;
	int layer = add_text_dlg.m_layer;
	CString test_str = add_text_dlg.m_str;
	CancelHighlight();
	CText * new_text = m_doc->m_tlist->AddText( x, y, angle, mirror, bNegative,
		layer, font_size, stroke_width, &test_str );
	new_text->m_uid = m_sel_text->m_uid;
	m_doc->m_tlist->RemoveText( m_sel_text );
	m_sel_text = new_text;
	m_doc->m_tlist->HighlightText( m_sel_text );

	// start dragging if requested in dialog
	if( add_text_dlg.m_bDrag )
		OnTextMove();
	else
		Invalidate( FALSE );
	m_doc->ProjectModified( TRUE );
#endif
}

// start adding board outline by dragging line for first side
//
void CFreePcbView::OnAddBoardOutline()
{
	m_doc->m_vis[LAY_BOARD_OUTLINE] = TRUE;
	m_dlist->SetLayerVisible( LAY_BOARD_OUTLINE, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	CancelHighlight();
	int ib = m_doc->m_board_outline.GetSize() - 1;
	m_sel_id.Set( ID_BOARD, -1, ID_OUTLINE, -1, ib, ID_SEL_CORNER, -1, 0 );
	m_polyline_style = CPolyLine::STRAIGHT;
	m_dlist->StartDraggingArray( pDC, p.x, p.y, 0, LAY_BOARD_OUTLINE, 2 );
	SetCursorMode( CUR_ADD_BOARD );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// move a board outline corner
//
void CFreePcbView::OnBoardCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_doc->m_board_outline[m_sel_id.I2()].X( m_sel_id.I3() );
	m_from_pt.y = m_doc->m_board_outline[m_sel_id.I2()].Y( m_sel_id.I3() );
	m_doc->m_board_outline[m_sel_id.I2()].StartDraggingToMoveCorner( pDC, m_sel_id.I3(), p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_BOARD_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// edit a board outline corner
//
void CFreePcbView::OnBoardCornerEdit()
{
	DlgEditBoardCorner dlg;
	CString str ((LPCSTR) IDS_CornerPosition);
	int x = m_doc->m_board_outline[m_sel_id.I2()].X(m_sel_id.I3());
	int y = m_doc->m_board_outline[m_sel_id.I2()].Y(m_sel_id.I3());
	dlg.Init( &str, m_doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForBoardOutlines( TRUE, m_doc->m_undo_list );
		m_doc->m_board_outline[m_sel_id.I2()].MoveCorner( m_sel_id.I3(),
			dlg.X(), dlg.Y() );
		CancelSelection();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// delete a board corner
//
void CFreePcbView::OnBoardCornerDelete()
{
	if( m_doc->m_board_outline[m_sel_id.I2()].NumCorners() < 4 )
	{
		CString s ((LPCSTR) IDS_BoardOutlineHasTooFewCorners);
		AfxMessageBox( s );
		return;
	}
	SaveUndoInfoForBoardOutlines( TRUE, m_doc->m_undo_list );
	m_doc->m_board_outline[m_sel_id.I2()].DeleteCorner( m_sel_id.I3() );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// insert a new corner in a side of a board outline
//
void CFreePcbView::OnBoardSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_doc->m_board_outline[m_sel_id.I2()].StartDraggingToInsertCorner( pDC, m_sel_id.I3(), p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_BOARD_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete entire board outline
//
void CFreePcbView::OnBoardDeleteOutline()
{
	SaveUndoInfoForBoardOutlines( TRUE, m_doc->m_undo_list );
	m_doc->m_board_outline.RemoveAt( m_sel_id.I2() );
	id new_id = m_sel_id;
	for( int i=m_sel_id.I2(); i<m_doc->m_board_outline.GetSize(); i++ )
	{
		CPolyLine * poly = &m_doc->m_board_outline[i];
		new_id.SetI2( i );
		poly->SetParentId( &new_id );
	}
	m_doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

// move a copper area corner
//
void CFreePcbView::OnAreaCornerMove()
{
#ifndef CPT2
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_sel_net->area[m_sel_id.I2()].X( m_sel_id.I3() );
	m_from_pt.y = m_sel_net->area[m_sel_id.I2()].Y( m_sel_id.I3() );
	m_doc->m_nlist->StartDraggingAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
#endif
}

// delete a copper area corner
//
void CFreePcbView::OnAreaCornerDelete()
{
#ifndef CPT2
	carea * area;
	area = &m_sel_net->area[m_sel_id.I2()];
	if( area->NumCorners() > 3 )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_doc->m_undo_list );
//		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_doc->m_undo_list );
		area->DeleteCorner( m_sel_id.I3() );
		CancelHighlight();
		m_doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
		m_doc->ProjectModified( TRUE );
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else
		OnAreaCornerDeleteArea();
#endif
}

// delete entire area
//
void CFreePcbView::OnAreaCornerDeleteArea()
{
	OnAreaSideDeleteArea();
	m_doc->ProjectModified( TRUE );
}

//insert a new corner in a side of a copper area
//
void CFreePcbView::OnAreaSideAddCorner()
{
#ifndef CPT2
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_doc->m_nlist->StartDraggingInsertedAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
#endif
}

// delete entire area
//
void CFreePcbView::OnAreaSideDeleteArea()
{
#ifndef CPT2
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_DELETE, TRUE, m_doc->m_undo_list );
//	SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_DELETE, TRUE, m_doc->m_undo_list );
	m_doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
	if( m_doc->m_vis[LAY_RAT_LINE] )
		m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// detect state where nothing is selected or being dragged
//
BOOL CFreePcbView::CurNone()
{
	return( m_cursor_mode == CUR_NONE_SELECTED );
}

// detect any selected state
//
BOOL CFreePcbView::CurSelected()
{
	return( m_cursor_mode > CUR_NONE_SELECTED && m_cursor_mode < CUR_NUM_SELECTED_MODES );
}

// detect any dragging state
//
BOOL CFreePcbView::CurDragging()
{
	return( m_cursor_mode > CUR_NUM_SELECTED_MODES && m_cursor_mode < CUR_NUM_MODES );
}

// detect states using routing grid
//
BOOL CFreePcbView::CurDraggingRouting()
{
	return( m_cursor_mode == CUR_DRAG_RAT
		|| m_cursor_mode == CUR_DRAG_VTX
		|| m_cursor_mode == CUR_DRAG_VTX_INSERT
		|| m_cursor_mode == CUR_DRAG_END_VTX
		|| m_cursor_mode == CUR_ADD_AREA
		|| m_cursor_mode == CUR_DRAG_AREA_1
		|| m_cursor_mode == CUR_DRAG_AREA
		|| m_cursor_mode == CUR_DRAG_AREA_INSERT
		|| m_cursor_mode == CUR_DRAG_AREA_MOVE
		|| m_cursor_mode == CUR_ADD_AREA_CUTOUT
		|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
		|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT
		|| m_cursor_mode == CUR_ADD_SMCUTOUT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_1
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE
		|| m_cursor_mode == CUR_DRAG_STUB
		|| m_cursor_mode == CUR_MOVE_SEGMENT
		|| m_cursor_mode == CUR_DRAG_VTX_INSERT
		);
}

// detect states using placement grid
//
BOOL CFreePcbView::CurDraggingPlacement()
{
	return( m_cursor_mode == CUR_ADD_BOARD
		|| m_cursor_mode == CUR_DRAG_BOARD_1
		|| m_cursor_mode == CUR_DRAG_BOARD
		|| m_cursor_mode == CUR_DRAG_BOARD_INSERT
		|| m_cursor_mode == CUR_DRAG_BOARD_MOVE
		|| m_cursor_mode == CUR_DRAG_PART
		|| m_cursor_mode == CUR_DRAG_REF
		|| m_cursor_mode == CUR_DRAG_TEXT
		|| m_cursor_mode == CUR_DRAG_GROUP
		|| m_cursor_mode == CUR_DRAG_GROUP_ADD
		|| m_cursor_mode == CUR_MOVE_ORIGIN );
}

// snap cursor if required and set m_last_cursor_point
//
void CFreePcbView::SnapCursorPoint( CPoint wp, UINT nFlags )
{
	// see if we need to snap at all
	if( CurDragging() )
	{
		// yes, set snap modes based on cursor mode and SHIFT and CTRL keys
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set modes
			if( nFlags != -1 )
			{
				/* CPT removed.  Doesn't work at all, and couldn't figure out the point of it
				if( nFlags & MK_CONTROL )
				{
					// control key held down
					m_snap_mode = SM_GRID_LINES;
					m_inflection_mode = IM_NONE;
				}
				else
				*/
				{
					m_snap_mode = SM_GRID_POINTS;
					if( m_doc->m_snap_angle == 45 )
					{
						if( nFlags & MK_SHIFT )
							m_inflection_mode = IM_45_90;
						else
							m_inflection_mode = IM_90_45;
					}
					else if( m_doc->m_snap_angle == 90 )
						m_inflection_mode = IM_90;

				}
				m_dlist->SetInflectionMode( m_inflection_mode );
			}
		}
		else
		{
			// for other dragging modes, always use grid points with no inflection
			m_snap_mode = SM_GRID_POINTS;
			m_inflection_mode = IM_NONE;
			m_dlist->SetInflectionMode( m_inflection_mode );
		}
		// set grid spacing
		int grid_spacing;
		if( CurDraggingPlacement() )
		{
			grid_spacing = m_doc->m_part_grid_spacing;
		}
		else if( CurDraggingRouting() )
		{
			grid_spacing = m_doc->m_routing_grid_spacing;
		}
		else if( m_doc->m_units == MIL )
		{
			grid_spacing = m_doc->m_pcbu_per_wu;
		}
		else if( m_doc->m_units == MM )
		{
			grid_spacing = m_doc->m_pcbu_per_wu;               
		}
		else 
			ASSERT(0);
		// see if we need to snap to angle
		if( m_doc->m_snap_angle && (wp != m_snap_angle_ref)
			&& ( m_cursor_mode == CUR_DRAG_RAT
			|| m_cursor_mode == CUR_DRAG_STUB
			|| m_cursor_mode == CUR_DRAG_AREA_1
			|| m_cursor_mode == CUR_DRAG_AREA
			|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
			|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT
			|| m_cursor_mode == CUR_DRAG_BOARD_1
			|| m_cursor_mode == CUR_DRAG_BOARD ) )
		{
			// yes, check snap mode
			if( m_snap_mode == SM_GRID_LINES )
				SnapToGridLine(wp, grid_spacing);
			else
				// old code.  CPT factored out.
				// snap to angle only if the starting point is on-grid
				SnapToAngle(wp, grid_spacing);
		}
		else
			m_snap_mode = SM_GRID_POINTS;

		// snap to grid points if needed
		if( m_snap_mode == SM_GRID_POINTS )
			SnapToGridPoint(wp, grid_spacing);
	}

	if( CurDragging() )
	{
		// update drag operation
		if( wp != m_last_cursor_point )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->Drag( pDC, wp.x, wp.y );
			ReleaseDC( pDC );
			// show relative distance
			if( m_cursor_mode == CUR_DRAG_GROUP
				|| m_cursor_mode == CUR_DRAG_GROUP_ADD
				|| m_cursor_mode == CUR_DRAG_PART
				|| m_cursor_mode == CUR_DRAG_VTX
				|| m_cursor_mode ==  CUR_DRAG_BOARD_MOVE
				|| m_cursor_mode == CUR_DRAG_AREA_MOVE
				|| m_cursor_mode ==  CUR_DRAG_SMCUTOUT_MOVE
				|| m_cursor_mode ==  CUR_DRAG_MEASURE_2
				|| m_cursor_mode == CUR_MOVE_SEGMENT
				)
			{
				ShowRelativeDistance( wp.x - m_from_pt.x, wp.y - m_from_pt.y );
			}
		}
	}
	// update cursor position
	m_last_cursor_point = wp;
	ShowCursor();
}

LONG CFreePcbView::OnChangeVisibleGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_doc->m_visual_grid_spacing = fabs( m_doc->m_visible_grid[lp] );
	else
		ASSERT(0);
	m_dlist->SetVisibleGrid( TRUE, m_doc->m_visual_grid_spacing );
	Invalidate( FALSE );
	m_doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangePlacementGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_doc->m_part_grid_spacing = fabs( m_doc->m_part_grid[lp] );
	else
		ASSERT(0);
	m_doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangeRoutingGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_doc->m_routing_grid_spacing = fabs( m_doc->m_routing_grid[lp] );
	else
		ASSERT(0);
	SetFocus();
	m_doc->ProjectModified( TRUE );
	return 0;
}

LONG CFreePcbView::OnChangeSnapAngle( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
		{
			m_doc->m_snap_angle = 45;
			m_inflection_mode = IM_90_45;
		}
		else if( lp == 1 )
		{
			m_doc->m_snap_angle = 90;
			m_inflection_mode = IM_90;
		}
		else
		{
			m_doc->m_snap_angle = 0;
			m_inflection_mode = IM_NONE;
		}
	}
	else
		ASSERT(0);
	m_doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangeUnits( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
			m_doc->m_units = MIL;
		else
			m_doc->m_units = MM;
	}
	else
		ASSERT(0);
	// CPT: m_doc->ProjectModified( TRUE );
	SetFocus();
	ShowSelectStatus();
	// CPT:
	if( m_cursor_mode == CUR_DRAG_GROUP	|| m_cursor_mode == CUR_DRAG_GROUP_ADD || m_cursor_mode == CUR_DRAG_PART
				|| m_cursor_mode == CUR_DRAG_VTX || m_cursor_mode ==  CUR_DRAG_BOARD_MOVE || m_cursor_mode == CUR_DRAG_AREA_MOVE
				|| m_cursor_mode ==  CUR_DRAG_SMCUTOUT_MOVE || m_cursor_mode ==  CUR_DRAG_MEASURE_2 || m_cursor_mode == CUR_MOVE_SEGMENT)
		ShowRelativeDistance( m_last_cursor_point.x - m_from_pt.x, m_last_cursor_point.y - m_from_pt.y );
	// End CPT
	return 0;
}


void CFreePcbView::OnSegmentDeleteTrace()
{
	OnRatlineDeleteConnection();
}

void CFreePcbView::OnAreaCornerProperties()
{
#ifndef CPT2
	// reuse board corner dialog
	DlgEditBoardCorner dlg;
	CString str ((LPCSTR) IDS_CornerPosition);
	CPoint pt = m_doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
	dlg.Init( &str, m_doc->m_units, pt.x, pt.y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_doc->m_undo_list );
//		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_doc->m_undo_list );
		m_doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is,
			dlg.X(), dlg.Y() );
		m_doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
		m_doc->m_nlist->HighlightAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
#endif
}

void CFreePcbView::OnRefProperties()
{
#ifndef CPT2
	CDlgRefText dlg;
	dlg.Initialize( m_doc->m_plist, m_sel_part, &m_doc->m_footprint_cache_map );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// edit this part
		SaveUndoInfoForPart( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
		m_sel_part->m_ref_layer = FlipLayer( m_sel_part->side, dlg.m_layer );
		m_doc->m_plist->ResizeRefText( m_sel_part, dlg.m_height, dlg.m_width, dlg.m_vis );
		m_doc->ProjectModified( TRUE );
		CancelHighlight();
		if( m_cursor_mode == CUR_PART_SELECTED )
			m_doc->m_plist->SelectPart( m_sel_part );
		else if( m_cursor_mode == CUR_REF_SELECTED 
			&& m_sel_part->m_ref_size && m_sel_part->m_ref_vis )
			m_doc->m_plist->SelectRefText( m_sel_part );
		else
			CancelSelection();
		Invalidate( FALSE );
	}
#endif
}

#if 0
void CFreePcbView::OnVertexProperties()
{
	DlgEditBoardCorner dlg;
	CString str = "Vertex Position";
	int x = m_sel_vtx->x;
	int y = m_sel_vtx->y;
	dlg.Init( &str, m_doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY,
			TRUE, m_doc->m_undo_list );
		CancelHighlight();
		m_doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.X(), dlg.Y() );
		m_doc->ProjectModified( TRUE );
		m_doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
}
#endif

BOOL CFreePcbView::OnEraseBkgnd(CDC* pDC)
{
	// Erase the left and bottom panes, the PCB area is always redrawn
	m_left_pane_invalid = TRUE;
	return FALSE;
}

void CFreePcbView::OnBoardSideConvertToStraightLine()
{
	m_doc->m_board_outline[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::STRAIGHT );
	m_doc->m_board_outline[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnBoardSideConvertToArcCw()
{
	m_doc->m_board_outline[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::ARC_CW );
	m_doc->m_board_outline[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnBoardSideConvertToArcCcw()
{
	m_doc->m_board_outline[m_sel_id.I2()].SetSideStyle( m_sel_id.I3(), CPolyLine::ARC_CCW );
	m_doc->m_board_outline[m_sel_id.I2()].HighlightSide( m_sel_id.I3() );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// unroute entire connection
//
void CFreePcbView::OnUnrouteTrace()
{
#ifndef CPT2
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	cconnect * c = m_sel_con;
	for( int is=0; is<c->NumSegs(); is++ )
		m_doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );
	m_doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
	m_doc->m_nlist->SetAreaConnections( m_sel_net );
	// try to reselect
	m_sel_id = m_sel_id.Con()->SegByIndex(0).Id();
	if( m_sel_id.Resolve() )
	{
		SelectItem( m_sel_id );
		SetCursorMode( CUR_RAT_SELECTED );
	}
	else
	{
		CancelSelection();
	}
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// save undo info for a group, for UNDO_GROUP_MODIFY or UNDO_GROUP_DELETE
//
void CFreePcbView::SaveUndoInfoForGroup( int type, carray<cpcb_item> *items, CUndoList * list )
{
#ifndef CPT2
	if( type != UNDO_GROUP_MODIFY && type != UNDO_GROUP_DELETE )
		ASSERT(0);

	void * ptr;

	// first, mark all nets as unselected and set new_event flag
	m_doc->m_nlist->MarkAllNets(0);
	list->NewEvent();

	// save info for all relevant nets
	for( int i=0; i<ids->GetSize(); i++ )
	{
		id sid = (*ids)[i];
		if( sid.T1() == ID_PART )
		{
			cpart * part = (cpart*)(*ptrs)[i];
			for( int ip=0; ip<part->pin.GetSize(); ip++ )
			{
				cnet * net = (cnet*)part->pin[ip].net;
				if( net )
				{
					if( net->utility == FALSE )
					{
						// unsaved
						SaveUndoInfoForNetAndConnectionsAndAreas( net, FALSE, list );
						net->utility = TRUE;
					}
				}
			}
		}
		else if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)(*ptrs)[i];
			if( net )
			{
				if( net->utility == FALSE )
				{
					// unsaved
					SaveUndoInfoForNetAndConnectionsAndAreas( net, FALSE, list );
					net->utility = TRUE;
				}
			}
		}
	}
	// save undo info for all parts and texts in group
	for( int i=0; i<ids->GetSize(); i++ )
	{
		id sid = (*ids)[i];
		if( sid.T1() == ID_PART )
		{
			cpart * part = (cpart*)(*ptrs)[i];
			SaveUndoInfoForPart( part,
				CPartList::UNDO_PART_MODIFY, NULL, FALSE, list );
		}
		else if( sid.T1() == ID_TEXT )
		{
			CText * text = (CText*)(*ptrs)[i];
			if( type == UNDO_GROUP_MODIFY )
				SaveUndoInfoForText( text, CTextList::UNDO_TEXT_MODIFY, FALSE, list );
			else if( type == UNDO_GROUP_DELETE )
				SaveUndoInfoForText( text, CTextList::UNDO_TEXT_DELETE, FALSE, list );
		}
	}
	// save undo info for all sm cutouts
	SaveUndoInfoForSMCutouts( FALSE, list );
	// save undo info for all board outlines
	SaveUndoInfoForBoardOutlines( FALSE, list );

	// now save undo descriptor );
	ptr = (void*)CreateGroupDescriptor( list, ptrs, ids, type );
	list->Push( UNDO_GROUP, ptr, &UndoGroupCallback );
#endif
}

// save undo info for two existing parts and all nets connected to them,
// assuming that the parts will be modified (not deleted or added)
//
void CFreePcbView::SaveUndoInfoFor2PartsAndNets( cpart2 * part1, cpart2 * part2, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	void * ptr;
	cpart * part;

	if( new_event )
		list->NewEvent();
	for( int i=0; i<2; i++ )
	{
		if( i==0 )
			part = part1;
		else
			part = part2;
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
				net->utility = 0;
		}
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
			{
				if( net->utility == 0 )
				{
					for( int ic=0; ic<net->NumCons(); ic++ )
					{
						undo_con * u_con = m_doc->m_nlist->CreateConnectUndoRecord( net, ic );
						list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
							&m_doc->m_nlist->ConnectUndoCallback, u_con->size );
					}
					undo_net * u_net = m_doc->m_nlist->CreateNetUndoRecord( net );
					list->Push( CNetList::UNDO_NET_MODIFY, u_net,
						&m_doc->m_nlist->NetUndoCallback, u_net->size );
					net->utility = 1;
				}
			}
		}
	}
	// now save undo info for parts
	undo_part * u_part1 = m_doc->m_plist->CreatePartUndoRecord( part1, NULL );
	list->Push( CPartList::UNDO_PART_MODIFY, u_part1,
		&m_doc->m_plist->PartUndoCallback, u_part1->size );
	undo_part * u_part2 = m_doc->m_plist->CreatePartUndoRecord( part2, NULL );
	list->Push( CPartList::UNDO_PART_MODIFY, u_part2,
		&m_doc->m_plist->PartUndoCallback, u_part2->size );
	// now save undo descriptor
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, 0, &part1->ref_des, &part2->ref_des, 0, 0, NULL, NULL );
		list->Push( UNDO_2_PARTS_AND_NETS, ptr, &UndoCallback );
	}
#endif
}

// save undo info for net, all connections and all areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndAreas( cnet2 * net, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	if( new_event )
		ASSERT( 0 );
	SaveUndoInfoForNetAndConnections( net,
		CNetList::UNDO_NET_MODIFY, new_event, list );
	for( int ia=0; ia<net->NumAreas(); ia++ )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_MODIFY, FALSE, list );
#endif
}

// save undo info for net, all connections and
// a single copper area
// type may be:
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndArea( cnet2 * net, carea2 * area,
														   int type, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	if( new_event )
	{
		list->NewEvent();
	}
	SaveUndoInfoForArea( net, iarea, type, FALSE, m_doc->m_undo_list );
	SaveUndoInfoForNetAndConnections( net,
		CNetList::UNDO_NET_MODIFY, FALSE, m_doc->m_undo_list );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, &net->name, NULL, iarea, 0, NULL, NULL );
		list->Push( UNDO_NET_AND_CONNECTIONS_AND_AREA, ptr, &UndoCallback );
	}
#endif
}

// save undo info for a copper area to be modified or deleted
// type may be:
//	CNetList::UNDO_AREA_ADD	if area will be added
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForArea( carea2 * area, int type, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	void *ptr;
	if( new_event )
	{
		list->NewEvent();
		SaveUndoInfoForNet( net, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	int nc = 1;
	if( type != CNetList::UNDO_AREA_ADD )
	{
		nc = net->area[iarea].NumContours();
		if( !net->area[iarea].Closed() )
			nc--;
	}
	if( nc > 0 )
	{
		undo_area * undo = m_doc->m_nlist->CreateAreaUndoRecord( net, iarea, type );
		list->Push( type, (void*)undo, &m_doc->m_nlist->AreaUndoCallback, undo->size );
	}
	// now save undo descriptor
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, type, &net->name, NULL, iarea, 0, NULL, NULL );
		list->Push( UNDO_AREA, ptr, &UndoCallback );
	}
#endif
}

// save undo info for all of the areas in a net
//
void CFreePcbView::SaveUndoInfoForAllAreasInNet( cnet2 * net, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	if( new_event )
	{
		list->NewEvent();		// flag new undo event
		SaveUndoInfoForNet( net, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	for( int ia=net->area.GetSize()-1; ia>=0; ia-- )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_DELETE, FALSE, list );
	undo_area * u_area = m_doc->m_nlist->CreateAreaUndoRecord( net, 0, CNetList::UNDO_AREA_CLEAR_ALL );
	list->Push( CNetList::UNDO_AREA_CLEAR_ALL, u_area, &m_doc->m_nlist->AreaUndoCallback, u_area->size );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, &net->name, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_AREAS_IN_NET, ptr, &UndoCallback );
	}
#endif
}

// save undo info for all of the areas in two nets
//
void CFreePcbView::SaveUndoInfoForAllAreasIn2Nets( cnet2 * net1, cnet2 * net2, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	if( new_event )
	{
		list->NewEvent();		// flag new undo event
		SaveUndoInfoForNet( net1, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
		SaveUndoInfoForNet( net2, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	SaveUndoInfoForAllAreasInNet( net1, FALSE, list );
	SaveUndoInfoForAllAreasInNet( net2, FALSE, list );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, &net1->name, &net2->name, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_AREAS_IN_2_NETS, ptr, &UndoCallback );
	}
#endif
}

// save undo info for all nets (but not areas)
//
void CFreePcbView::SaveUndoInfoForAllNets( BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	POSITION pos;
	CString name;
	CMapStringToPtr * m_map = &m_doc->m_nlist->m_map;
	void * net_ptr;
	if( new_event )
		list->NewEvent();		// flag new undo event
	// traverse map of nets
	for( pos = m_map->GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map->GetNextAssoc( pos, name, net_ptr );
		cnet * net = (cnet*)net_ptr;
		void * ptr;
		// loop through all connections in net
		for( int ic=0; ic<net->NumCons(); ic++ )
		{
			undo_con * u_con = m_doc->m_nlist->CreateConnectUndoRecord( net, ic );
			list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
				&m_doc->m_nlist->ConnectUndoCallback, u_con->size );
		}
		undo_net * u_net = m_doc->m_nlist->CreateNetUndoRecord( net );
		list->Push( CNetList::UNDO_NET_MODIFY, u_net,
			&m_doc->m_nlist->NetUndoCallback, u_net->size );
	}
#endif
}

// save undo info for all nets including areas
//
void CFreePcbView::SaveUndoInfoForAllNetsAndConnectionsAndAreas( BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	POSITION pos;
	CString name;
	CMapStringToPtr * m_map = &m_doc->m_nlist->m_map;
	void * net_ptr;
	if( new_event )
		list->NewEvent();		// flag new undo event
	// traverse map of nets
	for( pos = m_map->GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map->GetNextAssoc( pos, name, net_ptr );
		cnet * net = (cnet*)net_ptr;
		SaveUndoInfoForNetAndConnectionsAndAreas( net, FALSE, list );
	}
#endif
}

void CFreePcbView::SaveUndoInfoForMoveOrigin( int x_off, int y_off, CUndoList * list )
{
#ifndef CPT2
	// now push onto undo list
	undo_move_origin * undo = m_doc->CreateMoveOriginUndoRecord( x_off, y_off );
	list->NewEvent();
	list->Push( 0, (void*)undo, &m_doc->MoveOriginUndoCallback );
	// save top-level descriptor
	void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, x_off, y_off, NULL, NULL );
	list->Push( UNDO_MOVE_ORIGIN, ptr, &UndoCallback );
#endif
}

void CFreePcbView::SaveUndoInfoForBoardOutlines( BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	// now push onto undo list
	if( new_event )
		list->NewEvent();		// flag new undo event

	// get number of closed board outlines
	int n_closed = 0;
	for( int i=0; i<m_doc->m_board_outline.GetSize(); i++ )
	{
		CPolyLine * poly = &m_doc->m_board_outline[i];
		if( poly->Closed() )
			n_closed = i+1;
	}
	// push all board outlines onto undo list
	for( int i=0; i<n_closed; i++ )
	{
		CPolyLine * poly = &m_doc->m_board_outline[i];
		undo_board_outline * undo = m_doc->CreateBoardOutlineUndoRecord( poly );
		list->Push( UNDO_BOARD, (void*)undo, &m_doc->BoardOutlineUndoCallback );
	}
	list->Push( UNDO_BOARD_OUTLINE_CLEAR_ALL, NULL, &m_doc->BoardOutlineUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_BOARD_OUTLINES, ptr, &UndoCallback );
	}
#endif
}

void CFreePcbView::SaveUndoInfoForSMCutouts( BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	// push undo info onto list
	if( new_event )
		list->NewEvent();		// flag new undo event
	// get last closed cutout
	int i;
	int n_closed = 0;
	for( i=0; i<m_doc->m_sm_cutout.GetSize(); i++ )
	{
		CPolyLine * poly = &m_doc->m_sm_cutout[i];
		if( poly->Closed() )
			n_closed = i+1;
		else
			break;
	}
	// push all closed cutouts onto undo list
	for( i=0; i<n_closed; i++ )
	{
		CPolyLine * poly = &m_doc->m_sm_cutout[i];
		undo_sm_cutout * undo = m_doc->CreateSMCutoutUndoRecord( poly );
		list->Push( UNDO_SM_CUTOUT, (void*)undo, &m_doc->SMCutoutUndoCallback );
	}
	// create UNDO_SM_CUTOUT_CLEAR_ALL record and push it
	list->Push( UNDO_SM_CUTOUT_CLEAR_ALL, NULL, &m_doc->SMCutoutUndoCallback );
	// now push top-level callback for redoing
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_SM_CUTOUTS, ptr, &UndoCallback );
	}
#endif
}

void CFreePcbView::SaveUndoInfoForText( ctext * text, int type, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	// create new undo record and push onto undo list
	undo_text * undo = m_doc->m_tlist->CreateUndoRecord( text );
	if( new_event )
		list->NewEvent();		// flag new undo event
	list->Push( type, (void*)undo, &m_doc->m_tlist->TextUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, NULL, NULL, 0, 0, NULL, (void *)undo );
		list->Push( UNDO_TEXT, ptr, &UndoCallback );
	}
#endif
}

void CFreePcbView::SaveUndoInfoForText( undo_text * u_text, int type, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	// copy undo record and push onto undo list
	undo_text * undo = new undo_text;
	*undo = *u_text;
	if( new_event )
		list->NewEvent();		// flag new undo event
	list->Push( type, (void*)undo, &m_doc->m_tlist->TextUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, NULL, NULL, 0, 0, NULL, (void*)undo );
		list->Push( UNDO_TEXT, ptr, &UndoCallback );
	}
#endif
}


void CFreePcbView::OnViewEntireBoard()
{
	if( m_doc->m_board_outline.GetSize() )
	{
		// get boundaries of board outline
		int max_x = INT_MIN;
		int min_x = INT_MAX;
		int max_y = INT_MIN;
		int min_y = INT_MAX;
		for( int ib=0; ib<m_doc->m_board_outline.GetSize(); ib++ )
		{
			for( int ic=0; ic<m_doc->m_board_outline[0].NumCorners(); ic++ )
			{
				if( m_doc->m_board_outline[ib].X( ic ) > max_x )
					max_x = m_doc->m_board_outline[ib].X( ic );
				if( m_doc->m_board_outline[ib].X( ic ) < min_x )
					min_x = m_doc->m_board_outline[ib].X( ic );
				if( m_doc->m_board_outline[ib].Y( ic ) > max_y )
					max_y = m_doc->m_board_outline[ib].Y( ic );
				if( m_doc->m_board_outline[ib].Y( ic ) < min_y )
					min_y = m_doc->m_board_outline[ib].Y( ic );
			}
		}
		// reset window to enclose board outline
//		m_org_x = min_x - (max_x - min_x)/20;	// in pcbu
//		m_org_y = min_y - (max_y - min_y)/20;	// in pcbu
		double x_pcbu_per_pixel = 1.1 * (double)(max_x - min_x)/(m_client_r.right - m_left_pane_w);
		double y_pcbu_per_pixel = 1.1 * (double)(max_y - min_y)/(m_client_r.bottom - m_bottom_pane_h);
		m_pcbu_per_pixel = max( x_pcbu_per_pixel, y_pcbu_per_pixel );
		m_org_x = (max_x + min_x)/2 - (m_client_r.right - m_left_pane_w)/2 * m_pcbu_per_pixel;
		m_org_y = (max_y + min_y)/2 - (m_client_r.bottom - m_bottom_pane_h)/2 * m_pcbu_per_pixel;
		CRect screen_r;
		GetWindowRect( &screen_r );		// in pixels
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
		Invalidate( FALSE );
	}
	else
	{
		CString s ((LPCSTR) IDS_BoardOutlineDoesNotExist);
		AfxMessageBox( s );
	}
}

void CFreePcbView::OnViewAllElements()
{
	// reset window to enclose all elements
	BOOL bOK = FALSE;
	CRect r;
	// parts
	int test = m_doc->m_plist->GetPartBoundaries( &r );
	if( test != 0 )
		bOK = TRUE;
	int max_x = r.right;
	int min_x = r.left;
	int max_y = r.top;
	int min_y = r.bottom;
	// board outline
	for( int ib=0; ib<m_doc->m_board_outline.GetSize(); ib++ )
	{
		r = m_doc->m_board_outline[ib].GetBounds();
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	// nets
	if( m_doc->m_nlist->GetNetBoundaries( &r ) )
	{
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	// texts
	if( m_doc->m_tlist->GetTextBoundaries( &r ) )
	{
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	if( bOK )
	{
		// reset window
//		m_org_x = min_x - (max_x - min_x)/20;	// NM
//		m_org_y = min_y - (max_y - min_y)/20;	// NM
		double x_pcbu_per_pixel = 1.1 * (double)(max_x - min_x)/(m_client_r.right - m_left_pane_w);
		double y_pcbu_per_pixel = 1.1 * (double)(max_y - min_y)/(m_client_r.bottom - m_bottom_pane_h);
		m_pcbu_per_pixel = max( x_pcbu_per_pixel, y_pcbu_per_pixel );
		m_org_x = (max_x + min_x)/2 - (m_client_r.right - m_left_pane_w)/2 * m_pcbu_per_pixel;
		m_org_y = (max_y + min_y)/2 - (m_client_r.bottom - m_bottom_pane_h)/2 * m_pcbu_per_pixel;
	}
	// CPT:  fixed bug that occurred when viewing an empty doc (m_dlist->SetMapping() was never called, with unpredictable results)
	else 
		m_org_x = m_org_y = 0, m_pcbu_per_pixel = 127000;
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
		m_org_x, m_org_y );
	Invalidate( FALSE );
	// end CPTm
}

void CFreePcbView::OnAreaEdgeHatchStyle()
{
	CDlgSetAreaHatch dlg;
	dlg.Init( m_sel_net->area[m_sel_id.I2()].GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int hatch = dlg.GetHatch();
		m_sel_net->area[m_sel_id.I2()].SetHatch( hatch );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnPartEditFootprint()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnPartEditThisFootprint()
{
	m_doc->m_edit_footprint = TRUE;
	theApp.OnViewFootprint();
}

// Offer new footprint from the Footprint Editor
//
void CFreePcbView::OnExternalChangeFootprint( CShape * fp )
{
#ifndef CPT2
	CString str, s ((LPCSTR) IDS_DoYouWishToReplaceTheFootprintOfPart);
	str.Format( s, m_sel_part->ref_des, fp->m_name );
	int ret = AfxMessageBox( str, MB_YESNO );
	if( ret == IDYES )
	{
		// OK, see if a footprint of the same name is already in the cache
		void * ptr;
		BOOL found = m_doc->m_footprint_cache_map.Lookup( fp->m_name, ptr );
		if( found )
		{
			// see how many parts are using it, not counting the current one
			CShape * old_fp = (CShape*)ptr;
			int num = m_doc->m_plist->GetNumFootprintInstances( old_fp );
			if( m_sel_part->shape == old_fp )
				num--;
			if( num <= 0 )
			{
				// go ahead and replace it
				m_doc->m_plist->UndrawPart( m_sel_part );
				old_fp->Copy( fp );
				m_doc->m_plist->PartFootprintChanged( m_sel_part, old_fp );
				m_doc->ResetUndoState();
			}
			else
			{
				// offer to overwrite or rename it
				CDlgDupFootprintName dlg;
				CString mess, s ((LPCSTR) IDS_WarningAFootprintNamedIsAlreadyInUse);
				mess.Format( s, fp->m_name );
				dlg.Initialize( &mess, &m_doc->m_footprint_cache_map );
				int ret = dlg.DoModal();
				if( ret == IDOK )
				{
					// clicked "OK"
					if( dlg.m_replace_all_flag )
					{
						// replace all instances of footprint
						old_fp->Copy( fp );
						m_doc->m_plist->FootprintChanged( old_fp );
						m_doc->ResetUndoState();
					}
					else
					{
						// assign new name to footprint and put in cache
						CShape * shape = new CShape;
						shape->Copy( fp );
						shape->m_name = *dlg.GetNewName();
						m_doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
						m_doc->m_plist->PartFootprintChanged( m_sel_part, shape );
						m_doc->ResetUndoState();
					}
				}
			}
		}
		else
		{
			// footprint name not found in cache, add the new footprint
			CShape * shape = new CShape;
			shape->Copy( fp );
			m_doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
			m_doc->m_plist->PartFootprintChanged( m_sel_part, shape );
			m_doc->ResetUndoState();
		}
		m_doc->ProjectModified( TRUE );
		CancelHighlight();
		m_doc->m_plist->SelectRefText( m_sel_part );
		m_doc->m_plist->HighlightPart( m_sel_part );
		Invalidate( FALSE );
	}
#endif
}

// find a part in the layout, center window on it and select it
//
void CFreePcbView::OnViewFindpart()
{
#ifndef CPT2
	CDlgFindPart dlg;
	dlg.Initialize( m_doc->m_plist );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString * ref_des = &dlg.sel_ref_des;
		cpart * part = m_doc->m_plist->GetPartByName( *ref_des );
		if( part )
		{
			if( part->shape )
			{
				dl_element * dl_sel = part->dl_sel;
				int xc = (m_dlist->Get_x( dl_sel ) + m_dlist->Get_xf( dl_sel ))/2;
				int yc = (m_dlist->Get_y( dl_sel ) + m_dlist->Get_yf( dl_sel ))/2;
				m_org_x = xc - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
				m_org_y = yc - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
				CRect screen_r;
				GetWindowRect( &screen_r );
				m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
					m_org_x, m_org_y );
				CPoint p(xc, yc);
				p = m_dlist->PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
				SelectPart( part );
				Invalidate( FALSE );
			}
			else
			{
				CString s ((LPCSTR) IDS_SorryThisPartDoesntHaveAFootprint);
				AfxMessageBox( s );
			}
		}
		else
		{
			CString s ((LPCSTR) IDS_SorryThisPartDoesntExist);
			AfxMessageBox( s );
		}
	}
#endif
}

void CFreePcbView::OnFootprintWizard()
{
	m_doc->OnToolsFootprintwizard();
}

void CFreePcbView::OnFootprintEditor()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnCheckPartsAndNets()
{
	m_doc->OnToolsCheckPartsAndNets();
}

void CFreePcbView::OnDrc()
{
	m_doc->OnToolsDrc();
}

void CFreePcbView::OnClearDRC()
{
	m_doc->OnToolsClearDrc();
}

void CFreePcbView::OnViewAll()
{
	OnViewAllElements();
}

void CFreePcbView::OnAddSoldermaskCutout()
{
	CDlgAddMaskCutout dlg;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// force layer visible
		int il = dlg.m_layer;
		m_doc->m_vis[il] = TRUE;
		m_dlist->SetLayerVisible( il, TRUE );
		// start dragging rectangle
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CancelHighlight();
		SetCursorMode( CUR_ADD_SMCUTOUT );
		m_polyline_layer = il;
		m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
			m_last_cursor_point.y, 0, il, 2 );
		m_polyline_style = CPolyLine::STRAIGHT;
		m_polyline_hatch = dlg.m_hatch;
		Invalidate( FALSE );
		ReleaseDC( pDC );
	}
}

void CFreePcbView::OnSmCornerMove()
{
	CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = poly->X( m_sel_id.I3() );
	m_from_pt.y = poly->Y( m_sel_id.I3() );
	poly->StartDraggingToMoveCorner( pDC, m_sel_id.I3(), p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_SMCUTOUT_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnSmCornerSetPosition()
{
	CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
	DlgEditBoardCorner dlg;
	CString str ((LPCSTR) IDS_CornerPosition);
	int x = poly->X(m_sel_id.I3());
	int y = poly->Y(m_sel_id.I3());
	dlg.Init( &str, m_doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
		poly->MoveCorner( m_sel_id.I3(),
			dlg.X(), dlg.Y() );
		CancelSelection();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnSmCornerDeleteCorner()
{
	CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
	if( poly->NumCorners() < 4 )
	{
		CString s ((LPCSTR) IDS_SolderMaskCutoutHasTooFewCorners);
		AfxMessageBox( s );
		return;
	}
	SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
	poly->DeleteCorner( m_sel_id.I3() );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnSmCornerDeleteCutout()
{
	SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
	m_doc->m_sm_cutout.RemoveAt( m_sel_id.I2() );
	id new_id = m_sel_id;
	for( int i=m_sel_id.I2(); i<m_doc->m_sm_cutout.GetSize(); i++ )
	{
		CPolyLine * poly = &m_doc->m_sm_cutout[i];
		new_id.SetI2( i );
		poly->SetParentId( &new_id );
	}
	m_doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

// insert corner into solder mask cutout side and start dragging
void CFreePcbView::OnSmSideInsertCorner()
{
	CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	poly->StartDraggingToInsertCorner( pDC, m_sel_id.I3(), p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_SMCUTOUT_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );

}

// change hatch style for solder mask cutout
void CFreePcbView::OnSmSideHatchStyle()
{
	CPolyLine * poly = &m_doc->m_sm_cutout[m_sel_id.I2()];
	CDlgSetAreaHatch dlg;
	dlg.Init( poly->GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForSMCutouts( TRUE, m_doc->m_undo_list );
		int hatch = dlg.GetHatch();
		poly->SetHatch( hatch );
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnSmSideDeleteCutout()
{
	OnSmCornerDeleteCutout();
}

// change side of part
void CFreePcbView::OnPartChangeSide()
{
#ifndef CPT2
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
	CancelHighlight();
	m_doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->side = 1 - m_sel_part->side;
	m_doc->m_plist->DrawPart( m_sel_part );
	m_doc->m_nlist->PartMoved( m_sel_part );
	if( m_doc->m_vis[LAY_RAT_LINE] )
		m_doc->m_nlist->OptimizeConnections( m_sel_part, m_doc->m_auto_ratline_disable, 
										m_doc->m_auto_ratline_min_pins );
	m_doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// rotate part clockwise 90 degrees clockwise
//
void CFreePcbView::OnPartRotate()
{
#ifndef CPT2
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
	CancelHighlight();
	m_doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 90)%360;
	m_doc->m_plist->DrawPart( m_sel_part );
	m_doc->m_nlist->PartMoved( m_sel_part );
	if( m_doc->m_vis[LAY_RAT_LINE] )
		m_doc->m_nlist->OptimizeConnections( m_sel_part, m_doc->m_auto_ratline_disable, 
										m_doc->m_auto_ratline_min_pins );
	m_doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

void CFreePcbView::OnPartRotateCCW()
{
#ifndef CPT2
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list );
	CancelHighlight();
	m_doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 270)%360;
	m_doc->m_plist->DrawPart( m_sel_part );
	m_doc->m_nlist->PartMoved( m_sel_part );
	if( m_doc->m_vis[LAY_RAT_LINE] )
		m_doc->m_nlist->OptimizeConnections( m_sel_part, m_doc->m_auto_ratline_disable, 
										m_doc->m_auto_ratline_min_pins );
	m_doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}


void CFreePcbView::OnNetSetWidth()
{
#ifndef CPT2
	SetWidth( 2 );
	CancelHighlight();
	m_doc->m_nlist->HighlightNetConnections( m_sel_net );
#endif
}

void CFreePcbView::OnConnectSetWidth()
{
#ifndef CPT2
	SetWidth( 1 );
	CancelHighlight();
	m_doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
#endif
}

void CFreePcbView::OnSegmentAddVertex()
{
#ifndef CPT2

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	SetCursorMode( CUR_DRAG_VTX_INSERT );
	m_doc->m_nlist->StartDraggingSegmentNewVertex( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_sel_seg->m_layer,
		m_sel_seg->m_width, 2 );
#endif
}

void CFreePcbView::OnConnectUnroutetrace()
{
	OnUnrouteTrace();
}

void CFreePcbView::OnConnectDeletetrace()
{
	OnSegmentDeleteTrace();
}

void CFreePcbView::OnSegmentChangeLayer()
{
	ChangeTraceLayer( 0, m_sel_seg->m_layer );
}

void CFreePcbView::OnConnectChangeLayer()
{
	ChangeTraceLayer( 1 );
}

void CFreePcbView::OnNetChangeLayer()
{
	ChangeTraceLayer( 2 );
}

// change layer of routed trace segments
// if mode = 0, current segment
// if mode = 1, current connection
// if mode = 2, current net
//
void CFreePcbView::ChangeTraceLayer( int mode, int old_layer )
{
#ifndef CPT2
	CDlgChangeLayer dlg;
	dlg.Initialize( mode, old_layer, m_doc->m_num_copper_layers );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int err = 0;
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		cconnect * c = m_sel_con;
		if( dlg.m_apply_to == 0 )
		{
			err = m_doc->m_nlist->ChangeSegmentLayer( m_sel_net,
						m_sel_id.I2(), m_sel_id.I3(), dlg.m_new_layer );
			if( err )
			{
				CString s ((LPCSTR) IDS_UnableToChangeLayerForThisSegment);
				AfxMessageBox( s );
			}
		}
		else if( dlg.m_apply_to == 1 )
		{
			for( int is=0; is<c->NumSegs(); is++ )
			{
				if( c->SegByIndex(is).m_layer >= LAY_TOP_COPPER )
				{
					err += m_doc->m_nlist->ChangeSegmentLayer( m_sel_net,
						m_sel_id.I2(), is, dlg.m_new_layer );
				}
			}
			if( err )
			{
				CString s ((LPCSTR) IDS_UnableToChangeLayerForAllSegments);
				AfxMessageBox( s );
			}
		}
		else if( dlg.m_apply_to == 2 )
		{
			for( int ic=0; ic<m_sel_net->NumCons(); ic++ )
			{
				cconnect * c = m_sel_net->ConByIndex(ic);
				for( int is=0; is<c->NumSegs(); is++ )
				{
					if( c->SegByIndex(is).m_layer >= LAY_TOP_COPPER )
					{
						err += m_doc->m_nlist->ChangeSegmentLayer( m_sel_net,
							ic, is, dlg.m_new_layer );
					}
				}
			}
			if( err )
			{
				CString s ((LPCSTR) IDS_UnableToChangeLayerForAllSegments);
				AfxMessageBox( s );
			}
		}
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
#endif
}

void CFreePcbView::OnNetEditnet()
{
#ifndef CPT2
	CDlgEditNet dlg;
	netlist_info nl;
	m_doc->m_nlist->ExportNetListInfo( &nl );
	int inet = -1;
	for( int i=0; i<nl.GetSize(); i++ )
	{
		if( nl[i].net == m_sel_net )
		{
			inet = i;
			break;
		}
	}
	if( inet == -1 )
		ASSERT(0);
	dlg.Initialize( &nl, inet, m_doc->m_plist, FALSE, TRUE, m_doc->m_units,
		&m_doc->m_w, &m_doc->m_v_w, &m_doc->m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_doc->ResetUndoState();
		CancelSelection();
		m_doc->m_nlist->ImportNetListInfo( &nl, 0, NULL,
			m_doc->m_trace_w, m_doc->m_via_w, m_doc->m_via_hole_w );
		Invalidate( FALSE );
	}
#endif
}

void CFreePcbView::OnToolsMoveOrigin()
{
	CDlgMoveOrigin dlg;
	dlg.Initialize( m_doc->m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( dlg.m_drag )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			CancelHighlight();
			SetCursorMode( CUR_MOVE_ORIGIN );
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
				m_last_cursor_point.y, 0, LAY_SELECTION, 2 );
			Invalidate( FALSE );
			ReleaseDC( pDC );
		}
		else
		{
			SaveUndoInfoForMoveOrigin( -dlg.m_x, -dlg.m_x, m_doc->m_undo_list );
			MoveOrigin( -dlg.m_x, -dlg.m_y );
			OnViewAllElements();
			Invalidate( FALSE );
		}
	}
}

// move origin of coord system by moving everything
// by (x_off, y_off)
//
void CFreePcbView::MoveOrigin( int x_off, int y_off )
{
	for( int ib=0; ib<m_doc->m_board_outline.GetSize(); ib++ )
		m_doc->m_board_outline[ib].MoveOrigin( x_off, y_off );
	m_doc->m_plist->MoveOrigin( x_off, y_off );
	m_doc->m_nlist->MoveOrigin( x_off, y_off );
	m_doc->m_tlist->MoveOrigin( x_off, y_off );
	for( int ism=0; ism<m_doc->m_sm_cutout.GetSize(); ism++ )
		m_doc->m_sm_cutout[ism].MoveOrigin( x_off, y_off );
}

void CFreePcbView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// save starting position in pixels
	m_bLButtonDown = TRUE;
	m_bDraggingRect = FALSE;
	m_start_pt = point;
	CView::OnLButtonDown(nFlags, point);
	SetCapture();									// CPT
	m_bDontDrawDragRect = false;					// CPT
}

// Select all items in rectangle
// CPT2 Updated
//
void CFreePcbView::SelectItemsInRect( CRect r, BOOL bAddToGroup )
{
	if( !bAddToGroup )
		CancelSelection();
	r.NormalizeRect();

	// find parts in rect
	if( m_sel_mask & (1<<SEL_MASK_PARTS ) )
	{
		citer<cpart2> ip (&m_doc->m_plist->parts);
		for (cpart2 *p = ip.First(); p; p = ip.Next())
		{
			CRect pr;
			if( p->GetBoundingRect( &pr ) )
				if (r.PtInRect(pr.TopLeft()) && r.PtInRect(pr.BottomRight()))
					m_sel.Add(p);
		}
	}

	// find trace segments and vertices contained in rect
	if( m_sel_mask & (1<<SEL_MASK_CON ) )
	{
		citer<cnet2> in (&m_doc->m_nlist->nets);
		for (cnet2* net = in.First(); net; net = in.Next())
		{
			citer<cconnect2> ic (&net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next())
			{
				citer<cseg2> is (&c->segs);
				for (cseg2 *s = is.First(); s; s = is.Next())
				{
					if (!m_doc->m_vis[s->m_layer]) continue;
					cvertex2 *pre = s->preVtx, *post = s->postVtx;
					bool bPreInR = r.PtInRect( CPoint(pre->x, pre->y) );
					bool bPostInR = r.PtInRect( CPoint(post->x, post->y) );
					if (bPreInR && bPostInR && m_doc->m_vis[s->m_layer])
						m_sel.Add(s);
					if (bPreInR && pre->IsVia())
						m_sel.Add(pre->tee? (cpcb_item*) pre->tee: pre);
					if (bPostInR && post->IsVia())
						m_sel.Add(post->tee? (cpcb_item*) post->tee: post);
				}
			}
		}
	}

	// find texts in rect
	if( m_sel_mask & (1<<SEL_MASK_TEXT ) )
	{
		citer<ctext> it (&m_doc->m_tlist->texts);
		for (ctext *t = it.First(); t; t = it.Next())
			if (r.PtInRect(t->m_br.TopLeft()) && r.PtInRect(t->m_br.BottomRight()))
				m_sel.Add(t);
	}

	// find copper area sides in rect
	if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
	{
		citer<cnet2> in (&m_doc->m_nlist->nets);
		for (cnet2* net = in.First(); net; net = in.Next())
		{
			citer<carea2> ia (&net->areas);
			for (carea2 *a = ia.First(); a; a = ia.Next())
			{
				citer<ccontour> ictr (&a->contours);
				for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
				{
					citer<cside> is (&ctr->sides);
					for (cside *s = is.First(); s; s = is.Next())
					{
						CPoint pre (s->preCorner->x, s->preCorner->y);
						CPoint post (s->postCorner->x, s->postCorner->y);
						if (r.PtInRect(pre) && r.PtInRect(post))
							m_sel.Add(s);
					}
				}
			}
		}
	}

#ifndef CPT2 // TODO later
	// find solder mask cutout sides in rect
	if( m_sel_mask & (1<<SEL_MASK_SM ) )
	{
		for( int im=0; im<m_doc->m_sm_cutout.GetSize(); im++ )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[im];
			for( int ic=0; ic<poly->NumContours(); ic++ )
			{
				int istart = poly->ContourStart(ic);
				int iend = poly->ContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->X(ic1);
					int y1 = poly->Y(ic1);
					int x2 = poly->X(ic2);
					int y2 = poly->Y(ic2);
					if( InRange( x1, r.left, r.right )
						&& InRange( x2, r.left, r.right )
						&& InRange( y1, r.top, r.bottom )
						&& InRange( y2, r.top, r.bottom )
						&& m_doc->m_vis[poly->Layer()] )
					{
						id sm_id = poly->Id();
						sm_id.SetSubSubType( ID_SEL_SIDE, poly->SideUID(is) );
						if( FindItemInGroup( poly, &sm_id ) == -1 )
						{
							m_sel_ids.Add( sm_id );
							m_sel_ptrs.Add( NULL );
						}
					}
				}
			}
		}
	}

	// find board outline sides in rect
	if( m_sel_mask & (1<<SEL_MASK_BOARD ) )
	{
		for( int im=0; im<m_doc->m_board_outline.GetSize(); im++ )
		{
			CPolyLine * poly = &m_doc->m_board_outline[im];
			for( int ic=0; ic<poly->NumContours(); ic++ )
			{
				int istart = poly->ContourStart(ic);
				int iend = poly->ContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->X(ic1);
					int y1 = poly->Y(ic1);
					int x2 = poly->X(ic2);
					int y2 = poly->Y(ic2);
					if( InRange( x1, r.left, r.right )
						&& InRange( x2, r.left, r.right )
						&& InRange( y1, r.top, r.bottom )
						&& InRange( y2, r.top, r.bottom )
						&& m_doc->m_vis[poly->Layer()] )
					{
						id bd_id = poly->Id();
						bd_id.SetSubSubType( ID_SEL_SIDE, poly->SideUID(is) );
						if( FindItemInGroup( poly, &bd_id ) == -1 )
						{
							m_sel_ids.Add( bd_id );
							m_sel_ptrs.Add( NULL );
						}
					}
				}
			}
		}
	}
#endif

	HighlightSelection();
	m_lastKeyWasArrow = FALSE;
	m_lastKeyWasGroupRotate = false;
	FindGroupCenter();
}

void CFreePcbView::FinishRouting(cseg2 *rat)
{
	// CPT2: new helper for when user completes routing a ratline (while mode==CUR_DRAG_RAT, hitting F4 or clicking the dest. pin)
	SaveUndoInfoForNetAndConnections( rat->m_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
	CPoint pi = m_snap_angle_ref;
	CPoint pf = m_last_cursor_point;
	CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
	if( pp != pi )
	{
		bool bInserted = rat->InsertSegment( pp.x, pp.y, m_active_layer, m_active_width, m_dir );
		if( !bInserted )
			return;
	}
	bool bInserted = rat->InsertSegment( pf.x, pf.y, m_active_layer, m_active_width, m_dir );
	if( !bInserted )
		return;
	// finish trace if necessary
	rat->Route(m_active_layer, m_active_width);
}

// Start dragging group being moved or added
// If group is being added (i.e. pasted):
//	bAdd = TRUE;
//	x, y = coords for cursor point for dragging
//
void CFreePcbView::StartDraggingGroup( BOOL bAdd, int x, int y )
{
#ifndef CPT2
	if( !bAdd )
	{
		SetCursorMode( CUR_DRAG_GROUP );
	}
	else
	{
		SetCursorMode( CUR_DRAG_GROUP_ADD );
		m_last_mouse_point.x = x;
		m_last_mouse_point.y = y;
	}

	// snap dragging point to placement grid
	SnapCursorPoint( m_last_mouse_point, -1 );
	m_from_pt = m_last_cursor_point;

	// make texts, parts and segments invisible
	m_dlist->SetLayerVisible( LAY_HILITE, FALSE );
	int n_parts = 0;
	int n_segs = 0;
	int n_texts = 0;
	int n_area_sides = 0;
	int n_sm_sides = 0;
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.IsPart() )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			m_doc->m_plist->MakePartVisible( part, FALSE );
			n_parts++;
		}
		else if( sid.IsSeg() )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			dl_element * dl = net->ConByIndex(sid.I2())->SegByIndex(sid.I3()).dl_el;
			m_dlist->Set_visible( dl, FALSE );
			m_doc->m_nlist->SetViaVisible( net, sid.I2(), sid.I3(), FALSE );
			m_doc->m_nlist->SetViaVisible( net, sid.I2(), sid.I3()+1, FALSE );
			n_segs++;
		}
		else if( sid.IsAreaSide() )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.I2()];
//			a->SetSideVisible( sid.I3(), FALSE );
			a->MakeVisible( FALSE );
			n_area_sides++;
		}
		else if( sid.IsMaskSide() )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
//			poly->SetSideVisible( sid.I3(), FALSE );
			poly->MakeVisible( FALSE );
			n_sm_sides++;
		}
		else if( sid.IsBoardSide() )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
//			poly->SetSideVisible( sid.I3(), FALSE );
			poly->MakeVisible( FALSE );
			n_sm_sides++;
		}
		else if( sid.IsText() )
		{
			// make text strokes invisible
			CText * text = (CText*)m_sel_ptrs[i];
			for( int is=0; is<text->m_stroke.GetSize(); is++ )
				((dl_element*)text->m_stroke[is].dl_el)->visible = 0;
			n_texts++;
		}
	}

	// set up dragline array
	m_dlist->MakeDragLineArray( n_parts*4 + n_segs + n_texts*4 + n_area_sides + n_sm_sides );
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.IsPart() )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			int xi = part->shape->m_sel_xi;
			int xf = part->shape->m_sel_xf;
			if( part->side )
			{
				xi = -xi;
				xf = -xf;
			}
			int yi = part->shape->m_sel_yi;
			int yf = part->shape->m_sel_yf;
			CPoint p1( xi, yi );
			CPoint p2( xf, yi );
			CPoint p3( xf, yf );
			CPoint p4( xi, yf );
			RotatePoint( &p1, part->angle, zero );
			RotatePoint( &p2, part->angle, zero );
			RotatePoint( &p3, part->angle, zero );
			RotatePoint( &p4, part->angle, zero );
			p1.x += part->x - m_from_pt.x;
			p2.x += part->x - m_from_pt.x;
			p3.x += part->x - m_from_pt.x;
			p4.x += part->x - m_from_pt.x;
			p1.y += part->y - m_from_pt.y;
			p2.y += part->y - m_from_pt.y;
			p3.y += part->y - m_from_pt.y;
			p4.y += part->y - m_from_pt.y;
			m_dlist->AddDragLine( p1, p2 );
			m_dlist->AddDragLine( p2, p3 );
			m_dlist->AddDragLine( p3, p4 );
			m_dlist->AddDragLine( p4, p1 );
		}
		else if( sid.IsSeg() )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			cconnect * c = net->ConByIndex(sid.I2());
			cseg * s = &c->SegByIndex(sid.I3());
			cvertex * v1 = &s->GetPreVtx();
			cvertex * v2 = &s->GetPostVtx();
			CPoint p1( v1->x - m_from_pt.x, v1->y - m_from_pt.y );
			CPoint p2( v2->x - m_from_pt.x, v2->y - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.IsAreaSide() )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.I2()];
			CPolyLine * poly = a;
			int icontour = poly->Contour(sid.I3());
			int ic1 = sid.I3();
			int ic2 = sid.I3()+1;
			if( ic2 > poly->ContourEnd(icontour) )
				ic2 = poly->ContourStart(icontour);
			CPoint p1( poly->X(ic1) - m_from_pt.x, poly->Y(ic1) - m_from_pt.y );
			CPoint p2( poly->X(ic2) - m_from_pt.x, poly->Y(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.IsMaskSide() )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
			int icontour = poly->Contour(sid.I3());
			int ic1 = sid.I3();
			int ic2 = sid.I3()+1;
			if( ic2 > poly->ContourEnd(icontour) )
				ic2 = poly->ContourStart(icontour);
			CPoint p1( poly->X(ic1) - m_from_pt.x, poly->Y(ic1) - m_from_pt.y );
			CPoint p2( poly->X(ic2) - m_from_pt.x, poly->Y(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.IsBoardSide() )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
			int icontour = poly->Contour(sid.I3());
			int ic1 = sid.I3();
			int ic2 = sid.I3()+1;
			if( ic2 > poly->ContourEnd(icontour) )
				ic2 = poly->ContourStart(icontour);
			CPoint p1( poly->X(ic1) - m_from_pt.x, poly->Y(ic1) - m_from_pt.y );
			CPoint p2( poly->X(ic2) - m_from_pt.x, poly->Y(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.IsText() )
		{
			CText * text = (CText*)m_sel_ptrs[i];
			CPoint p1( m_dlist->Get_x( text->dl_sel ), m_dlist->Get_y( text->dl_sel ) );
			CPoint p2( m_dlist->Get_xf( text->dl_sel ), m_dlist->Get_y( text->dl_sel ) );
			CPoint p3( m_dlist->Get_xf( text->dl_sel ), m_dlist->Get_yf( text->dl_sel ) );
			CPoint p4( m_dlist->Get_x( text->dl_sel ), m_dlist->Get_yf( text->dl_sel ) );
			p1 -= m_from_pt;
			p2 -= m_from_pt;
			p3 -= m_from_pt;
			p4 -= m_from_pt;
			m_dlist->AddDragLine( p1, p2 );
			m_dlist->AddDragLine( p2, p3 );
			m_dlist->AddDragLine( p3, p4 );
			m_dlist->AddDragLine( p4, p1 );
		}

	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p;
	p.x  = m_from_pt.x;
	p.y  = m_from_pt.y;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dlist->StartDraggingArray( pDC, m_from_pt.x, m_from_pt.y, 0, LAY_SELECTION, TRUE );
	Invalidate( FALSE );
	ReleaseDC( pDC );
#endif
}

void CFreePcbView::CancelDraggingGroup()
{
#ifndef CPT2
	m_dlist->StopDragging();
	// make elements visible again
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			m_doc->m_plist->MakePartVisible( part, TRUE );
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			dl_element * dl = net->ConByIndex(sid.I2())->SegByIndex(sid.I3()).dl_el;
			m_dlist->Set_visible( dl, TRUE );
			m_doc->m_nlist->SetViaVisible( net, sid.I2(), sid.I3(), TRUE );
			m_doc->m_nlist->SetViaVisible( net, sid.I2(), sid.I3()+1, TRUE );
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_AREA
			&& sid.T3() == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.I2()];
//			a->SetSideVisible( sid.I3(), TRUE );
			a->MakeVisible( TRUE );
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK
			&& sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
//			poly->SetSideVisible( sid.I3(), TRUE );
			poly->MakeVisible( TRUE );
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD
			&& sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
//			poly->SetSideVisible( sid.I3(), TRUE );
			poly->MakeVisible( TRUE );
		}
		else if( sid.T1() == ID_TEXT )
		{
			// make text strokes invisible
			CText * text = (CText*)m_sel_ptrs[i];
			for( int is=0; is<text->m_stroke.GetSize(); is++ )
				((dl_element*)text->m_stroke[is].dl_el)->visible = TRUE;
		}
	}
	m_dlist->SetLayerVisible( LAY_HILITE, TRUE );
	SetCursorMode( CUR_GROUP_SELECTED );
	Invalidate( FALSE );
#endif
}

void CFreePcbView::OnGroupMove()
{
	if( GluedPartsInGroup() )
	{
		CString s ((LPCSTR) IDS_ThisGroupContainsGluedParts);
		int ret = AfxMessageBox( s, MB_OKCANCEL );
		if( ret != IDOK )
			return;
	}
	m_dlist->SetLayerVisible( LAY_RAT_LINE, FALSE );
	StartDraggingGroup();
}


// Move group of parts and trace segments
//
void CFreePcbView::MoveGroup( int dx, int dy )
{
#ifndef CPT2
	UngluePartsInGroup();

	// mark all parts and nets as unselected
	m_doc->m_nlist->MarkAllNets(0);
	m_doc->m_plist->MarkAllParts(0);

	// mark all corners of solder mask cutouts as unmoved
	for( int im=0; im<m_doc->m_sm_cutout.GetSize(); im++ )
	{
		CPolyLine * poly = &m_doc->m_sm_cutout[im];
		poly->SetUtility(0);
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}
	// mark all corners of board outlines as unmoved
	for( int im=0; im<m_doc->m_board_outline.GetSize(); im++ )
	{
		CPolyLine * poly = &m_doc->m_board_outline[im];
		poly->SetUtility(0);
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}

	// mark nets of ID_NET items as selected
	// mark areas as selected and undraw them
	// mark solder mask cutouts as selected and undraw them
	// mark board outlines as selected and undraw them
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			net->utility = TRUE;
			if( sid.T2() == ID_AREA )
			{
				cnet * net = (cnet*)m_sel_ptrs[i];
				carea * a = &net->area[sid.I2()];
				a->utility = TRUE;
				CPolyLine * poly = a;
				poly->SetUtility(1);
				poly->Undraw();
			}
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
			poly->SetUtility(1);
			poly->Undraw();
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_OUTLINE && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
			poly->SetUtility(1);
			poly->Undraw();
		}
	}

	// mark all relevant parts, nets, connections, segments, vertices 
	// and areas as selected
	// and move text and copper area corners
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		bool bOK = sid.Resolve();
		if( !bOK )
			ASSERT(0);
		if( sid.IsSeg() )
		{
			// segment
			cconnect * c = sid.Con();	// this connection
			cseg * s = sid.Seg();		// this segment
			cvertex * pre_v = &s->GetPreVtx();
			cvertex * post_v = &s->GetPostVtx();
			c->utility = TRUE;			// mark connection selected
			s->utility = TRUE;			// mark segment selected
			pre_v->utility =  TRUE;		// mark adjacent vertices as selected
			post_v->utility =  TRUE;
		}
		else if( sid.IsVtx() )
		{
			// vertex
			cconnect * c = sid.Con();	// this connection
			cvertex * v = sid.Vtx();	// this vertex
			c->utility = TRUE;			// mark connection selected
			v->utility = TRUE;			// mark vertex selected
		}
		else if( sid.IsPart() )
		{
			// part
			cpart * part = sid.Part();
			part->utility = TRUE;	// mark part selected
		}
		else if( sid.IsText() )
		{
			// text
//			CText * t = (CText*)m_sel_ptrs[i];
			CText * t = sid.Text();
			m_doc->m_tlist->MoveText( t, t->m_x+dx, t->m_y+dy, t->m_angle,
				t->m_mirror, t->m_bNegative, t->m_layer );
		}
		else if( sid.IsAreaSide() )
		{
			// area side
			cnet * net = (cnet*)m_sel_ptrs[i];
			CPolyLine * poly = &net->area[sid.I2()];
			int icontour = poly->Contour(sid.I3());
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->X(ic1) + dx );
				poly->SetY( ic1, poly->Y(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->X(ic2) + dx );
				poly->SetY( ic2, poly->Y(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
		}
		// sm_cutout side
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
			int icontour = poly->Contour(0);
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->X(ic1) + dx );
				poly->SetY( ic1, poly->Y(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->X(ic2) + dx );
				poly->SetY( ic2, poly->Y(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
		}
		// board outline side
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
			int icontour = poly->Contour(0);
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->X(ic1) + dx );
				poly->SetY( ic1, poly->Y(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->X(ic2) + dx );
				poly->SetY( ic2, poly->Y(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
		}
		else
			ASSERT(0);
	}

	// now redraw areas, solder mask cutouts and board outlines
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			net->utility = TRUE;
			if( sid.T2() == ID_AREA )
			{
				cnet * net = (cnet*)m_sel_ptrs[i];
				carea * a = &net->area[sid.I2()];
				CPolyLine * poly = a;
				if( poly->Utility() )
				{
					poly->Draw();
					poly->SetUtility(0);	// clear flag so only redraw once
				}
			}
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
			if( poly->Utility() )
			{
				poly->Draw();
				poly->SetUtility(0);	// clear flag so only redraw once
			}
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_OUTLINE && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
			if( poly->Utility() )
			{
				poly->Draw();
				poly->SetUtility(0);	// clear flag so only redraw once
			}
		}
	}

	// assume utility flags have been set on selected parts,
	// nets, connections, segments and vertices

	// move parts in group
	cpart * part = m_doc->m_plist->GetFirstPart();
	while( part != NULL )
	{
		if( part->utility )
		{
			// move part
			m_doc->m_plist->Move( part, part->x+dx, part->y+dy, part->angle, part->side );
			// find segments which connect to this part and move them
			// use net->utility2 to avoid repeats
			CIterator_cnet iter_net(m_doc->m_nlist);
			cnet * net;
			for( net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
				net->utility2 = 0;
			for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
			{
				net = (cnet*)part->pin[ip].net; 
				if( net && net->utility2 == 0 )
				{
					net->utility2 = 1;
					for( int ic=0; ic<net->NumCons(); ic++ )
					{
						cconnect * c = net->ConByIndex(ic);
						int nsegs = c->NumSegs();
						if( nsegs )
						{
							int p1 = c->start_pin;
							int p2 = c->end_pin;
#if 0
							if( net->pin[p1].part == part )
							{
								// starting pin is on part
								if( !c->SegByIndex(0).utility && c->SegByIndex(0).layer != LAY_RAT_LINE )
								{
									// first segment is not selected, unroute it
									if( !c->SegByIndex(0).utility )
										m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, 0 );
								}
								// move vertex if not selected
								if( !c->VtxByIndex(0).utility )
								{
									m_doc->m_nlist->MoveVertex( net, ic, 0,
										part->pin[pin_index1].x, part->pin[pin_index1].y );
									c->VtxByIndex(0).utility2 = 1; // moved
								}
							}
#endif
							if( p1 != cconnect::NO_END )
							{
								if( net->pin[p1].part == part )
								{
									// starting pin is on part
									if( c->SegByIndex(0).m_layer != LAY_RAT_LINE )
									{
										// unroute it if not selected
										if( !c->SegByIndex(0).utility )
											m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, 0, dx, dy, 0 );
									}
									// modify vertex position if necessary
									if( !c->VtxByIndex(0).utility )
									{
										// move vertex if unselected
										CString pin_name1 = net->pin[p1].pin_name;
										int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
										m_doc->m_nlist->MoveVertex( net, ic, 0,
											part->pin[pin_index1].x, part->pin[pin_index1].y );
										c->VtxByIndex(0).utility2 =  1;	// moved

									}
								}
							}
							if( p2 != cconnect::NO_END )
							{
								if( net->pin[p2].part == part )
								{
									// ending pin is on part
									if( c->SegByIndex(nsegs-1).m_layer != LAY_RAT_LINE )
									{
										// unroute it if not selected
										if( !c->SegByIndex(nsegs-1).utility )
											m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, nsegs-1, dx, dy, 1 );
									}
									// modify vertex position if necessary
									if( !c->VtxByIndex(nsegs).utility )
									{
										// move vertex if unselected
										CString pin_name2 = net->pin[p2].pin_name;
										int pin_index2 = part->shape->GetPinIndexByName( pin_name2 );
										m_doc->m_nlist->MoveVertex( net, ic, nsegs,
											part->pin[pin_index2].x, part->pin[pin_index2].y );
										c->VtxByIndex(nsegs).utility2 =  1;	// moved

									}
								}
							}
						}
					}
				}
			}
		}
		part = m_doc->m_plist->GetNextPart( part );
	}

	// get selected segments
	CIterator_cnet iter_net(m_doc->m_nlist);
	cnet * net = iter_net.GetFirst();
	while( net != NULL )
	{
		if( net->utility )
		{
			CIterator_cconnect iter_con( net );
			for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
			{
				int ic = iter_con.GetIndex();
				if( c->utility )
				{
					// undraw entire trace
					c->Undraw();
					CIterator_cseg iter_seg( c );
					for( cseg * s=iter_seg.GetFirst(); s; s=iter_seg.GetNext() )
					{
						int is = iter_seg.GetIndex();
						if( s->utility )
						{
							// move trace segment by flagging adjacent vertices
							cvertex * pre_v = &s->GetPreVtx();		// pre vertex
							cvertex * post_v = &s->GetPostVtx();	// post vertex

							// flag adjacent vertices as selected
							pre_v->utility = TRUE;
							post_v->utility = TRUE;
						}
					}
					// now move vertices
					CIterator_cvertex iter_vtx( c );
					for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
					{
						int iv = iter_vtx.GetIndex();
						if( v->utility && !v->utility2 )
						{
							// selected and not already moved
							// if adjacent segments were not selected, unroute them.  CPT r295 do this _before_ moving the vertex
							if( iv>0 )
							{
								cseg * pre_s = &c->SegByIndex(iv-1);
								if( pre_s->utility == 0 )
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv-1, dx, dy, 1 );
							}
							if( iv<c->NumSegs() )
							{
								cseg * post_s = &c->SegByIndex(iv);
								if( post_s->utility == 0 )
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv, dx, dy, 0 );
							}
							v->x += dx;
							v->y += dy;
							v->utility2 = TRUE;	// moved
						}
					}

					// now some special cases
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							// move trace segment
							cseg * s = &c->SegByIndex(is);			// this segment
							cvertex * pre_v = &s->GetPreVtx();		// pre vertex
							cvertex * post_v = &s->GetPostVtx();	// post vertex
							// connection starting part or NULL
							cpart * part1 = NULL;	
							if( c->start_pin != cconnect::NO_END )
								part1 = net->pin[c->start_pin].part;
							// connection ending part or NULL
							cpart * part2 = NULL;				
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// special case, first segment of trace selected but part not selected
							if( part1 )
							{
								if( part1->utility == FALSE && is == 0 )
								{
									// insert ratline as new first segment, unselected
									CPoint new_v_pt( pre_v->x, pre_v->y );
									CPoint old_v_pt = m_doc->m_plist->GetPinPoint( part1, net->pin[c->start_pin].pin_name );		// pre vertex coords
									m_doc->m_nlist->MoveVertex( net, ic, 0, old_v_pt.x, old_v_pt.y );
									m_doc->m_nlist->InsertSegment( net, ic, 0, new_v_pt.x, new_v_pt.y, LAY_RAT_LINE, 0, 0 );
									c->SegByIndex(0).utility = 0;
									c->VtxByIndex(0).utility = 0;
									is++;
								}
							}

							// special case, last segment of trace selected but part not selected
							if( part2 )
							{
								if( part2->utility == FALSE && is == c->NumSegs()-1 )
								{
									// insert ratline as new last segment
									int old_w = c->SegByIndex(c->NumSegs()-1).m_width;
									int old_layer = c->SegByIndex(c->NumSegs()-1).m_layer;
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
									CPoint new_v_pt( c->VtxByIndex(c->NumSegs()).x, c->VtxByIndex(c->NumSegs()).y );
									CPoint old_v_pt = m_doc->m_plist->GetPinPoint( part2, net->pin[c->end_pin].pin_name );
									m_doc->m_nlist->MoveVertex( net, ic, c->NumSegs(), old_v_pt.x, old_v_pt.y );
									BOOL bInserted = m_doc->m_nlist->InsertSegment( net, ic, c->NumSegs()-1,
										new_v_pt.x, new_v_pt.y, old_layer, old_w, 0 );
									c->SegByIndex(c->NumSegs()-2).utility = 1;
									c->SegByIndex(c->NumSegs()-1).utility = 0;
								}
							}
						}
					}
					m_doc->m_nlist->MergeUnroutedSegments( net, ic );	
					c->Draw();
				}
			}

			// now deal with tees that have been moved
			// requiring that stubs attached to tees have to move as well
			// if attached segments have not been selected, they must be unrouted
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->end_pin == cconnect::NO_END )
				{
					cvertex * end_vtx = &c->VtxByIndex(c->NumSegs());
					cseg * end_seg = &c->SegByIndex(c->NumSegs()-1);
					if( int id = end_vtx->tee_ID )
					{
						// stub tee
						int tee_ic;
						int tee_iv;
						BOOL bFound = m_doc->m_nlist->FindTeeVertexInNet( net, id, &tee_ic, &tee_iv );
						if ( !bFound )
						{
							end_vtx->tee_ID = 0;
						}
						else
						{
							cvertex * tee_vtx;
							tee_vtx = &net->ConByIndex(tee_ic)->VtxByIndex(tee_iv);
							if( tee_vtx->utility2 )
							{
								// tee-vertex was moved
								end_vtx->x = tee_vtx->x;
								end_vtx->y = tee_vtx->y;
								if( !end_seg->utility )
								{
									// attached segment not selected
									c->Undraw();
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
									c->Draw();
								}
							}
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}

	// merge unrouted segments for all traces
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			m_doc->m_nlist->MergeUnroutedSegments( net, ic );
		}
	}

	//** this shouldn't be necessary
	CNetList * nl = m_doc->m_nlist;
	for( cnet * n=iter_net.GetFirst(); n; n=iter_net.GetNext() )
		nl->RehookPartsToNet( n );
	//**
	m_doc->m_nlist->SetAreaConnections();

	// regenerate selection list from utility flags
	// first, remove all segments and vertices
	for( int i=m_sel_ids.GetSize()-1; i>=0; i-- )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_SEG )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
	}
	// add segments and vertices back into group
	net = iter_net.GetFirst();
	while( net )
	{
		if( net->utility )
		{
			// selected net
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility )
				{
					// selected connection
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							m_sel_ptrs.Add( net );
							id sid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
								ID_SEL_SEG, c->SegByIndex(is).UID(), is );
							m_sel_ids.Add( sid );
							dl_element * dl = c->SegByIndex(is).dl_el;
							if( dl )
								dl->visible = 1;	// restore visibility
						}
					}
					for( int iv=0; iv<c->NumSegs()+1; iv++ )
					{
						cvertex * v = &c->VtxByIndex(iv);
						if( v->utility )
						{
							if( v->via_w || v->tee_ID )
							{
								m_sel_ptrs.Add( net );
								id vid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
									ID_SEL_VERTEX, v->UID(), iv );
								m_sel_ids.Add( vid );
								if( v->via_w )
								{
									int n_el = v->dl_el.GetSize();
									for( int il=0; il<n_el; il++ )
										v->dl_el[il]->visible = 1;
								}
							}
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}
	groupAverageX+=dx;
	groupAverageY+=dy;
#endif
}


// Find item in group by id
// returns index of item if found, otherwise -1
// CPT2 TODO All one will need to do is to check m_sel.Contains(item)

int CFreePcbView::FindItemInGroup( void * ptr, id * tid )
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ptrs[i] == ptr && m_sel_ids[i] == *tid )
			return i;
	}
	return -1;
}

// Test for glued parts in group
//
BOOL CFreePcbView::GluedPartsInGroup()
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ids[i].T1() == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			if( part->glued )
				return TRUE;
		}
	}
	return FALSE;
}

// Unglue parts in group
// returns index of item if found, otherwise -1
//
void CFreePcbView::UngluePartsInGroup()
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ids[i].T1() == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->glued = FALSE;
		}
	}
}


void CFreePcbView::OnAddSimilarArea()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CancelHighlight();
	SetCursorMode( CUR_ADD_AREA );
	m_active_layer = m_sel_net->area[m_sel_ia].Layer();
	m_doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPolyLine::STRAIGHT;
	m_polyline_hatch = m_sel_net->area[m_sel_ia].GetHatch();
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnAreaEdit()
{
#ifndef CPT2
	CDlgAddArea dlg;
	int layer = m_sel_net->area[m_sel_id.I2()].Layer();
	int hatch = m_sel_net->area[m_sel_id.I2()].GetHatch();
	dlg.Initialize( m_doc->m_nlist, m_doc->m_num_layers, m_sel_net, layer, hatch );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		cnet * net = dlg.m_net;
		if( m_sel_net == net )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_doc->m_undo_list );
		}
		else
		{
			// move area to new net
			SaveUndoInfoForAllAreasIn2Nets( m_sel_net, net, TRUE, m_doc->m_undo_list );
			int ia = m_doc->m_nlist->AddArea( net, dlg.m_layer, 0, 0, 0 );
			net->area[ia].Copy( &m_sel_net->area[m_sel_ia] );
			net->area[ia].SetPtr( net );
			id new_id = net->area[ia].Id();
			new_id.SetI2( ia );
			net->area[ia].SetParentId( &new_id );
			m_doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia ); 
			m_doc->m_nlist->SetAreaConnections( net, ia );
			if( m_doc->m_vis[LAY_RAT_LINE] )
				m_doc->m_nlist->OptimizeConnections(  net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
			CancelSelection();
			m_sel_net = net;
			m_sel_id.SetI2( ia );
		}
		m_sel_net->area[m_sel_ia].Undraw();
		m_sel_net->area[m_sel_ia].SetLayer( dlg.m_layer );
		m_sel_net->area[m_sel_ia].SetHatch( dlg.m_hatch );
		m_sel_net->area[m_sel_ia].Draw();
		int ret = m_doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
		if( ret == -1 )
		{
			CString s ((LPCSTR) IDS_ErrorUnableToClipPolygonDueToIntersectingArc);
			AfxMessageBox( s );
			CancelSelection();
			m_doc->OnEditUndo();
		}
		if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE  );
		CancelSelection();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
#endif
}

void CFreePcbView::OnAreaEdgeApplyClearances()
{
#if 0	//** this is for testing only
	SaveUndoInfoForAllNetsAndConnectionsAndAreas( TRUE, m_doc->m_undo_list );
	m_doc->m_nlist->ApplyClearancesToArea( m_sel_net, m_sel_ia, m_doc->m_cam_flags,
		m_doc->m_fill_clearance, m_doc->m_min_silkscreen_stroke_wid,
		m_doc->m_thermal_width, m_doc->m_hole_clearance );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

void CFreePcbView::ReselectNetItemIfConnectionsChanged( int new_ic )
{
#ifndef CPT2

	if( new_ic >= 0 && new_ic < m_sel_net->NumCons()
		&& (m_cursor_mode == CUR_SEG_SELECTED
		|| m_cursor_mode == CUR_RAT_SELECTED
		|| m_cursor_mode == CUR_VTX_SELECTED
		|| m_cursor_mode == CUR_END_VTX_SELECTED
		|| m_cursor_mode == CUR_CONNECT_SELECTED
		|| m_cursor_mode == CUR_NET_SELECTED ) )
	{
		CancelHighlight();
		m_sel_id.SetI2( new_ic );
		if( m_cursor_mode == CUR_SEG_SELECTED )
			m_doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		else if( m_cursor_mode == CUR_RAT_SELECTED )
			m_doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		else if( m_cursor_mode == CUR_VTX_SELECTED )
			m_doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
		else if( m_cursor_mode == CUR_END_VTX_SELECTED )
			m_doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
		else if( m_cursor_mode == CUR_CONNECT_SELECTED )
			m_doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
		else if( m_cursor_mode == CUR_NET_SELECTED )
			m_doc->m_nlist->HighlightNet( m_sel_net );
	}
	else
		CancelSelection();
#endif
}

void CFreePcbView::OnGroupCopy() 
	// CPT:  added inner routine that has a return value
	{ DoGroupCopy(); }

bool CFreePcbView::DoGroupCopy()
{
#ifndef CPT2
	// CPT:  added return value (true on success).  
	// clear clipboard
	m_doc->clip_sm_cutout.SetSize(0);
	m_doc->clip_board_outline.SetSize(0);
	m_doc->clip_tlist->RemoveAllTexts();
	m_doc->clip_nlist->RemoveAllNets();
	m_doc->clip_plist->RemoveAllParts();

	// set pointers
	CArray<CPolyLine> * g_sm = &m_doc->clip_sm_cutout;
	CArray<CPolyLine> * g_bd = &m_doc->clip_board_outline;
	CPartList * g_pl = m_doc->clip_plist;
	CNetList * g_nl = m_doc->clip_nlist;
	CTextList * g_tl = m_doc->clip_tlist;

	// add all parts and text from group
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		bool bOK = sid.Resolve();
		if( !bOK )
			ASSERT(0);
		if( sid.T1() == ID_PART && sid.T2() == ID_SEL_RECT )
		{
			// add part to group partlist
			cpart * part = (cpart*)m_sel_ptrs[i];
			CShape * shape = part->shape;
			cpart * g_part = g_pl->Add( part->shape, &part->ref_des, &part->package, part->x, part->y,
				part->side, part->angle, 1, 0, part->m_ref_vis );
			// set ref text parameters
			g_part->m_ref_angle = part->m_ref_angle;
			g_part->m_ref_size = part->m_ref_size;
			g_part->m_ref_w = part->m_ref_w;
			g_part->m_ref_xi = part->m_ref_xi;
			g_part->m_ref_yi = part->m_ref_yi;
			g_part->m_ref_layer = part->m_ref_layer;
			g_part->m_ref_vis = part->m_ref_vis;
			// set value parameters
			g_part->value = part->value;
			g_part->m_value_angle = part->m_value_angle;
			g_part->m_value_size = part->m_value_size;
			g_part->m_value_w = part->m_value_w;
			g_part->m_value_xi = part->m_value_xi;
			g_part->m_value_yi = part->m_value_yi;
			g_part->m_value_layer = part->m_value_layer;
			g_part->m_value_vis = part->m_value_vis;
			// add pin nets to group netlist
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				part_pin * pin = &part->pin[ip];
				CShape * shape = part->shape;
				cnet * net = pin->net;
				if( net )
				{
					// add net to group netlist if not already added
					cnet * g_net = g_nl->GetNetPtrByName( &net->name );
					if( g_net == NULL )
					{
						g_net = g_nl->AddNet( net->name, net->NumPins(), net->def_w, net->def_via_w, net->def_via_hole_w );
					}
					// add pin to net
					CString pin_name = shape->GetPinNameByIndex( ip );
					g_net->AddPin( &part->ref_des, &pin_name, FALSE );
				}
			}
		}
		else if( sid.T1() == ID_TEXT && sid.T2() == ID_TEXT && sid.T3() == ID_SEL_TXT )
		{
			// add text string to group textlist
			CText * t = (CText*)m_sel_ptrs[i];
			g_tl->AddText( t->m_x, t->m_y, t->m_angle, t->m_mirror,  t->m_bNegative,
				t->m_layer, t->m_font_size, t->m_stroke_width, &t->m_str, FALSE );
		}
	}

	// mark all connections and areas as unchecked
	CIterator_cnet iter_net(m_doc->m_nlist);
	cnet * net = iter_net.GetFirst();
	while( net )
	{
		for( int ic=0; ic<net->NumCons(); ic++ )
			net->ConByIndex(ic)->utility = FALSE;
		for( int ia=0; ia<net->NumAreas(); ia++ )
			net->area[ia].utility = FALSE;
		net = iter_net.GetNext();
	}

	// check all selected areas and connections
	g_nl->ClearTeeIDs();
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT )
		{
			// connection, only add if between parts in group and/or tees
			cnet * net = (cnet*)m_sel_ptrs[i];
			cconnect * c = net->ConByIndex(sid.I2());
			if( c->utility == FALSE )
			{
				cnet * g_net = g_nl->GetNetPtrByName( &net->name );
				if( g_net == NULL )
				{
					g_net = g_nl->AddNet( net->name, net->NumPins(), net->def_w, net->def_via_w, net->def_via_hole_w );
				}
				// test start and end pins
				BOOL bStartPinInGroup = FALSE;
				BOOL bEndPinInGroup = FALSE;
				cpin * pin1 = NULL;
				cpin * pin2 = NULL;
				cpart * part1 = NULL;
				cpart * part2 = NULL;
				if( c->start_pin != cconnect::NO_END )
				{
					pin1 = &net->pin[c->start_pin];
					part1 = pin1->part;
				}
				if( c->end_pin != cconnect::NO_END )
				{
					pin2 = &net->pin[c->end_pin];
					part2 = pin2->part;
				}
				// loop through all group parts
				cpart * g_part = g_pl->GetFirstPart();
				while( g_part )
				{
					if( part1 )
					{
						if( part1->ref_des == g_part->ref_des )
							bStartPinInGroup = TRUE;
					}
					if( part2 )
					{
						if( part2->ref_des == g_part->ref_des )
							bEndPinInGroup = TRUE;
					}
					g_part = g_pl->GetNextPart( g_part );
				}
				if( (bStartPinInGroup || !pin1) && (bEndPinInGroup || !pin2) )
				{
					// add connection to group net, and copy all segments and vertices
					cconnect * g_c = NULL;
					int g_ic = -1;
					if( bStartPinInGroup )
					{
						// make connection from pin1
						int p1 = g_nl->GetNetPinIndex( g_net, &pin1->ref_des, &pin1->pin_name );
						if( bEndPinInGroup )
						{
							// pin1-pin2 connnection
							int p2 = g_nl->GetNetPinIndex( g_net, &pin2->ref_des, &pin2->pin_name );
							g_c = g_net->AddConnectFromPinToPin( p1, p2, &g_ic );
						}

						else
						{
							// pin1-tee connection
							g_c = g_net->AddConnectFromPin( p1, &g_ic );
						}
					}
					else if( bEndPinInGroup )
					{
						// tee-pin2 connection, make connection from pin2 then reverse it
						int p2 = g_nl->GetNetPinIndex( g_net, &pin2->ref_des, &pin2->pin_name );
						g_c = g_net->AddConnectFromPin( p2, &g_ic );
						g_c->ReverseDirection();
					}
					else
					{
						// tee-tee connection
						g_c = g_net->AddConnect( &g_ic);
						g_c->PrependVertex( *c->FirstVtx() );
					}
					if( !g_c )
						continue;

					g_c->SetNumSegs( c->NumSegs() );
					for( int is=0; is<c->NumSegs(); is++ )
					{
						g_c->SegByIndex(is) = c->SegByIndex(is);
						g_c->SegByIndex(is).m_dlist = NULL;
						g_c->SegByIndex(is).dl_el = NULL;
						g_c->SegByIndex(is).dl_sel = NULL;
						g_c->SegByIndex(is).m_con = g_c;
						g_c->VtxByIndex(is) = c->VtxByIndex(is);	// this zeros graphics elements in c
						c->VtxByIndex(is) = g_c->VtxByIndex(is);	// this restores them, and zeros g_c
						g_c->VtxByIndex(is).m_con = g_c;
						g_nl->AddTeeID( g_c->VtxByIndex(is).tee_ID );
					}
					g_c->VtxByIndex(c->NumSegs()) = c->VtxByIndex(c->NumSegs());
					c->VtxByIndex(c->NumSegs()) = g_c->VtxByIndex(c->NumSegs());
					g_c->VtxByIndex(c->NumSegs()).m_con = g_c;
//					g_c->vtx[c->NumSegs()].m_bDrawingEnabled = FALSE;
					g_nl->AddTeeID( g_c->VtxByIndex(c->NumSegs()).tee_ID );

					// remove any routed segments that are not in group
					for( int is=0; is<c->NumSegs(); is++ )
					{
						cseg * s = &c->SegByIndex(is);
						if( c->SegByIndex(is).m_layer != LAY_RAT_LINE )
						{
							// routed segment, is this in group ?
							id search_id = s->Id();
							BOOL bInGroup = FALSE;
							for( int i=0; i<m_sel_ids.GetSize(); i++ )
							{
								id t_id = m_sel_ids[i];
								if( t_id == search_id )
								{
									// this segment is in group
									bInGroup = TRUE;
									break;
								}
							}
							if( !bInGroup )
							{
								// not in group, unroute it
								g_nl->UnrouteSegmentWithoutMerge( g_net, g_ic, is );
							}
						}
					}
					// remove any vias or tees that are not in group
					for( int iv=1; iv<c->NumSegs(); iv++ )
					{
						cvertex * v = &c->VtxByIndex(iv);
						if( v->tee_ID || v->via_w )
						{
							// is this in group?
							id search_id = sid;
							search_id.SetT3( ID_SEL_VERTEX );
							search_id.SetI3( iv );
							BOOL bInGroup = FALSE;
							for( int i=0; i<m_sel_ids.GetSize(); i++ )
							{
								id t_id = m_sel_ids[i];
								if( t_id == search_id )
								{
									// this vertex is in group
									bInGroup = TRUE;
									break;
								}
							}
							if( !bInGroup )
							{
								// delete the vertex from group
								cvertex * g_v = &g_c->VtxByIndex(iv);
								g_v->tee_ID = 0;
								g_v->force_via_flag = 0;
								g_v->via_w = 0;
								if( c->end_pin == cconnect::NO_END && iv == c->NumSegs() )
								{
									// last vertex of stub trace, just delete last segment
									//** TODO should actually remove segment, but this could change
									// the connection array
									g_nl->UnrouteSegmentWithoutMerge( g_net, g_ic, iv-1 );
								}
								else
								{
									// deleting vertex between two segments
									g_nl->UnrouteSegmentWithoutMerge( g_net, g_ic, iv-1 );
									g_nl->UnrouteSegmentWithoutMerge( g_net, g_ic, iv );
								}
								//**
							}
						}
					}
					// merge unrouted segments
					g_nl->MergeUnroutedSegments( g_net, g_ic );
				}
			}
			c->utility = TRUE;	// mark as checked
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_AREA
			&& sid.T3() == ID_SEL_SIDE )
		{
			// area side selected
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.I2()];
			CPolyLine * p = a;
			if( a->utility == 0 )
			{
				// first area side found, mark area as selected and
				// all other sides as unselected
				for( int is=0; is<p->NumSides(); is++ )
					p->SetUtility( is, 0 );
				a->utility = 1;
			}
			p->SetUtility( sid.I3(), 1 );	// mark this side as selected
		}
	}

	g_nl->CleanUpAllConnections();

	// now check areas, only copy areas if all sides are selected
	net = iter_net.GetFirst();
	while( net )
	{
		for( int ia=0; ia<net->NumAreas(); ia++ )
		{
			carea * a = &net->area[ia];
			if( a->utility )
			{
				BOOL bAllSides = TRUE;
				CPolyLine * p = a;
				for( int is=0; is<p->NumSides(); is++ )
				{
					if( p->Utility( is ) == 0 )
					{
						bAllSides = FALSE;
						break;
					}
				}
				if( bAllSides )
				{
					// add area to group
					cnet * g_net = g_nl->GetNetPtrByName( &net->name );
					if( g_net == NULL )
					{
						g_net = g_nl->AddNet( net->name, net->NumPins(), net->def_w, net->def_via_w, net->def_via_hole_w );
					}
					int g_ia = g_nl->AddArea( g_net, p->Layer(), p->X(0), p->Y(0),
						p->GetHatch() );
					CPolyLine * g_p = &g_net->area[g_ia];
					g_p->Copy( p );
					id g_id;
					g_id = g_p->Id();
					g_id.SetI2( g_ia );
					g_p->SetParentId( &g_id );
				}
			}
		}
		net = iter_net.GetNext();
	}

	// now remove any nets with zero pins, connections and areas
	CIterator_cnet iter_net_g(g_nl);
	net = iter_net_g.GetFirst();
	while( net )
	{
		if( net->NumPins() == 0 && net->NumCons() == 0 && net->NumAreas() == 0 )
			g_nl->RemoveNet( net );
		net = iter_net_g.GetNext();
	}

	// mark all sm_cutouts as unselected
	for( int ism=0; ism<m_doc->m_sm_cutout.GetSize(); ism++ )
	{
		for( int is=0; is<m_doc->m_sm_cutout[ism].NumSides(); is++ )
			m_doc->m_sm_cutout[ism].SetUtility( is, 0 );
	}
	// find selected sides
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			m_doc->m_sm_cutout[sid.I2()].SetUtility( sid.I3(), 1 );
		}
	}
	// copy to group
	for( int ism=0; ism<m_doc->m_sm_cutout.GetSize(); ism++ )
	{
		CPolyLine * p = &m_doc->m_sm_cutout[ism];
		BOOL bAllSides = TRUE;
		for( int is=0; is<p->NumSides(); is++ )
		{
			if( p->Utility( is ) == 0 )
			{
				bAllSides = FALSE;
				break;
			}
		}
		if( bAllSides )
		{
			// add to group
			int g_ism = g_sm->GetSize();
			g_sm->SetSize(g_ism+1);
			CPolyLine * g_p = &(*g_sm)[g_ism];
			g_p->Copy( p );
			id sid = g_p->Id();
			sid.SetI2( g_ism );
			g_p->SetParentId( &sid );
		}
	}

	// mark all board outlines as unselected
	for( int ibd=0; ibd<m_doc->m_board_outline.GetSize(); ibd++ )
	{
		for( int is=0; is<m_doc->m_board_outline[ibd].NumSides(); is++ )
			m_doc->m_board_outline[ibd].SetUtility( is, 0 );
	}
	// find selected sides
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD && sid.T3() == ID_SEL_SIDE )
		{
			m_doc->m_board_outline[sid.I2()].SetUtility( sid.I3(), 1 );
		}
	}
	// copy to group
	for( int ibd=0; ibd<m_doc->m_board_outline.GetSize(); ibd++ )
	{
		CPolyLine * p = &m_doc->m_board_outline[ibd];
		BOOL bAllSides = TRUE;
		for( int is=0; is<p->NumSides(); is++ )
		{
			if( p->Utility( is ) == 0 )
			{
				bAllSides = FALSE;
				break;
			}
		}
		if( bAllSides )
		{
			// add to group
			int g_ibd = g_bd->GetSize();
			g_bd->SetSize(g_ibd+1);
			CPolyLine * g_p = &(*g_bd)[g_ibd];
			g_p->Copy( p );
			id sid = g_p->Id();
			sid.SetI2( g_ibd );
			g_p->SetParentId( &sid );
		}
	}

	// see if anything copied
	if( !iter_net_g.GetFirst() && !g_pl->GetFirstPart() && !g_sm->GetSize() 
		&& !g_bd->GetSize() && !g_tl->GetNumTexts() )
	{
		CString s ((LPCSTR) IDS_NothingCopiedRememberThatTracesMustBeConnected);
		AfxMessageBox( s );
		CWnd* pMain = AfxGetMainWnd();
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
			pMain->DrawMenuBar();
		}
		return false;								// CPT
	}
	else
	{
		CWnd* pMain = AfxGetMainWnd();
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_ENABLED );
			pMain->DrawMenuBar();
		}
	}
#endif
	return true;									// CPT
}

// function to find all stub traces ending on tee and mark them for removal,
// then looks for any tees on that stub and recurses
//
void MarkStubsForRemoval( cnet * net, int tee_ID )
{
	CIterator_cconnect iter_con(net);
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		cvertex * end_v = &c->VtxByIndex(c->NumSegs());
		if( c->end_pin == cconnect::NO_END && end_v->tee_ID == tee_ID )
		{
			// if already marked for removal, ignore
			if( c->utility != 2 || net->utility != 1 )
			{
				// else flag this stub for removal, and search for other tees in stub
				c->utility = 2;
				net->utility = 1;
				for( int iv=1; iv<c->NumSegs(); iv++ )
				{
					cvertex * v = &c->VtxByIndex(iv);
					if( v->tee_ID )
					{
						MarkStubsForRemoval( net, v->tee_ID );
					}
				}
			}
		}
	}
}

void CFreePcbView::OnGroupCut()
{
	// CPT: took advantage of return value from the new DoGroupCopy().
	if (DoGroupCopy())
		OnGroupDelete();
}

// Remove all elements in group from project
//
void CFreePcbView::OnGroupDelete()
{
	DeleteGroup( &m_sel_ptrs, &m_sel_ids );
	CancelSelection();
	m_doc->ProjectModified( TRUE );
}

void CFreePcbView::DeleteGroup( CArray<void*> * grp_ptr, CArray<id> * grp_id )
{
#ifndef CPT2

	CPartList * pl =  m_doc->m_plist;
	CNetList * nl = m_doc->m_nlist;
	cpart * part;
	cnet * net;

	// create undo descriptor before deletion
	undo_group_descriptor * undo = (undo_group_descriptor*)CreateGroupDescriptor( m_doc->m_undo_list,
		grp_ptr, grp_id, UNDO_GROUP_DELETE );

	// mark all parts and nets as unmodified
	nl->MarkAllNets( 0 );
	for( part=pl->GetFirstPart(); part; part=pl->GetNextPart(part) )
		part->utility = 0;

	// loop through selected items and mark parts and nets that need to be saved
	// for undoing
	for( int i=0; i<grp_id->GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		void * ptr = (*grp_ptr)[i];
		if( this_id.T1() == ID_PART && this_id.T2() == ID_SEL_RECT )
		{
			cpart * part = (cpart *) (*grp_ptr)[i];
			part->utility = 1;		// this part will be deleted
			if( part->shape )
			{
				for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
				{
					cnet * pin_net = part->pin[ip].net;
					if( pin_net )
						pin_net->utility = 1;	// this net will be modified
				}
			}
		}
		if( this_id.T1() == ID_NET && this_id.T2() == ID_CONNECT )
		{
			cnet * net = (cnet *) (*grp_ptr)[i];
			net->utility = 1;		// this net will be modified
		}
	}
	// save undo info
	m_doc->m_undo_list->NewEvent();
	CIterator_cnet iter_net(nl);
	for( net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
		if( net->utility )
			SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE, m_doc->m_undo_list );
	for( part=pl->GetFirstPart(); part; part=pl->GetNextPart(part) )
		if( part->utility )
			SaveUndoInfoForPart( part,
			CPartList::UNDO_PART_DELETE, NULL, FALSE, m_doc->m_undo_list );

	// mark all nets as unmodified (again)
	nl->MarkAllNets( 0 );
	// mark all sm_cutout sides as unselected
	for( int ism=0; ism<m_doc->m_sm_cutout.GetSize(); ism++ )
		for( int is=0; is<m_doc->m_sm_cutout[ism].NumSides(); is++ )
			m_doc->m_sm_cutout[ism].SetUtility( is, 0 );
	// mark all board outline sides as unselected
	for( int ibd=0; ibd<m_doc->m_board_outline.GetSize(); ibd++ )
		for( int is=0; is<m_doc->m_board_outline[ibd].NumSides(); is++ )
			m_doc->m_board_outline[ibd].SetUtility( is, 0 );

	// unroute selected trace segments
	for( int i=0; i<(*grp_id).GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		void * ptr = (*grp_ptr)[i];
		if( this_id.T1() == ID_NET )
		{
			cnet * net = (cnet*)ptr;
			if( this_id.T2() == ID_CONNECT )
			{
				// don't actually delete connections, just unroute
				int ic = this_id.I2();
				cconnect * c = net->ConByIndex(ic);
				if( this_id.T3() == ID_SEL_SEG )
				{
					// unroute segment
					int is = this_id.I3();
					nl->UnrouteSegmentWithoutMerge( net, ic, is );
					net->utility = 1;	// flag net as modified
					if( c->utility == 0 )
						c->utility = 1;		// flag connection as modified
				}
				else if( this_id.T3() == ID_SEL_VERTEX )
				{
					// unforce via
					int iv = this_id.I3();
					cvertex * v = &c->VtxByIndex(iv);
					if( v->force_via_flag )
					{
						v->force_via_flag = 0;
						net->utility = 1;
						if( c->utility == 0 )
							c->utility = 1;		// flag connection as modified
					}
				}
				else
					ASSERT(0);
			}
		}
	}

	// find non-branch stub traces with no end via and remove trailing unrouted segments
	net = iter_net.GetFirst();
	while( net )
	{
		if( net->utility )
		{
			for( int ic=net->NumCons()-1; ic>=0; ic-- )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility == 1 )
				{
					cvertex * end_v = &c->VtxByIndex(c->NumSegs());
					if( c->end_pin == cconnect::NO_END )
					{
						int is=c->NumSegs()-1;
						cseg * s = &c->SegByIndex(is);
						cvertex * next_v = &c->VtxByIndex(is+1);
						if( s->m_layer == LAY_RAT_LINE && next_v->force_via_flag == 0 && next_v->tee_ID == 0 )
						{
							if( c->NumSegs() == 1 )
								c->utility = 2;		// mark connection for deletion
							else
								nl->RemoveSegment( net, ic, is, FALSE, FALSE );	// remove segment
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}

	// remove connections marked for deletion and merge unrouted segments
	net = iter_net.GetFirst();
	while( net )
	{
		if( net->utility )
		{
			CIterator_cconnect iter_con( net );
			for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
			{
				int ic = iter_con.GetIndex();
				if( c->utility == 2 )
				{
					net->RemoveConnectAdjustTees( c );
				}
				else if( c->utility == 1 )
				{
					nl->MergeUnroutedSegments( net, ic );
				}
			}
		}
		net = iter_net.GetNext();
	}
	// remove board outlines, copper areas, parts, texts and sm_cutouts
	for( int i=0; i<(*grp_id).GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		if( this_id.T1() == ID_NET && this_id.T2() == ID_AREA && this_id.T3() == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)(*grp_ptr)[i];
			carea * a = &net->area[this_id.I2()];
			a->SetUtility( this_id.I3(), 1 );	// mark for deletion
			a->utility = 1;
			net->utility = 1;						// mark as modified
		}
		if( this_id.T1() == ID_PART && this_id.T2() == ID_SEL_RECT )
		{
			cpart * part = (cpart *) (*grp_ptr)[i];
			nl->PartDeleted( part, FALSE );
			pl->Remove( part );
		}
		else if( this_id.T1() == ID_TEXT )
		{
			CText * text = (CText *)(*grp_ptr)[i];
			SaveUndoInfoForText( text, CTextList::UNDO_TEXT_DELETE, FALSE, m_doc->m_undo_list );
			m_doc->m_tlist->RemoveText( text );
		}
		else if( this_id.T1() == ID_MASK && this_id.T3() == ID_SEL_SIDE )
			m_doc->m_sm_cutout[this_id.I2()].SetUtility( this_id.I3(), 1 );	// mark for deletion
		else if( this_id.T1() == ID_BOARD && this_id.T3() == ID_SEL_SIDE )
			m_doc->m_board_outline[this_id.I2()].SetUtility( this_id.I3(), 1 );	// mark for deletion
	}
	// delete sm_cutouts and renumber them
	BOOL bUndoSaved = FALSE;
	for( int ism=m_doc->m_sm_cutout.GetSize()-1; ism>=0; ism-- )
	{
		BOOL bDelete = TRUE;
		for( int is=0; is<m_doc->m_sm_cutout[ism].NumSides(); is++ )
			if( m_doc->m_sm_cutout[ism].Utility( is ) == 0 )
				bDelete = FALSE;
		if( bDelete )
		{
			if( !bUndoSaved )
				SaveUndoInfoForSMCutouts( FALSE, m_doc->m_undo_list );
			m_doc->m_sm_cutout.RemoveAt( ism );
			bUndoSaved = TRUE;
		}
	}
	for( int ism=0; ism<m_doc->m_sm_cutout.GetSize(); ism++ )
	{
			id new_id = m_doc->m_sm_cutout[ism].Id();
			new_id.SetI2( ism );
			m_doc->m_sm_cutout[ism].SetParentId( &new_id );
	}
	// delete board outlines and renumber them
	bUndoSaved = FALSE;
	for( int ibd=m_doc->m_board_outline.GetSize()-1; ibd>=0; ibd-- )
	{
		BOOL bDelete = TRUE;
		for( int is=0; is<m_doc->m_board_outline[ibd].NumSides(); is++ )
			if( m_doc->m_board_outline[ibd].Utility( is ) == 0 )
				bDelete = FALSE;
		if( bDelete )
		{
			if( !bUndoSaved )
				SaveUndoInfoForBoardOutlines( FALSE, m_doc->m_undo_list );
			m_doc->m_board_outline.RemoveAt( ibd );
			bUndoSaved = TRUE;
		}
	}
	for( int ibd=0; ibd<m_doc->m_board_outline.GetSize(); ibd++ )
	{
			id new_id = m_doc->m_board_outline[ibd].Id();
			new_id.SetI2( ibd );
			m_doc->m_board_outline[ibd].SetParentId( &new_id );
	}
	// delete copper areas or cutouts if all sides are in group
	net = iter_net.GetFirst();
	while( net )
	{
		for( int ia=net->NumAreas()-1; ia>=0; ia-- )
		{
			carea * a = &net->area[ia];
			if( a->utility )
			{
				// see if entire area can be deleted
				BOOL bDelete = TRUE;
				for( int is=0; is<a->ContourEnd(0); is++ )
					if( a->Utility( is ) == 0 )
						bDelete = FALSE;
				if( bDelete )
				{
					SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_DELETE, FALSE, m_doc->m_undo_list );
					nl->RemoveArea( net, ia );
				}
				else if( a->NumContours() > 1 )
				{
					// see if cutouts can be deleted
					BOOL bCutoutsDeleted = FALSE;
					for( int icont=a->NumContours()-1; icont>0; icont-- )
					{
						int istart = a->ContourStart( icont );
						int iend = a->ContourEnd( icont );
						bDelete = TRUE;
						for( int is=istart; is<=iend; is++ )
							if( a->Utility( is ) == 0 )
								bDelete = FALSE;
						if( bDelete )
						{
							a->RemoveContour( icont );
							bCutoutsDeleted = TRUE;
						}
					}
					if( bCutoutsDeleted )
						SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_MODIFY, FALSE, m_doc->m_undo_list );
				}
			}
		}
		nl->SetAreaConnections( net );
		net = iter_net.GetNext();
	}
	// clean up
	m_doc->m_undo_list->Push( UNDO_GROUP, (void*)undo, &UndoGroupCallback );
#endif
}

void CFreePcbView::OnGroupPaste()
{
#ifndef CPT2
	void * vp;
	// pointers to group lists
	CPartList * g_pl = m_doc->clip_plist;
	CTextList * g_tl = m_doc->clip_tlist;
	CNetList * g_nl = new CNetList( NULL, g_pl, m_doc );	// make copy to modify
	g_nl->Copy( m_doc->clip_nlist );
	g_nl->MarkAllNets( 0 );
	CArray<CPolyLine> * g_sm = &m_doc->clip_sm_cutout;
	CArray<CPolyLine> * g_bd = &m_doc->clip_board_outline;

	// pointers to project lists
	CPartList * pl = m_doc->m_plist;
	CNetList * nl = m_doc->m_nlist;
	CTextList * tl = m_doc->m_tlist;
	CArray<CPolyLine> * sm = &m_doc->m_sm_cutout;
	CArray<CPolyLine> * bd = &m_doc->m_board_outline;

	// get paste options
	CDlgGroupPaste dlg;
	dlg.Initialize( g_nl );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// start pasting
		CancelSelection();
		SetCursorMode( CUR_GROUP_SELECTED );
		m_sel_id.SetT1(ID_MULTI);
		m_doc->m_undo_list->NewEvent();
		nl->MarkAllNets( 0 );	// mark all nets as unsaved
		BOOL bDragGroup = !dlg.m_position_option;
		double min_d = (double)INT_MAX*(double)INT_MAX;
		int min_x = INT_MAX;	// lowest-left point for dragging group
		int min_y = INT_MAX;

		// make a map of all reference designators in project, including
		// refs in the netlist that don't exist in the partlist
		CMapStringToPtr ref_des_map;
		cpart * part = pl->GetFirstPart();
		while( part )
		{
			ref_des_map.SetAt( part->ref_des, NULL );
			part = pl->GetNextPart( part );
		}
		CIterator_cnet iter_net(nl);
		cnet * net = iter_net.GetFirst();
		while( net )
		{
			for( int ip=0; ip<net->NumPins(); ip++ )
			{
				cpin * p = &net->pin[ip];
				if( !ref_des_map.Lookup( p->ref_des, vp ) )
				{
					ref_des_map.SetAt( p->ref_des, NULL );
				}
			}
			net = iter_net.GetNext();
		}

		// add parts from group, renaming if necessary
		cpart * g_part = g_pl->GetFirstPart();
		while( g_part )
		{
			CString conflicted_ref;
			CString g_prefix;
			int g_num = ParseRef( &g_part->ref_des, &g_prefix );
			BOOL bConflict = FALSE;
			// make new ref
			CString new_ref = g_part->ref_des;
			if( dlg.m_ref_option == 2 )
			{
				// add offset to ref
				new_ref.Format( "%s%d", g_prefix, g_num + dlg.m_ref_offset );
			}
			if( dlg.m_ref_option != 1 && ref_des_map.Lookup( new_ref, vp ) )
			{
				// new ref conflicts with existing ref in project
				conflicted_ref = new_ref;
				bConflict = TRUE;
			}
			if( dlg.m_ref_option == 1 || bConflict )
			{
				// use next available ref
				int max_num = 0;
				POSITION pos;
				CString key;
				void * ptr;
				for( pos = ref_des_map.GetStartPosition(); pos != NULL; )
				{
					ref_des_map.GetNextAssoc( pos, key, ptr );
					CString prefix;
					int i = ParseRef( &key, &prefix );
					if( prefix == g_prefix && i > max_num )
						max_num = i;
				}
				new_ref.Format( "%s%d", g_prefix, max_num+1 );
			}
			if( bConflict )
			{
				// ref in group conflicts with ref in project
				CString s ((LPCSTR) IDS_PartAlreadyExistsInProjectItWillBeRenamed), mess;
				mess.Format(s, conflicted_ref, new_ref);
				AfxMessageBox( mess );
				bConflict = TRUE;
			}
			// now change part refs in group netlist
			CIterator_cnet iter_net_g(g_nl);
			net = iter_net_g.GetFirst();
			while( net )
			{
				for( int ip=0; ip<net->NumPins(); ip++ )
				{
					cpin * pin = &net->pin[ip];
					if( pin->utility == 0 && pin->ref_des == g_part->ref_des )
					{
						pin->ref_des = new_ref;
						pin->part = NULL;
						pin->utility = 1;	// only change it once
					}
				}
				net = iter_net_g.GetNext();
			}
			// add new part
			cpart * prj_part = pl->Add( g_part->shape, &new_ref, &g_part->package,
				g_part->x + dlg.m_dx, g_part->y + dlg.m_dy,
				g_part->side, g_part->angle, 1, 0, g_part->m_ref_vis );
			ref_des_map.SetAt( new_ref, NULL );
			SaveUndoInfoForPart( prj_part,
				CPartList::UNDO_PART_ADD, &prj_part->ref_des, FALSE, m_doc->m_undo_list );
			pl->UndrawPart( prj_part );

			// set ref text parameters
			prj_part->m_ref_angle = g_part->m_ref_angle;
			prj_part->m_ref_size = g_part->m_ref_size;
			prj_part->m_ref_w = g_part->m_ref_w;
			prj_part->m_ref_xi = g_part->m_ref_xi;
			prj_part->m_ref_yi = g_part->m_ref_yi;
			prj_part->m_ref_vis = g_part->m_ref_vis;
			prj_part->m_ref_layer = g_part->m_ref_layer;
			// set value parameters
			prj_part->value = g_part->value;
			prj_part->m_value_angle = g_part->m_value_angle;
			prj_part->m_value_size = g_part->m_value_size;
			prj_part->m_value_w = g_part->m_value_w;
			prj_part->m_value_xi = g_part->m_value_xi;
			prj_part->m_value_yi = g_part->m_value_yi;
			prj_part->m_value_vis = g_part->m_value_vis;
			prj_part->m_value_layer = g_part->m_value_layer;
			pl->DrawPart( prj_part );
			// find closest part to lower left corner
			double d = prj_part->x + prj_part->y;
			if( d < min_d )
			{
				min_d = d;
				min_x = prj_part->x;
				min_y = prj_part->y;
			}
			// add pointer and id to group selector array
			m_sel_ptrs.Add( prj_part );
			INT_PTR i = m_sel_ids.Add( prj_part->m_id );
			m_sel_ids[i].SetT2( ID_SEL_RECT );
			// end of loop, get next group part
			g_part = g_pl->GetNextPart( g_part );
		}

		// add nets from group
		// rename net if necessary
		CString g_suffix;
		if( dlg.m_net_rename_option == 0 )
		{
			// get highest group suffix already in project
			int max_g_num = 0;
			cnet * net = iter_net.GetFirst();
			while( net )
			{
				int n = net->name.ReverseFind( '_' );
				if( n > 0 )
				{
					CString prefix;
					CString test_suffix = net->name.Right( net->name.GetLength() - n - 1 );
					int g_num = ParseRef( &test_suffix, &prefix );
					if( prefix == "$G" )
						max_g_num = max( g_num, max_g_num );
				}
				net = iter_net.GetNext();
			}
			g_suffix.Format( "_$G%d", max_g_num + 1 );
		}
		// now loop through all nets in group and add or merge with project
		cnet * prj_net = NULL;	// project net
		CIterator_cnet iter_net_g(g_nl);
		cnet * g_net = iter_net_g.GetFirst();	// group net
		while( g_net )
		{
			// see if there are routed segments in this net
			BOOL bRouted = FALSE;
			if( dlg.m_pin_net_option == 1 )
			{
				for( int ic=0; ic<g_net->NumCons(); ic++ )
				{
					cconnect * c = g_net->ConByIndex(ic);
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).m_width > 0 )
						{
							bRouted = TRUE;
							break;
						}
					}
					if( bRouted )
						break;
				}
			}
			// only add if there are areas, or routed segments if requested
			if( (dlg.m_pin_net_option == 0 || bRouted) || g_net->NumAreas() > 0 )
			{
				// OK, add this net to project
				// utility flag is set in the Group Paste dialog for nets which
				// should be merged (i.e. not renamed)
				if( dlg.m_net_name_option == 1 && g_net->utility == 0 )
				{
					// rename net
					CString new_name;
					if( dlg.m_net_rename_option == 1 )
					{
						// get next "Nnnnnn" net name
						cnet * net = iter_net.GetFirst();
						int max_num = 0;
						CString prefix;
						while( net )
						{
							int num = ParseRef( &net->name, &prefix );
							if( prefix == "N" && num > max_num )
								max_num = num;
							net = iter_net.GetNext();
						}
						new_name.Format( "N%05d", max_num+1 );
					}
					else
					{
						// add group suffix
						new_name = g_net->name + g_suffix;
					}
					// add new net
					prj_net = nl->AddNet( new_name, g_net->NumPins(),
						g_net->def_w, g_net->def_via_w, g_net->def_via_hole_w );
					SaveUndoInfoForNet( prj_net, CNetList::UNDO_NET_ADD, FALSE, m_doc->m_undo_list );
					prj_net->utility = 1;	// mark as saved
				}
				else
				{
					// merge group net with project net of same name
					prj_net = nl->GetNetPtrByName( &g_net->name );
					if( !prj_net )
					{
						// no project net with the same name
						prj_net = nl->AddNet( g_net->name, g_net->NumPins(),
							g_net->def_w, g_net->def_via_w, g_net->def_via_hole_w );
						SaveUndoInfoForNet( prj_net, CNetList::UNDO_NET_ADD, FALSE, m_doc->m_undo_list );
						prj_net->utility = 1;	// mark as saved
					}
					else if( prj_net->utility == 0 )
					{
						SaveUndoInfoForNetAndConnectionsAndAreas( prj_net, FALSE, m_doc->m_undo_list );
						prj_net->utility = 1;	// mark as saved
					}
				}
				if( !prj_net )
					ASSERT(0);
				// now create map for renaming tees
				CMap<int,int,int,int> tee_map;
				// connect group part pins to project net
				for( int ip=0; ip<g_net->NumPins(); ip++ )
				{
					cpin * pin = &g_net->pin[ip];
					BOOL bAdd = TRUE;
					if( dlg.m_pin_net_option == 1 )
					{
						// only add pin if connected to a routed trace
						bAdd = FALSE;
						for( int ic=0; ic<g_net->NumCons(); ic++ )
						{
							cconnect * c = g_net->ConByIndex(ic);
							if( c->start_pin == ip || c->end_pin == ip )
							{
								for( int is=0; is<c->NumSegs(); is++ )
								{
									if( c->SegByIndex(is).m_width > 0 )
									{
										bAdd = TRUE;
										break;
									}
								}
							}
							if( bAdd )
								break;
						}
					}
					if( bAdd )
						prj_net->AddPin( &pin->ref_des, &pin->pin_name, FALSE );
				}
				// create new traces
				for( int g_ic=0; g_ic<g_net->NumCons(); g_ic++ )
				{
					cconnect * g_c = g_net->ConByIndex(g_ic);
					// get start pin of connection in new net
					int new_start_pin = cconnect::NO_END;
					if( g_c->start_pin != cconnect::NO_END )
					{
						CString g_start_ref_des = g_net->pin[g_c->start_pin].ref_des;
						CString g_start_pin_name = g_net->pin[g_c->start_pin].pin_name;
						new_start_pin = nl->GetNetPinIndex( prj_net, &g_start_ref_des, &g_start_pin_name );
					}
					// get end pin of connection in new net
					int new_end_pin = cconnect::NO_END;
					if( g_c->end_pin != cconnect::NO_END )
					{
						CString g_end_ref_des = g_net->pin[g_c->end_pin].ref_des;
						CString g_end_pin_name = g_net->pin[g_c->end_pin].pin_name;
						new_end_pin = nl->GetNetPinIndex( prj_net, &g_end_ref_des, &g_end_pin_name );
					}
					int ic;
					if( new_start_pin != -1 && new_end_pin != -1 )
					{
						// pin-pin connection
						prj_net->AddConnectFromPinToPin( new_start_pin, new_end_pin, &ic );
					}
					if( new_start_pin != -1 && new_end_pin == -1 )
					{
						// pin-tee
						prj_net->AddConnectFromPin( new_start_pin, &ic );
					}
					else if( new_start_pin == -1 && new_end_pin != -1 )
					{
						// tee-pin
						cconnect * c = prj_net->AddConnectFromPin( new_end_pin, &ic );
						c->ReverseDirection();
					}
					else
					{
						// tee-tee
						cconnect * c = prj_net->AddConnect( &ic );
					}
					// copy it and draw it
					if( ic < 0 )
						ASSERT(0);
					else
					{
						// copy connection
						cconnect * c = prj_net->ConByIndex(ic);
						c->Undraw();
						c->SetNumSegs( g_c->NumSegs() );
						for( int is=0; is<c->NumSegs(); is++ )
						{
							cseg * s = &c->SegByIndex(is);
							cvertex * v = &c->VtxByIndex(is);
							*s = g_c->SegByIndex(is);
							s->m_con = c;
							s->m_dlist = m_dlist;
							s->dl_el = NULL;
							s->dl_sel = NULL;
							*v = g_c->VtxByIndex(is);
							v->m_dlist = m_dlist;
							v->m_con = c;
							v->dl_sel = NULL;
							v->dl_hole = NULL;
							v->dl_el.SetSize(0);
							id seg_id( ID_NET, prj_net->UID(), ID_CONNECT, c->UID(), ic, 
								ID_SEL_SEG, s->UID(), is );
							m_sel_ptrs.Add( prj_net );
							m_sel_ids.Add( seg_id );
							id vtx_id( ID_NET, prj_net->UID(), ID_CONNECT, c->UID(), ic, 
								ID_SEL_VERTEX, v->UID(), is );
							m_sel_ptrs.Add( prj_net );
							m_sel_ids.Add( vtx_id );
						}
						cvertex * v = &c->VtxByIndex(c->NumSegs());
						*v = g_c->VtxByIndex(g_c->NumSegs());
						v->m_con = c;
						v->m_dlist = m_dlist;
						v->dl_sel = NULL;
						v->dl_hole = NULL;
						v->dl_el.SetSize(0);
						if( c->end_pin != cconnect::NO_END )
						{
							id vtx_id( ID_NET, prj_net->UID(), ID_CONNECT, c->UID(), ic,
								ID_SEL_VERTEX, v->UID(), c->NumSegs() );
							m_sel_ptrs.Add( prj_net );
							m_sel_ids.Add( vtx_id );
						}
						for( int iv=0; iv<c->NumSegs()+1; iv++ )
						{
							c->VtxByIndex(iv).x += dlg.m_dx;
							c->VtxByIndex(iv).y += dlg.m_dy;
							if( int g_id = c->VtxByIndex(iv).tee_ID )
							{
								// assign new tee_ID
								int new_id;
								BOOL bFound = tee_map.Lookup( abs(g_id), new_id );
								if( !bFound )
								{
									new_id = nl->GetNewTeeID();
									tee_map.SetAt( abs(g_id), new_id );
								}
								if( g_id > 0 )
									c->VtxByIndex(iv).tee_ID = new_id;
								else
									c->VtxByIndex(iv).tee_ID = -new_id;
							}
							// update lower-left corner
							double d = c->VtxByIndex(iv).x + c->VtxByIndex(iv).y;
							if( d < min_d )
							{
								min_d = d;
								min_x = c->VtxByIndex(iv).x;
								min_y = c->VtxByIndex(iv).y;
							}
						}
						c->Draw();
					}
				}
				// add copper areas
				for( int g_ia=0; g_ia<g_net->NumAreas(); g_ia++ )
				{
					carea * ga = &g_net->area[g_ia];
					CPolyLine * gp = ga;
					int ia = nl->AddArea( prj_net, gp->Layer(),
						gp->X(0), gp->Y(0), gp->GetHatch() );
					CPolyLine * p = &prj_net->area[ia];
					id p_id = p->Id();
					p->Copy( gp );
					p->SetParentId( &p_id );
					p->SetPtr( prj_net );
					for( int is=0; is<p->NumSides(); is++ )
					{
						int x = p->X(is);
						int y = p->Y(is);
						p->SetX( is, x + dlg.m_dx );
						p->SetY( is, y + dlg.m_dy );
						p_id.SetU2( p->UID() );
						p_id.SetI2( ia );
						p_id.SetSubSubType( ID_SEL_SIDE, p->SideUID(is), is );
						m_sel_ids.Add( p_id );
						m_sel_ptrs.Add( prj_net );
						// update lower-left corner
						double d = x + y;
						if( d < min_d )
						{
							min_d = d;
							min_x = x;
							min_y = y;
						}
					}
					p->Draw( prj_net->m_dlist );
				}
			}
			g_net = iter_net_g.GetNext();
		}
		// now destroy modified g_nl and restore links in g_pl
		delete g_nl;

		// add sm_cutouts
		int grp_size = g_sm->GetSize();
		int old_size = sm->GetSize();
		if( grp_size > 0 )
		{
			SaveUndoInfoForSMCutouts( FALSE, m_doc->m_undo_list );
			sm->SetSize( old_size + grp_size );
			for( int g_ism=0; g_ism<grp_size; g_ism++ )
			{
				int ism = g_ism + old_size;
				CPolyLine * g_p = &(*g_sm)[g_ism];
				CPolyLine * p = &(*sm)[ism];
				p->Copy( g_p );
				id p_id = p->Id();
				p_id.SetU2( p->UID() );
				p_id.SetI2( ism );
				p->SetParentId( &p_id );
				for( int is=0; is<p->NumSides(); is++ )
				{
					int x = p->X(is);
					int y = p->Y(is);
					p->SetX( is, x + dlg.m_dx );
					p->SetY( is, y + dlg.m_dy );
					p_id.SetU2( p->UID() );
					p_id.SetI2( ism );
					p_id.SetSubSubType( ID_SEL_SIDE, p->SideUID(is), is );
					m_sel_ids.Add( p_id );
					m_sel_ptrs.Add( NULL );
					// update lower-left corner
					double d = x + y;
					if( d < min_d )
					{
						min_d = d;
						min_x = x;
						min_y = y;
					}
				}
				p->Draw( m_dlist );
			}
		}

		// add board outlines
		grp_size = g_bd->GetSize();
		old_size = bd->GetSize();
		if( grp_size > 0 )
		{
			SaveUndoInfoForBoardOutlines( FALSE, m_doc->m_undo_list );
			bd->SetSize( old_size + grp_size );
			//**
			CPolyLine * p1 = &(*bd)[0];
			CPolyLine * p2 = &(*bd)[1];
			for( int g_ibd=0; g_ibd<grp_size; g_ibd++ )
			{
				int ibd = g_ibd + old_size;
				CPolyLine * g_p = &(*g_bd)[g_ibd];	// group poly
				CPolyLine * p = &(*bd)[ibd];		// project poly
				p->Copy( g_p );
				id p_id = p->Id();
				p_id.SetU2( p->UID() );		// root id
				p_id.SetI2( ibd );
				p->SetParentId( &p_id );
				for( int is=0; is<p->NumSides(); is++ )
				{
					int x = p->X(is);
					int y = p->Y(is);
					p->SetX( is, x + dlg.m_dx );
					p->SetY( is, y + dlg.m_dy );
					p_id.SetI2( ibd );
					p_id.SetSubSubType( ID_SEL_SIDE, p->SideUID(is), is );
					int ns = m_sel_ids.GetSize();
					m_sel_ids.Add( p_id );		//**
					id sid = m_sel_ids[ns];	//**
					m_sel_ptrs.Add( NULL );
					// update lower-left corner
					double d = x + y;
					if( d < min_d )
					{
						min_d = d;
						min_x = x;
						min_y = y;
					}
				}
				p->Draw( m_dlist );
			}
		}

		// add text
		CIterator_CText iter_t( g_tl );
		for( CText * t = iter_t.GetFirst(); t != NULL; t = iter_t.GetNext() )		
		{
			CText * new_text = m_doc->m_tlist->AddText( t->m_x+dlg.m_dx, t->m_y+dlg.m_dy, t->m_angle,
				t->m_mirror, t->m_bNegative, t->m_layer, t->m_font_size, t->m_stroke_width,
				&t->m_str, TRUE );
			SaveUndoInfoForText( new_text, CTextList::UNDO_TEXT_ADD, FALSE, m_doc->m_undo_list );
			id t_id( ID_TEXT, -1, ID_TEXT, -1, -1, ID_SEL_TXT );
			m_sel_ids.Add( t_id );
			m_sel_ptrs.Add( new_text );
			CRect text_bounds;
			m_doc->m_tlist->GetTextRectOnPCB( new_text, &text_bounds );
			double d = text_bounds.left + text_bounds.bottom;
			if( d < min_d )
			{
				min_d = d;
				min_x = text_bounds.left;
				min_y = text_bounds.bottom;
			}
		}

		HighlightGroup();
		if( bDragGroup )
		{
			if( min_x == INT_MAX || min_y == INT_MAX ) {
				CString s ((LPCSTR) IDS_NoItemsToDrag);
				AfxMessageBox( s );
			}
			else
				StartDraggingGroup( TRUE, min_x, min_y );
		}
		else
		{
			FindGroupCenter();
			if( m_doc->m_vis[LAY_RAT_LINE] )
			{
				for( net=iter_net.GetFirst(); net; net=iter_net.GetNext() )
				{
					m_doc->m_nlist->OptimizeConnections( net, -1, m_doc->m_auto_ratline_disable,
						m_doc->m_auto_ratline_min_pins, TRUE ); 
				}
			}
			// CPT:
			if (m_sel_ids.GetSize()==1)
				ConvertSingletonGroup();
			// end CPT
		}
		m_doc->ProjectModified( TRUE );
	}
#endif
}

void CFreePcbView::OnGroupSaveToFile()
{
	// Copy group to pseudo-clipboard.  CPT:  took advantage of return value from new DoGroupCopy()...
	if (!DoGroupCopy()) return;

	CString s ((LPCSTR) IDS_PCBFiles);
	CFileDialog dlg( 0, "fpc", NULL, 0,
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );
	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_doc->m_parent_folder;
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		CString pathname = dlg.GetPathName();
		// write project file
		CStdioFile pcb_file;
		int err = pcb_file.Open( pathname, CFile::modeCreate | CFile::modeWrite, NULL );
		if( !err )
		{
			// error opening partlist file
			CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
			mess.Format( s, pathname );
			AfxMessageBox( mess );
		}
		else
		{
			// write clipboard to file
			try
			{
				// make map of all footprints used by group
				CMapStringToPtr clip_cache_map;
				cpart * part = m_doc->clip_plist->GetFirstPart();
				while( part )
				{
					void * vp;
					if( part->shape )
						if( !clip_cache_map.Lookup( part->shape->m_name, vp ) )
							clip_cache_map.SetAt( part->shape->m_name, part->shape );
					part = m_doc->clip_plist->GetNextPart( part );
				}
				m_doc->WriteOptions( &pcb_file );
				m_doc->WriteFootprints( &pcb_file, &clip_cache_map );
				m_doc->WriteBoardOutline( &pcb_file, &m_doc->clip_board_outline );
				m_doc->WriteSolderMaskCutouts( &pcb_file, &m_doc->clip_sm_cutout );
				m_doc->clip_plist->WriteParts( &pcb_file );
				m_doc->clip_nlist->WriteNets( &pcb_file );
				m_doc->clip_tlist->WriteTexts( &pcb_file );
				pcb_file.WriteString( "[end]\n" );
				pcb_file.Close();
			}
			catch( CString * err_str )
			{
				// error
				AfxMessageBox( *err_str );
				delete err_str;
				CDC * pDC = GetDC();
				OnDraw( pDC );
				ReleaseDC( pDC );
				return;
			}
		}
	}
}

void CFreePcbView::OnEditCopy()
{
	// CPT2 rewrote, but TODO I must rewrite OnGroupCopy (and preferably rename it)
	if( !m_doc->m_project_open )
		return;
	if (m_sel.GetSize()==0) {
		CString str ((LPCSTR) IDS_UnableToCopyAnything);
		AfxMessageBox(str);
		}
	else
		OnGroupCopy();										
}

void CFreePcbView::OnEditPaste()
{
	if( !m_doc->m_project_open )
		return;
	OnGroupPaste();
}


void CFreePcbView::OnEditCut()
{
	// CPT2 rewrote
	if( !m_doc->m_project_open )
		return;
	if (m_sel.GetSize()==0) {
		CString str ((LPCSTR) IDS_UnableToCutAnything);
		AfxMessageBox(str);
		}
	else if (DoGroupCopy())
		OnGroupDelete();
}

void CFreePcbView::RotateGroup()
{
#ifndef CPT2
	UngluePartsInGroup();

	// mark all parts and nets as unselected
	m_doc->m_nlist->MarkAllNets(0);
	m_doc->m_plist->MarkAllParts(0);

	// mark all corners of solder mask cutouts as unmoved
	for( int im=0; im<m_doc->m_sm_cutout.GetSize(); im++ )
	{
		CPolyLine * poly = &m_doc->m_sm_cutout[im];
		poly->SetUtility(0);
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}

	// mark all corners of board outlines as unmoved
	for( int im=0; im<m_doc->m_board_outline.GetSize(); im++ )
	{
		CPolyLine * poly = &m_doc->m_board_outline[im];
		poly->SetUtility(0);
		for( int ic=0; ic<poly->NumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}

	// mark selected nets, mark and undraw areas, mask cutouts and board outlines
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			net->utility = TRUE;	// mark as selected
			if( sid.T2() == ID_AREA )
			{
				carea * a = &net->area[sid.I2()];
				a->utility = TRUE;
				CPolyLine * poly = a;
				poly->SetUtility(1);
				poly->Undraw();
			}
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
			poly->SetUtility(1);
			poly->Undraw();
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_OUTLINE && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
			poly->SetUtility(1);
			poly->Undraw();
		}
	}

	int tempx;
	// mark all relevant parts, nets, connections and segments as selected
	// and move text and copper area corners
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_SEG )
		{
			// segment
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			int is = sid.I3();
			cconnect * c = net->ConByIndex(ic);	// this connection
			cseg * s = &c->SegByIndex(is);				// this segment
			cvertex * pre_v = &s->GetPreVtx();
			cvertex * post_v = &s->GetPostVtx();
			c->utility = TRUE;					// mark connection selected
			s->utility = TRUE;					// mark segment selected
			pre_v->utility =  TRUE;				// mark adjacent vertices as selected
			post_v->utility =  TRUE;
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_VERTEX )
		{
			// vertex
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			int iv = sid.I3();
			cconnect * c = net->ConByIndex(ic);	// this connection
			cvertex * v = &c->VtxByIndex(iv);
			c->utility = TRUE;					// mark connection selected
			v->utility = TRUE;					// mark vertex selected
		}
		else if( sid.T1() == ID_PART && sid.T2() == ID_SEL_RECT )
		{
			// part
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->utility = TRUE;	// mark part selected
		}
		else if( sid.T1() == ID_TEXT && sid.T2() == ID_TEXT && sid.T3() == ID_SEL_TXT )
		{
			// text
			CText * t = (CText*)m_sel_ptrs[i];
			m_doc->m_tlist->MoveText( t, groupAverageX + t->m_y - groupAverageY,
				groupAverageY - t->m_x + groupAverageX, (t->m_angle+90)%360,
				t->m_mirror, t->m_bNegative, t->m_layer );
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_AREA && sid.T3() == ID_SEL_SIDE )
		{
			// area side
			cnet * net = (cnet*)m_sel_ptrs[i];
			CPolyLine * poly = &net->area[sid.I2()];
			int icontour = poly->Contour(sid.I3());
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				tempx=poly->X(ic1);
				poly->SetX( ic1, groupAverageX + poly->Y(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY -tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				tempx=poly->X(ic2);
				poly->SetX( ic2, groupAverageX + poly->Y(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
		}
		// sm_cutout side
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
			int icontour = poly->Contour(0);
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				tempx=poly->X(ic1);
				poly->SetX( ic1, groupAverageX + poly->Y(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				tempx=poly->X(ic2);
				poly->SetX( ic2, groupAverageX + poly->Y(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
		}
		// board outline side
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_BOARD && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
			int icontour = poly->Contour(0);
			int istart = poly->ContourStart(icontour);
			int iend = poly->ContourEnd(icontour);
			int ic1 = sid.I3();
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			if( !poly->Utility(ic1) )
			{
				// unmoved, move it
				tempx=poly->X(ic1);
				tempx=poly->X(ic1);
				poly->SetX( ic1, groupAverageX + poly->Y(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->Utility(ic2) )
			{
				// unmoved, move it
				tempx=poly->X(ic2);
				poly->SetX( ic2, groupAverageX + poly->Y(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY - tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
		}
		else
			ASSERT(0);
	}

	// now redraw areas, solder mask cutouts and board outlines
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			net->utility = TRUE;
			if( sid.T2() == ID_AREA )
			{
				cnet * net = (cnet*)m_sel_ptrs[i];
				carea * a = &net->area[sid.I2()];
				CPolyLine * poly = a;
				if( poly->Utility() )
				{
					poly->Draw();
					poly->SetUtility(0);	// clear flag so only redraw once
				}
			}
		}
		else if( sid.T1() == ID_MASK && sid.T2() == ID_MASK && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[sid.I2()];
			if( poly->Utility() )
			{
				poly->Draw();
				poly->SetUtility(0);	// clear flag so only redraw once
			}
		}
		else if( sid.T1() == ID_BOARD && sid.T2() == ID_OUTLINE && sid.T3() == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_doc->m_board_outline[sid.I2()];
			if( poly->Utility() )
			{
				poly->Draw();
				poly->SetUtility(0);	// clear flag so only redraw once
			}
		}
	}

	// move parts in group
	cpart * part = m_doc->m_plist->GetFirstPart();
	while( part != NULL )
	{
		if( part->utility )
		{
			// move part
			m_doc->m_plist->Move( part, groupAverageX + part->y - groupAverageY,
				groupAverageY - part->x + groupAverageX, (part->angle+90)%360, part->side );
			// find segments which connect to this part and move them
			cnet * net;
			for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
			{
				net = (cnet*)part->pin[ip].net;
				if( net )
				{
					for( int ic=0; ic<net->NumCons(); ic++ )
					{
						cconnect * c = net->ConByIndex(ic);
						int nsegs = c->NumSegs();
						if( nsegs )
						{
							int p1 = c->start_pin;
							CString pin_name1 = net->pin[p1].pin_name;
							int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
							int p2 = c->end_pin;
							if( net->pin[p1].part == part )
							{
								// starting pin is on part
								if( !c->SegByIndex(0).utility && c->SegByIndex(0).m_layer != LAY_RAT_LINE )
								{
									// first segment is not selected, unroute it
									if( !c->SegByIndex(0).utility )
										m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, 0 );
								}
								// move vertex if not selected
								if( !c->VtxByIndex(0).utility )
								{
									m_doc->m_nlist->MoveVertex( net, ic, 0,
										part->pin[pin_index1].x, part->pin[pin_index1].y );
									c->VtxByIndex(0).utility2 = 1; // moved
								}
							}
							if( p2 != cconnect::NO_END )
							{
								if( net->pin[p2].part == part )
								{
									// ending pin is on part
									if( c->SegByIndex(nsegs-1).m_layer != LAY_RAT_LINE )
									{
										// unroute it if not selected
										if( !c->SegByIndex(nsegs-1).utility )
											m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, nsegs-1 );
									}
									// modify vertex position if necessary
									if( !c->VtxByIndex(nsegs).utility )
									{
										// move vertex if unselected
										CString pin_name2 = net->pin[p2].pin_name;
										int pin_index2 = part->shape->GetPinIndexByName( pin_name2 );
										m_doc->m_nlist->MoveVertex( net, ic, nsegs,
											part->pin[pin_index2].x, part->pin[pin_index2].y );
										c->VtxByIndex(nsegs).utility2 =  1;	// moved

									}
								}
							}
						}
					}
				}
			}
		}
		part = m_doc->m_plist->GetNextPart( part );
	}

	// get selected segments
	CIterator_cnet iter_net(m_doc->m_nlist);
	cnet * net = iter_net.GetFirst();
	while( net != NULL )
	{
		if( net->utility )
		{
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility )
				{
					// undraw entire trace
					c->Undraw();
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							// move trace segment by flagging adjacent vertices
							cseg * s = &c->SegByIndex(is);				// this segment
							cvertex * pre_v = &s->GetPreVtx();		// pre vertex
							cvertex * post_v = &s->GetPostVtx();	// post vertex
							CPoint old_pre_v_pt( pre_v->x, pre_v->y );		// pre vertex coords
							CPoint old_post_v_pt( post_v->x, post_v->y );	// post vertex coords
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// flag adjacent vertices as selected
							pre_v->utility = TRUE;
							post_v->utility = TRUE;

							// unroute adjacent segments unless they are also being moved
							if( is>0 )
							{
								// test for preceding segment
								if( !c->SegByIndex(is-1).utility )
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is-1 );
							}
							if( is < c->NumSegs()-1 )
							{
								// test for following segment and not end of stub trace
								if( !c->SegByIndex(is+1).utility && (part2 || is < c->NumSegs()-2) )
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is+1 );
							}
						}
					}
					// now move vertices
					for( int iv=0; iv<=c->NumSegs(); iv++ )
					{
						cvertex * v = &c->VtxByIndex(iv);
						if( v->utility && !v->utility2 )
						{
							// selected and not already moved
							tempx=v->x;
							v->x =groupAverageX + v->y -groupAverageY;
							v->y =groupAverageY -tempx + groupAverageX;
							v->utility2 = TRUE;	// moved
							// if adjacent segments were not selected, unroute them
							if( iv>0 )
							{
								cseg * pre_s = &c->SegByIndex(iv-1);
								if( pre_s->utility == 0 )
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv-1 );
							}
							if( iv<c->NumSegs() )
							{
								cseg * post_s = &c->SegByIndex(iv);
								if( post_s->utility == 0 )
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv );
							}
						}
					}

					// now some special cases
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							// move trace segment
							cseg * s = &c->SegByIndex(is);				// this segment
							cvertex * pre_v = &s->GetPreVtx();		// pre vertex
							cvertex * post_v = &s->GetPostVtx();	// post vertex
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// special case, first segment of trace selected but part not selected
							if( part1->utility == FALSE && is == 0 )
							{
								// insert ratline as new first segment, unselected
								CPoint new_v_pt( pre_v->x, pre_v->y );
								CPoint old_v_pt = m_doc->m_plist->GetPinPoint( part1, net->pin[c->start_pin].pin_name );		// pre vertex coords
								m_doc->m_nlist->MoveVertex( net, ic, 0, old_v_pt.x, old_v_pt.y );
								m_doc->m_nlist->InsertSegment( net, ic, 0, new_v_pt.x, new_v_pt.y, LAY_RAT_LINE, 0, 0 );
								c->SegByIndex(0).utility = 0;
								c->VtxByIndex(0).utility = 0;
								is++;
							}

							// special case, last segment of trace selected but part not selected
							if( part2 )
							{
								if( part2->utility == FALSE && is == c->NumSegs()-1 )
								{
									// insert ratline as new last segment
									int old_w = c->SegByIndex(c->NumSegs()-1).m_width;
									int old_layer = c->SegByIndex(c->NumSegs()-1).m_layer;
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
									CPoint new_v_pt( c->VtxByIndex(c->NumSegs()).x, c->VtxByIndex(c->NumSegs()).y );
									CPoint old_v_pt = m_doc->m_plist->GetPinPoint( part2, net->pin[c->end_pin].pin_name );
									m_doc->m_nlist->MoveVertex( net, ic, c->NumSegs(), old_v_pt.x, old_v_pt.y );
									BOOL bInserted = m_doc->m_nlist->InsertSegment( net, ic, c->NumSegs()-1,
										new_v_pt.x, new_v_pt.y, old_layer, old_w, 0 );
									c->SegByIndex(c->NumSegs()-2).utility = 1;
									c->SegByIndex(c->NumSegs()-1).utility = 0;
								}
							}
						}
					}
					m_doc->m_nlist->MergeUnroutedSegments( net, ic );	// this also redraws connection
				}
			}

			// now deal with tees that have been moved
			// requiring that stubs attached to tees have to move as well
			// if attached segments have not been selected, they must be unrouted
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->end_pin == cconnect::NO_END )
				{
					cvertex * end_vtx = &c->VtxByIndex(c->NumSegs());
					cseg * end_seg = &c->SegByIndex(c->NumSegs()-1);
					if( int id = end_vtx->tee_ID )
					{
						// stub tee
						int tee_ic;
						int tee_iv;
						BOOL bFound = m_doc->m_nlist->FindTeeVertexInNet( net, id, &tee_ic, &tee_iv );
						if ( !bFound )
						{
							end_vtx->tee_ID = 0;
						}
						else
						{
							cvertex * tee_vtx;
							tee_vtx = &net->ConByIndex(tee_ic)->VtxByIndex(tee_iv);
							if( tee_vtx->utility2 )
							{
								// tee-vertex was moved
								end_vtx->x = tee_vtx->x;
								end_vtx->y = tee_vtx->y;
								if( !end_seg->utility )
								{
									// attached segment not selected
									c->Undraw();
									m_doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->NumSegs()-1 );
									c->Draw();
								}
							}
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}

	// merge unrouted segments for all traces
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT
			&& sid.T3() == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.I2();
			m_doc->m_nlist->MergeUnroutedSegments( net, ic );
		}
	}

	//** this shouldn't be necessary
	CNetList * nl = m_doc->m_nlist;
	for( cnet * n=iter_net.GetFirst(); n; n=iter_net.GetNext() )
		nl->RehookPartsToNet( n );
	//**
	m_doc->m_nlist->SetAreaConnections();

	// regenerate selection list from utility flags
	// first, remove all segments and vertices
	for( int i=m_sel_ids.GetSize()-1; i>=0; i-- )
	{
		id sid = m_sel_ids[i];
		if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_SEG )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
		else if( sid.T1() == ID_NET && sid.T2() == ID_CONNECT && sid.T3() == ID_SEL_VERTEX )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
	}
	// add segments and vertices back into group
	net = iter_net.GetFirst();
	while( net )
	{
		if( net->utility )
		{
			// selected net
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				if( c->utility )
				{
					// selected connection
					for( int is=0; is<c->NumSegs(); is++ )
					{
						if( c->SegByIndex(is).utility )
						{
							cseg * s = &c->SegByIndex(is);
							m_sel_ptrs.Add( net );
							id sid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
								ID_SEL_SEG, s->UID(), is );
							m_sel_ids.Add( sid );
							c->SegByIndex(is).dl_el->visible = 1;	// restore visibility
						}
					}
					for( int iv=0; iv<c->NumSegs()+1; iv++ )
					{
						cvertex * v = &c->VtxByIndex(iv);
						if( v->utility )
						{
							if( v->via_w || v->tee_ID )
							{
								m_sel_ptrs.Add( net );
								id vid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
									ID_SEL_VERTEX, v->UID(), iv );
								m_sel_ids.Add( vid );
								if( v->via_w )
								{
									int n_el = v->dl_el.GetSize();
									for( int il=0; il<n_el; il++ )
										v->dl_el[il]->visible = 1;
								}
							}
						}
					}
				}
			}
		}
		net = iter_net.GetNext();
	}
#endif
}

void CFreePcbView::FindGroupCenter()
{
#ifndef CPT2
	int groupNumberItems = 0;
	groupAverageX = groupAverageY = 0;

	// find parts
	if( m_sel_mask & (1<<SEL_MASK_PARTS ) )  // may not be necessary??
	{
		cpart * part = m_doc->m_plist->GetFirstPart();
		while( part )
		{
			id pid( ID_PART, -1, ID_SEL_RECT );
			if( FindItemInGroup( part, &pid ) != -1 )
			{
				groupAverageX+=part->x;
				groupAverageY+=part->y;
				groupNumberItems++;
			}
			part = m_doc->m_plist->GetNextPart( part );
		}
	}

	// find trace segments and vertices contained in rect
	CIterator_cnet iter_net(m_doc->m_nlist);
	if( m_sel_mask & (1<<SEL_MASK_CON ) )
	{
		cnet * net = iter_net.GetFirst();
		while( net )
		{
			for( int ic=0; ic<net->NumCons(); ic++ )
			{
				cconnect * c = net->ConByIndex(ic);
				for( int is=0; is<c->NumSegs(); is++ )
				{
					cseg * s = &c->SegByIndex(is);
					cvertex * pre_v = &s->GetPreVtx();
					cvertex * post_v = &s->GetPostVtx();

					if( m_doc->m_vis[s->m_layer] )
					{
						// add segment to selection list and highlight it
						id sid( ID_NET, net->UID(), ID_CONNECT, c->UID(), ic, 
							ID_SEL_SEG, s->UID(), is );
						if( FindItemInGroup( net, &sid ) != -1 )
						{
							groupAverageX+=pre_v->x+post_v->x;
							groupAverageY+=pre_v->y+post_v->y;
							groupNumberItems+=2;
						}
					}
				}
			}
			net = iter_net.GetNext();
		}
	}

	// find texts in group
	if( m_sel_mask & (1<<SEL_MASK_TEXT ) )
	{
		CIterator_CText iter_t( m_doc->m_tlist );
		for( CText * t = iter_t.GetFirst(); t != NULL; t = iter_t.GetNext() )		
		{
			if( m_doc->m_vis[t->m_layer] )
			{
				// add text to selection list and highlight it
				id sid( ID_TEXT, -1, ID_TEXT, -1, -1, ID_SEL_TXT );
				if( FindItemInGroup( t, &sid ) != -1 )
				{
					groupAverageX+=m_dlist->Get_x( t->dl_sel );
					groupAverageY+=m_dlist->Get_y( t->dl_sel );
					groupNumberItems++;
				}
			}
		}
	}

	// find copper area sides in rect
	if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
	{
		cnet * net = iter_net.GetFirst();
		while( net )
		{
			if( net->NumAreas() )
			{
				for( int ia=0; ia<net->NumAreas(); ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * poly = a;
					for( int ic=0; ic<poly->NumContours(); ic++ )
					{
						int istart = poly->ContourStart(ic);
						int iend = poly->ContourEnd(ic);
						for( int is=istart; is<=iend; is++ )
						{
							int ic1, ic2;
							ic1 = is;
							if( is < iend )
								ic2 = is+1;
							else
								ic2 = istart;
							int x1 = poly->X(ic1);
							int y1 = poly->Y(ic1);
							int x2 = poly->X(ic2);
							int y2 = poly->Y(ic2);
							if( m_doc->m_vis[poly->Layer()] )
							{
								id aid( ID_NET, net->UID(), ID_AREA, a->UID(), ia, 
									ID_SEL_SIDE, -1, is );
								if( FindItemInGroup( net, &aid ) != -1 )
								{
									groupAverageX+=x1+x2;
									groupAverageY+=y1+y2;
									groupNumberItems+=2;
								}
							}
						}
					}
				}
			}
			net = iter_net.GetNext();
		}
	}

	// find solder mask cutout sides in rect
	if( m_sel_mask & (1<<SEL_MASK_SM ) )
	{
		for( int im=0; im<m_doc->m_sm_cutout.GetSize(); im++ )
		{
			CPolyLine * poly = &m_doc->m_sm_cutout[im];
			for( int ic=0; ic<poly->NumContours(); ic++ )
			{
				int istart = poly->ContourStart(ic);
				int iend = poly->ContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->X(ic1);
					int y1 = poly->Y(ic1);
					int x2 = poly->X(ic2);
					int y2 = poly->Y(ic2);
					if( m_doc->m_vis[poly->Layer()] )
					{
						id smid( ID_MASK, -1, ID_MASK, -1, im, 
							ID_SEL_SIDE, -1, is );
						if( FindItemInGroup( poly, &smid ) != -1 )
						{
							groupAverageX+=x1+x2;
							groupAverageY+=y1+y2;
							groupNumberItems+=2;
						}
					}
				}
			}
		}
	}

	// find board outline sides in rect
	if( m_sel_mask & (1<<SEL_MASK_BOARD ) )
	{
		for( int im=0; im<m_doc->m_board_outline.GetSize(); im++ )
		{
			CPolyLine * poly = &m_doc->m_board_outline[im];
			for( int ic=0; ic<poly->NumContours(); ic++ )
			{
				int istart = poly->ContourStart(ic);
				int iend = poly->ContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->X(ic1);
					int y1 = poly->Y(ic1);
					int x2 = poly->X(ic2);
					int y2 = poly->Y(ic2);
					if( m_doc->m_vis[poly->Layer()] )
					{
						id bd_id( ID_BOARD, ID_BOARD, im, ID_SEL_SIDE, is );
						if( FindItemInGroup( poly, &bd_id ) != -1 )
						{
							groupAverageX+=x1+x2;
							groupAverageY+=y1+y2;
							groupNumberItems+=2;
						}
					}
				}
			}
		}
	}

	if(groupNumberItems>0)
	{
		groupAverageX/=groupNumberItems;
		groupAverageY/=groupNumberItems;

		double x=floor(groupAverageX/m_doc->m_part_grid_spacing +.5);
		groupAverageX=(long long)(m_doc->m_part_grid_spacing*x);
		x=floor(groupAverageY/m_doc->m_part_grid_spacing +.5);
		groupAverageY=(long long)(m_doc->m_part_grid_spacing*x);
	}
#endif
}


// save undo info for part, prior to editing operation
// type may be:
//	UNDO_PART_DELETE	if part will be deleted
//	UNDO_PART_MODIFY	if part will be modified (e.g. moved)
//	UNDO_PART_ADD		if part will be added
// for UNDO_PART_ADD, use reference designator to identify part, ignore cpart * part
// on callback, ref_des will be used to find part, then name will be changed to part->ref_des
//
void CFreePcbView::SaveUndoInfoForPart( cpart2 * part, int type, CString * ref_des, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	undo_part * u_part;
	if( new_event )
		list->NewEvent();
	if( type == CPartList::UNDO_PART_ADD )
		u_part = m_doc->m_plist->CreatePartUndoRecord( NULL, ref_des );
	else if( ref_des )
		u_part = m_doc->m_plist->CreatePartUndoRecord( part, ref_des );
	else
		u_part = m_doc->m_plist->CreatePartUndoRecord( part, &part->ref_des );

	list->Push( type, u_part, &m_doc->m_plist->PartUndoCallback, u_part->size );

	void * ptr;
	if( new_event )
	{
		if( type == CPartList::UNDO_PART_ADD )
			ptr = CreateUndoDescriptor( list, type, ref_des, NULL, 0, 0, ref_des, NULL );
		else if( ref_des )
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, ref_des, NULL );
		else
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, &part->ref_des, NULL );
		list->Push( UNDO_PART, ptr, &UndoCallback );
	}
#endif
}

// save undo info for a part and all nets connected to it
// type may be:
//	UNDO_PART_DELETE	if part will be deleted
//	UNDO_PART_MODIFY	if part will be modified (e.g. moved or ref_des changed)
// note that the ref_des may be different than the part->ref_des
// on callback, ref_des will be used to find part, then name will be changed to part->ref_des
//
void CFreePcbView::SaveUndoInfoForPartAndNets( cpart2 * part, int type, CString * ref_des, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	void * ptr;
	if( new_event )
		list->NewEvent();
	// set utility = 0 for all nets affected
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
			net->utility = 0;
	}
	// save undo info for all nets affected
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			if( net->utility == 0 )
			{
				SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE, list );
				net->utility = 1;
			}
		}
	}
	// save undo info for part
	SaveUndoInfoForPart( part, type, ref_des, FALSE, list );

	// save top-level descriptor
	if( new_event )
	{
		if( ref_des )
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, ref_des, NULL );
		else
			ptr = CreateUndoDescriptor( list, type, &part->ref_des, NULL, 0, 0, &part->ref_des, NULL );
		list->Push( UNDO_PART_AND_NETS, ptr, &UndoCallback );
	}
#endif
}

// save undo info for a net (not connections or areas)
//
void CFreePcbView::SaveUndoInfoForNet( cnet2 * net, int type, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	void * ptr;
	if( new_event )
		list->NewEvent();
	undo_net * u_net = m_doc->m_nlist->CreateNetUndoRecord( net );
	list->Push( type, u_net, &m_doc->m_nlist->NetUndoCallback, u_net->size );
#endif
}

// save undo info for a net and connections, not areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnections( cnet2 * net, int type, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	void * ptr;
	if( new_event )
		list->NewEvent();
	if( type != CNetList::UNDO_NET_ADD )
		for( int ic=net->NumCons()-1; ic>=0; ic-- )
		{
			cconnect * c = net->ConByIndex(ic);
			SaveUndoInfoForConnection( net, ic, FALSE, list );
		}
	SaveUndoInfoForNet( net, type, FALSE, list );
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, type, &net->name, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_NET_AND_CONNECTIONS, ptr, &UndoCallback );
	}
#endif
}

// save undo info for a connection
// Note: this is now ONLY called from other Undo functions, it should never be used on its own
//
void CFreePcbView::SaveUndoInfoForConnection( cconnect2 * con, BOOL new_event, CUndoList * list )
{
#ifndef CPT2
	if( new_event )
		list->NewEvent();
	undo_con * u_con = m_doc->m_nlist->CreateConnectUndoRecord( net, ic );
	list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
		&m_doc->m_nlist->ConnectUndoCallback, u_con->size );
#endif
}

// top-level description of undo operation
// list is the CUndoList that it will be pushed to
//

void * CFreePcbView::CreateUndoDescriptor( CUndoList * list, int type, CString * name1, CString * name2,
										  int int1, int int2, CString * str1, void * ptr )
{
	undo_descriptor * u_d = new undo_descriptor;
	u_d->view = this;
	u_d->list = list;
	u_d->type = type;
	if( name1 )
		u_d->name1 = *name1;
	if( name2 )
		u_d->name2 = *name2;
	u_d->int1 = int1;
	u_d->int2 = int2;
	if( str1 )
		u_d->str1 = *str1;
	u_d->ptr = ptr;
	return (void*)u_d;
}

// initial callback from undo/redo stack
// used to push redo/undo info onto the other stack
// note this is a static function (i.e. global)
//
void CFreePcbView::UndoCallback( int type, void * ptr, BOOL undo )
{
#ifndef CPT2

	undo_descriptor * u_d = (undo_descriptor*)ptr;
	if( undo )
	{
		CFreePcbView * view = u_d->view;
		// if callback was from undo_list, push info to redo list, and vice versa
		CUndoList * redo_list;
		if( u_d->list == view->m_doc->m_undo_list )
			redo_list = view->m_doc->m_redo_list;
		else
			redo_list = view->m_doc->m_undo_list;
		undo_text * u_text = (undo_text *)u_d->ptr;
		// save undo/redo info
		if( type == UNDO_PART )
		{
			cpart * part = view->m_doc->m_plist->GetPartByName( u_d->str1 );	//use new ref des
			if( u_d->type == CPartList::UNDO_PART_ADD )
			{
				view->SaveUndoInfoForPartAndNets( part, CPartList::UNDO_PART_DELETE, &u_d->str1, TRUE, redo_list );
			}
			else if( u_d->type == CPartList::UNDO_PART_MODIFY )
			{
				view->SaveUndoInfoForPart( part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, redo_list );
			}
		}
		else if( type == UNDO_PART_AND_NETS )
		{
			cpart * part = view->m_doc->m_plist->GetPartByName( u_d->str1 );
			if(u_d->type == CPartList::UNDO_PART_DELETE )
				view->SaveUndoInfoForPart( NULL, CPartList::UNDO_PART_ADD, &u_d->name1, TRUE, redo_list );
			else if( u_d->type == CPartList::UNDO_PART_MODIFY )
				view->SaveUndoInfoForPartAndNets( part, CPartList::UNDO_PART_MODIFY, &u_d->name1, TRUE, redo_list );
		}
		else if( type == UNDO_2_PARTS_AND_NETS )
		{
			cpart * part = view->m_doc->m_plist->GetPartByName( u_d->name1 );
			cpart * part2 = view->m_doc->m_plist->GetPartByName( u_d->name2 );
			view->SaveUndoInfoFor2PartsAndNets( part, part2, TRUE, redo_list );
		}
		else if( type == UNDO_NET_AND_CONNECTIONS )
		{
			cnet * net = view->m_doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			view->SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, redo_list );
		}
		else if( type == UNDO_AREA )
		{
			cnet * net = view->m_doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			if( u_d->type == CNetList::UNDO_AREA_ADD )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_DELETE, TRUE, redo_list );
			else if( u_d->type == CNetList::UNDO_AREA_DELETE )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_ADD, TRUE, redo_list );
			else if( type == UNDO_AREA )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_MODIFY, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_AREAS_IN_NET )
		{
			cnet * net = view->m_doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			view->SaveUndoInfoForAllAreasInNet( net, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_AREAS_IN_2_NETS )
		{
			cnet * net1 = view->m_doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			cnet * net2 = view->m_doc->m_nlist->GetNetPtrByName( &u_d->name2 );
			view->SaveUndoInfoForAllAreasIn2Nets( net1, net2, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_BOARD_OUTLINES )
		{
			view->SaveUndoInfoForBoardOutlines( TRUE, redo_list );
		}
		else if( type == UNDO_ALL_SM_CUTOUTS )
		{
			view->SaveUndoInfoForSMCutouts( TRUE, redo_list );
		}
		else if( type == UNDO_TEXT )
		{
			if( u_d->type == CTextList::UNDO_TEXT_ADD )
				view->SaveUndoInfoForText( u_text, CTextList::UNDO_TEXT_DELETE, TRUE, redo_list );
			else if( u_d->type == CTextList::UNDO_TEXT_MODIFY )
			{
				int uid = u_text->m_uid;
				CText * text = view->m_doc->m_tlist->GetText( uid );
				if( !text )
					ASSERT(0);	// uid not found
				view->SaveUndoInfoForText( text, CTextList::UNDO_TEXT_MODIFY, TRUE, redo_list );
			}
			else if( u_d->type == CTextList::UNDO_TEXT_DELETE )
				view->SaveUndoInfoForText( u_text, CTextList::UNDO_TEXT_ADD, TRUE, redo_list );
		}
		else if( type == UNDO_MOVE_ORIGIN )
		{
			view->SaveUndoInfoForMoveOrigin( -u_d->int1, -u_d->int2, redo_list );
		}
		else
			ASSERT(0);
	}
	delete(u_d);	// delete the undo record
#endif
}

// callback for undoing group operations
// note this is a static function (i.e. global)
//
void CFreePcbView::UndoGroupCallback( int type, void * ptr, BOOL undo )
{
#ifndef CPT2
	undo_group_descriptor * u_d = (undo_group_descriptor*)ptr;
	if( undo )
	{
		CFreePcbView * view = u_d->view;
		CFreePcbDoc * doc = view->m_doc;
		// if callback was from undo_list, push info to redo list, and vice versa
		CUndoList * redo_list;
		if( u_d->list == view->m_doc->m_undo_list )
			redo_list = view->m_doc->m_redo_list;
		else
			redo_list = view->m_doc->m_undo_list;
		if( u_d->type == UNDO_GROUP_MODIFY || u_d->type == UNDO_GROUP_ADD )
		{
			// reconstruct pointers from names of items (since they may have changed)
			// and save the current status of the group
			int n_items = u_d->m_ids.GetSize();
			CArray<void*> ptrs;
			ptrs.SetSize( n_items );
			for( int i=0; i<n_items; i++ )
			{
				CString * str_ptr = &u_d->str[i];
				id this_id = u_d->m_ids[i];
				if( this_id.T1() == ID_PART )
				{
					cpart * part = doc->m_plist->GetPartByName( *str_ptr );
					if( part )
						ptrs[i] = (void*)part;
					else
						ASSERT(0);	// couldn't find part
				}
				else if( this_id.T1() == ID_NET )
				{
					cnet * net = doc->m_nlist->GetNetPtrByName( str_ptr );
					if( net )
						ptrs[i] = (void*)net;
					else
						ASSERT(0);	// couldn't find net
				}
				else if( this_id.T1() == ID_TEXT )
				{
					CText * text = doc->m_tlist->GetText( this_id.U1()  );
					if( text )
						ptrs[i] = (void*)text;
					else
						ASSERT(0);	// couldn't find text
				}
			}
			if( u_d->type == UNDO_GROUP_MODIFY )
				view->SaveUndoInfoForGroup( u_d->type, &ptrs, &u_d->m_ids, redo_list );
			else if( u_d->type == UNDO_GROUP_ADD )
			{
				// delete group
				view->DeleteGroup( &ptrs, &u_d->m_ids );
			}
		}
		else if( u_d->type == UNDO_GROUP_DELETE )
		{
			// just copy the undo record with type UNDO_GROUP_ADD
			undo_group_descriptor * new_u_d = new undo_group_descriptor;
			new_u_d->list = redo_list;
			new_u_d->type = UNDO_GROUP_ADD;
			new_u_d->view = u_d->view;
			int n_items = u_d->m_ids.GetSize();
			new_u_d->str.SetSize( n_items );
			new_u_d->m_ids.SetSize( n_items );
			for( int i=0; i<n_items; i++ )
			{
				new_u_d->m_ids[i] = u_d->m_ids[i];
				new_u_d->str[i] = u_d->str[i];
			}
			redo_list->NewEvent();
			redo_list->Push( UNDO_GROUP, (void*)new_u_d, &view->UndoGroupCallback );
		}
	}
	delete(u_d);	// delete the undo record
#endif
}

// create descriptor used for undo/redo of groups
// mainly a list of the items in the group
// since pointers cannot be used for undo/redo since they may change,
// net names, reference designators and guids are saved as strings
//
void * CFreePcbView::CreateGroupDescriptor( CUndoList * list, CArray<void*> * ptrs, CArray<id> * ids, int type )
{
	undo_group_descriptor * undo = new undo_group_descriptor;
	int n_items = ids->GetSize();
	undo->view = this;
	undo->list = list;
	undo->type = type;
	undo->str.SetSize( n_items );
	undo->m_ids.SetSize( n_items );
	for( int i=0; i<n_items; i++ )
	{
		id this_id = (*ids)[i];
		undo->m_ids[i] = this_id;
		if( this_id.T1() == ID_PART )
		{
			cpart * part = (cpart*)(*ptrs)[i];
			undo->str[i] = part->ref_des;
		}
		else if( this_id.T1() == ID_NET )
		{
			cnet * net = (cnet*)(*ptrs)[i];
			undo->str[i] = net->name;
		}
	}
	return undo;
}


void CFreePcbView::OnGroupRotate() 
{
	CancelHighlight();
	if( !m_lastKeyWasArrow && !m_lastKeyWasGroupRotate)
	{
		if( GluedPartsInGroup() )
		{
			CString s ((LPCSTR) IDS_ThisGroupContainsGluedPartsUnglueAndRotate);
			int ret = AfxMessageBox( s, MB_YESNO );
			if( ret != IDYES )
				return;
		}
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel, m_doc->m_undo_list );
		m_lastKeyWasGroupRotate=true;
	}
	RotateGroup( );
	HighlightSelection();
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// enable/disable the main menu
// used when dragging
//
void CFreePcbView::SetMainMenu( BOOL bAll )
{
	CFrameWnd * pMainWnd = (CFrameWnd*)AfxGetMainWnd();
	if( bAll )
	{
		pMainWnd->SetMenu(&theApp.m_main);
		if( m_doc->m_project_modified )
			m_doc->ProjectModified( TRUE, FALSE );
	}
	else
		pMainWnd->SetMenu(&theApp.m_main_drag);
	return;
}

void CFreePcbView::OnRefShowPart()
{
	cpart * part = m_sel_part;
	CancelSelection();
	dl_element * dl_sel = part->dl_sel;
	int xc = (m_dlist->Get_x( dl_sel ) + m_dlist->Get_xf( dl_sel ))/2;
	int yc = (m_dlist->Get_y( dl_sel ) + m_dlist->Get_yf( dl_sel ))/2;
	m_org_x = xc - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
	m_org_y = yc - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
		m_org_x, m_org_y );
	CPoint p(xc, yc);
	p = m_dlist->PCBToScreen( p );
	SetCursorPos( p.x, p.y - 4 );
	SelectPart( part );
}

void CFreePcbView::OnAreaSideStyle()
{
#ifndef CPT2
	CDlgSideStyle dlg;
	int style = m_sel_net->area[m_sel_ia].SideStyle( m_sel_id.I3() );
	dlg.Initialize( style );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_doc->m_undo_list );
		CancelHighlight();
		m_sel_net->area[m_sel_ia].SetSideStyle( m_sel_id.I3(), dlg.m_style );
		m_doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_id.I3() );
		m_doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
														m_doc->m_auto_ratline_min_pins, TRUE );
	}
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

// move text string for value
//
void CFreePcbView::OnValueMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint cur_p = m_dlist->PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	m_doc->m_plist->StartDraggingValue( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_VALUE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnValueProperties()
{
#ifndef CPT2
	CDlgValueText dlg;
	dlg.Initialize( m_doc->m_plist, m_sel_part );
	int ret =  dlg.DoModal();
	if( ret == IDOK )
	{
		// edit this part
		SaveUndoInfoForPart( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list ); 
		int value_layer = dlg.m_layer;
		value_layer = FlipLayer( m_sel_part->side, value_layer );
		m_doc->m_plist->SetValue( m_sel_part, &m_sel_part->value, 
			m_sel_part->m_value_xi, m_sel_part->m_value_yi, 
			m_sel_part->m_value_angle,
			dlg.m_height, dlg.m_width, dlg.m_vis, value_layer );
		m_doc->ProjectModified( TRUE );
		CancelHighlight();
		if( m_cursor_mode == CUR_PART_SELECTED )
			m_doc->m_plist->SelectPart( m_sel_part );
		else if( m_cursor_mode == CUR_VALUE_SELECTED 
			&& m_sel_part->m_value_size && m_sel_part->m_value_vis )
			m_doc->m_plist->SelectValueText( m_sel_part );
		else
			CancelSelection();
		Invalidate( FALSE );
	}
#endif
}

void CFreePcbView::OnValueShowPart()
{
	OnRefShowPart();
}

void CFreePcbView::OnPartEditValue()
{
	OnValueProperties();
}


void CFreePcbView::OnRefRotateCW()
{
#ifndef CPT2

	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list ); 
	CancelHighlight();
	m_doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_ref_angle = (m_sel_part->m_ref_angle + 90)%360;
	m_doc->m_plist->DrawPart( m_sel_part );
	m_doc->m_plist->SelectRefText( m_sel_part );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

void CFreePcbView::OnRefRotateCCW()
{
#ifndef CPT2
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list ); 
	CancelHighlight();
	m_doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_ref_angle = (m_sel_part->m_ref_angle + 270)%360;
	m_doc->m_plist->DrawPart( m_sel_part );
	m_doc->m_plist->SelectRefText( m_sel_part );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

void CFreePcbView::OnValueRotateCW()
{
#ifndef CPT2
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list ); 
	CancelHighlight();
	m_doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_value_angle = (m_sel_part->m_value_angle + 90)%360;
	m_doc->m_plist->DrawPart( m_sel_part );
	m_doc->m_plist->SelectValueText( m_sel_part );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}

void CFreePcbView::OnValueRotateCCW()
{
#ifndef CPT2
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_doc->m_undo_list ); 
	CancelHighlight();
	m_doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_value_angle = (m_sel_part->m_value_angle + 270)%360;
	m_doc->m_plist->DrawPart( m_sel_part );
	m_doc->m_plist->SelectValueText( m_sel_part );
	m_doc->ProjectModified( TRUE );
	Invalidate( FALSE );
#endif
}


void CFreePcbView::OnSegmentMove()
{
#ifndef CPT2
//	m_doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	id id = m_sel_id;
	int ic = m_sel_id.I2();
	int ivtx = m_sel_id.I3();
	m_dragging_new_item = 0;
	
	m_last_pt.x = m_sel_prev_vtx->x;
	m_last_pt.y = m_sel_prev_vtx->y;

	m_from_pt.x = m_sel_vtx->x;
	m_from_pt.y = m_sel_vtx->y;

	m_to_pt.x = m_sel_next_vtx->x;
	m_to_pt.y = m_sel_next_vtx->y;

	int nsegs = m_sel_con->NumSegs();
	int use_third_segment = ivtx < nsegs - 1;
	if(use_third_segment)
	{
		m_next_pt.x = m_sel_next_next_vtx->x;	// Shouldn't really do this if we're off the edge?
		m_next_pt.y = m_sel_next_next_vtx->y;
	}

	CPoint p;
//	p = m_last_mouse_point;

	p.x = (m_to_pt.x - m_from_pt.x) / 2 + m_from_pt.x;
	p.y = (m_to_pt.y - m_from_pt.y) / 2 + m_from_pt.y;


	m_doc->m_nlist->StartMovingSegment( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2, use_third_segment );
	SetCursorMode( CUR_MOVE_SEGMENT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
#endif
}

// CPT (all that follows):

void CFreePcbView::ActiveWidthUp(CDC * pDC) {
  // Increase the active routing width to the next value in document's width table greater than the current value.  Also check
  // the current net's default width value, and make that value one of the options:
  // CPT2 updated.
  int cWidths = m_doc->m_w.GetSize();
  #define widthAt(i) (m_doc->m_w.GetAt(i))
  int defaultW = m_doc->m_trace_w;
  cnet2 *net = m_sel.First()->GetNet();
  if (net->def_w) defaultW = net->def_w;
  int i;
  for (i=0; i<cWidths; i++)
    if (m_active_width < widthAt(i)) break;
  if (m_active_width < defaultW && (i==cWidths || defaultW < widthAt(i)))
    m_active_width = defaultW;
  else if (i<cWidths)
    m_active_width = widthAt(i);
  // Change the display of the temporary dragged seg:
  m_dlist->ChangeRoutingLayer( pDC, m_active_layer, LAY_SELECTION, m_active_width);
  Invalidate(FALSE);
  ShowSelectStatus();
  }

void CFreePcbView::ActiveWidthDown(CDC * pDC) {
  // Similar to ActiveWidthUp().
  // CPT2 updated.
  int cWidths = m_doc->m_w.GetSize();
  #define widthAt(i) (m_doc->m_w.GetAt(i))
  int defaultW = m_doc->m_trace_w;
  cnet2 *net = m_sel.First()->GetNet();
  if (net->def_w) defaultW = net->def_w;
  int i;
  for (i=cWidths-1; i>=0; i--)
    if (m_active_width > widthAt(i)) break;
  if (m_active_width > defaultW && (i<0 || defaultW > widthAt(i)))
    m_active_width = defaultW;
  else if (i>=0)
    m_active_width = widthAt(i);
  // Change the display of the temporary dragged seg:
  m_dlist->ChangeRoutingLayer( pDC, m_active_layer, LAY_SELECTION, m_active_width);
  Invalidate(FALSE);
  ShowSelectStatus();
  }

void CFreePcbView::RoutingGridUp() {
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.RoutingGridUp();
	}

void CFreePcbView::RoutingGridDown() {
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.RoutingGridDown();
	}

void CFreePcbView::UnitToggle(bool bShiftKeyDown) {
	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.UnitToggle(bShiftKeyDown, &(m_doc->m_visible_grid), &(m_doc->m_part_grid), &(m_doc->m_routing_grid));
	}

void CFreePcbView::ToggleSelectionState(cpcb_item *item) 
{
	// CPT2 updated arg & rewrote.  Included a few things from the old version of SelectItem().
	// If the item specified is part of the selection group, remove it from the selection group.  Otherwise,
	// add it to the group.
	if( m_highlight_net )
		CancelHighlightNet();
	if (m_sel.Contains(item))
		m_sel.Remove(item);
	else
		m_sel.Add(item);
	if (m_sel.GetSize()==0)
		CancelSelection();
	else
		HighlightSelection();
}


// CPT
void CFreePcbView::HandleNoShiftLayerKey(int layer, CDC *pDC) 
{
	if( !m_doc->m_vis[layer] ) 
	{
		PlaySound( TEXT("CriticalStop"), 0, 0 );
		CString s ((LPCSTR) IDS_CantRouteOnInvisibleLayer);
		AfxMessageBox( s );
		return;
	}
	if (m_cursor_mode != CUR_DRAG_STUB && m_cursor_mode != CUR_DRAG_RAT)
	{
		m_active_layer = layer;
		ShowActiveLayer();
		return;
	}

	cvertex2 *start = NULL;
	if (m_cursor_mode == CUR_DRAG_STUB) 
		start = m_sel.First()->ToVertex();
	else if (m_cursor_mode == CUR_DRAG_RAT)
	{
		cseg2 *rat = m_sel.First()->ToSeg();
		start = m_dir==0? rat->preVtx: rat->postVtx;
	}
	if (start && start->pin && start->pin->pad_layer!=LAY_PAD_THRU)
		// Changing layer while routing from an SMT pad is illegal
		layer = -1,
		PlaySound( TEXT("CriticalStop"), 0, 0 );
	else
		m_dlist->ChangeRoutingLayer( pDC, layer, LAY_SELECTION, 0 ),
		m_active_layer = layer,
		ShowActiveLayer();
}

void CFreePcbView::HandleShiftLayerKey(int layer, CDC *pDC) {
#ifndef CPT2
	if( m_cursor_mode == CUR_SEG_SELECTED )	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		m_sel_id.Con()->Undraw();	// AMW: updated for new id functions, etc.
		cconnect * c = m_sel_id.Con();
		cseg * seg = m_sel_id.Seg();
		seg->m_layer = layer;
		m_sel_id.Con()->Draw();		
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		}
	else if( m_cursor_mode == CUR_CONNECT_SELECTED ) {
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_doc->m_undo_list );
		m_sel_id.Con()->Undraw();	// AMW: updated for new id functions, etc.
		cconnect * c = m_sel_id.Con();
		for( int is=0; is<c->NumSegs(); is++ ) {
			cseg * seg = &c->SegByIndex(is);
			seg->m_layer = layer;
			}
		m_sel_id.Con()->Draw();	
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		}
	else if( m_cursor_mode == CUR_AREA_CORNER_SELECTED || m_cursor_mode == CUR_AREA_SIDE_SELECTED ) {
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_doc->m_undo_list );
		carea * a = &m_sel_net->area[m_sel_ia];
		a->Undraw();
		a->SetLayer( layer );
		a->Draw( m_dlist );
		int ret = m_doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
		if( ret == -1 ) {
			// error
			CString s ((LPCSTR) IDS_ErrorUnableToClipPolygon);
			AfxMessageBox( s );
			m_doc->OnEditUndo();
			}
		else if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections(  m_sel_net, -1, m_doc->m_auto_ratline_disable,
					m_doc->m_auto_ratline_min_pins, TRUE  );
		CancelSelection();
		m_doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		}
#endif
	}		
// end CPT