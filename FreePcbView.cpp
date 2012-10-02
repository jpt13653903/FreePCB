// FreePcbView.cpp : implementation of the CFreePcbView class
//
// CPT2 TODO maybe try putting the functions in this file into some sort of logical order.

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
#include "Part.h"


// globals
extern CFreePcbApp theApp;
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
	CONTEXT_TEXT,
	CONTEXT_AREA_CORNER,
	CONTEXT_AREA_EDGE,
	CONTEXT_BOARD_CORNER,
	CONTEXT_BOARD_SIDE,
	CONTEXT_VERTEX,				// CPT2 was CONTEXT_END_VERTEX, taking over as the main one
	CONTEXT_SM_CORNER,
	CONTEXT_SM_SIDE,
	CONTEXT_CONNECT,
	CONTEXT_NET,
	CONTEXT_GROUP,
	CONTEXT_VALUE_TEXT,
	CONTEXT_TEE					// CPT2 added
};

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView

IMPLEMENT_DYNCREATE(CFreePcbView, CView)

BEGIN_MESSAGE_MAP(CFreePcbView, CView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_MOUSEWHEEL()
	// Standard printing commands
//	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
//	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
//	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
//	ON_WM_SYSCHAR()
//  ON_WM_SYSCOMMAND()
ON_WM_ERASEBKGND()
ON_WM_KEYUP()
ON_WM_LBUTTONUP()
ON_WM_SETCURSOR()
ON_WM_MOVE()
ON_WM_CONTEXTMENU()
ON_MESSAGE( WM_USER_VISIBLE_GRID, OnChangeVisibleGrid )
ON_COMMAND(ID_ADD_PART, OnAddPart)							// CPT2 used to be in CFreePcbDoc, trying it out here...
ON_COMMAND(ID_ADD_BOARDOUTLINE, OnAddBoardOutline)
ON_COMMAND(ID_ADD_AREA, OnAddArea)
ON_COMMAND(ID_ADD_TEXT, OnTextAdd)
ON_COMMAND(ID_ADD_NET, OnAddNet)
ON_COMMAND(ID_ADD_SOLDERMASKCUTOUT, OnAddSoldermaskCutout)
ON_COMMAND(ID_VIEW_ENTIREBOARD, OnViewEntireBoard)
ON_COMMAND(ID_VIEW_ALLELEMENTS, OnViewAllElements)
ON_COMMAND(ID_VIEW_FINDPART, OnViewFindpart)
ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
ON_COMMAND(ID_EDIT_CUT, OnEditCut)
ON_COMMAND(ID_EDIT_SAVEGROUPTOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_TOOLS_MOVEORIGIN, OnToolsMoveOrigin)
ON_COMMAND(ID_TOOLS_DRC, OnToolsDrc)
ON_COMMAND(ID_TOOLS_CLEAR_DRC, OnToolsClearDrc)
ON_COMMAND(ID_TOOLS_SHOWDRCERRORLIST, OnToolsShowDRCErrorlist)
ON_COMMAND(ID_TOOLS_REPEATDRC, OnToolsRepeatDrc)

ON_COMMAND(ID_NONE_ADDPART, OnAddPart)
ON_COMMAND(ID_NONE_ADDTEXT, OnTextAdd)
ON_COMMAND(ID_NONE_ADDBOARDOUTLINE, OnAddBoardOutline)
ON_COMMAND(ID_NONE_ADDSMCUTOUT, OnAddSoldermaskCutout)
ON_COMMAND(ID_NONE_ADDCOPPERAREA, OnAddArea)
ON_COMMAND(ID_NONE_FOOTPRINTWIZARD, OnFootprintWizard)
ON_COMMAND(ID_NONE_FOOTPRINTEDITOR, OnFootprintEditor)
ON_COMMAND(ID_NONE_CHECKPARTSANDNETS, OnCheckPartsAndNets)
ON_COMMAND(ID_NONE_DRC, OnToolsDrc)
ON_COMMAND(ID_NONE_REPEATDRC, OnToolsRepeatDrc)
ON_COMMAND(ID_NONE_CLEARDRCERRORS, OnToolsClearDrc)
ON_COMMAND(ID_NONE_VIEWALL, OnViewAll)
ON_COMMAND(ID_TEXT_DELETE, OnTextDelete)
ON_COMMAND(ID_TEXT_MOVE, OnTextMove)
ON_COMMAND(ID_TEXT_EDIT, OnTextEdit)
ON_COMMAND(ID_REF_MOVE, OnRefMove)							// These also cover the value-text context menu
ON_COMMAND(ID_REF_PROPERTIES, OnRefProperties)		
ON_COMMAND(ID_REF_SHOWPART, OnRefShowPart)
ON_COMMAND(ID_REF_ROTATECW, OnRefRotateCW)
ON_COMMAND(ID_REF_ROTATECCW, OnRefRotateCCW)
ON_COMMAND(ID_PART_MOVE, OnPartMove)
ON_COMMAND(ID_PART_CHANGESIDE, OnPartChangeSide)
ON_COMMAND(ID_PART_ROTATE, OnPartRotateCW)
ON_COMMAND(ID_PART_ROTATECOUNTERCLOCKWISE, OnPartRotateCCW)
ON_COMMAND(ID_PART_GLUE, OnPartGlue)
ON_COMMAND(ID_PART_UNGLUE, OnPartUnglue)
ON_COMMAND(ID_PART_OPTIMIZE, OnPartOptimize)
ON_COMMAND(ID_PART_PROPERTIES, OnPartProperties)			// CPT2 used to be in CFreePcbDoc, trying it out here...
ON_COMMAND(ID_PART_EDITFOOTPRINT, OnPartEditThisFootprint)
ON_COMMAND(ID_PART_SET_REF, OnPartRefProperties)
ON_COMMAND(ID_PART_EDITVALUE, OnPartValueProperties)
ON_COMMAND(ID_PART_DELETE, OnPartDelete)
ON_COMMAND(ID_PAD_OPTIMIZERATLINES, OnPadOptimize)
ON_COMMAND(ID_PAD_ADDTONET, OnPadAddToNet)
ON_COMMAND(ID_PAD_DETACHFROMNET, OnPadDetachFromNet)
ON_COMMAND(ID_PAD_CONNECTTOPIN, OnPadStartRatline)
ON_COMMAND(ID_PAD_STARTSTUBTRACE, OnPadStartTrace)
ON_COMMAND(ID_SEGMENT_MOVE, OnSegmentMove)
ON_COMMAND(ID_SEGMENT_SETWIDTH, OnSegmentSetWidth)
ON_COMMAND(ID_SEGMENT_CHANGELAYER, OnSegmentChangeLayer)
ON_COMMAND(ID_SEGMENT_ADDVERTEX, OnSegmentAddVertex)
ON_COMMAND(ID_SEGMENT_UNROUTE, OnSegmentUnroute)
ON_COMMAND(ID_SEGMENT_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_SEGMENT_DELETE, OnSegmentDelete)
ON_COMMAND(ID_SEGMENT_DELETECONNECTION, OnDeleteConnection)
ON_COMMAND(ID_RATLINE_COMPLETE, OnRatlineComplete)
ON_COMMAND(ID_RATLINE_DELETECONNECTION, OnDeleteConnection)
ON_COMMAND(ID_RATLINE_LOCKCONNECTION, OnRatlineLockConnection)
ON_COMMAND(ID_RATLINE_UNLOCKCONNECTION, OnRatlineUnlockConnection)
ON_COMMAND(ID_RATLINE_ROUTE, OnRatlineRoute)
ON_COMMAND(ID_RATLINE_OPTIMIZE, OnOptimize)
ON_COMMAND(ID_RATLINE_CHANGEPIN, OnRatlineChangeEndPin)
ON_COMMAND(ID_VERTEX_MOVE, OnVertexMove)
ON_COMMAND(ID_VERTEX_EDIT, OnVertexProperties)
ON_COMMAND(ID_VERTEX_ADDVIA, OnVertexAddVia)
ON_COMMAND(ID_VERTEX_REMOVEVIA, OnVertexRemoveVia)
ON_COMMAND(ID_VERTEX_SETSIZE, OnVertexProperties)
ON_COMMAND(ID_VERTEX_STARTTRACE, OnVertexStartTrace)
ON_COMMAND(ID_VERTEX_STARTRATLINE, OnVertexStartRatline)
ON_COMMAND(ID_VERTEX_UNROUTECONNECTION, OnUnrouteTrace)
ON_COMMAND(ID_VERTEX_DELETE, OnVertexDelete)
ON_COMMAND(ID_VERTEX_DELETECONNECTION, OnDeleteConnection)
// ON_COMMAND(ID_VERTEX_PROPERITES, OnVertexProperties)
ON_COMMAND(ID_VERTEX_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_TEE_MOVE, OnTeeMove)
ON_COMMAND(ID_TEE_SETPOSITION, OnTeeProperties)
ON_COMMAND(ID_TEE_ADDVIA, OnVertexAddVia)
ON_COMMAND(ID_TEE_REMOVEVIA, OnVertexRemoveVia)
ON_COMMAND(ID_TEE_SETSIZE, OnTeeProperties)
ON_COMMAND(ID_TEE_STARTRATLINE, OnVertexStartRatline)
ON_COMMAND(ID_TEE_STARTTRACE, OnVertexStartTrace)
ON_COMMAND(ID_TEE_DELETE, OnTeeDelete)
ON_COMMAND(ID_ENDVERTEX_MOVE, OnVertexMove)					// CPT2 these ID_ENDVERTEX_xxx's are basically defunct
ON_COMMAND(ID_ENDVERTEX_ADDSEGMENTS, OnVertexStartTrace)
ON_COMMAND(ID_ENDVERTEX_ADDCONNECTION, OnVertexStartRatline)
ON_COMMAND(ID_ENDVERTEX_DELETE, OnVertexDelete)
ON_COMMAND(ID_ENDVERTEX_EDIT, OnVertexProperties)
ON_COMMAND(ID_ENDVERTEX_SETSIZE, OnVertexProperties)
ON_COMMAND(ID_CONNECT_SETWIDTH, OnConnectSetWidth)
ON_COMMAND(ID_CONNECT_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_CONNECT_DELETETRACE, OnDeleteConnection)
ON_COMMAND(ID_CONNECT_CHANGELAYER, OnConnectChangeLayer)
ON_COMMAND(ID_BOARDCORNER_MOVE, OnPolyCornerMove)
ON_COMMAND(ID_BOARDCORNER_EDIT, OnPolyCornerEdit)
ON_COMMAND(ID_BOARDCORNER_DELETECORNER, OnPolyCornerDelete)
ON_COMMAND(ID_BOARDCORNER_DELETEOUTLINE, OnPolyDelete)
ON_COMMAND(ID_BOARDSIDE_DELETEOUTLINE, OnPolyDelete)
ON_COMMAND(ID_AREACORNER_MOVE, OnPolyCornerMove)
ON_COMMAND(ID_AREACORNER_DELETE, OnPolyCornerDelete)
ON_COMMAND(ID_AREACORNER_DELETEAREA, OnPolyDelete)
ON_COMMAND(ID_AREACORNER_ADDCUTOUT, OnPolyAddCutout)
ON_COMMAND(ID_AREACORNER_DELETECUTOUT, OnPolyDeleteCutout)
ON_COMMAND(ID_AREACORNER_PROPERTIES, OnPolyCornerEdit)
ON_COMMAND(ID_AREACORNER_ADDNEWAREA, OnAddSimilarArea)
ON_COMMAND(ID_AREACORNER_EDITAREA, OnAreaEdit)
ON_COMMAND(ID_POLYSIDE_INSERTCORNER, OnPolySideAddCorner)
ON_COMMAND(ID_POLYSIDE_CONVERTTOSTRAIGHT, OnPolySideConvertToStraightLine)
ON_COMMAND(ID_POLYSIDE_CONVERTTOARC_CW, OnPolySideConvertToArcCw)
ON_COMMAND(ID_POLYSIDE_CONVERTTOARC_CCW, OnPolySideConvertToArcCcw)
ON_COMMAND(ID_POLYSIDE_EXCLUDEREGION, OnPolyAddCutout)
ON_COMMAND(ID_POLYSIDE_REMOVECONTOUR, OnPolyDeleteCutout)
ON_COMMAND(ID_AREAEDGE_ADDNEWAREA, OnAddSimilarArea)
ON_COMMAND(ID_AREAEDGE_EDITAREA, OnAreaEdit)
ON_COMMAND(ID_AREAEDGE_APPLYCLEARANCES, OnAreaEdgeApplyClearances)
ON_COMMAND(ID_AREAEDGE_DELETE, OnPolyDelete)
ON_COMMAND(ID_AREAEDGE_STARTTRACE, OnAreaStartTrace)
ON_COMMAND(ID_SMCORNER_MOVE, OnPolyCornerMove)
ON_COMMAND(ID_SMCORNER_SETPOSITION, OnPolyCornerEdit)
ON_COMMAND(ID_SMCORNER_EXCLUDEREGION, OnPolyAddCutout)
ON_COMMAND(ID_SMCORNER_PROPERTIES, OnSmEdit)
ON_COMMAND(ID_SMCORNER_DELETECORNER, OnPolyCornerDelete)
ON_COMMAND(ID_SMCORNER_REMOVECONTOUR, OnPolyDeleteCutout)
ON_COMMAND(ID_SMCORNER_DELETECUTOUT, OnPolyDelete)
ON_COMMAND(ID_SMSIDE_EDITCUTOUTPARAMS, OnSmEdit)
ON_COMMAND(ID_SMSIDE_DELETECUTOUT, OnPolyDelete)
ON_COMMAND(ID_NET_SETWIDTH, OnNetSetWidth)				// CPT2 defunct I think
ON_COMMAND(ID_NET_CHANGELAYER, OnNetChangeLayer)
ON_COMMAND(ID_NET_EDITNET, OnNetEditnet)
ON_COMMAND(ID_GROUP_MOVE, OnGroupMove)
ON_COMMAND(ID_GROUP_SAVETOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_GROUP_COPY, OnGroupCopy)
ON_COMMAND(ID_GROUP_CUT, OnGroupCut)
ON_COMMAND(ID_GROUP_DELETE, OnGroupDelete)
ON_COMMAND(ID_GROUP_ROTATE, OnGroupRotate)
ON_COMMAND(ID_GROUP_ROTATE_CCW, OnGroupRotateCCW)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView construction/destruction

CFreePcbView::CFreePcbView()
{
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	m_lastKeyWasArrow = m_lastKeyWasGroupRotate = FALSE;		// CPT
	m_highlight_net = NULL;
	m_units = MIL;
	m_bHandlingKeyPress = false;								// CPT2 logging stuff
	m_bReplayMode = m_bReplaying = false;						// ditto

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
	m_dir = 0;
	m_active_layer = LAY_TOP_COPPER;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	m_inflection_mode = IM_90_45;
	m_snap_mode = SM_GRID_POINTS;
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
	// CPT2 NB got rid of the old stuff with InvalidateLeftPane(), which wasn't working so well.  I've added double-buffering during
	// DrawLeftPane() and DrawBottomPane() to prevent flicker.
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
	if (CurDragging())
	{
		if (m_bReplayMode && !m_bReplaying)
			return;
		Log(LOG_LEFT_CLICK, &m_last_cursor_point.x, &m_last_cursor_point.y);
		LogXtra(&m_from_pt.x, &m_from_pt.y);
	}
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

		if( bShiftKeyDown )
			if( nHits>0 )
				m_sel_offset = SelectObjPopup( point );
			else
				m_sel_offset = -1;

		if( m_sel_offset >= 0 )
		{
			// Something to select!
			CPcbItem *item = m_hit_info[m_sel_offset].item;
			m_sel_prev = item;											// CPT

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
		}
		Invalidate( FALSE );
	}

	else if( m_cursor_mode == CUR_DRAG_PART )
	{
		// complete move
		CPart *part = m_sel.First()->ToPart();
		SetCursorMode( CUR_PART_SELECTED );
		CPoint p = m_dlist->WindowToPCB( point );
		part->CancelDragging();
		int old_angle = part->angle;
		int angle = old_angle + m_dlist->GetDragAngle();
		angle = angle % 360;
		int old_side = part->side;
		int side = old_side + m_dlist->GetDragSide();
		side &= 1;

		// save undo info for part and attached nets
		if (!m_dragging_new_item)
			part->SaveUndoInfo();
		m_dragging_new_item = FALSE;

		// now move it
		part->glued = 0;
		int dx = m_last_cursor_point.x - m_from_pt.x, dy = m_last_cursor_point.y - m_from_pt.y;
		part->Move( m_last_cursor_point.x, m_last_cursor_point.y, angle, side );
		part->PartMoved( dx, dy, m_dlist->GetDragAngle(), m_dlist->GetDragSide() );	//** AMW3 also needs to handle changes in side or angle
		if( m_doc->m_vis[LAY_RAT_LINE] )
			part->OptimizeConnections();
		m_doc->ProjectModified( TRUE );
		SetFKText( m_cursor_mode );
		part->Highlight();
	}

	else if( m_cursor_mode == CUR_DRAG_GROUP || m_cursor_mode == CUR_DRAG_GROUP_ADD )
	{
		// complete move
		m_doc->m_dlist->StopDragging();
		if( m_cursor_mode == CUR_DRAG_GROUP )
			SaveUndoInfoForGroup();
		MoveGroup( m_last_cursor_point.x - m_from_pt.x, m_last_cursor_point.y - m_from_pt.y );
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_doc->m_vis[LAY_RAT_LINE] );
		if( m_doc->m_vis[LAY_RAT_LINE] )
			m_doc->m_nlist->OptimizeConnections();
		m_doc->ProjectModified( TRUE, m_cursor_mode==CUR_DRAG_GROUP_ADD );
		HighlightSelection();
	}

	else if( m_cursor_mode == CUR_MOVE_ORIGIN )
	{
		// complete move
		CancelSelection();
		CPoint p = m_dlist->WindowToPCB( point );
		m_doc->m_dlist->StopDragging();
		m_doc->CreateMoveOriginUndoRecord( -m_last_cursor_point.x, -m_last_cursor_point.y );
		MoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y );
		m_doc->ProjectModified( TRUE );
		OnViewAllElements();
	}

	else if( m_cursor_mode == CUR_DRAG_REF )
	{
		// complete move
		CText *t = m_sel.First()->ToText();
		CPart *part = t->GetPart();
		m_doc->m_dlist->StopDragging();
		int drag_angle = m_dlist->GetDragAngle();
		// if part on bottom of board, drag angle is CCW instead of CW
		if( part->side && drag_angle )
			drag_angle = 360 - drag_angle;
		int angle = (t->m_angle + drag_angle) % 360;
		// save undo info  NB can save it for the reftext object only, no need to save the rest of the part.
		t->SaveUndoInfo();
		// now move it
		t->MustRedraw();
		t->MoveRelative( m_last_cursor_point.x, m_last_cursor_point.y, angle );
		m_doc->ProjectModified( TRUE );
		HighlightSelection();
	}

	else if( m_cursor_mode == CUR_DRAG_RAT )
	{
		// routing a ratline, add segment(s)
		CNet *highlight_net0 = m_highlight_net;        // AMW r274 save previous state
		CancelHighlight();
		CSeg *rat = m_sel.First()->ToSeg();
		CConnect *c = rat->m_con;
		CNet *net = rat->m_net;
		CPoint p = m_last_cursor_point;

		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );

		// test for reaching destination of ratline (which might be a pin-vtx, a tee-vtx, or any other vtx)
		CVertex *start = m_dir==0? rat->preVtx: rat->postVtx;
		CVertex *dest = m_dir==0? rat->postVtx: rat->preVtx;
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
			CPin *pin = num_hits? hit_info[0].item->ToPin(): NULL;
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
			c->SaveUndoInfo();
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

		if (bRatlineFinished)
			goto cancel_selection_and_goodbye;
		m_doc->ProjectModified( TRUE );
		// CPT2 Make sure cursor matches the snapped point:
		CPoint p2 = m_dlist->PCBToScreen( m_last_cursor_point );
		SetCursorPos( p2.x, p2.y );

		rat->StartDragging( pDC, m_last_cursor_point.x, m_last_cursor_point.y, 
							m_active_layer,	LAY_SELECTION, m_active_width, m_active_layer, m_dir, 2 );
		if( highlight_net0 )							// AMW r274
			m_highlight_net = highlight_net0,
			m_highlight_net->Highlight( rat );
		m_snap_angle_ref = m_last_cursor_point;
	}

	else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
	{
		// add trace segment and vertex
		m_dlist->StopDragging();
		CSeg *seg = m_sel.First()->ToSeg();
		CConnect *c = seg->m_con;
		c->SaveUndoInfo();
		seg->InsertSegment( m_last_cursor_point.x, m_last_cursor_point.y,
			seg->m_layer, seg->m_width, 0 );
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			c->GetNet()->SetThermals();		//** AMW3 added
			c->GetNet()->OptimizeConnections();
		}
		m_doc->ProjectModified( TRUE );
		CancelSelection();
	}

	else if( m_cursor_mode == CUR_DRAG_POLY_MOVE )
	{
		// CPT2 generalized clause that deals with dragging corners of areas, sm-cutouts, and board-outlines alike.
		CCorner *c = m_sel.First()->ToCorner();
		CPolyline *poly = c->GetPolyline();
		CNet *net = c->GetNet();
		m_dlist->StopDragging();
		poly->SaveUndoInfo();
		CPoint p = m_last_cursor_point;
		c->Move( p.x, p.y );
		if (!poly->PolygonModified( FALSE, TRUE ))
			m_doc->OnEditUndo();
		else if( poly->IsArea() && m_doc->m_vis[LAY_RAT_LINE] )
		{
			c->GetNet()->SetThermals();		//** AMW3 added
			net->OptimizeConnections();
		}
		m_doc->ProjectModified( TRUE );
		TryToReselectCorner( p.x, p.y );
	}

	else if( m_cursor_mode == CUR_DRAG_POLY_INSERT )
	{
		// CPT2 generalized clause that deals with inserting corners of areas, sm-cutouts, and board-outlines alike.
		CSide *s = m_sel.First()->ToSide();
		CPolyline *poly = s->GetPolyline();
		CNet *net = s->GetNet();
		m_dlist->StopDragging();
		poly->SaveUndoInfo();
		CPoint p = m_last_cursor_point;
		s->InsertCorner( p.x, p.y );
		if (!poly->PolygonModified( FALSE, TRUE ))
			m_doc->OnEditUndo();
		else if( poly->IsArea() && m_doc->m_vis[LAY_RAT_LINE] )
		{
			poly->GetNet()->SetThermals();		//** AMW3 added
			net->OptimizeConnections();
		}
		m_doc->ProjectModified( TRUE );
		CancelSelection();
	}

	else if( m_cursor_mode == CUR_ADD_AREA )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		CNet *net = m_sel.First()->GetNet();
		CArea *a = new CArea(net, m_active_layer, m_polyline_hatch, 2*NM_PER_MIL, 10*NM_PER_MIL);
		CContour *ctr = new CContour(a, true);
		CCorner *c = new CCorner(ctr, p.x, p.y);					// Constructor sets contour's head and tail
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_RAT_LINE, 1, 2 );
		SetCursorMode( CUR_DRAG_POLY_1 );
		m_drag_contour = ctr;
		m_poly_drag_mode = CUR_ADD_AREA;
		m_doc->ProjectModified( TRUE );
		m_snap_angle_ref = m_last_cursor_point;
	}

	else if( m_cursor_mode == CUR_DRAG_POLY_1 )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		CContour *ctr = m_drag_contour;
		ctr->AppendCorner(p.x, p.y, m_polyline_style);
		m_doc->Redraw();
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_RAT_LINE, 1, 2 );
		SetCursorMode( CUR_DRAG_POLY );
		m_doc->ProjectModified( true, true );			// CPT2 second arg indicates that this change gets combined with the earlier changes in a single
														// undo record
		m_snap_angle_ref = m_last_cursor_point;
	}

	else if( m_cursor_mode == CUR_DRAG_POLY )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		CContour *ctr = m_drag_contour;
		CCorner *head = ctr->head;
		if( p.x == head->x && p.y == head->y )
		{
			// cursor point is back at first point, close contour and finish up
			if (m_poly_drag_mode == CUR_ADD_POLY_CUTOUT)
				FinishAddPolyCutout();
			else
				FinishAddPoly();
			m_doc->m_dlist->StopDragging();
			CancelSelection();
		}
		else
		{
			// add cursor point
			ctr->AppendCorner(p.x, p.y, m_polyline_style);
			m_doc->Redraw();
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_RAT_LINE, 1, 2 );
			m_snap_angle_ref = m_last_cursor_point;
		}
		m_doc->ProjectModified( true, true );
	}

	else if( m_cursor_mode == CUR_ADD_POLY_CUTOUT )
	{
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		CPolyline *poly = m_sel.First()->GetPolyline();
		poly->MustRedraw();
		CContour *ctr = new CContour(poly, false);
		CCorner *c = new CCorner(ctr, p.x, p.y);					// Constructor sets contour's head and tail
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_RAT_LINE, 1, 2 );
		SetCursorMode( CUR_DRAG_POLY_1 );
		m_drag_contour = ctr;
		m_poly_drag_mode = CUR_ADD_POLY_CUTOUT;
		m_doc->ProjectModified( TRUE );
		m_snap_angle_ref = m_last_cursor_point;
	}

	else if( m_cursor_mode == CUR_ADD_SMCUTOUT )
	{
		// add poly for new cutout
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		CSmCutout *sm = new CSmCutout(m_doc, m_polyline_layer, m_polyline_hatch);
		CContour *ctr = new CContour(sm, true);
		CCorner *c = new CCorner(ctr, p.x, p.y);					// Constructor sets contour's head and tail
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_RAT_LINE, 1, 2 );
		SetCursorMode( CUR_DRAG_POLY_1 );
		m_drag_contour = ctr;
		m_poly_drag_mode = CUR_ADD_SMCUTOUT;
		m_doc->ProjectModified( TRUE );
		m_snap_angle_ref = m_last_cursor_point;
	}

	else if( m_cursor_mode == CUR_ADD_BOARD )
	{
		// place first corner of board outline
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		CBoard *b = new CBoard(m_doc);
		CContour *ctr = new CContour(b, true);
		CCorner *c = new CCorner(ctr, p.x, p.y);
		m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_RAT_LINE, 1, 2 );
		SetCursorMode( CUR_DRAG_POLY_1 );
		m_drag_contour = ctr;
		m_poly_drag_mode = CUR_ADD_BOARD;
		m_doc->ProjectModified( TRUE );
		m_snap_angle_ref = m_last_cursor_point;
	}

	else if( m_cursor_mode == CUR_DRAG_VTX )
	{
		// move vertex by modifying adjacent segments
		CVertex *vtx = m_sel.First()->ToVertex();
		vtx->CancelDragging();
		vtx->m_con->SaveUndoInfo();
		CPoint p = m_last_cursor_point;
		vtx->Move( p.x, p.y );
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			vtx->m_net->SetThermals();
			vtx->m_net->OptimizeConnections();
		}
		m_doc->ProjectModified( TRUE );
		HighlightSelection();
	}

	else if( m_cursor_mode == CUR_DRAG_TEE )
	{
		// move tee by modifying adjacent segments
		CTee *tee = m_sel.First()->ToTee();
		CNet *net = tee->GetNet();
		tee->CancelDragging();
		net->SaveUndoInfo( CNet::SAVE_CONNECTS );
		CPoint p = m_last_cursor_point;
		tee->Move( p.x, p.y );
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			net->SetThermals();
			net->OptimizeConnections();
		}
		m_doc->ProjectModified( TRUE );
		HighlightSelection();
	}

	else if( m_cursor_mode == CUR_MOVE_SEGMENT )
	{
		// move vertex by modifying adjacent segments and reconciling via
		m_doc->m_dlist->StopDragging();
		CSeg *seg = m_sel.First()->ToSeg();
		CConnect *c = seg->m_con;
		CNet *net = seg->m_net;
		net->SaveUndoInfo( CNet::SAVE_CONNECTS );		// Saves the whole net because of the possibility that one endpt is a tee
		CPoint cpi, cpf;
		m_doc->m_dlist->Get_Endpoints(&cpi, &cpf);
		ASSERT(cpi != cpf);								// Should be at least one grid snap apart.
		seg->preVtx->Move( cpi.x, cpi.y );
		seg->postVtx->Move( cpf.x, cpf.y );
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			net->SetThermals();
			net->OptimizeConnections();
		}
		m_doc->ProjectModified( TRUE );
		CancelSelection();
	}

	else if( m_cursor_mode == CUR_DRAG_NEW_RAT )
	{
		// dragging ratline to make a new connection from vertex or pin
		// test for hit on pin
		// CPT2 not thrilled by keeping the net highlight after a new ratline is made:
		// CNet *highlight_net0 = m_highlight_net;	// AMW r275 save previous state
		CancelHighlight();
		CPoint p = m_dlist->WindowToPCB( point );
		CArray<CHitInfo> hit_info;
		int num_hits = m_dlist->TestSelect(p.x, p.y, &hit_info,	bitPin|bitVia|bitTraceVtx|bitTee);
		if( num_hits )
		{
			// we have a hit
			CPcbItem *hit = hit_info[0].item;
			CPcbItem *sel0 = m_sel.First();
			if (hit->IsTee())
				hit = hit->ToTee()->vtxs.First();
			if (sel0->IsTee())
				sel0 = sel0->ToTee()->vtxs.First();
			
			if( hit->IsVertex() && sel0->IsPin() || hit->IsPin() && sel0->IsVertex() )
			{
				// connecting from vertex to pin, or from pin to vertex, sort it out
				CPin *pin = hit->IsPin()? hit->ToPin(): sel0->ToPin();
				CVertex *v = hit->IsVertex()? hit->ToVertex(): sel0->ToVertex();
				// now do it
				if( pin->net && pin->net != v->m_net )
				{
					// pin assigned to different net, can't connect it
					CString s ((LPCSTR) IDS_YouAreTryingToConnectATraceToAPin);
					AfxMessageBox( s );
					goto cancel_selection_and_goodbye;
				}
				// add ratline from vertex to pin
				CNet *net = v->m_net;
				if( !pin->net )
					pin->part->SaveUndoInfo(),
					net->AddPin( pin );
				net->SaveUndoInfo( CNet::SAVE_CONNECTS );
				CSeg *rat_seg = v->AddRatlineToPin( pin );
				m_doc->m_dlist->StopDragging();
				/* CPT2 seems undesirable to me
				if( highlight_net0 )
					m_highlight_net = highlight_net0,
					highlight_net0->Highlight();
				*/
				if( m_doc->m_vis[LAY_RAT_LINE] )
				{
					net->SetThermals();
					net->OptimizeConnections();
				}
				m_doc->ProjectModified( TRUE );
				SelectItem(rat_seg);
			}

			else if( hit->IsPin() && sel0->IsPin() )
			{
				// connecting pin to pin
				CPin *p0 = hit->ToPin(), *p1 = sel0->ToPin();
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
				if( p0->net != p1->net )
				{
					// one pin is unassigned, assign it to the other pin's net
					if( !p0->net )
					{
						p0->SaveUndoInfo();
						p1->net->AddPin( p0 );
					}
					else if( !p1->net )
					{
						p1->SaveUndoInfo();
						p0->net->AddPin( p1 );
					}
					else
						ASSERT(0);

				}
				else if( !p0->net && !p1->net )
				{
					// connecting 2 unassigned pins, select net
					DlgAssignNet dlg;
					dlg.m_nlist = m_doc->m_nlist;
					int ret = dlg.DoModal();
					if( ret != IDOK )
					{
						m_doc->m_dlist->StopDragging();
						HighlightSelection();
						goto goodbye;
					}
					CString name = dlg.m_net_str;
					CNet *net = m_doc->m_nlist->GetNetPtrByName(&name);
					p0->SaveUndoInfo();
					p1->SaveUndoInfo();
					if( net )
					{
						// assign pins to existing net
						net->AddPin(p0);
						net->AddPin(p1);
					}
					else
					{
						// make new net
						net = new CNet(m_doc->m_nlist, name, 0, 0, 0);
						net->AddPin(p0);
						net->AddPin(p1);
						net->AddPinsFromSyncFile();
					}
				}
				m_doc->m_dlist->StopDragging();
				p0->net->SaveUndoInfo( CNet::SAVE_CONNECTS );
				CSeg *rat_seg = p0->AddRatlineToPin( p1 );
				rat_seg->MustRedraw();
				/* CPT2 seems undesirable to me
				if( highlight_net0 )
					m_highlight_net = highlight_net0,
					highlight_net0->Highlight();
				*/
				if( m_doc->m_vis[LAY_RAT_LINE] )
				{
					p0->net->SetThermals();
					p0->net->OptimizeConnections();
				}
				m_doc->ProjectModified( TRUE );
				SelectItem(rat_seg);
			}

			else if( hit->IsVertex() && sel0->IsVertex() )
			{
				CVertex *v1 = hit->ToVertex(), *v2 = sel0->ToVertex();
				if( v1->m_net != v2->m_net )
				{
					CString s ((LPCSTR) IDS_YouAreTryingToConnectTracesOnDifferentNets);
					AfxMessageBox( s );
					goto cancel_selection_and_goodbye;
				}
				if( v1==v2 )
					goto goodbye;
				
				// OK, we should be able to do this, save undo info
				v1->m_net->SaveUndoInfo( CNet::SAVE_CONNECTS );
				CConnect *c1 = v1->m_con;
				CNet *net = v1->m_net;
				CTee *adjust_at_end = NULL;
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
				CConnect *new_c = new CConnect (net);
				CVertex *new_v = new CVertex (new_c, v1->x, v1->y);
				new_c->Start(new_v);
				new_c->AppendSegment(v2->x, v2->y, LAY_RAT_LINE, 0);
				CSeg *rat_seg = new_v->postSeg;
				if (!v1->tee)
					v1->tee = new CTee(net),
					v1->tee->Add(v1);
				v1->tee->Add(new_v);
				if (!v2->tee)
					v2->tee = new CTee(net),
					v2->tee->Add(v2);
				v2->tee->Add(new_c->tail);
				if (adjust_at_end && adjust_at_end != v1->tee && adjust_at_end != v2->tee)
					adjust_at_end->Adjust();
				v1->tee->Adjust();
				v2->tee->Adjust();
				m_doc->m_dlist->StopDragging();
				/* CPT2 seems undesirable to me
				if( highlight_net0 )
					m_highlight_net = highlight_net0,
					highlight_net0->Highlight(); 
				*/
				if( m_doc->m_vis[LAY_RAT_LINE] )
				{
					net->SetThermals();
					net->OptimizeConnections();
				}
				m_doc->ProjectModified( TRUE );
				SelectItem(rat_seg);
			}
		}
	}

	else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
	{
		// dragging ratline to change end pin of trace
		CSeg *rat = m_sel.First()->ToSeg();
		CNet *net = rat->GetNet();
		// see if pad selected
		CPoint p = m_dlist->WindowToPCB( point );
		m_sel_offset = -1;						// CPT r294
		CArray<CHitInfo> hit_info;
		CPin *new_pin = NULL;
		int num_hits = m_dlist->TestSelect( p.x, p.y, &hit_info, bitPin );
		if (num_hits)
			new_pin = hit_info[0].item->ToPin();
		if (new_pin && new_pin->net && new_pin->net != net)
		{
			// pin assigned to different net, can't connect it
			CString s ((LPCSTR) IDS_YouAreTryingToConnectToAPinOnADifferentNet);
			AfxMessageBox( s );
			new_pin = NULL;
		}
		if (new_pin)
		{
			// pin is OK to connect to!
			if (!new_pin->net)
			{
				// unassigned pin, assign it
				new_pin->SaveUndoInfo();
				net->SaveUndoInfo( CNet::SAVE_CONNECTS );
				net->AddPin(new_pin);
			}
			else
				// pin already assigned to this net
				rat->m_con->SaveUndoInfo();
			rat->MustRedraw();
			CVertex *pin_end = rat->preVtx->pin? rat->preVtx: rat->postVtx;
			pin_end->pin = new_pin;
			pin_end->Move( new_pin->x, new_pin->y );
		}
		m_dlist->Set_visible( rat->dl_el, TRUE );
		m_dlist->StopDragging();
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			net->SetThermals();
			net->OptimizeConnections();
		}
		m_doc->ProjectModified( TRUE );
		HighlightSelection();
	}

	else if( m_cursor_mode == CUR_DRAG_STUB 			// CPT2 used to be called CUR_DRAG_TRACE, renamed (less confusing I think)
			 || m_cursor_mode == CUR_DRAG_AREA_STUB )
	{
		CVertex *vtx0 = m_sel.First()->ToVertex();		// (The vertex where user first started the stub)
		CConnect *con0 = vtx0->m_con;
		CVertex *tail0 = con0->tail;					// (The vertex that will be the start of the new seg)
		CNet *net = vtx0->m_net;
		// test for hit on vertex or pin
		CDisplayList *dl = m_doc->m_dlist;
		CPoint p = m_last_cursor_point;
		CArray<CHitInfo> hit_info;
		int num_hits = dl->TestSelect(p.x, p.y, &hit_info, 
										bitVia | bitTraceVtx | bitTee | bitPin);
		if( num_hits )
		{
			// hit pin or vertex
			CPcbItem *hit = hit_info[0].item;
			if (hit->IsTee())
				hit = hit->ToTee()->vtxs.First();
			if( CPin *pin = hit->ToPin() )
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
						pin->part->SaveUndoInfo();
						net->SaveUndoInfo( CNet::SAVE_CONNECTS );
						net->AddPin( pin );
					}
					else
						con0->SaveUndoInfo();
					// Go for it
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
					if (m_cursor_mode == CUR_DRAG_AREA_STUB)
						con0->ConnectHeadToLayer( m_start_layer );
					// Cleanup
					m_dlist->StopDragging();
					m_doc->ProjectModified( TRUE );
					CancelSelection();
					goto goodbye;
				}
			}

			else if( CVertex *vtx1 = hit->ToVertex() )
			{
				if (vtx1->m_net == net && vtx1!=tail0) 
				{
					/* CPT2: Removed restriction
					if( vtx1->m_con == con0 )
					{
						CString s ((LPCSTR) IDS_CantConnectTraceToItself);
						AfxMessageBox( s );
						goto goodbye;
					}
					*/
					CDlgMyMessageBox dlg;
					if (dlg.Initialize( CONNECT_TO_VTX_WARNING ))
						dlg.DoModal();

					// Ready to connect to vertex!
					CPoint pi = m_snap_angle_ref;
					CPoint pf = m_last_cursor_point;
					CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
					net->SaveUndoInfo( CNet::SAVE_CONNECTS );
					if( pp != pi )
						con0->AppendSegment( pp.x, pp.y, m_active_layer, m_active_width ) ;
					con0->AppendSegment( pf.x, pf.y, m_active_layer, m_active_width );
					CPoint vtx1_point (vtx1->x, vtx1->y);
					if( pf != vtx1_point )
						con0->AppendSegment( vtx1->x, vtx1->y, m_active_layer, m_active_width );
					CVertex *tail1 = con0->tail;
					tail1->pin = pin;
					// CPT2 attach tail1 to vtx1 with a (possibly new) tee.  Afterwards Adjust() the tee (in case vtx1 was an end-vtx)
					if (vtx1->preSeg && vtx1->postSeg)
						// Mid-trace vertex:  split trace (SplitConnect() will set vtx1->tee)
						vtx1->SplitConnect();
					else if (!vtx1->tee)
						vtx1->tee = new CTee (net),
						vtx1->tee->Add(vtx1);
					vtx1->tee->Add(tail1);
					vtx1->tee->Adjust();					// Also reconciles the via
					tail0->ReconcileVia();
					if (m_cursor_mode == CUR_DRAG_AREA_STUB)
						con0->ConnectHeadToLayer( m_start_layer );
					// Cleanup
					m_dlist->StopDragging();
					if( m_doc->m_vis[LAY_RAT_LINE] )
						net->OptimizeConnections();
					m_doc->ProjectModified( TRUE );
					CancelSelection();
					goto goodbye;
				}
			}
		}

		// come here if not connecting to anything
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint pi = m_snap_angle_ref;
		CPoint pf = m_last_cursor_point;
		CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
		con0->SaveUndoInfo();
		if( pp != pi )
			con0->AppendSegment( pp.x, pp.y, m_active_layer, m_active_width ) ;
		con0->AppendSegment( pf.x, pf.y, m_active_layer, m_active_width );
		tail0->ReconcileVia();
		if (m_cursor_mode == CUR_DRAG_AREA_STUB)
			con0->ConnectHeadToLayer( m_start_layer );
		// Cleanup
		// CPT2 Make sure cursor matches the snapped point:
		CPoint p2 = m_dlist->PCBToScreen( m_last_cursor_point );
		SetCursorPos( p2.x, p2.y );
		m_dlist->StopDragging();
		CPoint p0 = m_last_cursor_point;
		con0->tail->StartDraggingStub( pDC, p0.x, p0.y, m_active_layer, m_active_width, m_active_layer, 2, m_inflection_mode );
		m_snap_angle_ref = m_last_cursor_point;
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			net->SetThermals();
			net->OptimizeConnections();
		}
		m_doc->ProjectModified( TRUE );
		if( m_highlight_net )					// AMW r275 re-highlight net
			m_highlight_net->Highlight();
	}

	else if (m_cursor_mode == CUR_AREA_START_TRACE)
	{
		CArea *area = m_sel.First()->GetArea();
		CNet *net = area->m_net;
		net->SaveUndoInfo( CNet::SAVE_NET_ONLY );
		pDC = GetDC();
		SetDCToWorldCoords( pDC );
		pDC->SelectClipRgn( &m_pcb_rgn );
		CPoint p = m_last_cursor_point;
		m_snap_angle_ref = p;
		m_start_layer = area->m_layer;
		if (!area->TestPointInside(p.x, p.y))
		{
			CDlgMyMessageBox dlg;
			if (dlg.Initialize( AREA_START_TRACE_WARNING ))
				dlg.DoModal();
		}
		// now add new connection and first vertex.  The 1st vertex becomes the selection, which we refer to when we get to OnLButtonUp again
		CConnect *new_c = new CConnect (net);
		CVertex *new_v = new CVertex (new_c, p.x, p.y);
		new_c->Start(new_v);
		m_sel.RemoveAll();
		m_sel.Add(new_v);
		net->GetWidth( &m_active_width );
		new_v->StartDraggingStub( pDC, p.x, p.y, m_active_layer, m_active_width, m_active_layer, 2, m_inflection_mode );
		if( m_doc->m_bHighlightNet )
			m_highlight_net = net,
			net->Highlight();
		SetCursorMode( CUR_DRAG_AREA_STUB );
		ShowSelectStatus();
		Invalidate( FALSE );
		ReleaseDC( pDC );
		// NB note that m_doc->ProjectModified() is not called.  So an undo record won't be saved until after the first seg is created.
	}

	else if( m_cursor_mode == CUR_DRAG_TEXT )
	{
		CPoint p = m_last_cursor_point;
		m_dlist->StopDragging();
		CText *t = m_sel.First()->ToText();
		int angle = t->m_angle + m_dlist->GetDragAngle();
		if( angle>270 )
			angle = angle - 360;
		int mirror = (t->m_bMirror + m_dlist->GetDragSide()) % 2;
		t->MustRedraw();
		t->Move( m_last_cursor_point.x, m_last_cursor_point.y, angle, mirror, t->m_bNegative, t->m_layer );
		m_doc->ProjectModified( TRUE );
		SelectItem(t);
		m_dragging_new_item = FALSE;
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
	goto goodbye;

cancel_selection_and_goodbye:
	m_dlist->StopDragging();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
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
	m_bLButtonDown = true;				// CPT r294. Basically double-clicking is a moribund concept in this program, and should be treated like 2 rapid
										// single clicks.  This statement ensures that we get the usual results when the OnLButtonUp() gets called 
										// momentarily
	CView::OnLButtonDblClk(nFlags, point);
}

// right mouse button
//
void CFreePcbView::OnRButtonDown(UINT nFlags, CPoint point)
{
	// ALSO USED TO CANCEL DRAGGING WHEN THE ESC KEY IS HIT.  (Sub-optimal system?)
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_RIGHT_CLICK);
	m_sel_offset = -1;							// CPT r294:  the current series of left-clicks has been interrupted...
	m_disable_context_menu = 1;
	CPcbItem *sel0 = m_sel.First();

	if( m_cursor_mode == CUR_DRAG_PART )
	{
		CPart *part = m_sel.First()->ToPart();
		part->CancelDragging();
		if( m_dragging_new_item )
		{
			CancelSelection();
			m_doc->UndoNoRedo();	// remove the part
		}
		else
			part->Highlight();
		m_dragging_new_item = FALSE;
	}

	else if( m_cursor_mode == CUR_DRAG_REF )
	{
		CText *t = m_sel.First()->ToText();
		t->CancelDragging();
		t->Highlight();
	}

	else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
	{
		CSeg *rat = m_sel.First()->ToSeg();
		rat->CancelDragging();
		rat->Highlight();
	}

	else if( m_cursor_mode == CUR_DRAG_VTX )
	{
		CVertex *vtx = m_sel.First()->ToVertex();
		vtx->CancelDragging();
		vtx->Highlight();
	}
	else if( m_cursor_mode == CUR_DRAG_TEE )
	{
		CTee *tee = m_sel.First()->ToTee();
		tee->CancelDragging();
		tee->Highlight();
	}
	else if( m_cursor_mode == CUR_MOVE_SEGMENT )
	{
		CSeg *seg = m_sel.First()->ToSeg();
		seg->CancelMoving();
		seg->Highlight();
	}
	else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
	{
		CSeg *seg = m_sel.First()->ToSeg();
		seg->CancelDraggingNewVertex();
		seg->Highlight();
	}
	else if( m_cursor_mode == CUR_DRAG_NEW_RAT )
		m_dlist->StopDragging();

	else if( m_cursor_mode == CUR_DRAG_STUB || m_cursor_mode == CUR_DRAG_AREA_STUB )
	{
		m_dlist->StopDragging();
		CancelHighlight();
		CVertex *sel = m_sel.First()->ToVertex();
		CNet *net = sel->m_net;
		if (sel->m_con->NumSegs() > 0)
		{
			CVertex *tail = sel->m_con->tail;
			//** AMW3 removed automatic generation of via on end of stub
#if 0
			if ( net->NetAreaFromPoint(tail->x, tail->y, LAY_PAD_THRU) )		// "LAY_PAD_THRU" so that we find areas on any layer at (tail->x,tail->y)
				tail->ForceVia();
			// if( m_doc->m_vis[LAY_RAT_LINE] )			
			//	net->OptimizeConnections();										// CPT2 seems like this might cause trouble with undo?
#endif
			m_doc->Redraw();
			SelectItem(tail);
		}
		else
			m_doc->UndoNoRedo();
	}

	else if( m_cursor_mode == CUR_DRAG_RAT )
	{
		m_dlist->CancelHighlight();
		CSeg *rat = m_sel.First()->ToSeg();
		rat->CancelDragging();
		rat->Highlight();
	}

	else if( m_cursor_mode == CUR_DRAG_TEXT )
	{
		CText *t = m_sel.First()->ToText();
		t->CancelDragging();
		if( m_dragging_new_item )
		{
			t->Undraw();
			m_doc->m_tlist->texts.Remove( t );
			CancelSelection();
			m_dragging_new_item = 0;
		}
	}
	else if( m_cursor_mode == CUR_ADD_AREA || m_cursor_mode == CUR_ADD_POLY_CUTOUT
			 || m_cursor_mode == CUR_ADD_SMCUTOUT || m_cursor_mode == CUR_ADD_BOARD )
	{
		m_dlist->StopDragging();
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_POLY_INSERT )
	{
		CSide *s = m_sel.First()->ToSide();
		s->CancelDraggingNewCorner();
		s->Highlight();
	}
	else if( m_cursor_mode == CUR_DRAG_POLY_MOVE )
	{
		CCorner *c = m_sel.First()->ToCorner();
		c->CancelDragging();
		c->Highlight();
	}
	else if( m_cursor_mode == CUR_DRAG_POLY_1
		  || (m_cursor_mode == CUR_DRAG_POLY && m_drag_contour->NumCorners()<3) )
	{
		m_dlist->StopDragging();
		m_drag_contour->GetPolyline()->Undraw();
		CancelSelection();
	}
	else if( m_cursor_mode == CUR_DRAG_POLY )
	{
		if (m_poly_drag_mode == CUR_ADD_POLY_CUTOUT)
			FinishAddPolyCutout();
		else
			FinishAddPoly();
		m_dlist->StopDragging();
		CancelSelection();
	}

	else if( m_cursor_mode == CUR_DRAG_GROUP )
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_doc->m_vis[LAY_RAT_LINE] ),
		CancelDraggingGroup();
	else if( m_cursor_mode == CUR_DRAG_GROUP_ADD )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_doc->m_vis[LAY_RAT_LINE] );
		m_doc->UndoNoRedo();
	}
	else if( m_cursor_mode == CUR_DRAG_MEASURE_1 || m_cursor_mode == CUR_DRAG_MEASURE_2 )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
	}
	else if( m_cursor_mode == CUR_MOVE_ORIGIN )					// CPT2 added (oversight)
	{
		m_dlist->StopDragging();
		CancelSelection();
	}
	else if (m_cursor_mode == CUR_AREA_START_TRACE)
	{
		m_dlist->StopDragging();
		CancelSelection();
	}
	else
		m_disable_context_menu = 0;
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
	if( nChar == 'D' )
	{
		// 'd'
		m_doc->m_drelist->MakeHollowCircles();
		Invalidate( FALSE );
	}
	else if( nChar == VK_SHIFT || nChar == VK_CONTROL )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB || m_cursor_mode == CUR_DRAG_AREA_STUB)
		{
			// routing a trace segment, set mode
			if( nChar == VK_CONTROL )
				m_snap_mode = SM_GRID_POINTS;								// CPT2 TODO this business is currently dead weight
			if( nChar == VK_SHIFT && m_doc->m_snap_angle == 45 )
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
	m_bHandlingKeyPress = true;
	if( nChar == 'D' )
	{
		m_doc->m_drelist->MakeSolidCircles();
		Invalidate( FALSE );
	}
	else if( nChar == VK_SHIFT || nChar == VK_CONTROL )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB || m_cursor_mode == CUR_DRAG_AREA_STUB )
		{
			// routing a trace segment, set mode
			/* CPT2 disabled SM_GRID_LINES because its code is dysfunctional and I don't really get it.
			if( nChar == VK_CONTROL )
				m_snap_mode = SM_GRID_LINES; */
			if( nChar == VK_SHIFT && m_doc->m_snap_angle == 45 )
				m_inflection_mode = IM_45_90;
			m_dlist->SetInflectionMode( m_inflection_mode );
			Invalidate( FALSE );
		}
	}
	else
		HandleKeyPress( nChar, nRepCnt, nFlags );
	m_bHandlingKeyPress = false;

	// don't pass through SysKey F10
	if( nChar != 121 )
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFreePcbView::FinishArrowKey(int x, int y, int dx, int dy) 
{
	// CPT2: Helper for HandleKeyPress() below.  When user hits an arrow key, that routine moves the
	// relevant item, then calls here to redisplay and tidy up.
	if (!m_lastKeyWasArrow)
		m_totalArrowMoveX = 0,
		m_totalArrowMoveY = 0;
	m_totalArrowMoveX += dx;
	m_totalArrowMoveY += dy;
	m_lastArrowPosX = x, m_lastArrowPosY = y;
	// CPT2:  The second arg in ProjectModified() means that repeated arrow hits get combined into a single undo event:
	m_doc->ProjectModified( true, m_lastKeyWasArrow );		
	HighlightSelection();
	if (x==INT_MAX)
		// Show dx/dy only
		ShowRelativeDistance(m_totalArrowMoveX, m_totalArrowMoveY);
	else
		ShowRelativeDistance(x, y, m_totalArrowMoveX, m_totalArrowMoveY);
	m_lastKeyWasArrow = true;
}

// Key on keyboard pressed down
// CPT2 converted.

void CFreePcbView::HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( m_bDraggingRect )
		return;

	bool bShiftKeyDown = (GetKeyState(VK_SHIFT)&0x8000) != 0;
	bool bCtrlKeyDown = (GetKeyState(VK_CONTROL)&0x8000) != 0;

	CPoint p;
	GetCursorPos( &p );				// cursor pos in screen coords
	p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
	if( nChar == VK_HOME )
		{ OnViewAllElements(); return; }
	else if (HandlePanAndZoom(nChar, p))
		return;

#ifdef CPT2_LOG
	if (!m_bReplayMode && nChar==VK_F11)
		{ ReplayLoad(); return; }
	if (m_bReplayMode && !m_bReplaying)
		{ Replay(); return; }
	Log(LOG_KEY, (int*) &nChar);
	LogXtra(&bShiftKeyDown, &bCtrlKeyDown);
#endif

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

	if( nChar == 'F' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_TEE_SELECTED ) )
	{
		// force via at a vertex/tee
		OnVertexAddVia();
		return;
	}

	if( nChar == 'U' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_TEE_SELECTED ) )
	{
		// unforce via
		OnVertexRemoveVia();
		return;
	}

	if( nChar == 'T' || nChar == 'C' )
	{ 
		// CPT:  'T' key now toggles back and forth between selected item and trace.
		// CPT2:  Am now offering 'C' as an alternate to 'T'.  I'm developing the opinion that the UI should use "connect"/"connection" to mean
		//  a contiguous collection of vertices/segs, and "trace" to mean the opposite of "ratline" (i.e., a routed seg).
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
				|| m_cursor_mode == CUR_TEE_SELECTED
				|| m_cursor_mode == CUR_SEG_SELECTED
				|| m_cursor_mode == CUR_CONNECT_SELECTED 
				|| m_cursor_mode == CUR_RAT_SELECTED 
				|| m_cursor_mode == CUR_PAD_SELECTED 
				|| m_cursor_mode == CUR_AREA_CORNER_SELECTED 
				|| m_cursor_mode == CUR_AREA_SIDE_SELECTED
				|| m_cursor_mode == CUR_DRAG_RAT 
				|| m_cursor_mode == CUR_DRAG_STUB
				|| m_cursor_mode == CUR_DRAG_AREA_STUB
				|| m_cursor_mode == CUR_DRAG_NEW_RAT 
				|| m_cursor_mode == CUR_DRAG_VTX 
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
			CSeg *seg = m_sel.First()->ToSeg();
			CConnect *c = seg->m_con;
			CNet *net = seg->m_net;
			CSeg *remove = m_dir==0? seg->preVtx->preSeg: seg->postVtx->postSeg;
			if (remove)
			{
				c->SaveUndoInfo();
				m_active_layer = remove->m_layer;
				CancelHighlight();
				remove->RemoveMerge(!m_dir);
				m_doc->ProjectModified(true);
				SelectItem(seg);
				CVertex *start = m_dir==0? seg->preVtx: seg->postVtx;
				m_last_mouse_point.x = start->x;
				m_last_mouse_point.y = start->y;
				CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
				SetCursorPos( p.x, p.y );
				m_dlist->ChangeRoutingLayer( pDC, m_active_layer, LAY_SELECTION, 0 );
				OnRatlineRoute(false);										// Set the dlist dragging mode, highlight the net, show the active layer, etc.
			}
		}

		else if( m_cursor_mode == CUR_DRAG_STUB || m_cursor_mode == CUR_DRAG_AREA_STUB )
		{
			// routing stub trace
			CVertex *sel = m_sel.First()->ToVertex();
			CConnect *c = sel->m_con;
			CNet *net = sel->m_net;
			c->SaveUndoInfo();
			CancelHighlight();
			m_dlist->StopDragging();
			bool bConnectRemoved = false;
			if (c->NumSegs() < 2) 
				c->Remove(),
				bConnectRemoved = true;
			else
				bConnectRemoved = c->tail->preSeg->RemoveBreak();
			m_doc->Redraw();
			if (bConnectRemoved)
				CancelSelection();
			else 
			{
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
		else if (bCtrlKeyDown && nChar==VK_RIGHT)
		{
			AngleDown();
			return;
		}
		else if (bCtrlKeyDown && nChar==VK_LEFT)
		{
			AngleUp();
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
			OnAddPart();
		else if (fk == FK_ADD_SMCUTOUT)
			OnAddSoldermaskCutout();
		else if (fk == FK_ADD_BOARD)
			OnAddBoardOutline();
		else if( fk == FK_REDO_RATLINES )
		{
			m_doc->m_nlist->OptimizeConnections();					// Takes care of saving its own undo info
			m_doc->ProjectModified( true );
		}
		break;

	case CUR_PART_SELECTED:
		if( fk == FK_ARROW )
		{
			CPart *part = m_sel.First()->ToPart();
			if( part->glued )
			{
				CString s ((LPCSTR) IDS_ThisPartIsGluedDoYouWantToUnglueIt);
				int ret = AfxMessageBox( s, MB_YESNO );
				if( ret == IDYES )
					part->glued = 0;
				else
					return;
			}
			part->SaveUndoInfo();
			CancelHighlight();
			part->Move( part->x+dx, part->y+dy, part->angle, part->side );
			part->PartMoved( dx, dy );	// CPT
			if( m_doc->m_vis[LAY_RAT_LINE] )
				part->OptimizeConnections();
			FinishArrowKey(part->x, part->y, dx, dy);
		}
		else if( fk == FK_DELETE_PART || nChar == 46 )
			OnPartDelete();
		else if( fk == FK_EDIT_PART )
			OnPartProperties();
		else if( fk == FK_EDIT_FOOTPRINT )
			OnPartEditThisFootprint();
		else if( fk == FK_GLUE_PART )
			OnPartGlue();
		else if( fk == FK_UNGLUE_PART )
			OnPartUnglue();
		else if( fk == FK_MOVE_PART )
			OnPartMove();
		else if( fk == FK_ROTATE_CW )
			OnPartRotateCW();
		else if( fk == FK_ROTATE_CCW )
			OnPartRotateCCW();
		else if( fk == FK_REDO_RATLINES )
			OnPartOptimize();
		else if (fk == FK_SIDE)					// CPT2 added
			OnPartChangeSide();
		break;

	case CUR_REF_SELECTED:
	case CUR_VALUE_SELECTED:
		if( fk == FK_ARROW )
		{
			CText *t = m_sel.First()->ToText();
			t->SaveUndoInfo();
			CancelHighlight();
			CPoint ref_pt = t->GetAbsolutePoint();
			ref_pt.x += dx;
			ref_pt.y += dy;
			t->MoveRelative( ref_pt.x, ref_pt.y );
			FinishArrowKey(ref_pt.x, ref_pt.y, dx, dy);
		}
		else if( fk == FK_SET_PARAMS )
			OnRefProperties();
		else if( fk == FK_MOVE_REF || fk == FK_MOVE_VALUE )
			OnRefMove();
		else if( fk == FK_ROTATE_CW )
			OnRefRotateCW();
		else if( fk == FK_ROTATE_CCW )
			OnRefRotateCCW();
		break;

	case CUR_RAT_SELECTED:
		if( fk == FK_SET_TRACE_WIDTH )
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
		else if( fk == FK_DELETE_SEGMENT || nChar == 46 )		// CPT2 TODO.  I propose it makes sense to have the delete key remove the seg, not
			OnSegmentDelete();									// the whole trace as before
		else if( fk == FK_DELETE_CONNECT )
			OnDeleteConnection();
		else if( fk == FK_REDO_RATLINES )
			OnOptimize();
		break;

	case  CUR_SEG_SELECTED:
		if( fk == FK_ARROW )
		{
			CSeg *seg = m_sel.First()->ToSeg();
			if (seg->preVtx->pin || seg->postVtx->pin || seg->preVtx->tee || seg->postVtx->tee)
			{
				if (m_doc->m_bErrorSound)
					PlaySound( TEXT("CriticalStop"), 0, 0 );
				break;
			}
			CancelHighlight();

			// 1. Move the line defined by the segment.
			m_from_pt.x = seg->preVtx->x;
			m_from_pt.y = seg->preVtx->y;
			m_to_pt.x = seg->postVtx->x;
			m_to_pt.y = seg->postVtx->y;

			// CPT2.  The old code was I think problematic for the case that this seg is at the beginning of a trace.
			// Let's try:
			CSeg *prev = seg->preVtx->preSeg;
			CSeg *next = seg->postVtx->postSeg;
			if (prev)
				m_last_pt.x = prev->preVtx->x,
				m_last_pt.y = prev->preVtx->y;
			if (next)
				m_next_pt.x = next->postVtx->x,
				m_next_pt.y = next->postVtx->y;

			// 2. Move the endpoints of (xi, yi), (xf, yf) of the line by the mouse movement. This
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

			// 3. Find the intercept between the extended segment in motion and the leading segment IF ANY.
			if (prev)
			{
				double d_new_xi, d_new_yi;
				FindLineIntersection(m_last_pt.x, m_last_pt.y, m_from_pt.x, m_from_pt.y,
									 new_xi, new_yi, new_xf, new_yf,
									 &d_new_xi, &d_new_yi);
				i_nudge_xi = floor(d_new_xi + .5);
				i_nudge_yi = floor(d_new_yi + .5);
			}

			// 4. Find the intercept between the extended segment in motion and the trailing segment:
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
				seg->m_con->SaveUndoInfo();
				seg->preVtx->Move( i_nudge_xi, i_nudge_yi );
				seg->postVtx->Move( i_nudge_xf, i_nudge_yf );
			}
			else
			{
				if (m_doc->m_bErrorSound)
					PlaySound( TEXT("CriticalStop"), 0, 0 );
				seg->Highlight(); 
				break; 
			}

			// CPT2 TODO improve displayed values:
			FinishArrowKey(seg->preVtx->x, seg->preVtx->y, dx, dy);
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
		else if( fk == FK_DELETE_SEGMENT || nChar == 46 )		// CPT2 TODO.  I propose it makes sense to have the delete key remove the seg, not
			OnSegmentDelete();									// the whole trace as before
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT )
			OnDeleteConnection();
		else if( fk == FK_REDO_RATLINES )
			OnOptimize();
		break;

	case  CUR_VTX_SELECTED:
		if( fk == FK_ARROW )
		{
			CVertex *vtx = m_sel.First()->ToVertex();
			vtx->m_con->SaveUndoInfo();
			CancelHighlight();
			vtx->Move( vtx->x + dx, vtx->y + dy );
			FinishArrowKey(vtx->x, vtx->y, dx, dy);
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
		else if( fk == FK_ADD_VIA )
			OnVertexAddVia();
		else if( fk == FK_DELETE_VIA )
			OnVertexRemoveVia();
		else if( fk == FK_DELETE_VERTEX || nChar == 46 )
			OnVertexDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT )
			OnDeleteConnection();
		else if( fk == FK_REDO_RATLINES )
			OnOptimize();
		break;

	case CUR_TEE_SELECTED:
		if( fk == FK_ARROW )
		{
			CTee *tee = m_sel.First()->ToTee();
			CVertex *first = tee->vtxs.First();
			ASSERT(first);
			first->m_net->SaveUndoInfo( CNet::SAVE_CONNECTS );
			CancelHighlight();
			tee->Move( first->x + dx, first->y + dy );
			FinishArrowKey( first->x, first->y, dx, dy );
		}
		else if( fk == FK_SET_POSITION )
			OnTeeProperties();
		else if( fk == FK_VERTEX_PROPERTIES )
			OnTeeProperties();
		else if( fk == FK_START_TRACE )
			OnVertexStartTrace();
		else if( fk == FK_ADD_CONNECT )
			OnVertexStartRatline();
		else if( fk == FK_MOVE_VERTEX )
			OnTeeMove();
		else if( fk == FK_DELETE_VERTEX || nChar == 46 )
			OnTeeDelete();
		else if( fk == FK_REDO_RATLINES )
			OnOptimize();
		else if( fk == FK_ADD_VIA )
			OnVertexAddVia();
		else if( fk == FK_DELETE_VIA )
			OnVertexRemoveVia();
		break;

	case  CUR_CONNECT_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnConnectSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnConnectChangeLayer();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnOptimize();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnDeleteConnection();
		break;

	case  CUR_NET_SELECTED:					// CPT2 TODO is this defunct?
		if( fk == FK_SET_WIDTH )
			OnNetSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnNetChangeLayer();
		else if( fk == FK_EDIT_NET )
			OnNetEditnet();
		else if( fk == FK_REDO_RATLINES )
			OnOptimize();
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
			CText *text = m_sel.First()->ToText();
			text->SaveUndoInfo();
			CancelHighlight();
			text->Move( text->m_x + dx, text->m_y + dy,
						text->m_angle, text->m_bMirror,	text->m_bNegative, text->m_layer );
			FinishArrowKey(text->m_x, text->m_y, dx, dy);
		}
		else if( fk == FK_EDIT_TEXT )
			OnTextEdit();
		else if( fk == FK_MOVE_TEXT )
			OnTextMove();
		else if( fk == FK_DELETE_TEXT || nChar == 46 )
			OnTextDelete();
		else if ( fk == FK_ROTATE_CW )
			OnTextRotateCW();
		else if ( fk == FK_ROTATE_CCW )
			OnTextRotateCCW();
		break;

	case CUR_AREA_SIDE_SELECTED:
	case CUR_BOARD_SIDE_SELECTED:
	case CUR_SMCUTOUT_SIDE_SELECTED:
		if( fk == FK_ARROW )
		{
			CSide *s = m_sel.First()->ToSide();
			CPolyline *poly = s->GetPolyline();
			poly->SaveUndoInfo();
			MoveGroup(dx, dy);
			FinishArrowKey(INT_MAX, INT_MAX, dx, dy);
		}
		else if( fk == FK_POLY_STRAIGHT )
			OnPolySideConvertToStraightLine();
		else if( fk == FK_POLY_ARC_CW )
			OnPolySideConvertToArcCw();
		else if( fk == FK_POLY_ARC_CCW )
			OnPolySideConvertToArcCcw();
		else if( fk == FK_ADD_CORNER )
			OnPolySideAddCorner();
		else if( fk == FK_DELETE_OUTLINE || fk == FK_DELETE_AREA )
			OnPolyDelete();
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if (fk == FK_EDIT_CUTOUT)
			OnSmEdit();
		else if( fk == FK_EXCLUDE_RGN )
			OnPolyAddCutout();
		else if( fk == FK_REMOVE_CONTOUR )
			OnPolyDeleteCutout();
		else if( nChar == 46 )
		{
			if (m_sel.First()->ToSide()->IsOnCutout())
				OnPolyDeleteCutout();
			else
				OnPolyDelete();
		}
		else if (fk == FK_START_TRACE)
			OnAreaStartTrace();
		break;

	case CUR_AREA_CORNER_SELECTED:
	case CUR_BOARD_CORNER_SELECTED:
	case CUR_SMCUTOUT_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			CCorner *c = m_sel.First()->ToCorner();
			CPolyline *poly = c->GetPolyline();
			poly->SaveUndoInfo();
			CancelHighlight();
			p = CPoint (c->x+dx, c->y+dy);
			c->Move( p.x, p.y );
			if (!poly->PolygonModified( FALSE, TRUE ))
				// error
				m_doc->OnEditUndo();
			else
			{
				m_doc->Redraw();
				TryToReselectCorner( p.x, p.y );
				FinishArrowKey(p.x, p.y, dx, dy);
			}
		}
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if (fk == FK_EDIT_CUTOUT)
			OnSmEdit();
		else if( fk == FK_SET_POSITION )
			OnPolyCornerEdit();
		else if( fk == FK_MOVE_CORNER )
			OnPolyCornerMove();
		else if( fk == FK_DELETE_CORNER || nChar == 46 )
			OnPolyCornerDelete();
		else if( fk == FK_DELETE_AREA || fk == FK_DELETE_OUTLINE )
			OnPolyDelete();
		else if( fk == FK_EXCLUDE_RGN )
			OnPolyAddCutout();
		else if( fk == FK_REMOVE_CONTOUR )
			OnPolyDeleteCutout();
		break;

	case CUR_DRE_SELECTED:
		if( nChar == 46 )
		{
			CDre *d = m_sel.First()->ToDRE();
			d->Remove();
			m_doc->ProjectModified(true);
			CancelSelection();
		}
		else
		{
			// CPT2 new: arrow keys/function keys for moving through the sorted list of dre's.
			CDre *d = m_sel.First()->ToDRE();
			CDre *d2 = NULL;
			if (fk==FK_ARROW && dx>0 || fk==FK_NEXT_DRE)
				d2 = d->next;
			else if (fk==FK_ARROW && dx<0 || fk==FK_PREV_DRE)
				d2 = d->prev;
			else if (fk==FK_ARROW && dy<0 || fk==FK_LAST_DRE) 
				d2 = m_doc->m_drelist->tail;
			else if (fk==FK_ARROW && dy>0 || fk==FK_FIRST_DRE)
				d2 = m_doc->m_drelist->head;
			if (d2)
			{
				CancelSelection();
				SelectItem(d2);
				CDLElement * dl_sel = d2->dl_sel;
				int xc = m_dlist->Get_x( dl_sel );
				int yc = m_dlist->Get_y( dl_sel );
				m_org_x = xc - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
				m_org_y = yc - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
				CRect screen_r;
				GetWindowRect( &screen_r );
				m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
					m_org_x, m_org_y );
				CPoint p(xc, yc);
				p = m_dlist->PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
			}
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
			}
			SaveUndoInfoForGroup();
			MoveGroup( dx, dy );
			FinishArrowKey(INT_MAX, INT_MAX, dx, dy);
		}
		else if( fk == FK_MOVE_GROUP )
			OnGroupMove();
		else if( fk == FK_DELETE_GROUP || nChar == 46 )
			OnGroupDelete();
		else if(fk == FK_ROTATE_CW)								// CPT2 TODO allow ccw rotation also
			OnGroupRotate();
		else if (fk == FK_ROTATE_CCW)
			OnGroupRotateCCW();
		break;

	case CUR_DRAG_RAT:
	case CUR_DRAG_STUB:
	case CUR_DRAG_AREA_STUB:
		if( fk == FK_COMPLETE )
			// CUR_DRAG_RAT only.
			OnRatlineComplete();
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
	case CUR_DRAG_VTX_INSERT:
		if (fk==FK_RGRID_UP)
			RoutingGridUp();
		else if (fk==FK_RGRID_DOWN)
			RoutingGridDown();
		break;
	// end CPT

	case  CUR_DRAG_PART:
		if( fk == FK_ROTATE_CW )
			m_dlist->IncrementDragAngle( pDC, false );
		else if( fk == FK_ROTATE_CCW )
			m_dlist->IncrementDragAngle( pDC, true );
		else if( fk == FK_SIDE )
			m_dlist->FlipDragSide( pDC );
		break;

	case  CUR_DRAG_REF:
		if( fk == FK_ROTATE_CW )
			m_dlist->IncrementDragAngle( pDC, false );
		else if( fk == FK_ROTATE_CCW )
			m_dlist->IncrementDragAngle( pDC, true );
		break;

	case  CUR_DRAG_TEXT:
		if( fk == FK_ROTATE_CW )
			m_dlist->IncrementDragAngle( pDC, false );
		else if( fk == FK_ROTATE_CCW )
			m_dlist->IncrementDragAngle( pDC, true );
		break;

	case  CUR_DRAG_POLY:
	case  CUR_DRAG_POLY_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyline::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyline::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyline::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	default:
		break;
	}	// end switch

	ReleaseDC( pDC );
	if( !m_lastKeyWasArrow && !m_lastKeyWasGroupRotate )
		ShowSelectStatus();
}

// Mouse moved
//
void CFreePcbView::OnMouseMove(UINT nFlags, CPoint point)
{
	CCommonView::OnMouseMove(nFlags, point);
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
	if (m_cursor_mode != CUR_DRAG_NEW_RAT && m_cursor_mode != CUR_DRAG_RAT_PIN) 
		return;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain ) 
		return;
	CString pin_name = "", connect_to ((LPCSTR) IDS_ConnectTo), cant_connect ((LPCSTR) IDS_PinNetCantConnect);
	CPoint p = m_dlist->WindowToPCB( point );
	// CPT r294:  new args for TestSelect()...
	CArray<CHitInfo> hit_info;
	int num_hits = m_dlist->TestSelect( p.x, p.y, &hit_info, bitPin );
	if( num_hits > 0 )
	{
		CNet *net = m_sel.First()->GetNet();
		CPin *pin = hit_info[0].item->ToPin();			// hit_info[0] is the highest-priority hit
		if (!pin)
			;
		else if (net && pin->net && pin->net!=net)
			// it's a pin but we can't connect to it
			pin_name.Format(cant_connect, pin->part->ref_des, pin->pin_name, net->name);
		else
			// hit on pin
			pin_name = connect_to + pin->part->ref_des + "." + pin->pin_name + "?";
	}
	// no hit on pin
	pMain->DrawStatus(3, &pin_name); 
}


/////////////////////////////////////////////////////////////////////////
// Utility functions
//


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
			m_fkey_option[4] = FK_ADD_BOARD;				// CPT2 new
			m_fkey_option[5] = FK_ADD_SMCUTOUT;				// CPT2 new
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
		m_fkey_option[4] = FK_ROTATE_CW;
		m_fkey_option[5] = FK_ROTATE_CCW;
		m_fkey_option[6] = FK_SIDE;
		m_fkey_option[7] = FK_DELETE_PART;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_REF_SELECTED:
		m_fkey_option[0] = FK_SET_PARAMS;
		m_fkey_option[3] = FK_MOVE_REF;
		m_fkey_option[4] = FK_ROTATE_CW;				// CPT2.  Reorganizing function keys for rotation.  Where possible F5=rotate cw, F6=rotate ccw
		m_fkey_option[5] = FK_ROTATE_CCW;
		break;

	case CUR_VALUE_SELECTED:
		m_fkey_option[0] = FK_SET_PARAMS;
		m_fkey_option[3] = FK_MOVE_VALUE;
		m_fkey_option[4] = FK_ROTATE_CW;
		m_fkey_option[5] = FK_ROTATE_CCW;
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
		m_fkey_option[4] = FK_ROTATE_CW;
		m_fkey_option[5] = FK_ROTATE_CCW;
		m_fkey_option[7] = FK_DELETE_TEXT;
		break;

	case CUR_BOARD_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		m_fkey_option[7] = FK_DELETE_OUTLINE;
		break;

	case CUR_BOARD_SIDE_SELECTED:
		{
			m_fkey_option[0] = FK_POLY_STRAIGHT;
			m_fkey_option[1] = FK_POLY_ARC_CW;
			m_fkey_option[2] = FK_POLY_ARC_CCW;
			int style = m_sel.First()->ToSide()->m_style;
			if( style == CPolyline::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
			m_fkey_option[7] = FK_DELETE_OUTLINE;
			break;
		}

	case CUR_AREA_CORNER_SELECTED:
	case CUR_SMCUTOUT_CORNER_SELECTED:
		// CPT2: Assimilated smcutout menu to the area menu.  To lessen confusion the f-key text "Add cutout" has been changed in
		// all cases to "Exclude region", while "Delete cutout" has become "Remove contour"
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_EDIT_AREA;			// CPT2 was F2, now F5 to be consistent with selected sides
		m_fkey_option[5] = FK_DELETE_CORNER;
		if (m_sel.First()->ToCorner()->IsOnCutout())
			m_fkey_option[6] = FK_REMOVE_CONTOUR;
		else
			m_fkey_option[6] = FK_EXCLUDE_RGN;
		if (mode == CUR_AREA_CORNER_SELECTED)
			m_fkey_option[7] = FK_DELETE_AREA;
		else
			m_fkey_option[7] = FK_DELETE_OUTLINE;
		break;

	case CUR_AREA_SIDE_SELECTED:
	case CUR_SMCUTOUT_SIDE_SELECTED:
		{
			// CPT2:  Also assimilated these 2 menus.
			m_fkey_option[0] = FK_POLY_STRAIGHT;
			m_fkey_option[1] = FK_POLY_ARC_CW;
			m_fkey_option[2] = FK_POLY_ARC_CCW;
			int style = m_sel.First()->ToSide()->m_style;
			if( style == CPolyline::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
			if (mode == CUR_AREA_SIDE_SELECTED)
				m_fkey_option[4] = FK_EDIT_AREA;
			else
				m_fkey_option[4] = FK_EDIT_CUTOUT;
			m_fkey_option[5] = FK_START_TRACE;
			if( m_sel.First()->ToSide()->IsOnCutout() )
				m_fkey_option[6] = FK_REMOVE_CONTOUR;
			else
				m_fkey_option[6] = FK_EXCLUDE_RGN;
			if (mode == CUR_AREA_SIDE_SELECTED)
				m_fkey_option[7] = FK_DELETE_AREA;
			else
				m_fkey_option[7] = FK_DELETE_OUTLINE;
			break;
		}
		break;


	case CUR_SEG_SELECTED:
		{
			CSeg *seg = m_sel.First()->ToSeg();
			CConnect *con = seg->m_con;
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
			CSeg *seg = m_sel.First()->ToSeg();
			CConnect *c = seg->m_con;
			m_fkey_option[0] = FK_SET_TRACE_WIDTH;
			if( c->locked )
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
			if (!c->head->IsLooseEnd() && !c->tail->IsLooseEnd())
				m_fkey_option[5] = FK_UNROUTE_TRACE;
			m_fkey_option[6] = FK_DELETE_SEGMENT;
			m_fkey_option[7] = FK_DELETE_CONNECT;
			m_fkey_option[8] = FK_REDO_RATLINES;
			break;
		}

	case CUR_VTX_SELECTED:
		{
			CVertex *vtx = m_sel.First()->ToVertex();
			CConnect *con = vtx->m_con;
			m_fkey_option[0] = FK_VERTEX_PROPERTIES;
			m_fkey_option[1] = FK_START_TRACE;
			m_fkey_option[2] = FK_ADD_CONNECT;
			m_fkey_option[3] = FK_MOVE_VERTEX;
			if( vtx->IsVia() )
				m_fkey_option[4] = FK_DELETE_VIA;
			else
				m_fkey_option[4] = FK_ADD_VIA;
			if (!con->head->IsLooseEnd() && !con->tail->IsLooseEnd())
				m_fkey_option[5] = FK_UNROUTE_TRACE;
			m_fkey_option[6] = FK_DELETE_VERTEX;
			m_fkey_option[7] = FK_DELETE_CONNECT;
			m_fkey_option[8] = FK_REDO_RATLINES;
			break;
		}

	case CUR_TEE_SELECTED:
		// CPT2, case is new.
		m_fkey_option[0] = FK_VERTEX_PROPERTIES;
		m_fkey_option[1] = FK_START_TRACE;
		m_fkey_option[2] = FK_ADD_CONNECT;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		if( m_sel.First()->IsVia() )
			m_fkey_option[4] = FK_DELETE_VIA;
		else
			m_fkey_option[4] = FK_ADD_VIA;
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
		m_fkey_option[3] = FK_MOVE_GROUP;
		m_fkey_option[4] = FK_ROTATE_CW;
		m_fkey_option[5] = FK_ROTATE_CCW;
		m_fkey_option[7] = FK_DELETE_GROUP;
		break;

	case CUR_DRE_SELECTED:
		m_fkey_option[1] = FK_FIRST_DRE;
		m_fkey_option[2] = FK_PREV_DRE;
		m_fkey_option[3] = FK_NEXT_DRE;
		m_fkey_option[4] = FK_LAST_DRE;
		break;


	case CUR_DRAG_PART:
		m_fkey_option[1] = FK_SIDE;
		m_fkey_option[2] = FK_ROTATE_CW;
		m_fkey_option[3] = FK_ROTATE_CCW;
		break;

	case CUR_DRAG_REF:
		m_fkey_option[2] = FK_ROTATE_CW;
		m_fkey_option[3] = FK_ROTATE_CCW;
		break;

	case CUR_DRAG_TEXT:
		m_fkey_option[2] = FK_ROTATE_CW;
		m_fkey_option[3] = FK_ROTATE_CCW;
		break;

	case CUR_DRAG_VTX:
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
	case CUR_DRAG_AREA_STUB:
        // CPT
        m_fkey_option[0] = FK_ACTIVE_WIDTH_DOWN;
        m_fkey_option[1] = FK_ACTIVE_WIDTH_UP;
        m_fkey_option[4] = FK_RGRID_DOWN;
        m_fkey_option[5] = FK_RGRID_UP;
		// END cpt
		break;

	case CUR_DRAG_POLY:
	case CUR_DRAG_POLY_1:
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
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	int u = m_units;
	CString x_str, y_str, w_str, hole_str, via_w_str, via_hole_str;
	CString str, str0;
	CPcbItem *sel = m_sel.First();

	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		str.LoadStringA(IDS_NoSelection);
		break;

	case CUR_DRE_SELECTED:
		str.Format( "DRE %s", m_sel.First()->ToDRE()->str );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
	case CUR_BOARD_CORNER_SELECTED:
		{
			CCorner *c = m_sel.First()->ToCorner();
			CPolyline *poly = c->GetPolyline();
			CString lay_str;
			if( poly->GetLayer() == LAY_SM_TOP )
				lay_str.LoadStringA(IDS_Top3);
			else if (poly->GetLayer() == LAY_SM_BOTTOM)
				lay_str.LoadStringA(IDS_Bottom2);
			::MakeCStringFromDimension( &x_str, c->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, c->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			int rsrc = m_cursor_mode==CUR_SMCUTOUT_CORNER_SELECTED? IDS_SolderMaskCutoutCorner: IDS_BoardOutlineCorner;
			CString str0 ((LPCSTR) rsrc);
			str.Format( str0, poly->UID(), lay_str, c->UID(), x_str, y_str, "" );
		}
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
	case CUR_BOARD_SIDE_SELECTED:
		{
			CSide *side = m_sel.First()->ToSide();
			CPolyline *poly = side->GetPolyline();
			CString style_str;
			if( side->m_style == CPolyline::STRAIGHT )
				style_str.LoadStringA(IDS_Straight);
			else if( side->m_style == CPolyline::ARC_CW )
				style_str.LoadStringA(IDS_ArcCw);
			else if( side->m_style == CPolyline::ARC_CCW )
				style_str.LoadStringA(IDS_ArcCcw);
			CString lay_str;
			if( poly->GetLayer() == LAY_SM_TOP )
				lay_str.LoadStringA(IDS_Top3);
			else if (poly->GetLayer() == LAY_SM_BOTTOM )
				lay_str.LoadStringA(IDS_Bottom2);
			int rsrc = m_cursor_mode==CUR_SMCUTOUT_SIDE_SELECTED? IDS_SolderMaskCutoutSide: IDS_BoardOutlineSide;
			CString str0 ((LPCSTR) rsrc);
			str.Format( str0, poly->UID(), lay_str, side->UID(), style_str, "" );
		}
		break;

	case CUR_PART_SELECTED:
		{
			CPart *part = sel->ToPart();
			CString side ((LPCSTR) IDS_Top);
			if( part->side )
				side.LoadStringA(IDS_Bottom);
			::MakeCStringFromDimension( &x_str, part->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, part->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			int rep_angle = ::GetReportedAngleForPart( part->angle, part->shape->m_centroid->m_angle, part->side );
			str0.LoadStringA(IDS_PartXYAngle);
			str.Format( str0, part->ref_des, part->shape->m_name, x_str, y_str, rep_angle, side );
		}
		break;

	case CUR_REF_SELECTED:
	case CUR_VALUE_SELECTED:
		{
			CText *t = sel->ToText();
			CPoint pt = t->GetAbsolutePoint();
			::MakeCStringFromDimension( &x_str, pt.x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, pt.y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str0.LoadStringA(m_cursor_mode==CUR_REF_SELECTED? IDS_ReferenceText2: IDS_ValueText2);
			str.Format( str0, x_str, y_str );
			break;
		}

	case CUR_PAD_SELECTED:
		{
			CPin *pin = sel->ToPin();
			::MakeCStringFromDimension( &x_str, pin->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, pin->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			if( pin->net )
			{
				// pad attached to net
				CString str0 ((LPCSTR) IDS_PinOnNet);
				str.Format( str0, pin->part->ref_des,
					pin->pin_name, pin->net->name, x_str, y_str, "" );
			}
			else
			{
				// pad not attached to a net
				CString str0 ((LPCSTR) IDS_PinUnconnected);
				str.Format( str0, pin->part->ref_des,
					pin->pin_name, x_str, y_str, "" );
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

			CSeg *seg = sel->ToSeg();
			CConnect *con = seg->m_con;
			con->GetStatusStr( &str );
			if (m_cursor_mode != CUR_DRAG_RAT && con->NumSegs() == 1 && con->segs.First()->m_layer == LAY_RAT_LINE)
				;
			else
			{
				seg->GetStatusStr( &str0, width );
				str += "; " + str0;
			}
			break;
		}

	case CUR_DRAG_STUB:
	case CUR_DRAG_AREA_STUB:
		{
			// CPT2.  In this mode, the selected object is always the starting vertex.  Just show connection info, plus active width
			CString con_str, w_str, str0;
			CConnect *c = sel->ToVertex()->m_con;
			c->GetStatusStr( &con_str );
			::MakeCStringFromDimension( &w_str, m_active_width, m_units, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str0.LoadStringA(IDS_W);
			str.Format(str0, con_str, w_str);
			break;
		}

	case CUR_VTX_SELECTED:
		{
			CString con_str, vtx_str;
			CVertex *v = sel->ToVertex();
			v->m_con->GetStatusStr( &con_str );
			v->GetStatusStr( &vtx_str );
			str = con_str + "; " + vtx_str;
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
			CConnect *c = sel->ToConnect();
			CString con_str;
			c->GetStatusStr( &con_str );
			// get length of trace
			CString len_str;
			double len = 0;
			CIter<CSeg> is (&c->segs);
			for (CSeg *s = is.First(); s; s = is.Next())
			{
				double dx = s->preVtx->x - s->postVtx->x;
				double dy = s->preVtx->y - s->postVtx->y;
				len += sqrt( dx*dx + dy*dy );
			}
			::MakeCStringFromDimension( &len_str, (int)len, u, TRUE, TRUE, FALSE, u==MIL?1:3 );
			CString str0 ((LPCSTR) IDS_Length);
			str.Format(str0, con_str, len_str);
		}
		break;

	case CUR_TEXT_SELECTED:
		{
			CText *t = sel->ToText();
			CString neg_str = "";
			if( t->m_bNegative )
				neg_str.LoadStringA(IDS_Neg);
			::MakeCStringFromDimension( &x_str, t->m_x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, t->m_y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			CString str0 ((LPCSTR) IDS_Text2);
			str.Format( str0, x_str, y_str );
			str += neg_str;
			break;
		}

	case CUR_AREA_CORNER_SELECTED:
		{
			CCorner *c = sel->ToCorner();
			CPolyline *p = c->contour->poly;
			CNet *net = c->GetNet();
			::MakeCStringFromDimension( &x_str, c->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, c->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			CString str0 ((LPCSTR) IDS_CopperAreaCorner);
			str.Format( str0, net->name, p->UID(), c->UID(), x_str, y_str, "" );
		}
		break;

	case CUR_AREA_SIDE_SELECTED:
		{
			CSide *side = sel->ToSide();
			CPolyline *p = side->contour->poly;
			CNet *net = side->GetNet();
			if (!side->IsOnCutout()) 
				str0.LoadStringA(IDS_CopperAreaEdge),
				str.Format( str0, net->name, p->UID(), side->UID() );
			else
				str0.LoadStringA(IDS_CopperAreaCutoutEdge),
				str.Format( str0, net->name, p->UID(), side->contour->UID(), side->UID() );
		}
		break;

	case CUR_GROUP_SELECTED:
		str.LoadStringA(IDS_GroupSelected);
		break;

	case CUR_DRAG_POLY:
	case CUR_DRAG_POLY_1:
		str.LoadStringA(IDS_PlacingCornerOfPolygon);
		break;

	case CUR_DRAG_PART:
		str0.LoadStringA(IDS_MovingPart);
		str.Format( str0, sel->ToPart()->ref_des );
		break;

	case CUR_DRAG_REF:
		str0.LoadStringA(IDS_MovingRefTextForPart);
		str.Format( str0, sel->ToText()->m_str );
		break;

	case CUR_DRAG_VTX:
		str0.LoadStringA(IDS_RoutingNet);
		str.Format( str0, sel->GetNet()->name );
		break;

	case CUR_DRAG_TEXT:
		str0.LoadStringA(IDS_MovingText);
		str.Format( str0, sel->ToText()->m_str );
		break;

	case CUR_ADD_AREA:
		str.LoadStringA(IDS_PlacingFirstCornerOfCopperArea);
		break;

	case CUR_DRAG_POLY_INSERT:
		str0.LoadStringA(IDS_InsertingPolylineCorner);
		str.Format( str0, sel->UID() );
		break;

	case CUR_DRAG_POLY_MOVE:
		str0.LoadStringA(IDS_MovingPolylineCorner);
		str.Format( str0, sel->UID() );
		break;

	case CUR_DRAG_NEW_RAT:
		if( CPin *p = sel->ToPin() )
			str0.LoadStringA(IDS_AddingConnectionToPin),
			str.Format( str0, p->part->ref_des, p->pin_name );
		else if( sel->IsVertex() )
			str0.LoadStringA(IDS_AddingBranchToTrace),
			str.Format( str0, sel->GetNet()->name,	sel->UID() );
		break;

	case CUR_AREA_START_TRACE:
		str0.LoadStringA(IDS_StartingTraceFromAreaNet);
		str.Format(str0, sel->GetArea()->UID(), sel->GetNet()->name);
		break;

	case CUR_DRAG_MEASURE_1:
		str.LoadStringA(IDS_MeasurementModeLeftClickToStart);
		break;

	}

	if (m_bReplaying)
		str = m_log_status;
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

	CString str, str0;
	if( m_active_layer == LAY_TOP_COPPER )
		str.LoadStringA( IDS_Top3 );
	else if( m_active_layer == LAY_BOTTOM_COPPER )
		str.LoadStringA( IDS_Bottom2 );
	else if( m_active_layer > LAY_BOTTOM_COPPER )
		str0.LoadStringA(IDS_Inner2),
		str.Format( str0, m_active_layer - LAY_BOTTOM_COPPER );
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
	CNet *highlight_net0 = m_highlight_net;
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
	// Now also sets the cursor mode based on what's in m_sel.  Also checks that the items in the selection are all valid, removing the invalid ones.
	// r317 Now a virtual function within CCommonView.
	CancelHighlight();
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())	
		if (i->IsOnPcb())
			i->Highlight();
		else
			m_sel.Remove(i);
	CPcbItem *first = m_sel.First();
	if (m_sel.GetSize()==0)
		SetCursorMode( CUR_NONE_SELECTED );
	else if (m_sel.GetSize()>=2)
		FindGroupCenter(),
		SetCursorMode( CUR_GROUP_SELECTED );
	else if (CDre *dre = first->ToDRE())
	{
		SetCursorMode( CUR_DRE_SELECTED );
		CPcbItem *item1 = CPcbItem::FindByUid(dre->item1);
		CPcbItem *item2 = CPcbItem::FindByUid(dre->item2);
		if (item1)
			item1->Highlight();
		if (item2)
			item2->Highlight();
	}
	else if( first->IsBoardCorner() )
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
	else if( first->IsBoardSide() )
		SetCursorMode( CUR_BOARD_SIDE_SELECTED );
	else if( first->IsSmCorner() )
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
	else if( first->IsSmSide() )
		SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
	else if( first->IsPart() )
		SetCursorMode( CUR_PART_SELECTED );
	else if( first->IsRefText() )
		SetCursorMode( CUR_REF_SELECTED );				// CPT2 NB ref and value-text selection is independent of part selection (except that 
														// selecting a part does highlight the ref and value)
	else if( first->IsValueText() )
		SetCursorMode( CUR_VALUE_SELECTED );
	else if( first->IsText() )
		SetCursorMode( CUR_TEXT_SELECTED );				// CPT2 NB check for IsRefText() and IsValueText() before checking IsText()...
	else if( first->IsPin() )
		SetCursorMode( CUR_PAD_SELECTED );
	else if( CSeg *s = first->ToSeg() )
		if (s->m_layer == LAY_RAT_LINE)
			SetCursorMode( CUR_RAT_SELECTED );
		else
			SetCursorMode( CUR_SEG_SELECTED );
	else if( CVertex *v = first->ToVertex() )
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
	else
		// nothing selected (CPT2: UNLIKELY I HOPE)
		SetCursorMode( CUR_NONE_SELECTED );

	ShowSelectStatus();
	m_lastKeyWasArrow = FALSE;
	m_lastKeyWasGroupRotate = false;
	Invalidate(false);
}

// cancel net highlight, reselect selected item if not dragging
void CFreePcbView::CancelHighlightNet()
{
	CancelHighlight();
	if( !CurDragging() )
		HighlightSelection();
	m_highlight_net = NULL;
}

// set trace width using dialog
// enter with:
//	mode = 0 if called with segment selected
//	mode = 1 if called with connection selected
//	mode = 2 if called with net selected
//
void CFreePcbView::SetWidth( int mode )
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CNet *net = m_sel.First()->GetNet();
	CConnect *c = m_sel.First()->GetConnect();
	CSeg * seg = m_sel.First()->ToSeg();
	DlgSetSegmentWidth dlg;
	int w, via_w, via_hole_w;
	if (!m_bReplaying)
	{
		// Set params for dialog, then launch it.  If we're doing a replay from a log file, the Log() statements below will load in the
		// correct values.
		dlg.m_w = &m_doc->m_w;
		dlg.m_v_w = &m_doc->m_v_w;
		dlg.m_v_h_w = &m_doc->m_v_h_w;
		dlg.m_init_w = m_doc->m_trace_w;
		dlg.m_init_via_w = m_doc->m_via_w;
		dlg.m_init_via_hole_w = m_doc->m_via_hole_w;
		if( mode == 0 )
		{
			int seg_w = seg->m_width;
			if( seg_w )
				dlg.m_init_w = seg_w;
			else if( net->def_w )
				dlg.m_init_w = net->def_w;
		}
		else if( net->def_w )
			dlg.m_init_w = net->def_w;
		dlg.m_mode = mode;
		int ret = dlg.DoModal();
		if (ret != IDOK) 
			{ LogCancel(); return; }
		w = dlg.m_width;
		via_w = dlg.m_via_width;
		via_hole_w = dlg.m_hole_width;
		if( dlg.m_tv == 3 )
			w = 0;
		else if( dlg.m_tv == 2 )
			via_w = 0;
	}
	Log(LOG_SET_WIDTH, &dlg.m_def, &dlg.m_apply);
	LogXtra(&w, &via_w, &via_hole_w);

	// Save undo info (for the whole net, or just the connect as appropriate)
	if (dlg.m_apply == 3 || dlg.m_def == 2)
		net->SaveUndoInfo( CNet::SAVE_CONNECTS );
	else
		c->SaveUndoInfo();

	// set default values for net or connection.
	if( dlg.m_def == 2 )
	{
		// set default for net
		if( w )
			net->def_w = w;
		if( via_w )
		{
			net->def_via_w = via_w;
			net->def_via_hole_w = via_hole_w;
		}
	}

	// apply new widths to net, connection or segment
	if( dlg.m_apply == 3 )
		// apply width to net
		net->SetWidth( w, via_w, via_hole_w );
	else if( dlg.m_apply == 2 )
		// apply width to connection
		c->SetWidth( w, via_w, via_hole_w );
	else if( dlg.m_apply == 1 )
		// apply width to segment
		seg->SetWidth( w, via_w, via_hole_w );
	m_doc->ProjectModified( TRUE );
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

	// CPT2 new system for potentially selecting a new item when user right-clicks.
	RightClickSelect(point);

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
		break;

	case CUR_PART_SELECTED:
		{
			pPopup = menu.GetSubMenu(CONTEXT_PART);
			ASSERT(pPopup != NULL);
			if( m_sel.First()->ToPart()->glued )
				pPopup->EnableMenuItem( ID_PART_GLUE, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_PART_UNGLUE, MF_GRAYED );
			break;
		}

	case CUR_REF_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_REF_TEXT);
		ASSERT(pPopup != NULL);
		break;

	case CUR_VALUE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VALUE_TEXT);
		ASSERT(pPopup != NULL);
		break;

	case CUR_PAD_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PAD);
		ASSERT(pPopup != NULL);
		if( m_sel.First()->GetNet() )
			pPopup->EnableMenuItem( ID_PAD_ADDTONET, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_PAD_DETACHFROMNET, MF_GRAYED );
		break;

	case CUR_SEG_SELECTED:
		{
			pPopup = menu.GetSubMenu(CONTEXT_SEGMENT);
			ASSERT(pPopup != NULL);
			CSeg *seg = m_sel.First()->ToSeg();
			CConnect *con = seg->m_con;
			if (seg->preVtx->pin || seg->postVtx->pin || seg->preVtx->tee || seg->postVtx->tee)
				pPopup->EnableMenuItem( ID_SEGMENT_MOVE, MF_GRAYED );
			if (seg->preVtx->IsLooseEnd() || seg->postVtx->IsLooseEnd())
				pPopup->EnableMenuItem( ID_SEGMENT_UNROUTE, MF_GRAYED );
			if (con->head->IsLooseEnd() || con->tail->IsLooseEnd())
				pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
			break;
		}

	case CUR_RAT_SELECTED:
		{
			pPopup = menu.GetSubMenu(CONTEXT_RATLINE);
			ASSERT(pPopup != NULL);
			CSeg *seg = m_sel.First()->ToSeg();
			CConnect *con = seg->m_con;
			if( con->locked )
				pPopup->EnableMenuItem( ID_RATLINE_LOCKCONNECTION, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_RATLINE_UNLOCKCONNECTION, MF_GRAYED );
			if (con->head->IsLooseEnd() || con->tail->IsLooseEnd())
				pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
			if (seg->preVtx->pin && seg->postVtx->pin || !seg->postVtx->pin && !seg->preVtx->pin)
				pPopup->EnableMenuItem( ID_RATLINE_CHANGEPIN, MF_GRAYED );
			break;
		}

	case CUR_VTX_SELECTED:
		{
			pPopup = menu.GetSubMenu(CONTEXT_VERTEX);
			ASSERT(pPopup != NULL);
			CVertex *vtx = m_sel.First()->ToVertex();
			CConnect *con = vtx->m_con;
			if( vtx->IsVia() )
				pPopup->EnableMenuItem( ID_VERTEX_ADDVIA, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_VERTEX_REMOVEVIA, MF_GRAYED ),
				pPopup->EnableMenuItem( ID_VERTEX_SETSIZE, MF_GRAYED );
			if (con->head->IsLooseEnd() || con->tail->IsLooseEnd())
				pPopup->EnableMenuItem( ID_VERTEX_UNROUTECONNECTION, MF_GRAYED );
			break;
		}

	case CUR_TEE_SELECTED:
		{
			pPopup = menu.GetSubMenu(CONTEXT_TEE);
			ASSERT(pPopup != NULL);
			CTee *tee = m_sel.First()->ToTee();
			if( tee->IsVia() )
				pPopup->EnableMenuItem( ID_TEE_ADDVIA, MF_GRAYED );
			else
				pPopup->EnableMenuItem( ID_TEE_REMOVEVIA, MF_GRAYED ),
				pPopup->EnableMenuItem( ID_TEE_SETSIZE, MF_GRAYED );
			break;
		}

	case CUR_CONNECT_SELECTED:
		{
			pPopup = menu.GetSubMenu(CONTEXT_CONNECT);
			ASSERT(pPopup != NULL);
			CConnect *con = m_sel.First()->GetConnect();
			if (con->head->IsLooseEnd() || con->tail->IsLooseEnd())
				pPopup->EnableMenuItem( ID_CONNECT_UNROUTETRACE, MF_GRAYED );
			break;
		}

	/* CPT2 Obsolete
	case CUR_NET_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NET);
		ASSERT(pPopup != NULL);
		break;
	*/

	case CUR_TEXT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_TEXT);
		ASSERT(pPopup != NULL);
		break;

	case CUR_AREA_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_CORNER);
		ASSERT(pPopup != NULL);
		if (m_sel.First()->ToCorner()->IsOnCutout())
			pPopup->EnableMenuItem( ID_AREACORNER_ADDCUTOUT, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_AREACORNER_DELETECUTOUT, MF_GRAYED );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SM_CORNER);
		ASSERT(pPopup != NULL);
		if (m_sel.First()->ToCorner()->IsOnCutout())
			pPopup->EnableMenuItem( ID_SMCORNER_EXCLUDEREGION, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_SMCORNER_REMOVECONTOUR, MF_GRAYED );
		break;

	case CUR_BOARD_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_BOARD_CORNER);
		ASSERT(pPopup != NULL);
		break;

	case CUR_AREA_SIDE_SELECTED:
	case CUR_SMCUTOUT_SIDE_SELECTED:
	case CUR_BOARD_SIDE_SELECTED:
		{
			int context = CONTEXT_AREA_EDGE;
			if (m_cursor_mode == CUR_SMCUTOUT_SIDE_SELECTED)
				context = CONTEXT_SM_SIDE;
			else if (m_cursor_mode == CUR_BOARD_SIDE_SELECTED)
				context = CONTEXT_BOARD_SIDE;
			pPopup = menu.GetSubMenu( context );
			ASSERT(pPopup != NULL);
			CSide *s = m_sel.First()->ToSide();
			if (context != CONTEXT_BOARD_SIDE)
				if (s->IsOnCutout())
					pPopup->EnableMenuItem( ID_POLYSIDE_EXCLUDEREGION, MF_GRAYED );
				else
					pPopup->EnableMenuItem( ID_POLYSIDE_REMOVECONTOUR, MF_GRAYED );
			int style = s->m_style;
			if( style == CPolyline::STRAIGHT )
			{
				int xi = s->preCorner->x, yi = s->preCorner->y;
				int xf = s->postCorner->x, yf = s->postCorner->y;
				if( xi == xf || yi == yf )
				{
					pPopup->EnableMenuItem( ID_POLYSIDE_CONVERTTOARC_CW, MF_GRAYED );
					pPopup->EnableMenuItem( ID_POLYSIDE_CONVERTTOARC_CCW, MF_GRAYED );
				}
				pPopup->EnableMenuItem( ID_POLYSIDE_CONVERTTOSTRAIGHT, MF_GRAYED );
			}
			else if( style == CPolyline::ARC_CW )
			{
				pPopup->EnableMenuItem( ID_POLYSIDE_CONVERTTOARC_CW, MF_GRAYED );
				pPopup->EnableMenuItem( ID_POLYSIDE_INSERTCORNER, MF_GRAYED );
			}
			else if( style == CPolyline::ARC_CCW )
			{
				pPopup->EnableMenuItem( ID_POLYSIDE_CONVERTTOARC_CCW, MF_GRAYED );
				pPopup->EnableMenuItem( ID_POLYSIDE_INSERTCORNER, MF_GRAYED );
			}
			break;
		}

	case CUR_GROUP_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_GROUP);
		ASSERT(pPopup != NULL);
		break;
	}

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
}

// move part
//
void CFreePcbView::OnPartMove()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CPart *part = m_sel.First()->ToPart();
	// check for glue
	if( part->glued && !m_bReplaying)
	{
		CString s ((LPCSTR) IDS_ThisPartIsGluedDoYouWantToUnglueIt);
		int ret = AfxMessageBox( s, MB_YESNO );
		if( ret != IDYES )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_PART_MOVE);
	// drag part
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint p;
	p.x = part->x;
	p.y = part->y;
	m_from_pt = p;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	BOOL bRatlines = m_doc->m_vis[LAY_RAT_LINE];
	part->StartDragging( pDC, bRatlines, m_doc->m_auto_ratline_disable, m_doc->m_auto_ratline_min_pins );
	SetCursorMode( CUR_DRAG_PART );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// add text string
//
void CFreePcbView::OnTextAdd()
{
	if (m_bReplayMode && !m_bReplaying)
		return;

	// create, initialize and show dialog
	CDlgAddText dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( 0, m_doc->m_num_layers, 1, &CString(""), m_units,
				LAY_SILK_TOP, 0, 0, 0, 0, 0, 0, 0 );
		dlg.m_num_layers = m_doc->m_num_layers;
		dlg.m_bDrag = 1;
		int ret = dlg.DoModal();
		if( ret == IDCANCEL )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_TEXT_ADD, &dlg.m_x, &dlg.m_y, &dlg.m_angle);
	LogXtra(&dlg.m_bMirror, &dlg.m_bNegative, &dlg.m_height, &dlg.m_width);
	LogXtra(&dlg.m_layer, &dlg.m_bDrag);
	LogXtra(&dlg.m_str);

	int x = dlg.m_x;
	int y = dlg.m_y;
	int mirror = dlg.m_bMirror;
	BOOL bNegative = dlg.m_bNegative;
	int angle = dlg.m_angle;
	int font_size = dlg.m_height;
	int stroke_width = dlg.m_width;
	int layer = dlg.m_layer;
	CString str = dlg.m_str;
	m_doc->m_vis[layer] = TRUE;
	m_dlist->SetLayerVisible( layer, TRUE );

	if( dlg.m_bDrag )
	{
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
		x = p.x, y = p.y;
	}

	CText *t = m_doc->m_tlist->AddText( x, y, angle, mirror, bNegative,
			layer, font_size, stroke_width, &str );
	t->MustRedraw();
	m_doc->ProjectModified(true);
	SelectItem(t);

	if( dlg.m_bDrag )
	{
		// set pDC to PCB coords
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		m_dragging_new_item = 1;
		t->StartDragging( pDC );
		SetCursorMode( CUR_DRAG_TEXT );
		ReleaseDC( pDC );
	}
}

// delete text ... enter with text selected
//
void CFreePcbView::OnTextDelete()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_TEXT_DELETE);
	CText *t = m_sel.First()->ToText();
	t->SaveUndoInfo();
	m_doc->m_tlist->texts.Remove(t);
	t->Undraw();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

// move text, enter with text selected
//
void CFreePcbView::OnTextMove()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_TEXT_MOVE);
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to text origin
	CText *t = m_sel.First()->ToText();
	t->SaveUndoInfo();
	CPoint p (t->m_x, t->m_y);
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start moving
	CancelHighlight();
	m_dragging_new_item = false;
	t->StartDragging(pDC);
	SetCursorMode( CUR_DRAG_TEXT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// glue part.  CPT2 updated (it was easy)
//
void CFreePcbView::OnPartGlue()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PART_GLUE);
	CPart *part = m_sel.First()->ToPart();
	part->SaveUndoInfo(false);
	part->glued = 1;
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// unglue part
//
void CFreePcbView::OnPartUnglue()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PART_UNGLUE);
	CPart *part = m_sel.First()->ToPart();
	part->SaveUndoInfo(false);
	part->glued = 0;
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// delete part
//
void CFreePcbView::OnPartDelete()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CPart *part = m_sel.First()->ToPart();
	if( part->glued && !m_bReplaying )
	{
		CString s ((LPCSTR) IDS_ThisPartIsGluedDoYouWantToUnglueIt);
		int ret = AfxMessageBox( s, MB_YESNO );
		if( ret == IDNO )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_PART_DELETE);

	// CPT2 TODO.  The distinction that was previously drawn between PartDeleted() and PartDisconnected() was hard for me to figure out.  The former
	// seems to leave in place notional pins within nets that refer to parts no longer in existence.  I'm not clear why that would be useful, and I 
	// can imagine it possibly causing trouble in some circumstances.
	// Here's a proposal for a distinction that I think might be more useful.
	int ret = IDNO;
	if (part->IsAttachedToConnects()) 
	{
		CString mess, s ((LPCSTR) IDS_DeletingPartDoYouWishToEraseAllTraces);
		mess.Format( s, part->ref_des );
		ret = AfxMessageBox( mess, MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return;
	}
	part->SaveUndoInfo();
	// now do it
	if( ret == IDYES )
		part->Remove(true);
	else if( ret == IDNO )
		part->Remove(false);
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

// optimize all nets to part
//
void CFreePcbView::OnPartOptimize()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PART_OPTIMIZE);
	CPart *part = m_sel.First()->ToPart();
	part->OptimizeConnections( FALSE );
	m_doc->ProjectModified( TRUE );
}

void CFreePcbView::OnPartProperties()
{
	// CPT2.  This used to be in class CFreePcbDoc, but I'm thinking CFreePcbView is more logical.
	if (m_bReplayMode && !m_bReplaying)
		return;
	CPart *part = m_sel.First()->ToPart();
	partlist_info pli;
	CDlgAddPart dlg;
	if (!m_bReplaying)
	{
		int ip = part->m_pl->ExportPartListInfo( &pli, part );
		dlg.Initialize( &pli, ip, TRUE, FALSE, FALSE, 0, m_doc, m_units );
		int ret = dlg.DoModal();
		if( ret != IDOK )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_PART_PROPERTIES, &dlg.m_drag_flag);
	LogPartListInfo( &pli );
	part->SaveUndoInfo();
	m_doc->m_plist->ImportPartListInfo( &pli, 0 );
	if( dlg.m_drag_flag )
		ASSERT(0);
	if( m_doc->m_vis[LAY_RAT_LINE] && !m_doc->m_auto_ratline_disable )
		m_doc->m_nlist->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}

void CFreePcbView::OnPartRefProperties()
{
	// CPT2 new.  For use in the part context menu
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PART_REF_PROPERTIES);
	CPart *part = m_sel.First()->ToPart();
	part->MustRedraw();
	part->m_ref->m_bShown = true;
	m_doc->Redraw();
	SelectItem(part->m_ref);
	OnRefProperties();
}

void CFreePcbView::OnPartValueProperties()
{
	// CPT2 new.  For use in the part context menu
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PART_VALUE_PROPERTIES);
	CPart *part = m_sel.First()->ToPart();
	part->MustRedraw();
	part->m_value->m_bShown = true;
	if (part->m_value->m_font_size == 0)
		part->m_value->m_font_size = part->m_ref->m_font_size;
	if (part->value_text == "")
		part->value_text = part->m_value->m_str = "VALUE";
	m_doc->Redraw();
	SelectItem(part->m_value);
	OnRefProperties();
}

// move ref. designator text for part
//
void CFreePcbView::OnRefMove()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_REF_MOVE);
	CText *t = m_sel.First()->ToText();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint cur_p = m_dlist->PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	t->StartDragging( pDC );
	SetCursorMode( CUR_DRAG_REF );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// optimize net for this pad
//
void CFreePcbView::OnPadOptimize()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PAD_OPTIMIZE);
	CPin *pin = m_sel.First()->ToPin();
	if (!pin->net)
		return;
	pin->net->OptimizeConnections( false );
	m_doc->ProjectModified( TRUE );
}

// start stub trace from this pad
// CPT2 converted.  Quite similar to OnVertexStartTrace().
void CFreePcbView::OnPadStartTrace()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PAD_START_TRACE);
	CPin *pin = m_sel.First()->ToPin();
	CNet *net = pin->net;
	if( !net )
	{
		// CPT2 new.  User can pick the net now.
		DlgAssignNet dlg;
		if (!m_bReplaying)
		{
			dlg.m_nlist = m_doc->m_nlist;
			int ret = dlg.DoModal();
			if( ret != IDOK )
				{ LogCancel(); return; }
		}
		LogXtra(&dlg.m_net_str);
		CString name = dlg.m_net_str;
		net = m_doc->m_nlist->GetNetPtrByName(&name);
		if( !net )
			// make new net
			net = new CNet(m_doc->m_nlist, name, 0, 0, 0);
		else
			net->SaveUndoInfo( CNet::SAVE_CONNECTS );
		net->AddPin(pin);
		net->AddPinsFromSyncFile();
	}
	else
		net->SaveUndoInfo( CNet::SAVE_CONNECTS );

	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	m_snap_angle_ref.x = pin->x;
	m_snap_angle_ref.y = pin->y;
	// now add new connection and first vertex.  The 1st vertex becomes the selection, which we refer to when we get to OnLButtonUp again
	// NB it's unusual to have as the selection a vertex belonging to a pin.
	CConnect *new_c = new CConnect (net);
	CVertex *new_v = new CVertex (new_c, pin->x, pin->y);
	new_v->pin = pin;
	new_c->Start(new_v);
	m_sel.RemoveAll();
	m_sel.Add(new_v);
	net->GetWidth( &m_active_width );								// AMW r267 added
	if (pin->GetLayer() != LAY_PAD_THRU)
		m_active_layer = pin->GetLayer();							// CPT2 bug fix, added this
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
	// NB note that m_doc->ProjectModified() is not called.  So an undo record won't be saved until after the first seg is created.
}

// attach this pad to a net
//
void CFreePcbView::OnPadAddToNet()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CPin *pin = m_sel.First()->ToPin();
	DlgAssignNet dlg;
	if (!m_bReplaying)
	{
		dlg.m_nlist = m_doc->m_nlist;
		int ret = dlg.DoModal();
		if (ret != IDOK)
			{ LogCancel(); return; }
	}
	Log( LOG_ON_PAD_ADD_TO_NET );
	LogXtra( &dlg.m_net_str );

	CString name = dlg.m_net_str;
	name.Trim();
	CNet *net = m_doc->m_nlist->GetNetPtrByName( &name );
	if( !net )
	{
		// create new net if legal string
		if( name.GetLength() )
			net = new CNet(m_doc->m_nlist, name, 0, 0, 0);
		else
		{
			// blank net name
			CString s ((LPCSTR) IDS_IllegalNetName);
			AfxMessageBox( s );
			return;
		}
	}
	else
		// use selected net
		net->SaveUndoInfo( CNet::SAVE_NET_ONLY );

	// assign pin to net
	pin->SaveUndoInfo();
	net->AddPin(pin);
	net->AddPinsFromSyncFile();
	if( m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// remove this pad from net
//
void CFreePcbView::OnPadDetachFromNet()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PAD_DETACH_FROM_NET);
	CPin *pin = m_sel.First()->ToPin();
	pin->SaveUndoInfo();
	pin->net->SaveUndoInfo( CNet::SAVE_CONNECTS );
	pin->net->RemovePin(pin);
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// connect this pad to another pad
// CPT2 converted
//
void CFreePcbView::OnPadStartRatline()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PAD_START_RATLINE);
	CPin *pin = m_sel.First()->ToPin();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, pin->x, pin->y, LAY_RAT_LINE, 1, 1 );
	if( m_doc->m_bHighlightNet && pin->net )
		m_highlight_net = pin->net,
		pin->net->Highlight();
	SetCursorMode( CUR_DRAG_NEW_RAT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// connect this vertex to another pad with a tee connection
// CPT2 converted
//
void CFreePcbView::OnVertexStartRatline()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_VERTEX_START_RATLINE);
	CPcbItem *sel0 = m_sel.First();
	CVertex *v = sel0->ToVertex();
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
	SetCursorMode( CUR_DRAG_NEW_RAT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// start stub trace from vertex of an existing trace
// CPT2 converted.  Also now works if selected vtx is an end-vertex or a tee.  Also added an optional new bResetActiveWidth param

void CFreePcbView::OnVertexStartTrace() { OnVertexStartTrace(true); }

void CFreePcbView::OnVertexStartTrace(bool bResetActiveWidth)
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_VERTEX_START_TRACE);
	LogXtra(&bResetActiveWidth);
	CPcbItem *sel0 = m_sel.First();
	CVertex *v = sel0->ToVertex();
	if (sel0->IsTee())
		v = sel0->ToTee()->vtxs.First();
	CNet *net = v->m_net;
	net->SaveUndoInfo( CNet::SAVE_CONNECTS );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	m_snap_angle_ref.x = v->x;
	m_snap_angle_ref.y = v->y;
	// now add new connection and first vertex.  The 1st vertex becomes the selection, which we refer to when we get to OnLButtonUp again
	if (v->preSeg && v->postSeg)
		v->SplitConnect();
	CConnect *new_c;
	CVertex *new_v;
	if (v->tee) 
	{
		new_c = new CConnect (net);
		new_v = new CVertex (new_c, v->x, v->y);
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
	m_doc->Redraw();
	if( m_doc->m_bHighlightNet )
		m_highlight_net = net,
		net->Highlight();
	SetCursorMode( CUR_DRAG_STUB );
	ShowActiveLayer();
	ShowSelectStatus();
	Invalidate( FALSE );
	ReleaseDC( pDC );
	// Note that m_doc->ProjectModified() is not called, so the undo record will not be created until after the first seg is built.
}

// set width for this segment (not a ratline)
// CPT2 converted
//
void CFreePcbView::OnSegmentSetWidth()
{
	CSeg *seg = m_sel.First()->ToSeg();
	CancelHighlight();
	SetWidth( 0 );
	seg->Highlight();
}

// unroute this segment, convert to a ratline
// CPT2 converted
//
void CFreePcbView::OnSegmentUnroute()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_SEGMENT_UNROUTE);
	CSeg *seg = m_sel.First()->ToSeg();
	CConnect *c = seg->m_con;
	c->SaveUndoInfo();

	// edit connection segment
	// see if segments to pin also need to be unrouted
	// see if start vertex of this segment is in start pad of connection
	/* CPT2 TODO decide if this stuff is really necessary:
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
	*/
	seg->Unroute();
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}

// delete this segment
//
void CFreePcbView::OnSegmentDelete()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_SEGMENT_DELETE);
	CSeg *seg = m_sel.First()->ToSeg();
	seg->m_con->SaveUndoInfo();
	seg->RemoveBreak();								// Takes care of undrawing
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

// route this ratline.  CPT2 converted
//
void CFreePcbView::OnRatlineRoute() { OnRatlineRoute(true); }			// CPT2.  Need a version with the new bResetActiveWidth param

void CFreePcbView::OnRatlineRoute(bool bResetActiveWidth)
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_RATLINE_ROUTE);
	LogXtra(&bResetActiveWidth);
	CSeg *seg = m_sel.First()->ToSeg();
	CNet *net = seg->m_net;
	CVertex *preVtx = seg->preVtx, *postVtx = seg->postVtx;
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
	if (m_dir==0 && preVtx->pin && preVtx->pin->GetLayer()!=LAY_PAD_THRU)
		// Routing forward from an SMT pin. Force routing on pin's layer
		m_active_layer = preVtx->pin->GetLayer();
	else if (m_dir==1 && postVtx->pin && postVtx->pin->GetLayer()!=LAY_PAD_THRU)
		// Routing backward from an SMT:
		m_active_layer = postVtx->pin->GetLayer();

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
void CFreePcbView::OnOptimize()
{
	// CPT2 used when a net-item (seg, vertex, etc.) is selected and user hits F9.
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_OPTIMIZE);
	CNet *net = m_sel.First()->GetNet();
	net->OptimizeConnections( FALSE );
	if (!m_sel.First()->IsOnPcb())
		// OptimizeConnections() could have clobbered the selected item, if it's a ratline.
		CancelSelection();
	m_doc->ProjectModified( TRUE );
}

// change end-pin for ratline
//
void CFreePcbView::OnRatlineChangeEndPin()
{
	// CPT2 converted.  Somewhat similar to OnVertexStartRatline()
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_RATLINE_CHANGE_END_PIN);
	CSeg *seg = m_sel.First()->ToSeg();
	CVertex *pin_end, *no_pin_end;
	if (seg->preVtx->pin)
		pin_end = seg->preVtx, 
		no_pin_end = seg->postVtx;
	else
		pin_end = seg->postVtx,
		no_pin_end = seg->preVtx;
	ASSERT( pin_end->pin && !no_pin_end->pin );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighlight();
	m_dlist->Set_visible( seg->dl_el, false );
	m_dlist->StartDraggingRatLine( pDC, 0, 0, no_pin_end->x, no_pin_end->y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_RAT_PIN );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// change vertex/via properties
// CPT2 converted
//
void CFreePcbView::OnVertexProperties()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CVertex *vtx = m_sel.First()->ToVertex();
	CPoint v_pt( vtx->x, vtx->y );
	CDlgVia dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( vtx->via_w, vtx->via_hole_w, v_pt, m_units );
		int ret = dlg.DoModal();
		if( ret != IDOK )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_VERTEX_PROPERTIES, &dlg.m_via_w, &dlg.m_via_hole_w);
	LogXtra(&dlg.m_pt.x, &dlg.m_pt.y);

	CancelHighlight();
	vtx->m_con->SaveUndoInfo();
	vtx->via_w = dlg.m_via_w;
	vtx->via_hole_w = dlg.m_via_hole_w;
	if (vtx->via_w)
		vtx->force_via_flag = 1;					// So that ReconcileVia() won't tamper with the via.
	vtx->Move( dlg.m_pt.x, dlg.m_pt.y );
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}

// change tee/tee-via properties
// CPT2 converted
//
void CFreePcbView::OnTeeProperties()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CTee *tee = m_sel.First()->ToTee();
	if (tee->vtxs.GetSize()==0) return;				// Weird...
	CNet *net = tee->GetNet();
	CPoint v_pt( tee->vtxs.First()->x, tee->vtxs.First()->y );
	CDlgVia dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( tee->via_w, tee->via_hole_w, v_pt, m_units );
		int ret = dlg.DoModal();
		if( ret != IDOK )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_TEE_PROPERTIES, &dlg.m_via_w, &dlg.m_via_hole_w);
	LogXtra(&dlg.m_pt.x, &dlg.m_pt.y);

	CancelHighlight();
	net->SaveUndoInfo( CNet::SAVE_CONNECTS );
	tee->MustRedraw();
	tee->via_w = dlg.m_via_w;
	tee->via_hole_w = dlg.m_via_hole_w;
	if (tee->via_w)
		tee->ForceVia();						// So that ReconcileVia() won't tamper with the via.
	tee->Move( dlg.m_pt.x, dlg.m_pt.y );
	m_doc->ProjectModified( TRUE );
	tee->Highlight();
}

// move this vertex
// CPT2 converted
//
void CFreePcbView::OnVertexMove()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_VERTEX_MOVE);
	CVertex *vtx = m_sel.First()->ToVertex();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_dragging_new_item = 0;
	m_from_pt.x = vtx->x;
	m_from_pt.y = vtx->y;
	vtx->StartDragging( pDC, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_VTX );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnTeeMove()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_TEE_MOVE);
	CTee *tee = m_sel.First()->ToTee();
	if (tee->vtxs.GetSize()==0) return;					// Weird...
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_dragging_new_item = 0;
	m_from_pt.x = tee->vtxs.First()->x;
	m_from_pt.y = tee->vtxs.First()->y;
	tee->StartDragging( pDC, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_TEE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete this vertex
// CPT2 converted.
//
void CFreePcbView::OnVertexDelete()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_VERTEX_DELETE);
	CVertex *v = m_sel.First()->ToVertex();
	CConnect *c = v->m_con;
	CNet *net = v->m_net;
	c->SaveUndoInfo();
	v->Remove();
	if( m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

// delete this tee
// CPT2 adapted from OnVertexDelete()
//
void CFreePcbView::OnTeeDelete()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CTee *tee = m_sel.First()->ToTee();
	CNet *net = tee->GetNet();
	if (!m_bReplaying)
	{
		// ask to confirm
		CString s ((LPCSTR) IDS_YouAreDeletingATeeVertexContinue);
		int ret = AfxMessageBox( s,	MB_OKCANCEL );
		if( ret == IDCANCEL )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_TEE_DELETE);
	net->SaveUndoInfo( CNet::SAVE_CONNECTS );
	tee->Remove(true);
	if( m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

// force a via on any vertex
// CPT2 updated.  Used code from the 'F' clause of HandleKeyPress.  Also works if selected item is a tee.

void CFreePcbView::OnVertexAddVia()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_VERTEX_ADD_VIA);
	CPcbItem *sel = m_sel.First();
	CNet *net = sel->GetNet();
	sel->SaveUndoInfo();
	sel->ForceVia();
	if( m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
	SetFKText( m_cursor_mode );
}

// remove forced via on vertex
// CPT2 updated.  Used code from the 'U' clause of HandleKeyPress().  Works on tees.

void CFreePcbView::OnVertexRemoveVia()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_VERTEX_REMOVE_VIA);
	CPcbItem *sel = m_sel.First();
	CNet *net = sel->GetNet();
	sel->SaveUndoInfo();
	sel->UnforceVia();																							// CPT2 changed arg to true
	if (!sel->IsTee())
		sel->GetConnect()->MergeUnroutedSegments();
	if( m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
	SetFKText( m_cursor_mode );
}

// finish routing a connection by making a segment to the destination pad
// CPT2 converted, freely
//
void CFreePcbView::OnRatlineComplete()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_RATLINE_COMPLETE);
	CSeg *seg = m_sel.First()->ToSeg();
	CVertex *start = m_dir==0? seg->preVtx: seg->postVtx;
	CVertex *end = m_dir==0? seg->postVtx: seg->preVtx;
	CConnect *c = seg->m_con;
	CNet *net = seg->m_net;
	c->SaveUndoInfo();
	// CPT2 TODO.  I'm trying using FinishRouting() instead of CSeg::Route().  This gives us inflection points rather than just a 
	// straight line.  Desirable?
	bool bValid = true;
	if (start->pin)
	{
		int pad_layer = start->pin->GetLayer();
		if( pad_layer != LAY_PAD_THRU && m_active_layer != pad_layer )
			bValid = false;
	}
	if (end->pin)
	{
		int pad_layer = end->pin->GetLayer();
		if( pad_layer != LAY_PAD_THRU && m_active_layer != pad_layer )
			bValid = false;
	}

	if (bValid)
	{
		m_snap_angle_ref = CPoint(start->x, start->y);
		m_last_cursor_point = CPoint(end->x, end->y);
		if (m_active_width == 0)
			m_active_width = net->def_w? net->def_w: m_doc->m_trace_w;
		FinishRouting(seg);
		seg->CancelDragging();
		CancelSelection();
	}
	else if (m_doc->m_bErrorSound)
		PlaySound( TEXT("CriticalStop"), 0, 0 );
	m_doc->ProjectModified( TRUE );
}

// set width of a connection
//
void CFreePcbView::OnRatlineSetWidth()
{
	// CPT2.  It seemed to me less confusing to the user if we just have SetWidth(1) (set connect width only, no matter what)
	// if( m_sel.First()->GetConnect()->NumSegs() == 1 )
	// 	  SetWidth( 2 );
	// else
		  SetWidth( 1 );
	Invalidate( FALSE );
}

// delete a connection CPT2 converted.  Note that this works if the selection is a ratline, a regular seg, or a vertex.
//
void CFreePcbView::OnDeleteConnection()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CConnect *c = m_sel.First()->GetConnect();
	CNet *net = c->m_net;
	if( c->locked && !m_bReplaying )
	{
		CString s ((LPCSTR) IDS_YouAreTryingToDeleteALockedConnection);
		int ret = AfxMessageBox( s,	MB_YESNO );
		if( ret == IDNO )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_DELETE_CONNECTION);
	net->SaveUndoInfo( CNet::SAVE_CONNECTS );
	c->Remove();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

// lock a connection CPT2 converted
//
void CFreePcbView::OnRatlineLockConnection()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_RATLINE_LOCK_CONNECTION);
	CConnect *c = m_sel.First()->GetConnect();
	c->SaveUndoInfo();
	c->locked = 1;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// unlock a connection CPT2 converted
//
void CFreePcbView::OnRatlineUnlockConnection()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_RATLINE_UNLOCK_CONNECTION);
	CConnect *c = m_sel.First()->GetConnect();
	c->SaveUndoInfo();
	c->locked = 0;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_doc->ProjectModified( TRUE );
}

// edit a text string
//
void CFreePcbView::OnTextEdit()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CText *t = m_sel.First()->ToText();
	CDlgAddText dlg;
	if (!m_bReplaying)
	{
		// create dialog and pass parameters
		dlg.Initialize( 0, m_doc->m_num_layers, 0, &t->m_str,
			m_units, t->m_layer, t->m_bMirror, t->m_bNegative, t->m_angle, t->m_font_size,
			t->m_stroke_width, t->m_x, t->m_y );
		int ret = dlg.DoModal();
		if( ret == IDCANCEL )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_TEXT_EDIT, &dlg.m_x, &dlg.m_y, &dlg.m_angle);
	LogXtra(&dlg.m_bMirror, &dlg.m_bNegative, &dlg.m_height, &dlg.m_width);
	LogXtra(&dlg.m_layer, &dlg.m_bDrag);
	LogXtra(&dlg.m_str);

	// now replace old text with new one
	t->SaveUndoInfo();
	CancelHighlight();
	t->MustRedraw();
	t->m_x = dlg.m_x;
	t->m_y = dlg.m_y;
	t->m_bMirror = dlg.m_bMirror;
	t->m_bNegative = dlg.m_bNegative;
	t->m_angle = dlg.m_angle;
	t->m_font_size = dlg.m_height;
	t->m_stroke_width = dlg.m_width;
	t->m_layer = dlg.m_layer;
	t->m_str = dlg.m_str;
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
	// start dragging if requested in dialog
	if( dlg.m_bDrag )
	{
		CDC *pDC = GetDC();							// CPT2 TODO move this whole pDC business down into the StartDragging routines...
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		m_dragging_new_item = 1;
		t->StartDragging( pDC );
		SetCursorMode( CUR_DRAG_TEXT );
		ReleaseDC( pDC );
	}
}


// add copper area
//
void CFreePcbView::OnAddArea()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CDlgAddArea dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( m_doc->m_nlist, m_doc->m_num_layers, NULL, m_active_layer, -1 );
		int ret = dlg.DoModal();
		if( ret != IDOK ) 
			{ LogCancel(); return; }
		if( !dlg.m_net )
		{
			CString str, s ((LPCSTR) IDS_NetNotFound);
			str.Format( s, dlg.m_net_name );
			AfxMessageBox( str, MB_OK );
			LogCancel();
			return;
		}
	}
	Log( LOG_ON_ADD_AREA, &dlg.m_layer, &dlg.m_hatch );
	LogItem( (CPcbItem**) &dlg.m_net );

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	SetCursorMode( CUR_ADD_AREA );
	m_sel.Add(dlg.m_net);
	// make layer visible
	m_active_layer = dlg.m_layer;
	m_doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x, m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPolyline::STRAIGHT;
	m_polyline_hatch = dlg.m_hatch;
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// start adding board outline by dragging line for first side
//
void CFreePcbView::OnAddBoardOutline()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_ADD_BOARD_OUTLINE);
	m_doc->m_vis[LAY_BOARD_OUTLINE] = TRUE;
	m_dlist->SetLayerVisible( LAY_BOARD_OUTLINE, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CancelSelection();
	SetCursorMode( CUR_ADD_BOARD );
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x, m_last_cursor_point.y, 0, LAY_BOARD_OUTLINE, 2 );
	m_polyline_layer = LAY_BOARD_OUTLINE;
	m_polyline_style = CPolyline::STRAIGHT;
	m_polyline_hatch = CPolyline::NO_HATCH;
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnAddSoldermaskCutout()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CDlgAddMaskCutout dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( LAY_SM_TOP, CPolyline::NO_HATCH );
		int ret = dlg.DoModal();
		if( ret != IDOK ) 
			{ LogCancel(); return; }
	}
	Log(LOG_ON_ADD_SOLDER_MASK_CUTOUT, &dlg.m_layer, &dlg.m_hatch);

	// force layer visible
	int il = dlg.m_layer;
	m_doc->m_vis[il] = TRUE;
	m_dlist->SetLayerVisible( il, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CancelSelection();
	SetCursorMode( CUR_ADD_SMCUTOUT );
	m_polyline_layer = il;
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x, m_last_cursor_point.y, 0, il, 2 );
	m_polyline_style = CPolyline::STRAIGHT;
	m_polyline_hatch = dlg.m_hatch;
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// add copper area cutout
// CPT2 converted and generalized, so that smcutout cutouts are now possible
void CFreePcbView::OnPolyAddCutout()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_POLY_ADD_CUTOUT);
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CancelHighlight();
	SetCursorMode( CUR_ADD_POLY_CUTOUT );
	// make layer visible
	CPolyline *poly = m_sel.First()->GetPolyline();
	poly->SaveUndoInfo();
	if (poly->IsArea())
		// NB This doesn't work properly if poly is an CSmCutout (those layers can't be the active layer)
		m_active_layer = poly->GetLayer(),
		m_doc->m_vis[m_active_layer] = TRUE,
		m_dlist->SetLayerVisible( m_active_layer, TRUE ),
		ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPcbItem::STRAIGHT;
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::FinishAddPoly()
{
	CContour *ctr = m_drag_contour;
	CPolyline *poly = ctr->GetPolyline();
	CNet *net = poly->GetNet();
	poly->MustRedraw();
	poly->SaveUndoInfo();
	ctr->Close( m_polyline_style );
	if (!poly->PolygonModified( FALSE, FALSE ))
		m_doc->OnEditUndo();
	else if( poly->IsArea() && m_doc->m_vis[LAY_RAT_LINE] )
	{
		poly->GetNet()->SetThermals();		//** AMW3 added
		net->OptimizeConnections();
	}
	m_doc->ProjectModified(true, true);			// CPT2 takes care of creating an undo record, indicating that poly is a new creation.
												// Second arg indicates that we combine the current changes (the final side creation) with the
												// previous changes (the actual polyline creation) in a single undo record.
	CancelSelection();
}

void CFreePcbView::FinishAddPolyCutout() 
{
	CContour *ctr = m_drag_contour;
	CPolyline *poly = m_sel.First()->GetPolyline();
	CNet *net = poly->GetNet();
	poly->MustRedraw();
	poly->SaveUndoInfo();
	poly->Undraw();
	ctr->Close( m_polyline_style );
	ctr->SetPoly( poly );
	if (!poly->PolygonModified( FALSE, FALSE ))
		m_doc->OnEditUndo();
	else if( poly->IsArea() && m_doc->m_vis[LAY_RAT_LINE] )
	{
		poly->GetNet()->SetThermals();		//** AMW3 added
		net->OptimizeConnections();
	}
	m_doc->ProjectModified( true, true );
	CancelSelection();
}

void CFreePcbView::OnPolyDelete()
{
	// CPT2 generalized function used for areas, smcutouts and board outlines.  Works whether the current selection is a corner or a side.
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_POLY_DELETE);
	CPolyline *poly = m_sel.First()->GetPolyline();
	CNet *net = poly->GetNet();
	poly->SaveUndoInfo();
	poly->Remove();
	if (poly->IsArea())
		net->OptimizeConnections(),
		net->SetThermals();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

void CFreePcbView::OnPolyDeleteCutout()
{
	// CPT2 generalized function.  Now both areas and smcutouts can have "cutouts" (new terminology for the user, hopefully less confusing: 
	//  these are called "region exclusions" and the f-key that leads us here is called "remove contour")
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_POLY_DELETE_CUTOUT);
	CPcbItem *first = m_sel.First();
	CPolyline *poly = first->GetPolyline();
	CNet *net = poly->GetNet();
	CContour *ctr = NULL;
	if (CCorner *c = first->ToCorner())
		ctr = c->contour;
	else if (CSide *s = first->ToSide())
		ctr = s->contour;
	ASSERT( ctr && ctr != ctr->poly->main );
	poly->SaveUndoInfo();
	poly->MustRedraw();
	ctr->Remove();
	if (!poly->PolygonModified( FALSE, FALSE ))
		m_doc->OnEditUndo();
	else if( poly->IsArea() && m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	m_doc->ProjectModified( true );
	CancelSelection();
}


void CFreePcbView::OnPolyCornerMove()
{
	// CPT2 consolidated function for moving the corner of any polyline object (area, smcutout, board outline)
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_POLY_CORNER_MOVE);
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CCorner *c = m_sel.First()->ToCorner();
	CPoint p = m_last_mouse_point;
	m_from_pt.x = c->x;
	m_from_pt.y = c->y;
	c->StartDragging(pDC, p.x, p.y, 2);
	SetCursorMode( CUR_DRAG_POLY_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnPolyCornerDelete()
{
	// delete a polyline corner (for areas, smcutouts, and board-outlines)
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_POLY_CORNER_DELETE);
	CCorner *c = m_sel.First()->ToCorner();
	CPolyline *poly = c->GetPolyline();
	CNet *net = c->GetNet();
	poly->SaveUndoInfo();
	c->Remove();							// NB could result in the removal of a contour or even the whole area
	poly->PolygonModified( false, true );
	if( poly->IsArea() && m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

//insert a new corner in a side of a copper area
//
void CFreePcbView::OnPolySideAddCorner()
{
	// CPT2 consolidated function for inserting a corner on any polyline object (area, smcutout, board outline)
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_POLY_SIDE_ADD_CORNER);
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	CSide *s = m_sel.First()->ToSide();
	s->StartDraggingNewCorner( pDC, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_POLY_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnPolySideConvert(int style)
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_POLY_SIDE_CONVERT, &style);
	CSide *s = m_sel.First()->ToSide();
	if (s->m_style == style)
		return;
	CPolyline *poly = s->GetPolyline();
	CNet *net = s->GetNet();
	poly->SaveUndoInfo();
	poly->MustRedraw();
	s->m_style = style;
	// CPT2 I suppose I'll do PolygonModified(), though it's not likely to be important very often:
	if (!poly->PolygonModified( FALSE, TRUE ))
		m_doc->OnEditUndo();
	else if( poly->IsArea() && m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}

void CFreePcbView::OnPolySideConvertToStraightLine()
	{ OnPolySideConvert( CPolyline::STRAIGHT ); }

void CFreePcbView::OnPolySideConvertToArcCw()
	{ OnPolySideConvert( CPolyline::ARC_CW ); }

void CFreePcbView::OnPolySideConvertToArcCcw()
	{ OnPolySideConvert( CPolyline::ARC_CCW ); }

void CFreePcbView::OnPolyCornerEdit()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CCorner *c = m_sel.First()->ToCorner();
	CPolyline *poly = c->GetPolyline();
	DlgEditBoardCorner dlg;
	if (!m_bReplaying)
	{
		CString str ((LPCSTR) IDS_CornerPosition);
		dlg.Init( &str, m_units, c->x, c->y );
		int ret = dlg.DoModal();
		if( ret != IDOK ) 
			{ LogCancel(); return; }
	}
	Log(LOG_ON_POLY_CORNER_EDIT, &dlg.m_x, &dlg.m_y);

	poly->SaveUndoInfo();
	int x = dlg.m_x, y = dlg.m_y;
	c->Move( x, y );
	if (!poly->PolygonModified(false, true))				// CPT2 TODO figure out the system regarding which arg-values this function gets handed.
		m_doc->OnEditUndo();
	else if( poly->IsArea() && m_doc->m_vis[LAY_RAT_LINE] )
		poly->GetNet()->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	TryToReselectCorner( x, y );
	HighlightSelection();
}

// attempt to reselect polyline corner within currently selected net, based on position
// should be used after areas are modified
void CFreePcbView::TryToReselectCorner( int x, int y )
{
	if (m_sel.IsEmpty()) return;
	CPolyline *poly = m_sel.First()->GetPolyline();
	CancelSelection();
	if (!poly) return;
	bool lkwa = m_lastKeyWasArrow;
	CHeap<CPolyline> arr;
	poly->GetCompatiblePolylines( &arr );
	CIter<CPolyline> ip (&arr);
	for (CPolyline *p = ip.First(); p; p = ip.Next())
	{
		CIter<CContour> ictr (&p->contours);
		for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		{
			CIter<CCorner> ic (&ctr->corners);
			for (CCorner *c = ic.First(); c; c = ic.Next())
				if (c->x==x && c->y==y)
				{
					SelectItem(c); 
					m_lastKeyWasArrow = lkwa;			// SelectItem() might have screwed up the old value.
					return; 
				}
		}
	}
}

void CFreePcbView::OnSmEdit()
{
	// CPT2 new.  Lets user edit the sm-cutout's hatching and layer.  Reuses the CDlgAddMaskCutout interface.
	if (m_bReplayMode && !m_bReplaying)
		return;
	CSmCutout *sm = m_sel.First()->GetPolyline()->ToSmCutout();
	CDlgAddMaskCutout dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize(sm->m_layer, sm->m_hatch);
		int ret = dlg.DoModal();
		if( ret != IDOK ) 
			{ LogCancel(); return; }
	}
	Log(LOG_ON_SM_EDIT, &dlg.m_layer, &dlg.m_hatch);

	// force layer visible
	int il = dlg.m_layer;
	m_doc->m_vis[il] = TRUE;
	m_dlist->SetLayerVisible( il, TRUE );
	sm->SaveUndoInfo();
	sm->MustRedraw();
	sm->m_layer = il;
	sm->m_hatch = dlg.m_hatch;
	m_doc->ProjectModified( TRUE );
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

// detect states using routing grid.  CPT2 rejiggered
//
BOOL CFreePcbView::CurDraggingRouting()
{
	if (!CurDragging() || m_cursor_mode == CUR_DRAG_GROUP || m_cursor_mode == CUR_DRAG_GROUP_ADD)
		return false;
	CPcbItem *first = m_sel.First();
	if (first)
		return first->IsNetItem() || first->IsSmCorner() || first->IsSmSide();
	if (m_cursor_mode == CUR_DRAG_POLY_1 || m_cursor_mode == CUR_DRAG_POLY)
		return m_poly_drag_mode == CUR_ADD_AREA || m_poly_drag_mode == CUR_ADD_SMCUTOUT || m_poly_drag_mode == CUR_ADD_POLY_CUTOUT;
	return false;
}

// detect states using placement grid.  CPT2 rejiggered
//
BOOL CFreePcbView::CurDraggingPlacement()
{
	return CurDragging() && !CurDraggingRouting();
}

// snap cursor if required and set m_last_cursor_point
//
void CFreePcbView::SnapCursorPoint( CPoint wp, UINT nFlags )
{
	// see if we need to snap at all
	if( CurDragging() )
	{
		// yes, set snap modes based on cursor mode and SHIFT and CTRL keys
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB || m_cursor_mode == CUR_DRAG_AREA_STUB )
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
		else if( m_units == MIL )
		{
			grid_spacing = m_doc->m_pcbu_per_wu;
		}
		else if( m_units == MM )
		{
			grid_spacing = m_doc->m_pcbu_per_wu;               
		}
		else 
			ASSERT(0);
		// see if we need to snap to angle
		if( m_doc->m_snap_angle && (wp != m_snap_angle_ref)
			&& ( m_cursor_mode == CUR_DRAG_RAT
			|| m_cursor_mode == CUR_DRAG_STUB
			|| m_cursor_mode == CUR_DRAG_AREA_STUB
			|| m_cursor_mode == CUR_DRAG_POLY_1
			|| m_cursor_mode == CUR_DRAG_POLY ))
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
				|| m_cursor_mode == CUR_DRAG_TEE
				|| m_cursor_mode ==  CUR_DRAG_POLY_MOVE
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
			m_units = MIL;
		else
			m_units = MM;
	}
	else
		ASSERT(0);
	// CPT: m_doc->ProjectModified( TRUE );
	SetFocus();
	ShowCursor();
	ShowSelectStatus();
	if( m_cursor_mode == CUR_DRAG_GROUP	|| m_cursor_mode == CUR_DRAG_GROUP_ADD || m_cursor_mode == CUR_DRAG_PART
				|| m_cursor_mode == CUR_DRAG_VTX || m_cursor_mode ==  CUR_DRAG_POLY_MOVE 
				|| m_cursor_mode ==  CUR_DRAG_MEASURE_2 || m_cursor_mode == CUR_MOVE_SEGMENT)
		ShowRelativeDistance( m_last_cursor_point.x - m_from_pt.x, m_last_cursor_point.y - m_from_pt.y );
	else if (m_lastKeyWasArrow)
		ShowRelativeDistance(m_lastArrowPosX, m_lastArrowPosY, m_totalArrowMoveX, m_totalArrowMoveY);
	else
		ShowSelectStatus();
	return 0;
}

void CFreePcbView::OnRefProperties()
{
	// CPT2 converted.  This now works for value texts too.
	if (m_bReplayMode && !m_bReplaying)
		return;
	CText *t = m_sel.First()->ToText();
	CPart *part = t->GetPart();
	CDlgRefText dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( t, m_doc->m_slist );
		int ret = dlg.DoModal();
		if( ret != IDOK )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_REF_PROPERTIES, &dlg.m_height, &dlg.m_width, &dlg.m_layer);
	LogXtra(&dlg.m_vis);
	LogXtra(&dlg.m_str);

	part->SaveUndoInfo(false);
	CancelHighlight();
	t->MustRedraw();
	t->Resize(dlg.m_height, dlg.m_width);
	t->m_layer = FlipLayer(part->side, dlg.m_layer);
	t->m_bShown = dlg.m_vis;
	t->m_str = dlg.m_str;
	if (t->IsRefText())
		part->ref_des = dlg.m_str;
	else
		part->value_text = dlg.m_str;
	m_doc->ProjectModified( TRUE );
	if (dlg.m_vis && t->m_str != "")
		HighlightSelection();
	else
		CancelSelection();
}


// unroute entire connection
//
void CFreePcbView::OnUnrouteTrace()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_UNROUTE_TRACE);
	CConnect *c = m_sel.First()->GetConnect();
	c->SaveUndoInfo();
	c->MustRedraw();
	c->vtxs.RemoveAll();
	c->vtxs.Add(c->head); c->vtxs.Add(c->tail);
	c->segs.RemoveAll();
	CSeg *seg = new CSeg(c, LAY_RAT_LINE, 0);
	c->head->postSeg = seg;
	seg->preVtx = c->head;
	c->tail->preSeg = seg;
	seg->postVtx = c->tail;
	m_doc->ProjectModified( TRUE );
	SelectItem(seg);
}


void CFreePcbView::SaveUndoInfoForGroup()
{
	// CPT2. Reused the name of the old function, but completely readapted to the new undo system.
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
	{
		if (CConnect *c = i->GetConnect())
			// As a precaution, save the whole net if either end vertex is a tee
			if (c->head->tee || c->tail->tee)
				c->m_net->SaveUndoInfo( CNet::SAVE_CONNECTS );
			else
				c->SaveUndoInfo();
		else if (CPolyline *poly = i->GetPolyline())
			poly->SaveUndoInfo();
		else if (CPart *part = i->ToPart())
		{
			part->SaveUndoInfo();
			CIter<CPin> ipin (&part->pins);
			for (CPin *pin = ipin.First(); pin; pin = ipin.Next())
				if (pin->net)
					pin->net->SaveUndoInfo( CNet::SAVE_CONNECTS );
		}
		else
			i->SaveUndoInfo();
	}
}


void CFreePcbView::OnViewEntireBoard()
{
	int nOutlines = 0;
	int max_x = INT_MIN;
	int min_x = INT_MAX;
	int max_y = INT_MIN;
	int min_y = INT_MAX;
	CIter<CBoard> ib (&m_doc->boards);
	for (CBoard *b = ib.First(); b; b = ib.Next())
	{
		nOutlines++;
		CRect r = b->GetBounds();
		max_x = max(max_x, r.right);
		min_x = min(min_x, r.left);
		max_y = max(max_y, r.top);
		min_y = min(min_y, r.bottom);
	}
	if (nOutlines==0)
	{
		CString s ((LPCSTR) IDS_BoardOutlineDoesNotExist);
		AfxMessageBox( s );
		return;
	}

	// reset window to enclose board outline
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
	// board outline / sm-cutouts
	CIter<CBoard> ib (&m_doc->boards);
	for (CBoard *b = ib.First(); b; b = ib.Next())
	{
		r = b->GetBounds();
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	CIter<CSmCutout> ism (&m_doc->smcutouts);
	for (CSmCutout *sm = ism.First(); sm; sm = ism.Next())
	{
		r = sm->GetBounds();
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
	// end CPT
}

void CFreePcbView::OnPartEditFootprint()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnPartEditThisFootprint()
{
	if (m_bReplayMode) 
		{ HighlightSelection(); return; }
	CPart *part = m_sel.First()->ToPart();
	m_doc->m_edit_footprint = part->shape;
	theApp.OnViewFootprint();
}

// Offer new footprint from the Footprint Editor
//
void CFreePcbView::OnExternalChangeFootprint( CShape * fp )
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CPart *part = m_sel.First()->ToPart();
	if (!m_bReplaying)
	{
		CString str, s ((LPCSTR) IDS_DoYouWishToReplaceTheFootprintOfPart);
		str.Format( s, part->ref_des, fp->m_name );
		int ret = AfxMessageBox( str, MB_YESNO );
		if (ret != IDYES)
			{ LogCancel(); return; }
	}
	Log(LOG_ON_EXTERNAL_CHANGE_FP);
	LogShape(fp);

	// OK, see if a footprint of the same name is already in the cache
	CShape *cache_fp = m_doc->m_slist->GetShapeByName( &fp->m_name );
	if( cache_fp )
	{
		// see how many parts are using it, not counting the current one
		int num = m_doc->m_plist->GetNumFootprintInstances( cache_fp );
		if( part->shape == cache_fp )
			num--;
		if( num <= 0 )
		{
			// go ahead and replace it.  CPT2 SAVE SHAPE UNDO INFO
			cache_fp->SaveUndoInfo();
			cache_fp->Copy( fp );
			part->ChangeFootprint( cache_fp );
		}
		else
		{
			// offer to overwrite or rename it
			CDlgDupFootprintName dlg;
			if (!m_bReplaying)
			{
				CString mess, s ((LPCSTR) IDS_WarningAFootprintNamedIsAlreadyInUse);
				mess.Format( s, fp->m_name );
				dlg.Initialize( &mess, m_doc->m_slist );
				int ret = dlg.DoModal();
				if( ret != IDOK )
					{ LogCancel(); return; }
			}
			LogXtra( &dlg.m_replace_all_flag );
			LogXtra( &dlg.m_new_name_str );
			if( dlg.m_replace_all_flag )
			{
				// replace all instances of footprint.  CPT2 bugfix:  must invoke ChangeFootprint() explicitly for 
				// all parts with the old fp, not just "part".  Also, SAVE SHAPE UNDO INFO:
				cache_fp->SaveUndoInfo();
				cache_fp->Copy( fp );
				CIter<CPart> ip (&m_doc->m_plist->parts);
				for (CPart *p = ip.First(); p; p = ip.Next())
					if (p->shape == cache_fp)
						p->ChangeFootprint( cache_fp );
			}
			else
			{
				// assign new name to footprint and put in cache
				CShape * shape = new CShape( fp );
				shape->m_name = dlg.m_new_name_str;
				m_doc->m_slist->shapes.Add( shape );
				part->ChangeFootprint( shape );
			}
		}
	}
	else
	{
		// footprint name not found in cache, add the new footprint
		CShape * shape = new CShape( fp );
		m_doc->m_slist->shapes.Add( shape );
		part->ChangeFootprint( shape );
	}

	m_doc->m_nlist->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}

// find a part in the layout, center window on it and select it
//
void CFreePcbView::OnViewFindpart()
{
	CDlgFindPart dlg;
	dlg.Initialize( m_doc->m_plist );
	int ret = dlg.DoModal();
	if (ret != IDOK)
		return;
	CString * ref_des = &dlg.sel_ref_des;
	CPart * part = m_doc->m_plist->GetPartByName( ref_des );
	if( !part )
	{
		AfxMessageBox( CString((LPCSTR) IDS_SorryThisPartDoesntExist) );
		return;
	}
	if (!part->shape)
	{
		AfxMessageBox( CString((LPCSTR) IDS_SorryThisPartDoesntHaveAFootprint) );
		return;
	}
	CDLElement * dl_sel = part->dl_sel;
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
	SelectItem( part );
	Invalidate( FALSE );
}

void CFreePcbView::OnFootprintWizard()
	{ m_doc->OnToolsFootprintwizard(); }

void CFreePcbView::OnFootprintEditor()
	{ theApp.OnViewFootprint(); }

void CFreePcbView::OnCheckPartsAndNets()
{ 
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_CHECK_PARTS_AND_NETS);
	m_doc->OnToolsCheckPartsAndNets(); 
}

void CFreePcbView::OnToolsDrc()
{ 
	if (m_bReplayMode && !m_bReplaying)
		return;
	CancelSelection();
	DlgDRC dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( m_doc );
		int ret = dlg.DoModal();
		if (ret==IDCANCEL)
			{ LogCancel(); return; }
		// If we get here, then m_doc->m_dr and m_doc->m_annular_ring values will have been updated.
	}
	DesignRules *dr = &m_doc->m_dr;
	Log(LOG_ON_DRC);
	LogXtra(&m_doc->m_annular_ring_pins, &m_doc->m_annular_ring_vias, &dr->bCheckUnrouted, &dr->trace_width);
	LogXtra(&dr->pad_pad, &dr->pad_trace, &dr->trace_trace, &dr->hole_copper);
	LogXtra(&dr->annular_ring_pins, &dr->annular_ring_vias, &dr->board_edge_copper);
	LogXtra(&dr->board_edge_hole, &dr->hole_hole, &dr->copper_copper); 
	LogXtra(&dlg.bCheckOnExit);

	if (dlg.bCheckOnExit)
	{
		if( m_doc->m_vis[LAY_RAT_LINE] && !m_doc->m_auto_ratline_disable )
			m_doc->m_nlist->OptimizeConnections();
		m_doc->m_drelist->Clear();
		BringWindowToTop();
		CDlgLog *log = m_doc->m_dlg_log;
		log->ShowWindow( SW_SHOW );
		log->BringWindowToTop();
		log->Clear();
		log->UpdateWindow();
		m_doc->DRC( m_units, dr ); 
	}
	m_doc->ProjectModified( TRUE );
}

void CFreePcbView::OnToolsClearDrc()
{ 
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_CLEAR_DRC);
	CancelSelection();
	if( m_cursor_mode == CUR_DRE_SELECTED )
		CancelSelection();
	m_doc->m_drelist->Clear();
	m_doc->ProjectModified( true );
}

void CFreePcbView::OnToolsRepeatDrc()
{ 
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_REPEAT_DRC);
	CancelSelection();
	if( m_doc->m_vis[LAY_RAT_LINE] && !m_doc->m_auto_ratline_disable )
		m_doc->m_nlist->OptimizeConnections();
	m_doc->m_drelist->Clear();
	CDlgLog *log = m_doc->m_dlg_log;
	log->ShowWindow( SW_SHOW );   
	log->BringWindowToTop(); 
	log->Clear();
	log->UpdateWindow();
	m_doc->DRC( m_units, &m_doc->m_dr );
	m_doc->ProjectModified( true );
}

void CFreePcbView::OnToolsShowDRCErrorlist()
{
	// CPT2 TODO.  Apparently this was never implemented or was defunct.  Do we care?
}

void CFreePcbView::OnViewAll()
	{ OnViewAllElements(); }

// change side of part
void CFreePcbView::OnPartChangeSide()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PART_CHANGE_SIDE);
	CPart *part = m_sel.First()->ToPart();
	part->SaveUndoInfo();
	CancelHighlight();
	// CPT2 TODO.  I'm not thrilled by how the part has been getting translated when this operation occurs.  It results from the fact that (roughly) a 
	// part on top at (x,y) stretches to the right of (x,y) while a part on bottom at (x,y) stretches to the left (an unfortunate situation that 
	// is pretty well set in stone, given that old .fpc files rely on it).  So let's try the following.  Note that the kludgy
	// fix here is not applied when one switches sides via the part-property dlg.  Given how that dialog lets user specify x/y coords, and given the 
	// immutable way coordinates are interpreted for bottom-side parts, I don't think this kludge is usable there.
	CRect bounds1 = part->CalcSelectionRect();
	part->Move( part->x, part->y, part->angle, !part->side);
	CRect bounds2 = part->CalcSelectionRect();
	part->x += bounds1.left - bounds2.left;
	part->y += bounds1.bottom - bounds2.bottom;
	part->PartMoved( 1, 1, 0, 1 );	//** AMW3
	if( m_doc->m_vis[LAY_RAT_LINE] )
		part->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}

// rotate part
//
void CFreePcbView::OnPartRotateCW()
	{ OnPartRotate(90); }

void CFreePcbView::OnPartRotateCCW()
	{ OnPartRotate(270); }

void CFreePcbView::OnPartRotate( int angle ) {
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_PART_ROTATE, &angle);
	CPart *part = m_sel.First()->ToPart();
	part->SaveUndoInfo();
	CancelHighlight();
	part->MustRedraw();
	part->angle = (part->angle + angle) % 360;
	part->PartMoved( 1, 1, 1 );		//** AMW3
	if( m_doc->m_vis[LAY_RAT_LINE] )
		part->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}


void CFreePcbView::OnNetSetWidth()
{
	// CPT2 converted
	SetWidth( 2 );
	CancelHighlight();
	m_sel.First()->GetNet()->Highlight();
}

void CFreePcbView::OnConnectSetWidth()
{
	// CPT2 converted
	SetWidth( 1 );
	CancelHighlight();
	m_sel.First()->GetConnect()->Highlight();
}

void CFreePcbView::OnSegmentAddVertex()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_SEGMENT_ADD_VERTEX);
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	SetCursorMode( CUR_DRAG_VTX_INSERT );
	CSeg *seg = m_sel.First()->ToSeg();
	seg->StartDraggingNewVertex( pDC, p.x, p.y, seg->m_layer, seg->m_width, 2 );
	ReleaseDC(pDC);
}

void CFreePcbView::OnSegmentChangeLayer()
	{ ChangeTraceLayer( 0, m_sel.First()->GetLayer() ); }

void CFreePcbView::OnConnectChangeLayer()
	{ ChangeTraceLayer( 1 ); }

void CFreePcbView::OnNetChangeLayer()
	{ ChangeTraceLayer( 2 ); }

// change layer of routed trace segments
// if mode = 0, current segment
// if mode = 1, current connection
// if mode = 2, current net
//
void CFreePcbView::ChangeTraceLayer( int mode, int old_layer )
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CNet *net = m_sel.First()->GetNet();
	CConnect *c = m_sel.First()->GetConnect();
	CSeg *seg = m_sel.First()->ToSeg();
	CDlgChangeLayer dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( mode, old_layer, m_doc->m_num_copper_layers );
		int ret = dlg.DoModal();
		if( ret != IDOK ) 
			{ LogCancel(); return; }
	}
	Log(LOG_CHANGE_TRACE_LAYER, &mode, &dlg.m_apply_to, &dlg.m_new_layer);

	int err = 0;
	// CPT2 NB in the following SetLayer() calls MustRedraw() for the appropriate items
	if( dlg.m_apply_to == 0 )
	{
		c->SaveUndoInfo();
		err = seg->SetLayer( dlg.m_new_layer );
		if( err && !m_bReplaying )
		{
			CString s ((LPCSTR) IDS_UnableToChangeLayerForThisSegment);
			AfxMessageBox( s );
		}
	}
	else if( dlg.m_apply_to == 1 )
	{
		c->SaveUndoInfo();
		CIter<CSeg> is (&c->segs);
		for (CSeg *s = is.First(); s; s = is.Next())
			if( s->m_layer >= LAY_TOP_COPPER )
				err += s->SetLayer( dlg.m_new_layer );
		if( err && !m_bReplaying )
		{
			CString s ((LPCSTR) IDS_UnableToChangeLayerForAllSegments);
			AfxMessageBox( s );
		}
	}
	else if( dlg.m_apply_to == 2 )
	{
		net->SaveUndoInfo( CNet::SAVE_CONNECTS );
		CIter<CConnect> ic (&net->connects);
		for (CConnect *c = ic.First(); c; c = ic.Next())
		{
			CIter<CSeg> is (&c->segs);
			for (CSeg *s = is.First(); s; s = is.Next())
				if (s->m_layer >= LAY_TOP_COPPER)
					err += s->SetLayer( dlg.m_new_layer );
		}
		if( err && !m_bReplaying )
		{
			CString s ((LPCSTR) IDS_UnableToChangeLayerForAllSegments);
			AfxMessageBox( s );
		}
	}
	m_doc->ProjectModified( TRUE );
}

void CFreePcbView::OnNetEditnet()
{
	// CPT2 TODO evidently obsolete since nets can't be selected now, only highlighted
#ifndef CPT2
	CDlgEditNet dlg;
	netlist_info nli;
	m_doc->m_nlist->ExportNetListInfo( &nli );
	int inet = -1;
	for( int i=0; i<nli.GetSize(); i++ )
	{
		if( nli[i].net == m_sel_net )
		{
			inet = i;
			break;
		}
	}
	if( inet == -1 )
		ASSERT(0);
	dlg.Initialize( &nli, inet, m_doc->m_plist, FALSE, TRUE, m_units,
		&m_doc->m_w, &m_doc->m_v_w, &m_doc->m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_doc->ResetUndoState();
		CancelSelection();
		m_doc->m_nlist->ImportNetListInfo( &nli, 0, NULL,
			m_doc->m_trace_w, m_doc->m_via_w, m_doc->m_via_hole_w );
		Invalidate( FALSE );
	}
#endif
}

void CFreePcbView::OnToolsMoveOrigin()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CDlgMoveOrigin dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( m_units );
		int ret = dlg.DoModal();
		if( ret != IDOK )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_TOOLS_MOVE_ORIGIN, &dlg.m_drag, &dlg.m_x, &dlg.m_y);

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
		m_doc->CreateMoveOriginUndoRecord( -dlg.m_x, -dlg.m_x );
		MoveOrigin( -dlg.m_x, -dlg.m_y );
		m_doc->ProjectModified(true);
		OnViewAllElements();
	}
}

// move origin of coord system by moving everything
// by (x_off, y_off)
//
void CFreePcbView::MoveOrigin( int dx, int dy )
{
	m_doc->m_plist->MoveOrigin( dx, dy );
	m_doc->m_nlist->MoveOrigin( dx, dy );
	m_doc->m_tlist->MoveOrigin( dx, dy );
	CIter<CSmCutout> ism (&m_doc->smcutouts);
	for (CSmCutout *sm = ism.First(); sm; sm = ism.Next())
		sm->MustRedraw(),
		sm->Offset(dx, dy);
	CIter<CBoard> ib (&m_doc->boards);
	for (CBoard *b = ib.First(); b; b = ib.Next())
		b->MustRedraw(),
		b->Offset(dx, dy);
	m_doc->Redraw();
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
	if( m_sel_mask_bits & bitPart )
	{
		CIter<CPart> ip (&m_doc->m_plist->parts);
		for (CPart *p = ip.First(); p; p = ip.Next())
		{
			CRect pr;
			if( p->GetBoundingRect( &pr ) )
				if (r.PtInRect(pr.TopLeft()) && r.PtInRect(pr.BottomRight()))
					m_sel.Add(p);
		}
	}

	// find trace segments and vias contained in rect.
	if( m_sel_mask_bits & bitSeg )
	{
		CIter<CNet> in (&m_doc->m_nlist->nets);
		for (CNet* net = in.First(); net; net = in.Next())
		{
			CIter<CConnect> ic (&net->connects);
			for (CConnect *c = ic.First(); c; c = ic.Next())
			{
				CIter<CSeg> is (&c->segs);
				for (CSeg *s = is.First(); s; s = is.Next())
				{
					if (!m_doc->m_vis[s->m_layer]) continue;
					CVertex *pre = s->preVtx, *post = s->postVtx;
					bool bPreInR = r.PtInRect( CPoint(pre->x, pre->y) );
					bool bPostInR = r.PtInRect( CPoint(post->x, post->y) );
					if (bPreInR && bPostInR && m_doc->m_vis[s->m_layer])
						m_sel.Add(s);
					if (bPreInR && pre->IsVia())
						m_sel.Add(pre->tee? (CPcbItem*) pre->tee: pre);
					if (bPostInR && post->IsVia())
						m_sel.Add(post->tee? (CPcbItem*) post->tee: post);
				}
			}
		}
	}

	// find texts in rect
	if( m_sel_mask_bits & bitText )
	{
		CIter<CText> it (&m_doc->m_tlist->texts);
		for (CText *t = it.First(); t; t = it.Next())
			if (r.PtInRect(t->m_br.TopLeft()) && r.PtInRect(t->m_br.BottomRight()))
				m_sel.Add(t);
	}

	// find copper area sides in rect
	if( m_sel_mask_bits & bitAreaSide )
	{
		CIter<CNet> in (&m_doc->m_nlist->nets);
		for (CNet* net = in.First(); net; net = in.Next())
		{
			CIter<CArea> ia (&net->areas);
			for (CArea *a = ia.First(); a; a = ia.Next())
				a->GetSidesInRect( &r, &m_sel, m_doc->m_bDragNoSides );
		}
	}

	// find solder mask cutout sides in rect
	if( m_sel_mask_bits & bitSmSide )
	{
		CIter<CSmCutout> ism (&m_doc->smcutouts);
		for (CSmCutout *sm = ism.First(); sm; sm = ism.Next())
			sm->GetSidesInRect( &r, &m_sel, m_doc->m_bDragNoSides );
	}

	// find board outline sides in rect
	if( m_sel_mask_bits & bitBoardSide )
	{
		CIter<CBoard> ib (&m_doc->boards);
		for (CBoard *b = ib.First(); b; b = ib.Next())
			b->GetSidesInRect( &r, &m_sel, m_doc->m_bDragNoSides );
	}

	HighlightSelection();
	m_lastKeyWasArrow = FALSE;
	m_lastKeyWasGroupRotate = false;
	// FindGroupCenter();						Put this into HighlightSelection()
}

void CFreePcbView::FinishRouting(CSeg *rat)
{
	// CPT2: new helper for when user completes routing a ratline (while mode==CUR_DRAG_RAT, hitting F4 or clicking the dest. pin)
	rat->m_con->SaveUndoInfo();
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
	CancelHighlight();
	if( !bAdd )
		SetCursorMode( CUR_DRAG_GROUP );
	else
	{
		SetCursorMode( CUR_DRAG_GROUP_ADD );
		m_last_mouse_point.x = x;
		m_last_mouse_point.y = y;
	}

	// snap dragging point to placement grid
	SnapCursorPoint( m_last_mouse_point, -1 );
	// CPT2 changed (used to be m_last_cursor_point, but I think this makes more sense):
	m_from_pt = CPoint(groupAverageX, groupAverageY);

	// make texts, parts and segments invisible
	int n_parts = 0;
	int n_segs = 0;
	int n_texts = 0;
	int n_sides = 0;
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (CPart *p = i->ToPart())
		{
			p->SetVisible(false);
			n_parts++;
		}
		else if (CSeg *s = i->ToSeg())
		{
			s->SetVisible(false);
			s->preVtx->SetVisible(false);
			s->postVtx->SetVisible(false);
			n_segs++;
		}
		else if (CVertex *v = i->ToVertex())
			// Presumably a via
			v->SetVisible(false);
		else if (CSide *s = i->ToSide())
		{
			CPolyline *p = s->GetPolyline();
			p->SetVisible(false);
			n_sides++;
		}
		else if( CText *t = i->ToText() )
		{
			t->SetVisible(false);
			n_texts++;
		}

	// set up dragline array
	m_dlist->MakeDragLineArray( n_parts*4 + n_segs + n_texts*4 + n_sides );
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (CPart *part = i->ToPart())
		{
			CRect r;
			part->GetBoundingRect(&r);
			int x0 = r.left - m_from_pt.x, x1 = r.right - m_from_pt.x;
			int y0 = r.bottom - m_from_pt.y, y1 = r.top - m_from_pt.y;
			CPoint p1 (x0, y0);
			CPoint p2 (x1, y0);
			CPoint p3 (x1, y1);
			CPoint p4 (x0, y1);
			m_dlist->AddDragLine( p1, p2 );
			m_dlist->AddDragLine( p2, p3 );
			m_dlist->AddDragLine( p3, p4 );
			m_dlist->AddDragLine( p4, p1 );
		}
		else if (CSeg *s = i->ToSeg())
		{
			CVertex *v1 = s->preVtx, *v2 = s->postVtx;
			CPoint p1( v1->x - m_from_pt.x, v1->y - m_from_pt.y );
			CPoint p2( v2->x - m_from_pt.x, v2->y - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if (CSide *s = i->ToSide())
		{
			CCorner *c1 = s->preCorner, *c2 = s->postCorner;
			CPoint p1( c1->x - m_from_pt.x, c1->y - m_from_pt.y );
			CPoint p2( c2->x - m_from_pt.x, c2->y - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if (CText *t = i->ToText())
		{
			CPoint p1( m_dlist->Get_x( t->dl_sel ), m_dlist->Get_y( t->dl_sel ) );
			CPoint p2( m_dlist->Get_xf( t->dl_sel ), m_dlist->Get_y( t->dl_sel ) );
			CPoint p3( m_dlist->Get_xf( t->dl_sel ), m_dlist->Get_yf( t->dl_sel ) );
			CPoint p4( m_dlist->Get_x( t->dl_sel ), m_dlist->Get_yf( t->dl_sel ) );
			p1 -= m_from_pt;
			p2 -= m_from_pt;
			p3 -= m_from_pt;
			p4 -= m_from_pt;
			m_dlist->AddDragLine( p1, p2 );
			m_dlist->AddDragLine( p2, p3 );
			m_dlist->AddDragLine( p3, p4 );
			m_dlist->AddDragLine( p4, p1 );
		}

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_from_pt;
	CPoint cur_p = m_dlist->PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dlist->StartDraggingArray( pDC, m_from_pt.x, m_from_pt.y, 0, LAY_SELECTION, TRUE );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::CancelDraggingGroup()
{
	m_dlist->StopDragging();
	// make elements visible again
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (CPart *part = i->ToPart())
			part->SetVisible(true);
		else if (CSeg *seg = i->ToSeg())
		{
			seg->SetVisible(true);
			seg->preVtx->SetVisible(true);
			seg->postVtx->SetVisible(true);
		}
		else if (CVertex *v = i->ToVertex())
			// Presumably a via
			v->SetVisible(true);
		else if (CSide *s = i->ToSide())
			s->GetPolyline()->SetVisible(true);
		else if (CText *t = i->ToText())
			t->SetVisible(true);
	HighlightSelection();
	Invalidate( FALSE );
}

void CFreePcbView::OnGroupMove()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	if( GluedPartsInGroup() && !m_bReplaying )
	{
		CString s ((LPCSTR) IDS_ThisGroupContainsGluedParts);
		int ret = AfxMessageBox( s, MB_OKCANCEL );
		if( ret != IDOK )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_GROUP_MOVE);
	m_dlist->SetLayerVisible( LAY_RAT_LINE, FALSE );
	StartDraggingGroup();
}


void CFreePcbView::MoveGroup( int dx, int dy )
{
	SetCursor( LoadCursor( NULL, IDC_WAIT ) );
	UngluePartsInGroup();

	// Start by clearing utility flags for all pcb items.  NB not doing any undrawing yet.
	// Also NB the old routine used both item->utility and item->utility2 flags.  I'm going to use 2 bits within item->utility instead:
	enum { bitSel = 1, bitMoved = 2 };
	m_doc->m_nlist->MarkAllNets(0);
	m_doc->m_plist->MarkAllParts(0);
	CIter<CBoard> ib (&m_doc->boards);
	for (CBoard *b = ib.First(); b; b = ib.Next())
		b->MarkConstituents(0);
	CIter<CSmCutout> ism (&m_doc->smcutouts);
	for (CSmCutout *sm = ism.First(); sm; sm = ism.Next())
		sm->MarkConstituents(0);

	// Mark all relevant parts and segments as selected.
	// Also move text and all polyline corners.
	// CPT2 gather a new list of all vtxs that are going to move.  If a tee is going to move, add all its constituent vtxs.
	// Secondly gather a list of polylines that are changing.
	// Finally (in due course) gather a list of connects that are getting affected by the move.
	CHeap<CVertex> selVtxs;
	CHeap<CPolyline> changedPolys;
	CHeap<CConnect> changedCons;
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
	{
		if (CSeg *s = i->ToSeg())
		{
			// segment
			s->utility = bitSel;
			if (s->preVtx->tee) 
				selVtxs.Add( &s->preVtx->tee->vtxs );
			else
				selVtxs.Add( s->preVtx );
			if (s->postVtx->tee) 
				selVtxs.Add( &s->postVtx->tee->vtxs );
			else
				selVtxs.Add( s->postVtx );
		}
		else if (CVertex *v = i->ToVertex())
			// vertex (presumably a via, and not a tee member)
			selVtxs.Add(v);
		else if (CTee *t = i->ToTee())
			// tee (presumably a via)
			selVtxs.Add( &t->vtxs );
		else if (CPart *p = i->ToPart())
			// part
			p->utility = bitSel;

		else if (CText *t = i->ToText())
			t->Move( t->m_x+dx, t->m_y+dy, t->m_angle, t->m_bMirror, t->m_bNegative, t->m_layer );
		else if (CSide *s = i->ToSide())
		{
			// polyline side.  CPT2 this clause works whether it's an area, smcutout, or board side.
			changedPolys.Add( s->GetPolyline() );
			if (!s->preCorner->utility)
				s->preCorner->Move( s->preCorner->x + dx, s->preCorner->y + dy ),
				s->preCorner->utility = bitMoved;
			if (!s->postCorner->utility)
				s->postCorner->Move( s->postCorner->x + dx, s->postCorner->y + dy ),
				s->postCorner->utility = bitMoved;
		}
		else
			ASSERT(0);
	}

	// Old code used to redraw polylines here.  But CFreePcbDoc::Redraw() will do that, since CCorner::Move() invoked MustRedraw() for the
	// relevant polys. HOWEVER, we do invoke PolygonModified() for areas and sm-cutouts that have changed
	CIter<CPolyline> ipoly (&changedPolys);
	for (CPolyline *poly = ipoly.First(); poly; poly = ipoly.Next())
		if (!poly->IsBoard() && poly->IsOnPcb())
			poly->PolygonModified(false, false);
	
	// move parts in group
	CIter<CPart> ip (&m_doc->m_plist->parts);
	for (CPart *part = ip.First(); part; part = ip.Next()) 
	{
		if (!(part->utility & bitSel)) continue;
		// move part
		part->Move( part->x+dx, part->y+dy, part->angle, part->side );
		// find segments which connect to this part and move them
		CIter<CPin> ipin (&part->pins);
		for (CPin *pin = ipin.First(); pin; pin = ipin.Next())
		{
			if (!pin->net) continue;
			CIter<CConnect> ic (&pin->net->connects);
			for (CConnect *c = ic.First(); c; c = ic.Next())
			{
				if (c->segs.IsEmpty()) continue;			// mighty unlikely but hey...
				if (c->head->pin && c->head->pin->part == part)
				{
					// Connect's head is on the part!  If first seg isn't selected, unroute it and move the head vtx.
					// Refers to the position of c->head->pin, which was calculated by part->Move() above
					changedCons.Add(c);
					CSeg *s = c->head->postSeg;
					if (!(s->utility & bitSel))
						s->UnrouteWithoutMerge(dx, dy, 0),
						c->head->Move(c->head->pin->x, c->head->pin->y);
				}
				if (c->tail->pin && c->tail->pin->part == part)
				{
					// connect's tail is on the part!  If last seg isn't selected, unroute it and move the tail vtx.
					changedCons.Add(c);
					CSeg *s = c->tail->preSeg;
					if (!(s->utility & bitSel))
						s->UnrouteWithoutMerge(dx, dy, 1),
						c->tail->Move(c->tail->pin->x, c->tail->pin->y);
				}
			}
		}
	}

	// Time to move affected vtxs, by looping thru selVtxs.  NB The old code checked the utility2 flag for each one to see if it moved already
	//  (during the part-move above) but that's no longer necessary --- the part-moving code only changes vertices belonging to unselected segs.
	CIter<CVertex> iv (&selVtxs);
	for (CVertex *v = iv.First(); v; v = iv.Next())
	{
		CConnect *c = v->m_con;
		changedCons.Add( c );
		// if adjacent segments were not selected, unroute them.  CPT r295 do this _before_ moving the vertex
		if (v->preSeg && !(v->preSeg->utility & bitSel))
			v->preSeg->UnrouteWithoutMerge( dx, dy, 1 );
		if (v->postSeg && !(v->postSeg->utility & bitSel))
			v->postSeg->UnrouteWithoutMerge( dx, dy, 0 );
		// NB Do not call v->Move(), which would create trouble by automatically moving other members of a tee.  MustRedraw() will be called
		// when we loop thru changedCons, below
		v->x += dx;
		v->y += dy;
		v->ReconcileVia();

		// Special case: see if this vtx is on a pin for an unselected part.  Add a ratline if so.
		if (v->pin && !(v->pin->part->utility & bitSel))
			if (!v->preSeg)
			{
				c->PrependSegment(v->pin->x, v->pin->y, LAY_RAT_LINE, 0);
				c->head->pin = v->pin;
				v->pin = NULL;
			}
			else 
			{
				c->AppendSegment(v->pin->x, v->pin->y, LAY_RAT_LINE, 0);
				c->tail->pin = v->pin;
				v->pin = NULL;
			}
	}

	// Loop thru all altered connects, merging unrouted segments and marking for redrawing
	CIter<CConnect> ic (&changedCons);
	for (CConnect *c = ic.First(); c; c = ic.Next())
	{
		c->MustRedraw();
		c->MergeUnroutedSegments();
	}

	// CPT2 Old code explicitly regenerated selection list from utility flags.
	// I'm thinking this won't be necessary because HighlightSelection() does a validity check on the existing items (some polyline 
	// sides could have disappeared, and I guess
	// it's possible that MergeUnroutedSegments will have killed a selected item, though I'm not certain that would ever really happen)
	groupAverageX+=dx;
	groupAverageY+=dy;
	SetCursor( LoadCursor( NULL, IDC_ARROW ) );
}


// Test for glued parts in group.  CPT2 converted
//
BOOL CFreePcbView::GluedPartsInGroup()
{
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (CPart *p = i->ToPart())
			if (p->glued)
				return true;
	return false;
}

// Unglue parts in group. CPT2 converted
//
void CFreePcbView::UngluePartsInGroup()
{
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (CPart *p = i->ToPart())
			p->glued = false;
}

void CFreePcbView::RotateGroup()
{
	// CPT2 converted.  Very similar to MoveGroup(), of course
	UngluePartsInGroup();

	// Start by clearing utility flags for all pcb items.  NB not doing any undrawing yet
	// Also NB the old routine used both item->utility and item->utility2 flags.  I'm going to use 2 bits within item->utility instead:
	enum { bitSel = 1, bitMoved = 2 };
	m_doc->m_nlist->MarkAllNets(0);
	m_doc->m_plist->MarkAllParts(0);
	CIter<CBoard> ib (&m_doc->boards);
	for (CBoard *b = ib.First(); b; b = ib.Next())
		b->MarkConstituents(0);
	CIter<CSmCutout> ism (&m_doc->smcutouts);
	for (CSmCutout *sm = ism.First(); sm; sm = ism.Next())
		sm->MarkConstituents(0);

	// Mark all relevant parts and segments as selected.
	// Also move text and all polyline corners.
	// CPT2 gather a new list of all vtxs that are going to move.  If a tee is going to move, add all its constituent vtxs.
	// Secondly gather a list of polylines that are changing.
	// Finally (in due course) gather a list of connects that are getting affected by the move.
	CHeap<CVertex> selVtxs;
	CHeap<CConnect> changedCons;
	CHeap<CPolyline> changedPolys;
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
	{
		if (CSeg *s = i->ToSeg())
		{
			// segment
			s->utility = bitSel;
			if (s->preVtx->tee) 
				selVtxs.Add( &s->preVtx->tee->vtxs );
			else
				selVtxs.Add( s->preVtx );
			if (s->postVtx->tee) 
				selVtxs.Add( &s->postVtx->tee->vtxs );
			else
				selVtxs.Add( s->postVtx );
		}
		else if (CVertex *v = i->ToVertex())
			// vertex (presumably a via, and not a tee member)
			selVtxs.Add(v);
		else if (CTee *t = i->ToTee())
			// tee (presumably a via)
			selVtxs.Add( &t->vtxs );
		else if (CPart *p = i->ToPart())
			// part
			p->utility = bitSel;

		else if (CText *t = i->ToText())
			t->Move( groupAverageX + t->m_y - groupAverageY, groupAverageY - t->m_x + groupAverageX, (t->m_angle+90)%360,
				t->m_bMirror, t->m_bNegative, t->m_layer );
		else if (CSide *s = i->ToSide())
		{
			// polyline side.  CPT2 this clause works whether it's an area, smcutout, or board side.
			changedPolys.Add( s->GetPolyline() );
			int diff = groupAverageX - groupAverageY, sum = groupAverageX + groupAverageY;
			if (!s->preCorner->utility)
				s->preCorner->Move( s->preCorner->y + diff, sum - s->preCorner->x ),
				s->preCorner->utility = bitMoved;
			if (!s->postCorner->utility)
				s->postCorner->Move( s->postCorner->y + diff, sum - s->postCorner->x ),
				s->postCorner->utility = bitMoved;
		}
		else
			ASSERT(0);
	}

	// Old code used to redraw polylines here.  But CFreePcbDoc::Redraw() will do that, since CCorner::Move() invoked MustRedraw() for the
	// relevant polys. HOWEVER, we do invoke PolygonModified() for areas and sm-cutouts that have changed
	CIter<CPolyline> ipoly (&changedPolys);
	for (CPolyline *poly = ipoly.First(); poly; poly = ipoly.Next())
		if (!poly->IsBoard() && poly->IsOnPcb())
			poly->PolygonModified(false, false);
	
	// move parts in group
	CIter<CPart> ip (&m_doc->m_plist->parts);
	for (CPart *part = ip.First(); part; part = ip.Next()) 
	{
		if (!(part->utility & bitSel)) continue;
		// move part
		part->Move( groupAverageX + part->y - groupAverageY, groupAverageY - part->x + groupAverageX, 
					(part->angle+90) % 360, part->side );
		// find segments which connect to this part and move them
		CIter<CPin> ipin (&part->pins);
		for (CPin *pin = ipin.First(); pin; pin = ipin.Next())
		{
			if (!pin->net) continue;
			CIter<CConnect> ic (&pin->net->connects);
			for (CConnect *c = ic.First(); c; c = ic.Next())
			{
				if (c->segs.IsEmpty()) continue;			// mighty unlikely but hey...
				if (c->head->pin && c->head->pin->part == part)
				{
					// connect's head is on the part!  If first seg isn't selected, unroute it and move the head vtx
					changedCons.Add(c);
					CSeg *s = c->head->postSeg;
					if (!(s->utility & bitSel))
						s->UnrouteWithoutMerge(),
						c->head->Move(c->head->pin->x, c->head->pin->y);
				}
				if (c->tail->pin && c->tail->pin->part == part)
				{
					// connect's tail is on the part!  If last seg isn't selected, unroute it and move the tail vtx
					changedCons.Add(c);
					CSeg *s = c->tail->preSeg;
					if (!(s->utility & bitSel))
						s->UnrouteWithoutMerge(),
						c->tail->Move(c->tail->pin->x, c->tail->pin->y);
				}
			}
		}
	}

	// Time to move affected vtxs, by looping thru selVtxs.  NB The old code checked the utility2 flag for each one to see if it moved already
	//  (during the part-move above) but that's no longer necessary --- the part-moving code only changes vertices belonging to unselected segs.
	CIter<CVertex> iv (&selVtxs);
	for (CVertex *v = iv.First(); v; v = iv.Next())
	{
		CConnect *c = v->m_con;
		changedCons.Add( c );
		// if adjacent segments were not selected, unroute them.  CPT r295 do this _before_ moving the vertex
		if (v->preSeg && !(v->preSeg->utility & bitSel))
			v->preSeg->UnrouteWithoutMerge();
		if (v->postSeg && !(v->postSeg->utility & bitSel))
			v->postSeg->UnrouteWithoutMerge();
		// NB Do not call v->Move(), which would create trouble by automatically moving other members of a tee.  MustRedraw() will be called
		// when we loop thru changedCons, below
		int tempx = v->x;
		v->x = groupAverageX + v->y -groupAverageY;
		v->y = groupAverageY -tempx + groupAverageX;
		v->ReconcileVia();

		// Special case: see if this vtx is on a pin for an unselected part.  Add a ratline if so.
		if (v->pin && !(v->pin->part->utility & bitSel))
			if (!v->preSeg)
			{
				c->PrependSegment(v->pin->x, v->pin->y, LAY_RAT_LINE, 0);
				c->head->pin = v->pin;
				v->pin = NULL;
			}
			else 
			{
				c->AppendSegment(v->pin->x, v->pin->y, LAY_RAT_LINE, 0);
				c->tail->pin = v->pin;
				v->pin = NULL;
			}
	}

	// Loop thru all altered connects, merging unrouted segments and marking for redrawing
	CIter<CConnect> ic (&changedCons);
	for (CConnect *c = ic.First(); c; c = ic.Next())
	{
		c->MustRedraw();
		c->MergeUnroutedSegments();
	}

	// CPT2 Old code explicitly regenerated selection list from utility flags.
	// I'm thinking this won't be necessary because HighlightSelection() does a validity check on the existing items (some polyline 
	// sides could have disappeared, and I guess
	// it's possible that MergeUnroutedSegments will have killed a selected item, though I'm not certain that would ever really happen)
}

void CFreePcbView::OnGroupCopy() 
{
	// CPT2.  Rewrote so that the remaining restrictions on the copying of segments have been lifted (that they belong to connects that span 
	//   pins within the group or tees).  Also redid the copying of areas/smcutouts, so that if one side anywhere in the polyline is selected, the 
	//   whole polyline gets copied:  more consistent with the way that, elsewhere in the program, one selects a polyline by clicking a single side.
	//   At this point something is always copied, so the old inner routine DoGroupCopy with a boolean return value has been discarded.
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_GROUP_COPY);
	// clear clipboard
	m_doc->clip_nlist->nets.RemoveAll();
	m_doc->clip_plist->parts.RemoveAll();
	m_doc->clip_tlist->texts.RemoveAll();
	m_doc->clip_smcutouts.RemoveAll();
	m_doc->clip_boards.RemoveAll();
	m_doc->clip_slist->shapes.RemoveAll();

	// set clipboard pointers (all clipboard item variables are marked with '2')
	CPartList *pl2 = m_doc->clip_plist;
	CNetList *nl2 = m_doc->clip_nlist;
	CTextList *tl2 = m_doc->clip_tlist;
	CHeap<CSmCutout> *sm2 = &m_doc->clip_smcutouts;
	CHeap<CBoard> *bd2 = &m_doc->clip_boards;
	CShapeList *sl2 = m_doc->clip_slist;

	// Set the utility flag for all items that are selected (it's the segs and vias whose utilities we'll be looking at)
	m_doc->m_nlist->MarkAllNets( 0 );
	m_sel.SetUtility( 1 );
	// Gather a list of connects whose segments/vias are involved in the copy.  Also, gather a list of polylines that will be copied.
	CHeap<CConnect> copyCons;
	CHeap<CPolyline> copyPolys;
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (i->IsSeg() || i->IsVertex())
			copyCons.Add( i->GetConnect() );
		else if (CTee *t = i->ToTee())
		{
			CIter<CVertex> iv (&t->vtxs);
			for (CVertex *v = iv.First(); v; v = iv.Next())
				copyCons.Add( v->m_con );
		}
		else if (i->IsSide())
			copyPolys.Add( i->GetPolyline() );

	// add all parts and text from group
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (CPart *part = i->ToPart())
		{
			// add part to clipboard partlist
			CShape * shape = part->shape;
			CPart* part2 = pl2->Add( part->shape, &part->ref_des, &part->value_text, &part->package, part->x, part->y,
				part->side, part->angle, 1, 0 );
			// set ref text and value parameters
			part2->m_ref->Copy( part->m_ref );
			part2->m_value->Copy( part->m_value );
			// attach clipboard part's pins to the appropriate clipboard nets (pins were already created during creation of part2)
			CIter<CPin> ip (&part->pins);
			for (CPin *pin = ip.First(); pin; pin = ip.Next())
			{
				CNet * net = pin->net;
				if (!net) continue;
				// add net to clipboard netlist if not already added
				CNet *net2 = nl2->GetNetPtrByName( &net->name );
				if( !net2 )
					net2 = new CNet(nl2, net->name, net->def_w, net->def_via_w, net->def_via_hole_w);
				// add pin to net
				CPin *pin2 = part2->GetPinByName( &pin->pin_name );
				ASSERT(pin2);
				net2->AddPin(pin2);
			}
		}
		else if (CText *t = i->ToText())
			// add text string to clipboard textlist
			tl2->AddText( t->m_x, t->m_y, t->m_angle, t->m_bMirror,  t->m_bNegative,
				t->m_layer, t->m_font_size, t->m_stroke_width, &t->m_str );

	// Copy all the polylines
	CIter<CPolyline> ipoly (&copyPolys);
	for (CPolyline *poly = ipoly.First(); poly; poly = ipoly.Next())
	{
		CPolyline *poly2;
		if (CArea *a = poly->ToArea())
		{
			CNet *net = a->m_net;
			CNet *net2 = nl2->GetNetPtrByName( &net->name );
			if( !net2 )
				net2 = new CNet(nl2, net->name, net->def_w, net->def_via_w, net->def_via_hole_w);
			poly2 = new CArea(net2, a->m_layer, a->m_hatch, a->m_w, a->m_sel_box);
		}
		else if (CSmCutout *sm = poly->ToSmCutout())
			poly2 = new CSmCutout(m_doc, sm->m_layer, sm->m_hatch),
			sm2->Add(poly2->ToSmCutout());
		else if (CBoard *b = poly->ToBoard())
			poly2 = new CBoard(m_doc),
			bd2->Add(poly2->ToBoard());
		poly2->Copy( poly );
	}

	// Now deal with the connects.  Basically all the connects in copyCons, gathered earlier, will be copied in their entirety, but any segments 
	// that are not selected will be copied as ratlines.  At the end, MergeUnroutedSegments() will combine ratlines as needed and also snip off 
	// any ratlines with loose ends.
	int nextTeeNum = 1;
	CIter<CConnect> ic (&copyCons);
	for (CConnect *c = ic.First(); c; c = ic.Next())
	{
		if (c->NumSegs() == 0) continue;									// Unlikely but hey...
		CNet *net = c->m_net;
		CNet *net2 = nl2->GetNetPtrByName( &net->name );
		if( !net2 )
			net2 = new CNet(nl2, net->name, net->def_w, net->def_via_w, net->def_via_hole_w);
		CConnect *c2 = new CConnect(net2);
		CVertex *head = c->head;
		CVertex *head2 = new CVertex(c2, head->x, head->y);
		if (head->pin)
			head2->pin = pl2->GetPinByNames( &head->pin->part->ref_des, &head->pin->pin_name );		// Might be null, which is now legal.
		if (head->tee)
		{
			int util = head->tee->utility;
			if (!util)
				util = head->tee->utility = nextTeeNum++;
			CTee *tee2 = net2->tees.FindByUtility( util );
			if (!tee2)
				tee2 = new CTee(net2), 
				tee2->utility = util;
			tee2->Add(head2);
		}
		if (head->utility)
			// Head vertex of source connect must be a selected via.  Copy the via params over to head2.
			head2->force_via_flag = true,				// ??
			head2->via_w = head->via_w,
			head2->via_hole_w = head->via_hole_w;

		c2->Start(head2);
		CVertex *v = head;
		while (v->postSeg) 
		{
			v = v->postSeg->postVtx;
			int layer = v->preSeg->m_layer, width = v->preSeg->m_width;
			if (!v->preSeg->utility)
				layer = LAY_RAT_LINE,													// Source seg not selected, so copy it as a ratline.
				width = 0;
			c2->AppendSegment(v->x, v->y, layer, width);
			if (v->utility)
				// Source vertex must be a selected via.  Copy via params:
				c2->tail->force_via_flag = true,			// ??
				c2->tail->via_w = v->via_w,
				c2->tail->via_hole_w = v->via_hole_w;
		}

		CVertex *tail = c->tail, *tail2 = c2->tail;
		if (tail->pin)
			tail2->pin = pl2->GetPinByNames( &tail->pin->part->ref_des, &tail->pin->pin_name );		// Might be null, which is now legal.
		if (tail->tee)
		{
			int util = tail->tee->utility;
			if (!util)
				util = tail->tee->utility = nextTeeNum++;
			CTee *tee2 = net2->tees.FindByUtility( util );
			if (!tee2)
				tee2 = new CTee(net2), 
				tee2->utility = util;
			tee2->Add(tail2);
		}
	}

	// Go thru all the clipboard nets.  First, adjust all tees within the net.  Then scan through the connects, and call MergeUnroutedSegments() 
	// for each; finally reconcile vias for all vertices.
	CIter<CNet> in2 (&nl2->nets);
	for (CNet *net2 = in2.First(); net2; net2 = in2.Next())
	{
		CIter<CTee> it2 (&net2->tees);
		for (CTee *t2 = it2.First(); t2; t2 = it2.Next())
			t2->Adjust();
		CIter<CConnect> ic2 (&net2->connects);
		for (CConnect *c2 = ic2.First(); c2; c2 = ic2.Next())
		{
			c2->MergeUnroutedSegments();
			if (!net2->connects.Contains(c2)) 
				// c2 got clobbered completely by MergeUnroutedSegments()
				continue;
			// If c2 consists of a single pin-to-pin ratline, there's no sense keeping it.  During pasting OptimizeConnections() will just axe it
			// anyway.
			if (c2->NumSegs()==1 && c2->head->postSeg->m_layer == LAY_RAT_LINE && 
				c2->head->pin && c2->tail->pin)
					net2->connects.Remove(c2);
			CIter<CVertex> iv2 (&c2->vtxs);
			for (CVertex *v2 = iv2.First(); v2; v2 = iv2.Next())
				v2->ReconcileVia();
		}
	}

	// CPT2 r336.  Gather a new list of clipboard shapes:  make copies from the main slist (footprint cache), for each footprint referenced by parts
	// on the clipboard
	CMapPtrToPtr smap;										// Will track the relationship between shapes and their new copies.
	CIter<CPart> ip2 (&pl2->parts);
	CHeap<CShape> shapes;
	for (CPart *p2 = ip2.First(); p2; p2 = ip2.Next())
		if (p2->shape)
			shapes.Add(p2->shape);							// Culls duplicates
	CIter<CShape> is (&shapes);
	for (CShape *s = is.First(); s; s = is.Next())
	{
		CShape *s2 = new CShape(s);
		sl2->shapes.Add( s2 );
		smap.SetAt(s, s2);
	}
	// Go through the clipboard parts, and change their shape members to point to the copies.
	for (CPart *p2 = ip2.First(); p2; p2 = ip2.Next())
		p2->shape = (CShape*) smap[p2->shape];

	// NB at this point there's going to be a bunch of garbage in m_doc->redraw, clipboard items on which MustRedraw() got called.  Just empty 
	// that array out.  (Not strictly necessary since m_doc->Redraw() checks that what it draws is actually on the board, but still...)
	m_doc->redraw.RemoveAll();
	CWnd* pMain = AfxGetMainWnd();
	if (pMain != NULL)
	{
		CMenu* pMenu = pMain->GetMenu();
		CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
		submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_ENABLED );
		pMain->DrawMenuBar();
	}
}

void CFreePcbView::OnGroupCut()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_GROUP_CUT);
	OnGroupCopy();
	OnGroupDelete();
}

// Remove all elements in group from project
//
void CFreePcbView::OnGroupDelete()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_GROUP_DELETE);
	DeleteGroup();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
}

void CFreePcbView::DeleteGroup()
{
	// I'm proposing a new system with areas/smcutouts/board outlines.
	// If any side on a main contour is selected, delete the whole polyline.  Otherwise, if a side on a secondary (cutout) contour is selected,
	// delete that contour only.  More consistent with what happens when you select a single side.
	SetCursor( LoadCursor( NULL, IDC_WAIT ) );
	SaveUndoInfoForGroup();

	// CPT2 r336.  I'm taking the plunge and ditching the old system, where selected segments were not truly deleted, but just got unrouted.
	//  Instead let's do a RemoveBreak for each selected segment.  
	// r344:  experimental optimization.  Delete segments by connection, in geometrical order.  Detect it early if an entire connect is
	//  selected (which is fairly likely), and just call Remove() on the whole connect in that case.  This should eliminate a lot of 
	//  useless creation of temporary connects, as happened before when middle segs might get deleted first.
	CHeap<CConnect> changedCons;
	CHeap<CTee> changedTees;
	CIter<CPcbItem> ii (&m_sel);
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (CSeg *seg = i->ToSeg())
		{
			CConnect *c = seg->m_con;
			if (!changedCons.Contains(c))
				changedCons.Add(c),
				c->utility = 1,				// In c->utility we will track how many segs in c are selected
				c->segs.SetUtility(0);
			else
				c->utility++;
			seg->utility = 1;				// Also mark selected segs.
			if (seg->preVtx->tee)
				changedTees.Add( seg->preVtx->tee );
			if (seg->postVtx->tee)
				changedTees.Add( seg->postVtx->tee );
		}
	CHeap<CSeg> deleteSegs;
	CIter<CConnect> ic (&changedCons);
	for (CConnect *c = ic.First(); c; c = ic.Next())
		if (c->utility==c->NumSegs())
			// Whole connect is selected!  Note that we remove it without adjusting tees (we need the set of connects to remain stable)
			c->Remove(false);
		else
			// Append selected segs to deleteSegs in geometrical order
			for (CSeg *s = c->head->postSeg; s; s = s->postVtx->postSeg)
				if (s->utility)
					deleteSegs.Add(s);
	CIter<CSeg> is (&deleteSegs);
	for (CSeg *s = is.First(); s; s = is.Next())
		// It's just possible that s was eliminated on an earlier iteration (if it was some sort of dangling ratline).  So we check:
		if (s->IsOnPcb())
			s->RemoveBreak();
	// Double-check our tees:
	CIter<CTee> it (&changedTees);
	for (CTee *t = it.First(); t; t = it.Next())
		if (t->IsOnPcb())
			t->Adjust();

	// The easy part:  remove vias, polylines or their subcontours, parts, and texts
	for (CPcbItem *i = ii.First(); i; i = ii.Next())
		if (i->IsSeg())
			continue;
		else if (!i->IsOnPcb())
			// Presumably a side on a contour that's already been axed, or maybe a defunct via
			continue;
		else if (CVertex *vtx = i->ToVertex())
			vtx->force_via_flag = 0,
			vtx->ReconcileVia();								// Takes care of MustRedraw()
		else if (CSide *s = i->ToSide())
			if (s->IsOnCutout())
				s->contour->Remove();
			else
				s->contour->poly->Remove();
		else if (CPart *part = i->ToPart())
			part->Remove(false);
		else if (CText *t = i->ToText())
		{
			m_doc->m_tlist->texts.Remove(t);
			t->Undraw();
		}

	SetCursor( LoadCursor( NULL, IDC_ARROW ) );
}

int GetGNumber(CString *s)
{
	// Helper function for OnGroupPaste:  given string s, see if it ends with a substring of the form "_$Gxxx".  If so, return xxx's numeric 
	// value; otherwise return -1.
	int n = s->ReverseFind( '_' );
	if (n <= 0) return -1;
	CString prefix;
	CString test_suffix = s->Right( s->GetLength() - n - 1 );
	int g_num = ParseRef( &test_suffix, &prefix );
	if( prefix == "$G" )
		return g_num;
	return -1;
}

void CFreePcbView::OnGroupPaste()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	// pointers to clipboard lists (all clipboard item variables are marked with '2')
	CNetList *nl2 = m_doc->clip_nlist;
	CShapeList *sl2 = m_doc->clip_slist;
	// pointers to project lists
	CNetList *nl = m_doc->m_nlist;
	CShapeList *sl = m_doc->m_slist;

	// get paste options
	CDlgGroupPaste dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( nl2 );
		int ret = dlg.DoModal();
		if (ret != IDOK)
			{ LogCancel(); return; }
	}
	SetCursor( LoadCursor( NULL, IDC_WAIT ) );
	Log(LOG_ON_GROUP_PASTE, &dlg.m_flags, &dlg.m_ref_offset, &dlg.m_num_copies);
	LogXtra(&dlg.m_dx, &dlg.m_dy);

	// First off, go through and compare the shapes in sl2 with the existing shapes in main shape-list sl.  If necessary, add new shapes to sl,
	// and reconcile conflicts (different shapes, same name) as needed.  We may need new shape names, and for that we'll want a "g-suffix" 
	// of the form _$Gxxx (which may also be used in the naming of new nets).  Note that this all happens without the user having had any 
	// choice about it during the CDlgGroupPaste:  I'm hoping they'll be OK with that (not THAT frequent an occurrence, and it's better than the 
	// crashes/unpredictable behavior that must have arisen before when (say) people copied, edited a footprint, and then
	// pasted).  
	CString g_suffix;
	// get highest group suffix already in project, checking both nets and shapes.  CPT2 r336 in fact, check numbers for both project and clipboard...
	int g_num = 0, g;
	CIter<CNet> in (&nl->nets);
	for (CNet *net = in.First(); net; net = in.Next())
		g = GetGNumber(&net->name),
		g_num = max(g, g_num);
	CIter<CNet> in2 (&nl2->nets);
	for (CNet *net2 = in2.First(); net2; net2 = in2.Next())
		g = GetGNumber(&net2->name),
		g_num = max(g, g_num);
	CIter<CShape> is (&sl->shapes);
	for (CShape *s = is.First(); s; s = is.Next())
		g = GetGNumber(&s->m_name),
		g_num = max(g, g_num);
	CIter<CShape> is2 (&sl2->shapes);
	for (CShape *s2 = is2.First(); s2; s2 = is2.Next())
		g = GetGNumber(&s2->m_name),
		g_num = max(g, g_num);
	g_num++;
	g_suffix.Format( "_$G%d", g_num );
	// Do the shape comparisons:
	CMapPtrToPtr smap;
	bool bFirstConflict = true;
	for (CShape *s2 = is2.First(); s2; s2 = is2.Next())
	{
		CShape *s = sl->GetShapeByName(&s2->m_name);
		if (!s)
			// No conflict at all.  Make a new copy of s2, put it in the cache, and add an entry to smap (later, pasted
			// parts referring to s2 will need to reference the new copy)
			s = new CShape(s2),
			sl->shapes.Add(s),
			smap.SetAt(s2, s);
		else if (s->SameAs(s2))
			// No need to change cache.  But parts on the project will need to reference s rather than s2.
			smap.SetAt(s2, s);
		else 
		{
			// Conflict!  Make a new copy of s2 with a new g-suffixed name.  Put it in the cache and smap as usual.  Show user a log display.
			s = new CShape(s2);
			s->m_name = s2->m_name + g_suffix;
			sl->shapes.Add(s);
			smap.SetAt(s2, s);
			if (bFirstConflict)
			{
				bFirstConflict = false;
				CString line((LPCSTR) IDS_NoticeConflictsWereFoundBetweenTheFootprints);
				m_doc->m_dlg_log->Clear();
				m_doc->m_dlg_log->AddLine( line );
				m_doc->m_dlg_log->ShowWindow( SW_SHOW );
			}
			// When parts are pasted in (below) we'll send the info about them to the log
		}
	}

	// Save undo info about all CNet structures in the main netlist.  It's quite possible that the structures' connects/areas/tees/pins 
	// members will be changing (getting newly created subitems).
	for (CNet *n = in.First(); n; n = in.Next())
		n->SaveUndoInfo( CNet::SAVE_NET_ONLY );
	CancelSelection();
	bool bDrag = (dlg.m_flags & PASTE_DRAG) != 0;
	int dx = bDrag? 0: dlg.m_dx;
	int dy = bDrag? 0: dlg.m_dy;
	int nCopies = dlg.m_num_copies;
	int min_x = INT_MAX;										// lowest-left point for dragging group
	int min_y = INT_MAX;
	int min_d = INT_MAX;										// CPT2 changed: min_d was double to prevent overflow, 
																// but there was a bug (overflow was still possible in the old code)
	CHeap<CPolyline> pastedPolys;								// Maintain a list of pasted-in areas/smcutouts.
	// Here we go!
	for (int i=0; i<nCopies; i++)
		PasteSingle(dlg.m_flags, dx*(i+1), dy*(i+1), g_num+i, dlg.m_ref_offset*(i+1), smap,
					min_x, min_y, min_d, pastedPolys);

	// Mop up.
	if (!bDrag)
	{
		// CPT2 new feature.  This paste involves no dragging (user specified dx/dy).  
		// Therefore now's the time to check the newly pasted polylines (areas/smcutouts) for overlaps:
		CIter<CPolyline> ip (&pastedPolys);
		for (CPolyline *poly = ip.First(); poly; poly = ip.Next())
			if (poly->IsOnPcb())
				poly->PolygonModified(true, true);
		if( m_doc->m_vis[LAY_RAT_LINE] )
		{
			CIter<CNet> in (&nl->nets);
			for (CNet *net = in.First(); net; net = in.Next())
				net->OptimizeConnections(); 
		}
	}
	m_doc->ProjectModified(true);
	HighlightSelection();
	SetCursor( LoadCursor( NULL, IDC_ARROW ) );
	
	if (bDrag)
	{
		if( min_x == INT_MAX || min_y == INT_MAX )
			AfxMessageBox( CString((LPCSTR) IDS_NoItemsToDrag) );
		else
			StartDraggingGroup( TRUE, min_x, min_y );
	}
}

void CFreePcbView::PasteSingle(int flags, int dx, int dy, int g_num, int ref_offset, CMapPtrToPtr &smap,
	int &min_x, int &min_y, int &min_d, CHeap<CPolyline> &pastedPolys) 
{
	// CPT2 helper for OnGroupPaste(), which is now capable of multiple pastings.  This routine does a single one.  
	//		flags are the pasting options returned from the CDlgGroupPaste.
	//		dx/dy:  the amount to displace the pasted items.  Will be 0 in the event that flags & PASTE_DRAG.
	//		g_num:  the number to suffix to the names of unmerged nets (in the form net_$Gxxx), if that's the option user wanted.
	//		ref_offset: the constant to add to part reference names, if that's the option user wanted
	//		smap: a map showing which main-project shapes correspond to given clipboard-shapes.
	//		min_x, min_y, min_d:  outputs; provide a running tally of the minimal position for items in the paste.  Useful on return if flags&PASTE_DRAG.
	//		pastedPolys:  output; a running tally of all new polylines that get pasted in.
	// 1st, get pointers to clipboard lists (all clipboard item variables are marked with '2')
	CPartList *pl2 = m_doc->clip_plist;
	CTextList *tl2 = m_doc->clip_tlist;
	CNetList *nl2 = m_doc->clip_nlist;
	CHeap<CSmCutout> *sm2 = &m_doc->clip_smcutouts;
	CHeap<CBoard> *bd2 = &m_doc->clip_boards;
	CShapeList *sl2 = m_doc->clip_slist;

	// pointers to project lists
	CPartList *pl = m_doc->m_plist;
	CNetList *nl = m_doc->m_nlist;
	CTextList *tl = m_doc->m_tlist;
	CHeap<CSmCutout> *sm = &m_doc->smcutouts;
	CHeap<CBoard> *bd = &m_doc->boards;
	CShapeList *sl = m_doc->m_slist;

	pl->MarkAllParts( 0 );										// newly added parts & pins will be distinguished by values >0 in "utility"
	pl2->MarkAllParts( 0 );										// We also want to ensure that clipboard pins initially have 0 in "utility"

	// add parts from clipboard, renaming if necessary
	CString g_suffix;
	g_suffix.Format( "_$G%d", g_num );
	CIter<CPart> ip2 (&pl2->parts);
	int partId = 1;
	for (CPart *part2 = ip2.First(); part2; part2 = ip2.Next(), partId++)
	{
		CString conflicted_ref;
		CString prefix2;
		int num2 = ParseRef( &part2->ref_des, &prefix2 );
		BOOL bConflict = FALSE;
		// make new ref
		CString new_ref = part2->ref_des;
		if( flags & PASTE_PARTS_OFFSET )
			// add offset to ref
			new_ref.Format( "%s%d", prefix2, num2 + ref_offset );
		if( !(flags & PASTE_PARTS_NEXT_AVAIL) && pl->GetPartByName( &new_ref ) )
		{
			// Though user wanted a specific name, new ref conflicts with existing ref in project
			conflicted_ref = new_ref;
			bConflict = TRUE;
		}
		if( (flags & PASTE_PARTS_NEXT_AVAIL) || bConflict )
		{
			// use next available ref
			int max_num = 0;
			CIter<CPart> ip (&pl->parts);
			for (CPart *part = ip.First(); part; part = ip.Next())
			{
				CString prefix;
				int i = ParseRef( &part->ref_des, &prefix );
				if( prefix == prefix2 && i > max_num )
					max_num = i;
			}
			new_ref.Format( "%s%d", prefix2, max_num+1 );
		}
		if( bConflict )
		{
			// CPT2 TODO:  display this in the log?
			CString s ((LPCSTR) IDS_PartAlreadyExistsInProjectItWillBeRenamed), mess;
			mess.Format(s, conflicted_ref, new_ref);
			AfxMessageBox( mess );
		}
		
		// add new part "part" in the project, derived from "part2" on the clipboard.  The shape will be the one corresponding to
		// part2->shape, as set up in "smap" earlier:
		CShape *shape = (CShape*) smap[part2->shape];
		if (shape->m_name != part2->shape->m_name)
		{
			// This was an incoming shape with a name-conflict:  add the information about it to the log:
			CString line, str0 ((LPCSTR) IDS_PartFootprint);
			line.Format(str0, new_ref, shape->m_name);
			m_doc->m_dlg_log->AddLine(line);
		}
		CPart *part = pl->Add( shape, &new_ref, &part2->value_text, &part2->package, 
					   part2->x + dx, part2->y + dy, part2->side, part2->angle, 
					   1, 0 );
		part->MustRedraw();
		// set ref and value text parameters
		part->m_ref->Copy( part2->m_ref );
		part->m_ref->m_str = new_ref;
		part->m_ref->m_part = part;							// CPT2 omitting this was a nasty little bug to track down
		part->m_value->Copy( part2->m_value );
		part->m_value->m_part = part;
		// Set utility values so that we can later correlate part and part2
		part->utility = part2->utility = partId;
		// find closest part to lower left corner.  CPT2 the /2 prevents overflow:
		int d = part->x/2 + part->y/2;
		if( d < min_d )
			min_d = d,
			min_x = part->x,
			min_y = part->y;
		// Add part to new selection
		m_sel.Add(part);
	}
	m_doc->m_dlg_log->AddLine("\r\n\r\n");					// Separate any messages just produced from future messages


	// now loop through all nets on clipboard and add or merge with project
	CIter<CNet> in2 (&nl2->nets);
	for (CNet *net2 = in2.First(); net2; net2 = in2.Next())
	{
		if( flags & PASTE_NETS_IGNORE_TRACELESS )
		{
			// If user chose option 1, check if there are routed segments in net2
			bool bRouted = false;
			CIter<CConnect> ic2 (&net2->connects);
			for (CConnect *c2 = ic2.First(); c2; c2 = ic2.Next())
			{
				CIter<CSeg> is2 (&c2->segs);
				for (CSeg *s2 = is2.First(); s2; s2 = is2.Next())
					if (s2->m_width > 0)
						{ bRouted = true; goto routedLoopEnd; }
			}
			routedLoopEnd:
			// Only paste in net2 if it contains routed segs or areas
			if (!bRouted && net2->NumAreas()==0)
				continue;
		}

		// OK, add this net to project
		// utility flag is set in the Group Paste dialog for nets which
		// should be merged (i.e. not renamed)
		CNet *net = nl->GetNetPtrByName( &net2->name );
		if( (flags & PASTE_NETS_MERGE_SOME) && net2->utility == 0 )
		{
			CString new_name;
			if( !(flags & PASTE_NETS_RENAME_G) )
			{
				// get next "Nnnnnn" net name
				int max_num = 0;
				CIter<CNet> in (&nl->nets);
				for (CNet *net = in.First(); net; net = in.Next())
				{
					CString prefix;
					int num = ParseRef( &net->name, &prefix );
					if( prefix == "N" && num > max_num )
						max_num = num;
				}
				new_name.Format( "N%05d", max_num+1 );
			}
			else
				// add group suffix
				new_name = net2->name + g_suffix;
			// add new net
			net = new CNet(nl, new_name, net2->def_w, net2->def_via_w, net2->def_via_hole_w );
			net->AddPinsFromSyncFile();																// Mighty unlikely that this will do anything...
		}
		else if( !net )
			// no project net with the same name, so create a new one
			net = new CNet(nl, net2->name, net2->def_w, net2->def_via_w, net2->def_via_hole_w ),
			net->AddPinsFromSyncFile();
		else
			// will merge clipboard net into existing project net...
			;
		net->MustRedraw();

		// attach pins (belonging to newly-pasted parts) to the project net, based on the pins attached to the clipboard net.
		CIter<CPin> ipin2 (&net2->pins);
		int pinId = 1;
		for (CPin *pin2 = ipin2.First(); pin2; pin2 = ipin2.Next())
		{
			/* CPT2 TODO I propose that "dlg.m_pin_net_option==1" AKA "flags&PASTE_NETS_IGNORE_TRACELESS"
			   now simply means that group nets are not imported in at all if they have no
			   routed segs or areas.  That functionality was taken care of above.  The following additional condition is unnecessary under
			   the new system: 
			bool bAdd = true;
			if( dlg.m_pin_net_option == 1 )
			{
				// only add pin2 if it's connected to a [routed] trace.  CPT2 Maybe include pasted pin in "net" if it has just a ratline
				// attached.
				bAdd = false;
				CIter<CConnect> ic2 (&net2->connects);
				for (CConnect *c2 = ic2.First(); c2; c2 = ic2.Next())
					if (c2->head->pin == pin2 || c2->tail->pin == pin2)
						{ bAdd = true; break; }
			}
			if( !bAdd )
				continue;
			*/
			// Find the (new) part in the project correponding to pin2's part.  Use utility fields (set during part-pasting, above) 
			// to make the match, since ref-designators may not match up.
			CPart *part = pl->parts.FindByUtility( pin2->part->utility );
			ASSERT( part!=NULL );
			CPin *pin = part->GetPinByName( &pin2->pin_name );
			// Store correlated values in "utility" so that we can match pins up when importing actual connects.
			pin->utility = pin2->utility = pinId++;
			net->AddPin(pin);
		}

		// add tees to the project net based on the tees in the clipboard net
		CIter<CTee> it2 (&net2->tees);
		int teeId = 1;
		for (CTee *tee2 = it2.First(); tee2; tee2 = it2.Next())
		{
			CTee *tee = new CTee(net);
			tee->utility = tee2->utility = teeId++;
		}

		// create connects for the project net, based on the clipboard connects.
		CIter<CConnect> ic2 (&net2->connects);
		for (CConnect *c2 = ic2.First(); c2; c2 = ic2.Next())
		{
			CConnect *c = new CConnect(net);
			CVertex *head = new CVertex(c, c2->head->x + dx, c2->head->y + dy);
			head->force_via_flag = c2->head->force_via_flag;
			head->via_w = c2->head->via_w;
			head->via_hole_w = c2->head->via_hole_w;
			c->Start(head);
			CVertex *v2 = c2->head;
			while (v2->postSeg) 
			{
				v2 = v2->postSeg->postVtx;
				c->AppendSegment(v2->x + dx, v2->y + dy, v2->preSeg->m_layer, v2->preSeg->m_width);
				c->tail->force_via_flag = v2->force_via_flag;
				c->tail->via_w = v2->via_w;
				c->tail->via_hole_w = v2->via_hole_w;
				m_sel.Add( c->tail->preSeg );
			}

			// get start pin (if any) for new connection in project.  Use the correlated utility values set up during pin attachment (just above)
			if (c2->head->pin && c2->head->pin->utility)
				c->head->pin = net->pins.FindByUtility( c2->head->pin->utility );
			// similarly with the end pin.
			if (c2->tail->pin && c2->tail->pin->utility)
				c->tail->pin = net->pins.FindByUtility( c2->tail->pin->utility );
			// Likewise attach tees
			if (c2->head->tee)
			{
				CTee *tee = net->tees.FindByUtility( c2->head->tee->utility );
				tee->Add(c->head);
			}
			if (c2->tail->tee)
			{
				CTee *tee = net->tees.FindByUtility( c2->tail->tee->utility );
				tee->Add(c->tail);
			}

			// update lower-left corner
			CIter<CVertex> iv (&c->vtxs);
			for (CVertex *v = iv.First(); v; v = iv.Next())
			{
				int d = v->x/2 + v->y/2;
				if( d < min_d )
					min_d = d,
					min_x = v->x,
					min_y = v->y;
			}
		}

		// copy in net2's copper areas
		CIter<CArea> ia2 (&net2->areas);
		for (CArea *a2 = ia2.First(); a2; a2 = ia2.Next())
		{
			CArea *a = new CArea(net, a2->m_layer, a2->m_hatch, a2->m_w, a2->m_sel_box);
			a->Copy( a2 );
			a->Offset( dx, dy );
			a->AddSidesTo( &m_sel );
			pastedPolys.Add(a);
			// Update lower-left corner
			CIter<CCorner> ic (&a->main->corners);
			for (CCorner *c = ic.First(); c; c = ic.Next())
			{
				int d = c->x/2 + c->y/2;
				if( d < min_d )
					min_d = d,
					min_x = c->x,
					min_y = c->y;
			}
		}
	}

	// add sm_cutouts + boards
	CIter<CSmCutout> ism2 (sm2);
	for (CSmCutout *sm2 = ism2.First(); sm2; sm2 = ism2.Next())
	{
		CSmCutout *sm = new CSmCutout(m_doc, sm2->m_layer, sm2->m_hatch);
		sm->Copy( sm2 );
		sm->Offset(dx, dy);
		sm->MustRedraw();
		sm->AddSidesTo( &m_sel );
		pastedPolys.Add(sm);
		// Update lower-left corner
		CIter<CCorner> ic (&sm->main->corners);
		for (CCorner *c = ic.First(); c; c = ic.Next())
		{
			int d = c->x/2 + c->y/2;
			if( d < min_d )
				min_d = d,
				min_x = c->x,
				min_y = c->y;
		}
	}
	CIter<CBoard> ib2 (bd2);
	for (CBoard *b2 = ib2.First(); b2; b2 = ib2.Next())
	{
		CBoard *b = new CBoard(m_doc);
		b->Copy( b2 );
		b->Offset(dx, dy);
		b->MustRedraw();
		b->AddSidesTo( &m_sel );
		// Update lower-left corner
		CIter<CCorner> ic (&b->main->corners);
		for (CCorner *c = ic.First(); c; c = ic.Next())
		{
			int d = c->x/2 + c->y/2;
			if( d < min_d )
				min_d = d,
				min_x = c->x,
				min_y = c->y;
		}
	}

	// add text
	CIter<CText> it2 (&tl2->texts);
	for (CText *t2 = it2.First(); t2; t2 = it2.Next())
	{
		CText *t = tl->AddText( t2->m_x+dx, t2->m_y+dy, t2->m_angle, t2->m_bMirror, t2->m_bNegative, 
			t2->m_layer, t2->m_font_size, t2->m_stroke_width, &t2->m_str );
		t->MustRedraw();
		m_sel.Add( t );
		// update lower-left corner
		t->GenerateStrokes();									// Sets m_br (bounding rect)
		int d = t->m_br.left/2 + t->m_br.bottom/2;
		if( d < min_d )
			min_d = d,
			min_x = t->m_br.left,
			min_y = t->m_br.bottom;
	}
}

void CFreePcbView::OnGroupSaveToFile()
{
	// Copy group to pseudo-clipboard.
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_GROUP_SAVE_TO_FILE);
	OnGroupCopy();
	// A rare instance where we have a dialog box even when replaying:
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
	if( err != IDOK )
		return;
	
	CString pathname = dlg.GetPathName();
	// write project file
	CStdioFile pcb_file;
	if (!pcb_file.Open( pathname, CFile::modeCreate | CFile::modeWrite, NULL ))
	{
		// error opening partlist file
		CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
		mess.Format( s, pathname );
		AfxMessageBox( mess );
		return;
	}
	// write clipboard to file
	try
	{
		m_doc->WriteOptions( &pcb_file );
		m_doc->clip_slist->WriteShapes( &pcb_file );
		m_doc->WriteBoardOutline( &pcb_file, &m_doc->clip_boards );
		m_doc->WriteSolderMaskCutouts( &pcb_file, &m_doc->clip_smcutouts );
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


void CFreePcbView::OnEditCopy()
{
	// CPT2 rewrote, but TODO I must rewrite OnGroupCopy (and preferably rename it)
	if( !m_doc->m_project_open )
		return;
	if (m_sel.GetSize()==0) 
	{
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
		return;
		}
	OnGroupCopy();
	OnGroupDelete();
}


void CFreePcbView::OnAddSimilarArea()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_ADD_SIMILAR_AREA);
	CArea *a = m_sel.First()->GetArea();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CancelHighlight();
	SetCursorMode( CUR_ADD_AREA );
	m_active_layer = a->m_layer;
	m_doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPolyline::STRAIGHT;
	m_polyline_hatch = a->m_hatch;
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnAreaEdit()
{
	// CPT2 converted.
	if (m_bReplayMode && !m_bReplaying)
		return;
	CArea *a = m_sel.First()->GetArea();
	CNet *old_net = a->m_net;
	CDlgAddArea dlg;
	if (!m_bReplaying)
	{
		dlg.Initialize( m_doc->m_nlist, m_doc->m_num_layers, old_net, a->m_layer, a->m_hatch );
		int ret = dlg.DoModal();
		if( ret != IDOK ) 
			{ LogCancel(); return; }
	}	
	Log( LOG_ON_AREA_EDIT, &dlg.m_layer, &dlg.m_hatch );
	LogItem( (CPcbItem**) &dlg.m_net );

	a->SaveUndoInfo();
	a->MustRedraw();
	CNet *net = dlg.m_net;
	if( old_net != net )
	{
		// move area to new net
		net->SaveUndoInfo( CNet::SAVE_AREAS );
		old_net->SaveUndoInfo( CNet::SAVE_AREAS );
		a->SetNet(net);
		old_net->SetThermals();					// CPT2 TODO deal with the issue of thermals when undoing.
	}
	a->m_layer = dlg.m_layer;
	a->m_hatch = dlg.m_hatch;
	if (!a->PolygonModified( FALSE, TRUE ))		// Also calls net->SetThermals()
		m_doc->OnEditUndo();
	else if( m_doc->m_vis[LAY_RAT_LINE] )
		net->OptimizeConnections();
	m_doc->ProjectModified( TRUE );
	CancelSelection();
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

void CFreePcbView::OnAreaStartTrace()
{
	// CPT2 new.  Thought it seemed useful to let user start a trace from within an arbitrary copper area (say if he wants to link two
	// areas with a trace).
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_AREA_START_TRACE);
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CancelHighlight();
	SetCursorMode( CUR_AREA_START_TRACE );
	// make layer visible
	CArea *area = m_sel.First()->GetArea();
	m_active_layer = area->GetLayer(),
	m_doc->m_vis[m_active_layer] = TRUE,
	m_dlist->SetLayerVisible( m_active_layer, TRUE ),
	ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnGroupRotate(int bCcw) 
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CancelHighlight();
	if( !m_lastKeyWasArrow && !m_lastKeyWasGroupRotate && GluedPartsInGroup() && !m_bReplaying )
	{
		CString s ((LPCSTR) IDS_ThisGroupContainsGluedPartsUnglueAndRotate);
		int ret = AfxMessageBox( s, MB_YESNO );
		if( ret != IDYES )
			{ LogCancel(); return; }
	}
	Log(LOG_ON_GROUP_ROTATE, &bCcw);

	m_lastKeyWasGroupRotate=true;
	SaveUndoInfoForGroup();
	RotateGroup();
	if (bCcw)
		// A cheap-n-cheesy way to implement ccw rotation:
		RotateGroup(),
		RotateGroup();
	// CPT2 HighlightSelection may be changing groupAverageX/Y slightly, but in the event of repeated rotations this is undesirable.  Therefore save
	// and restore the current values.
	int groupAverageXOld = groupAverageX, groupAverageYOld = groupAverageY;
	m_doc->ProjectModified( TRUE, m_lastKeyWasArrow || m_lastKeyWasGroupRotate );
	HighlightSelection();
	groupAverageX = groupAverageXOld, groupAverageY = groupAverageYOld;
}

// enable/disable the main menu
// used when dragging.  CPT2 slight reorganization
//
void CFreePcbView::SetMainMenu()
{
	if (!m_doc->m_project_open)
		return;
	CFrameWnd * pMain = (CFrameWnd*)AfxGetMainWnd();
	if( !CurDragging() )
	{
		pMain->SetMenu(&theApp.m_main);
		CMenu* pMenu = pMain->GetMenu();
		CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
		if (m_doc->m_undo_pos == 0)
			submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		else
			submenu->EnableMenuItem( ID_EDIT_UNDO, MF_BYCOMMAND | MF_ENABLED );	
		if( m_doc->m_undo_pos == m_doc->m_undo_records.GetSize() )
			submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		else
			submenu->EnableMenuItem( ID_EDIT_REDO, MF_BYCOMMAND | MF_ENABLED );	
		pMain->DrawMenuBar();
	}
	else
		pMain->SetMenu(&theApp.m_main_drag);
}

void CFreePcbView::OnRefShowPart()
{
	// CPT2 also works for value texts
	CText *t = m_sel.First()->ToText();
	CPart *part = t->m_part;
	CancelSelection();
	CDLElement * dl_sel = part->dl_sel;
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
	SelectItem( part );
}

void CFreePcbView::OnRefRotateCW()
	{ OnRefRotate(90); }

void CFreePcbView::OnRefRotateCCW()
	{ OnRefRotate(270); }

void CFreePcbView::OnRefRotate(int angle)
{
	// CPT2 converted.  This works for valuetexts also
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_REF_ROTATE, &angle);
	CText *t = m_sel.First()->ToText();
	t->SaveUndoInfo();
	CancelHighlight();
	t->MustRedraw();
	t->m_angle = (t->m_angle + angle) % 360;
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}

void CFreePcbView::OnTextRotateCW()
	{ OnTextRotate(90); }

void CFreePcbView::OnTextRotateCCW()
	{ OnTextRotate(270); }

void CFreePcbView::OnTextRotate(int angle)
{
	// CPT2 converted.  Barely different than OnRefRotate()
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_TEXT_ROTATE, &angle);
	CText *t = m_sel.First()->ToText();
	t->SaveUndoInfo();
	t->MustRedraw();
	t->m_angle = (t->m_angle + angle) % 360;
	m_doc->ProjectModified( TRUE );
	HighlightSelection();
}


void CFreePcbView::OnSegmentMove()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	Log(LOG_ON_SEGMENT_MOVE);
	CSeg *seg = m_sel.First()->ToSeg();
	m_dragging_new_item = 0;
	// CPT2 NB before there was code to set m_last_pt and m_next_pt, but I'm letting CSeg::StartMoving() deal with those variables instead.
	m_from_pt.x = seg->preVtx->x;
	m_from_pt.y = seg->preVtx->y;
	m_to_pt.x = seg->postVtx->x;
	m_to_pt.y = seg->postVtx->y;
	CPoint p;
	p.x = (m_to_pt.x + m_from_pt.x) / 2;
	p.y = (m_to_pt.y + m_from_pt.y) / 2;

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	seg->StartMoving( pDC, p.x, p.y, 2 );
	SetCursorMode( CUR_MOVE_SEGMENT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// CPT (all that follows):

void CFreePcbView::ActiveWidthUp(CDC * pDC) {
  // Increase the active routing width to the next value in document's width table greater than the current value.  Also check
  // the current net's default width value, and make that value one of the options:
  // CPT2 updated.
  int cWidths = m_doc->m_w.GetSize();
  #define widthAt(i) (m_doc->m_w.GetAt(i))
  int defaultW = m_doc->m_trace_w;
  CNet *net = m_sel.First()->GetNet();
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
  CNet *net = m_sel.First()->GetNet();
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


// CPT
void CFreePcbView::HandleNoShiftLayerKey(int layer, CDC *pDC) 
{
	if( !m_doc->m_vis[layer] ) 
	{
		if (m_doc->m_bErrorSound)
			PlaySound( TEXT("CriticalStop"), 0, 0 );
		CString s ((LPCSTR) IDS_CantRouteOnInvisibleLayer);
		AfxMessageBox( s );
		return;
	}
	if (m_cursor_mode != CUR_DRAG_STUB && m_cursor_mode != CUR_DRAG_RAT && m_cursor_mode != CUR_DRAG_AREA_STUB)
	{
		m_active_layer = layer;
		ShowActiveLayer();
		return;
	}

	CVertex *start = NULL;
	if (m_cursor_mode == CUR_DRAG_STUB || m_cursor_mode == CUR_DRAG_AREA_STUB) 
		start = m_sel.First()->GetConnect()->tail;
	else if (m_cursor_mode == CUR_DRAG_RAT)
	{
		CSeg *rat = m_sel.First()->ToSeg();
		start = m_dir==0? rat->preVtx: rat->postVtx;
	}
	if (start && start->pin && start->pin->GetLayer()!=LAY_PAD_THRU)
	{
		// Changing layer while routing from an SMT pad is illegal
		layer = -1;
		if (m_doc->m_bErrorSound)
			PlaySound( TEXT("CriticalStop"), 0, 0 );
	}
	else
		m_dlist->ChangeRoutingLayer( pDC, layer, LAY_SELECTION, 0 ),
		m_active_layer = layer,
		ShowActiveLayer();
}

void CFreePcbView::HandleShiftLayerKey(int layer, CDC *pDC) {
	CNet *net = m_sel.First()->GetNet();
	if( m_cursor_mode == CUR_SEG_SELECTED )	{
		CSeg *seg = m_sel.First()->ToSeg();
		seg->MustRedraw();
		seg->m_con->SaveUndoInfo();
		seg->m_layer = layer;
		seg->preVtx->ReconcileVia();
		seg->postVtx->ReconcileVia();
		m_doc->ProjectModified( TRUE );
		}
	else if( m_cursor_mode == CUR_CONNECT_SELECTED ) {
		CConnect *c = m_sel.First()->GetConnect();
		c->SaveUndoInfo();
		c->MustRedraw();
		CIter<CSeg> is (&c->segs);
		for (CSeg *s = is.First(); s; s = is.Next())
			s->m_layer = layer;
		CIter<CVertex> iv (&c->vtxs);
		for (CVertex *v = iv.First(); v; v = iv.Next())
			v->ReconcileVia();
		m_doc->ProjectModified( TRUE );
		}
	else if( m_cursor_mode == CUR_AREA_CORNER_SELECTED || m_cursor_mode == CUR_AREA_SIDE_SELECTED ) {
		CArea *a = m_sel.First()->GetArea();
		a->SaveUndoInfo();
		a->MustRedraw();
		a->m_layer = layer;
		if (!a->PolygonModified( TRUE, TRUE ))
			m_doc->OnEditUndo();
		else if( m_doc->m_vis[LAY_RAT_LINE] )
			net->OptimizeConnections();
		m_doc->ProjectModified( TRUE );
		HighlightSelection();
		}
	}		

void CFreePcbView::OnAddPart()
{
	// CPT2 was in CFreePcbDoc, but CFreePcbView seems more logical to me.
	if (m_bReplayMode && !m_bReplaying)
		return;
	CDlgAddPart dlg;
	partlist_info pli;
	if (!m_bReplaying)
	{
		// invoke dialog
		m_doc->m_plist->ExportPartListInfo( &pli, NULL );
		dlg.Initialize( &pli, -1, TRUE, TRUE, FALSE, 0, m_doc, m_units );
		int ret = dlg.DoModal();
		if( ret != IDOK ) 
			{ LogCancel(); return; }
	}
	Log(LOG_ON_ADD_PART, &dlg.m_drag_flag);
	LogXtra(&dlg.m_ref_des);
	LogPartListInfo( &pli );

	// Create part from the partlist generated by the dlg.  If we're going to be dragging, move part (and its pins) under user's cursor right away.
	m_doc->m_plist->ImportPartListInfo( &pli, 0 );
	CPart *part = m_doc->m_plist->GetPartByName( &dlg.m_ref_des );
	if (dlg.m_drag_flag)
	{
		CPoint p;
		GetCursorPos( &p );				// cursor pos in screen coords
		p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
		part->Move(p.x, p.y);
	}
	m_doc->ProjectModified( TRUE );
	SelectItem( part );
	if( dlg.m_drag_flag )
	{
		m_dragging_new_item = TRUE;
		OnPartMove();
	}
}

void CFreePcbView::OnAddNet()
{
	if (m_bReplayMode && !m_bReplaying)
		return;
	CDlgEditNet dlg;
	netlist_info nli;
	if (!m_bReplaying)
	{
		m_doc->m_nlist->ExportNetListInfo( &nli );
		dlg.Initialize( &nli, -1, m_doc->m_plist, TRUE, TRUE,
							MIL, &m_doc->m_w, &m_doc->m_v_w, &m_doc->m_v_h_w );
		int ret = dlg.DoModal();
		if (ret!=IDOK)
			{ LogCancel(); return; }
	}
	Log(LOG_ON_ADD_NET);
	LogNetListInfo( &nli );
	m_doc->m_nlist->ImportNetListInfo( &nli, 0, NULL, m_doc->m_trace_w, m_doc->m_via_w, m_doc->m_via_hole_w );
}

// end CPT

#ifdef CPT2_LOG

// The logging functions are called from within each UI routine.  During normal operation, they simply add lines to the current log file
// (a file with a name equal to current-fpc-file-name + ".log");  the lines indicate the user's operation, plus any parameters (typically
// chosen by user from within dialog boxes).  During replay mode, however, the logging functions reverse this operation, reading in lines from
// the current log file and filling the passed-in parameter pointers accordingly.

void CFreePcbView::Log(int function, int *param1, int *param2, int *param3)
{
	if (!m_bReplayMode)
	{
		CString line;
		int p1 = param1? *param1: 0;
		int p2 = param2? *param2: 0;
		int p3 = param3? *param3: 0;
		line.Format("%d %d %d %d\r\n", function, p1, p2, p3);
		Log(line, true);
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		m_log_file.ReadString( line );
		if (line[0]=='q')
			throw new CString((LPCSTR) IDS_LoggerCancelledOperation);
		int numParams = ParseKeyString(&line, &cmdStr, &params) - 1;			// ParseKeyString returns number of params + 1
		int cmd = atoi(cmdStr);
		if (cmd != function)
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		if (param1)
			*param1 = numParams>0? atoi(params[0]): 0;
		if (param2)
			*param2 = numParams>1? atoi(params[1]): 0;
		if (param3)
			*param3 = numParams>2? atoi(params[2]): 0;
		// Parse the incoming 'u' and 's' lines (the first for correlating future uid values, the second for getting the correct item selection)
		m_log_file.ReadString( line );
		if (line=="" || line[0]!='u')
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		line.TrimLeft("u ");
		int uidLog = atoi(line), uidReplay = CPcbItem::GetNextUid();
		m_replay_uid_translate.Add( CPoint(uidLog, uidReplay) );
		m_log_file.ReadString( line );
		numParams = ParseKeyString(&line, &cmdStr, &params) - 1;
		if (numParams < 1 || cmdStr!="s")
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		m_cursor_mode = atoi(params[0]);
		m_sel.RemoveAll();
		for (int i=1; i<numParams; i++)
		{
			int uid = ReplayUidTranslate( atoi(params[i]) );
			CPcbItem *item = CPcbItem::FindByUid(uid);
			if (!item)
				throw new CString((LPCSTR) IDS_CorruptItemUidInLogFile);
			m_sel.Add(item);
		}
		SetFKText(m_cursor_mode);
	}
}

void CFreePcbView::Log(int function, long *param1, long *param2, long *param3)
{
	Log(function, (int*) param1, (int*) param2, (int*) param3);
}

void CFreePcbView::LogXtra(int *xtra1, int *xtra2, int *xtra3, int *xtra4)
{
	if (!m_bReplayMode) 
	{
		CString line;
		int x1 = xtra1? *xtra1: 0;
		int x2 = xtra2? *xtra2: 0;
		int x3 = xtra3? *xtra3: 0;
		int x4 = xtra4? *xtra4: 0;
		line.Format("x %d %d %d %d\r\n", x1, x2, x3, x4);
		Log(line, false);
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		m_log_file.ReadString( line );
		int numParams = ParseKeyString(&line, &cmdStr, &params) - 1;			// ParseKeyString returns number of params + 1
		if (numParams<1 || cmdStr!="x")
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		if (xtra1)
			*xtra1 = numParams>0? atoi(params[0]): 0;
		if (xtra2)
			*xtra2 = numParams>1? atoi(params[1]): 0;
		if (xtra3)
			*xtra3 = numParams>2? atoi(params[2]): 0;
		if (xtra4)
			*xtra4 = numParams>2? atoi(params[3]): 0;
	}
}

void CFreePcbView::LogXtra(long *xtra1, long *xtra2, long *xtra3, long *xtra4)
{
	LogXtra((int*) xtra1, (int*) xtra2, (int*) xtra3, (int*) xtra4);
}

void CFreePcbView::LogXtra(double *xtra1, double *xtra2, double *xtra3, double *xtra4)
{
	if (!m_bReplayMode) 
	{
		double zero = 0.;
		if (!xtra2) xtra2 = &zero;
		if (!xtra3) xtra3 = &zero;
		if (!xtra4) xtra4 = &zero;
		// To avoid rounding issues, output each double as a pair of ints.  DEPENDS ON HAVING 32-BIT ints & 64-BIT doubles
		int *x1 = (int*) xtra1, *x2 = (int*) xtra2, *x3 = (int*) xtra3, *x4 = (int*) xtra4;
		CString line;
		line.Format("xd %d %d  %d %d  %d %d  %d %d\r\n", x1[0], x1[1], x2[0], x2[1], x3[0], x3[1], x4[0], x4[1]);
		Log(line, false);
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		m_log_file.ReadString( line );
		int numParams = ParseKeyString(&line, &cmdStr, &params) - 1;			// ParseKeyString returns number of params + 1
		if (numParams<1 || cmdStr!="xd")
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		// To avoid rounding issues, the logger output each double as a pair of ints.  DEPENDS ON HAVING 32-BIT ints & 64-BIT doubles
		int *x1 = (int*) xtra1, *x2 = (int*) xtra2, *x3 = (int*) xtra3, *x4 = (int*) xtra4;
		if (xtra1 && numParams>=2)
			x1[0] = atoi(params[0]),
			x1[1] = atoi(params[1]);
		if (xtra2 && numParams>=4)
			x2[0] = atoi(params[2]),
			x2[1] = atoi(params[3]);
		if (xtra3 && numParams>=6)
			x3[0] = atoi(params[4]),
			x3[1] = atoi(params[5]);
		if (xtra4 && numParams>=8)
			x4[0] = atoi(params[6]),
			x4[1] = atoi(params[7]);
	}
}

void CFreePcbView::LogXtra(bool *xtra1, bool *xtra2, bool *xtra3, bool *xtra4)
{
	if (!m_bReplayMode) 
	{
		CString line;
		int x1 = *xtra1;
		int x2 = xtra2? *xtra2: 0;
		int x3 = xtra3? *xtra3: 0;
		int x4 = xtra4? *xtra4: 0;
		line.Format("x %d %d %d %d\r\n", x1, x2, x3, x4);
		Log(line, false);
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		m_log_file.ReadString( line );
		int numParams = ParseKeyString(&line, &cmdStr, &params) - 1;			// ParseKeyString returns number of params + 1
		if (numParams<1 || cmdStr!="x")
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		if (xtra1)
			*xtra1 = numParams>0? atoi(params[0]): 0;
		if (xtra2)
			*xtra2 = numParams>1? atoi(params[1]): 0;
		if (xtra3)
			*xtra3 = numParams>2? atoi(params[2]): 0;
		if (xtra4)
			*xtra4 = numParams>2? atoi(params[3]): 0;
	}
}

void CFreePcbView::LogXtra(CString *str)
{
	if (!m_bReplayMode)
	{
		CString line;
		line.Format("x \"%s\"\r\n", *str);
		Log(line, false);
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		m_log_file.ReadString( line );
		int numParams = ParseKeyString(&line, &cmdStr, &params) - 1;			// ParseKeyString returns number of params + 1
		if (numParams<1 || cmdStr!="x")
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		*str = params[0];
	}
}

void CFreePcbView::LogItem(CPcbItem **item)
{
	if (!m_bReplayMode)
	{
		CString line;
		line.Format("x %d\r\n", (*item)->UID());
		Log(line, false);
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		m_log_file.ReadString( line );
		int numParams = ParseKeyString(&line, &cmdStr, &params) - 1;			// ParseKeyString returns number of params + 1
		if (numParams<1 || cmdStr!="x")
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		int uid = ReplayUidTranslate( atoi(params[0]) );
		*item = CPcbItem::FindByUid( uid );
		if (!*item)
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
	}
}

void CFreePcbView::LogCancel()
{
	if (m_bReplayMode)
		return;									// Shouldn't happen
	CString line ("q\r\n");
	Log(line, false);
}

void CFreePcbView::LogNetListInfo(netlist_info *nli)
{
	// In logging mode, save all the info required to reconstruct nli exactly.  In replay mode, reconstruct it.
	// The amount of data saved is often excessive, but hey, let's keep the coding simple...
	if (!m_bReplayMode)
	{
		CString logFilePath = m_doc->m_path_to_folder + "\\" + m_doc->m_pcb_filename + ".log";
		CStdioFile logFile;
		if (!logFile.Open( LPCSTR(logFilePath), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive, NULL ))
			return;
		logFile.SeekToEnd();
		for (int i=0; i<nli->GetSize(); i++)
		{
			net_info *ni = &(*nli)[i];
			int numPins = ni->ref_des.GetSize();
			int net = ni->net? ni->net->UID(): -1;
			CString line, tmp;
			line.Format("n \"%s\" %d %d %d %d %d %d %d %d %d %d %d ", 
				ni->name, net, ni->visible, ni->w, ni->v_w, ni->v_h_w, 
				ni->apply_trace_width, ni->apply_via_width, ni->deleted, ni->merge_into, ni->modified, numPins);
			for (int j=0; j<numPins; j++)
				tmp.Format("%s %s ", ni->ref_des[j], ni->pin_name[j]),
				line += tmp;
			line += "\r\n";
			logFile.WriteString(line);
		}
		logFile.WriteString("@@@\r\n");
		logFile.Close();
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		int sz = 0;
		while (1)
		{
			if (!m_log_file.ReadString( line ))
				throw new CString((LPCSTR) IDS_EndOfLogFileReached);
			if (line[0]=='@')
				break;
			int numParams = ParseKeyString(&line, &cmdStr, &params) - 1;			// ParseKeyString returns number of params + 1
			if (numParams<12 || cmdStr!="n")
				throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
			nli->SetSize( ++sz );
			net_info *ni = &(*nli)[sz-1];
			ni->name = params[0];
			int uid = ReplayUidTranslate( atoi(params[1]) );
			CPcbItem *net = CPcbItem::FindByUid(uid);
			ni->net = net? net->ToNet(): NULL;
			ni->visible = atoi(params[2]);
			ni->w = atoi(params[3]);
			ni->v_w = atoi(params[4]); 
			ni->v_h_w = atoi(params[5]); 
			ni->apply_trace_width = atoi(params[6]); 
			ni->apply_via_width = atoi(params[7]);
			ni->deleted = atoi(params[8]); 
			ni->merge_into = atoi(params[9]); 
			ni->modified = atoi(params[10]); 
			int numPins = atoi(params[11]);
			if (numParams != 12 + 2*numPins)
				throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
			for (int i=0; i<numPins; i++)
				ni->ref_des.Add( params[12+i*2] ),
				ni->pin_name.Add( params[13+i*2] );
		}
	}
}

int ComparePointsByX(CPoint *p1, CPoint *p2) 
{
	// Helper used by LogPartListInfo(), for qsorting.
	return p1->x < p2->x? -1: 1;
}

void CFreePcbView::LogPartListInfo(partlist_info *pli)
{
	// In logging mode, save all the info required to reconstruct pli exactly.  In replay mode, reconstruct it.
	// This is the trickiest logging/replaying routine because of issues involving shapes that the logger may have loaded in during the editing of
	// the partlist-info
	if (!m_bReplayMode)
	{
		CString logFilePath = m_doc->m_path_to_folder + "\\" + m_doc->m_pcb_filename + ".log";
		CStdioFile logFile;
		if (!logFile.Open( LPCSTR(logFilePath), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive, NULL ))
			return;
		logFile.SeekToEnd();
		for (int i=0; i<pli->GetSize(); i++)
		{
			part_info *pi = &(*pli)[i];
			int part = pi->part? pi->part->UID(): -1;
			int shape = pi->shape? pi->shape->UID(): -1;
			CString line;
			line.Format("p \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\r\n", 
				pi->ref_des, pi->value, pi->package, pi->shape_name, pi->shape_file_name, 
				part, shape, pi->x, pi->y, pi->angle, pi->side, 
				pi->deleted, pi->bShapeChanged, pi->bOffBoard, 
				pi->ref_size, pi->ref_width, pi->ref_layer, pi->ref_vis,
				pi->value_size, pi->value_width, pi->value_layer, pi->value_vis);
			logFile.WriteString(line);
			if (pi->shape_file_name != "" && pi->shape)
			{
				// For shapes that do not come from the cache, we must log the info for reconstructing them
				pi->shape->WriteFootprint(&logFile);
				logFile.WriteString("end\r\n");
			}
		}
		logFile.WriteString("@@@\r\n");
		logFile.Close();
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		CHeap<CShape> newShapes;
		int sz = 0;
		while (1)
		{
			if (!m_log_file.ReadString( line ))
				throw new CString((LPCSTR) IDS_EndOfLogFileReached);
			if (line[0]=='@')
				break;
			int numParams = ParseKeyString(&line, &cmdStr, &params) - 1;			// ParseKeyString returns number of params + 1
			if (numParams<22 || cmdStr!="p")
				throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
			pli->SetSize( ++sz );
			part_info *pi = &(*pli)[sz-1];
			pi->ref_des = params[0];
			pi->value = params[1];
			pi->package = params[2];
			pi->shape_name = params[3];
			pi->shape_file_name = params[4];
			int partUid = ReplayUidTranslate( atoi(params[5]) );
			CPcbItem *part = CPcbItem::FindByUid(partUid);
			pi->part = part? part->ToPart(): NULL;
			int shapeUid = atoi(params[6]);											// Will deal with translation issues below...
			pi->x = atoi(params[7]);
			pi->y = atoi(params[8]);
			pi->angle = atoi(params[9]);
			pi->side = atoi(params[10]);
			pi->deleted = atoi(params[11]);
			pi->bShapeChanged = atoi(params[12]);
			pi->bOffBoard = atoi(params[13]);
			pi->ref_size = atoi(params[14]);
			pi->ref_width = atoi(params[15]);
			pi->ref_layer = atoi(params[16]);
			pi->ref_vis = atoi(params[17]);
			pi->value_size = atoi(params[18]);
			pi->value_width = atoi(params[19]);
			pi->value_layer = atoi(params[20]);
			pi->value_vis = atoi(params[21]);
			// Deal with the shape.
			if (shapeUid == -1)
				pi->shape = NULL;
			else if (pi->shape_file_name == "")
			{
				// Shape should be in the cache, so just translate the uid and get the object ptr.
				CPcbItem *shape = CPcbItem::FindByUid(ReplayUidTranslate(shapeUid));
				if (!shape)
					throw new CString((LPCSTR) IDS_CorruptShapeInLogFile);
				pi->shape = shape->ToShape();
			}
			else
			{
				// Create a new shape and get its specs from the log.  The created object's uid must be correlated with the shape-uid saved in the log.
				pi->shape = new CShape(m_doc);
				int err = pi->shape->MakeFromFile(&m_log_file, "", "", 0);
				if (err)
					throw new CString((LPCSTR) IDS_CorruptShapeInLogFile);
				pi->shape->utility = shapeUid;
				// Double check whether, on an earlier iteration, we already created a shape that corresponds to the logger's shapeUid.
				CShape *prevShape = newShapes.FindByUtility( shapeUid );
				if (prevShape)
					pi->shape = prevShape;
				else
					newShapes.Add(pi->shape),
					m_replay_uid_translate.Add( CPoint(shapeUid, pi->shape->UID()) );
			}
		}

		// Given how the just-finished loop may have added correspondences to m_replay_uid_translate, we need to sort that table
		// according to the first coordinate in each point.  This will ensure that ReplayUidTranslate() (q.v.) works correctly
		int numTranslations = m_replay_uid_translate.GetSize();
		qsort(m_replay_uid_translate.GetData(), numTranslations, sizeof(CPoint), (int (*)(const void*,const void*)) ComparePointsByX);
		// The last entry in (sorted) m_replay_uid_translate, which was created during the Log() command that ran most recently, is supposed to
		// translate the logger's next-uid value into the current replayer's next-uid.  But it may not contain the correct replay next-uid,
		// because of shapes created during the just-finished loop.  Therefore correct that last entry.
		CPoint *p = &m_replay_uid_translate[numTranslations-1];
		p->y = CPcbItem::GetNextUid();
	}
}

void CFreePcbView::LogShape(CShape *fp)
{
	if (!m_bReplayMode)
	{
		CString logFilePath = m_doc->m_path_to_folder + "\\" + m_doc->m_pcb_filename + ".log";
		CStdioFile logFile;
		if (!logFile.Open( LPCSTR(logFilePath), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive, NULL ))
			return;
		logFile.SeekToEnd();
		logFile.WriteString("f\r\n");
		fp->WriteFootprint(&logFile);
		logFile.WriteString("end\r\n");
	}

	else
	{
		CString line, cmdStr;
		CArray<CString> params;
		m_log_file.ReadString( line );
		if (line[0]!='f')
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		int err = fp->MakeFromFile(&m_log_file, "", "", 0);
		if (err)
			throw new CString((LPCSTR) IDS_CorruptShapeInLogFile);
	}
}


void CFreePcbView::Log(CString line, bool bSaveState) 
{
	// Inner routine that actually writes info to the log file.  If bSaveState is true, we write some additional info:
	//  --- a "u" line that indicates the current value of CPcbItem::next_uid;
	//  --- an "s" line that tells us the current cursor mode and the selected item(s)
	if (!m_doc->m_project_open) 
		return;
	CString logFilePath = m_doc->m_path_to_folder + "\\" + m_doc->m_pcb_filename + ".log";
	CStdioFile logFile;
	if (!logFile.Open( LPCSTR(logFilePath), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive, NULL ))
		return;
	logFile.SeekToEnd();
	logFile.WriteString(line);
	if (bSaveState)
	{
		line.Format("u %d\r\n", CPcbItem::GetNextUid());
		logFile.WriteString(line);
		line.Format("s %d ", m_cursor_mode);
		CString tmp;
		CIter<CPcbItem> ii (&m_sel);
		for (CPcbItem *i = ii.First(); i; i = ii.Next())
			tmp.Format("%d ", i->UID()),
			line += tmp;
		logFile.WriteString(line + "\r\n");
	}
	logFile.Close();
}

void CFreePcbView::Replay()
{
	// Replay a single instruction (or batch of instructions, e.g. a LOG_KEY followed by a LOG_ON_ADD_AREA if the logger did an F2 with nothing selected)
	m_bReplaying = true;
	CShape *fp;
	CString str;
	try
	{
		long pos = m_log_file.GetPosition();
		CString line, cmdStr;
		CArray<CString> params;
		if (!m_log_file.ReadString( line ))
			throw new CString((LPCSTR) IDS_EndOfLogFileReached);
		if (line[0]=='q')
			throw new CString((LPCSTR) IDS_LoggerCancelledOperation);
		if (!isdigit(line[0]))
			throw new CString((LPCSTR) IDS_CorruptLineInLogFile);
		int numParams = ParseKeyString(&line, &cmdStr, &params);
		int cmd = atoi(cmdStr);
		int param0 = numParams>0? atoi(params[0]): 0;
		m_log_file.Seek(pos, CFile::begin);				// Within whatever routine gets called, the first Log() command will need to parse "line" again
		switch (cmd)
		{
		case LOG_KEY:
			str.Format("Key press %d", param0);			// NB not bothering with the string table here
			HandleKeyPress(0, 0, 0);
			break;
		case LOG_LEFT_CLICK:
			str = "Left click";
			m_bLButtonDown = true;
			OnLButtonUp(0, CPoint(1000,0));				// Point (1000,0) will not be misinterpreted as a left-pane or bottom-pane click...
			break;
		case LOG_RIGHT_CLICK:
			str = "Right click/Esc";
			OnRButtonDown(0, CPoint(0,0));
			break;
		// Note that most of the following clauses will only get invoked if the logger did the operation via right-clicking or via menu items.
		// If logger used F-keys, then we'll enter the replaying via the LOG_KEY clause above.
		case LOG_ON_ADD_AREA:
			str = "OnAddArea";
			OnAddArea();
			break;
		case LOG_ON_ADD_BOARD_OUTLINE:
			str = "OnAddBoardOutline";
			OnAddBoardOutline();
			break;
		case LOG_ON_ADD_NET:
			str = "OnAddNet";
			OnAddNet();
			break;
		case LOG_ON_ADD_PART:
			str = "OnAddPart";
			OnAddPart();
			break;
		case LOG_ON_ADD_SIMILAR_AREA:
			str = "OnAddSimilarArea";
			OnAddSimilarArea();
			break;
		case LOG_ON_ADD_SOLDER_MASK_CUTOUT:
			str = "OnAddSolderMaskCutout";
			OnAddSoldermaskCutout();
			break;
		case LOG_ON_AREA_EDIT:
			str = "OnAreaEdit";
			OnAreaEdit();
			break;
		case LOG_ON_AREA_START_TRACE:
			str = "OnAreaStartTrace";
			OnAreaStartTrace();
			break;
		case LOG_ON_CHECK_PARTS_AND_NETS:
			str = "OnCheckPartsAndNets";
			OnCheckPartsAndNets();
			break;
		case LOG_ON_CLEAR_DRC:
			str = "OnToolsClearDrc";
			OnToolsClearDrc();
			break;
		case LOG_CHANGE_TRACE_LAYER:
			str.Format("ChangeTraceLayer %d", param0);
			ChangeTraceLayer(param0);
			break;
		case LOG_SET_WIDTH:
			str.Format("SetWidth %d", param0);
			SetWidth(param0);
			break;
		case LOG_ON_DELETE_CONNECTION:
			str = "OnDeleteConnection";
			OnDeleteConnection();
			break;
		case LOG_ON_DRC:
			str = "OnToolsDrc";
			OnToolsDrc();
			break;
		case LOG_ON_EXTERNAL_CHANGE_FP:
			str = "OnExternalChangeFootprint";
			fp = new CShape(m_doc);
			OnExternalChangeFootprint(fp);
			break;
		case LOG_ON_GROUP_COPY:
			str = "OnGroupCopy";
			OnGroupCopy();
			break;
		case LOG_ON_GROUP_CUT:
			str = "OnGroupCut";
			OnGroupCut();
			break;
		case LOG_ON_GROUP_DELETE:
			str = "OnGroupDelete";
			OnGroupDelete();
			break;
		case LOG_ON_GROUP_MOVE:
			str = "OnGroupMove";
			OnGroupMove();
			break;
		case LOG_ON_GROUP_PASTE:
			str = "OnGroupPaste";
			OnGroupPaste();
			break;
		case LOG_ON_GROUP_ROTATE:
			str = "OnGroupRotate";
			OnGroupRotate();
			break;
		case LOG_ON_GROUP_SAVE_TO_FILE:
			str = "OnGroupSaveToFile";
			OnGroupSaveToFile();
			break;
		case LOG_ON_OPTIMIZE:
			str = "OnOptimize";
			OnOptimize();
			break;
		case LOG_ON_PAD_ADD_TO_NET:
			str = "OnPadAddToNet";
			OnPadAddToNet();
			break;
		case LOG_ON_PAD_DETACH_FROM_NET:
			str = "OnPadDetachFromNet";
			OnPadDetachFromNet();
			break;
		case LOG_ON_PAD_OPTIMIZE:
			str = "OnPadOptimize";
			OnPadOptimize();
			break;
		case LOG_ON_PAD_START_RATLINE:
			str = "OnPadStartRatline";
			OnPadStartRatline();
			break;
		case LOG_ON_PAD_START_TRACE:
			str = "OnPadStartTrace";
			OnPadStartTrace();
			break;
		case LOG_ON_PART_CHANGE_SIDE:
			str = "OnPartChangeSide";
			OnPartChangeSide();
			break;
		case LOG_ON_PART_DELETE:
			str = "OnPartDelete";
			OnPartDelete();
			break;
		case LOG_ON_PART_GLUE:
			str = "OnPartGlue";
			OnPartGlue();
			break;
		case LOG_ON_PART_MOVE:
			str = "OnPartMove";
			OnPartMove();
			break;
		case LOG_ON_PART_OPTIMIZE:
			str = "OnPartOptimize";
			OnPartOptimize();
			break;
		case LOG_ON_PART_PROPERTIES:
			str = "OnPartProperties";
			OnPartProperties();
			break;
		case LOG_ON_PART_REF_PROPERTIES:
			str = "OnPartRefProperties";
			OnPartRefProperties();
			break;
		case LOG_ON_PART_ROTATE:
			str.Format("OnPartRotate %d", param0);
			OnPartRotate(param0);
			break;
		case LOG_ON_PART_UNGLUE:
			str = "OnPartUnglue";
			OnPartUnglue();
			break;
		case LOG_ON_PART_VALUE_PROPERTIES:
			str = "OnPartValueProperties";
			OnPartValueProperties();
			break;
		case LOG_ON_POLY_ADD_CUTOUT:
			str = "OnPolyAddCutout";
			OnPolyAddCutout();
			break;
		case LOG_ON_POLY_CORNER_DELETE:
			str = "OnPolyCornerDelete";
			OnPolyCornerDelete();
			break;
		case LOG_ON_POLY_CORNER_EDIT:
			str = "OnPolyCornerEdit";
			OnPolyCornerEdit();
			break;
		case LOG_ON_POLY_CORNER_MOVE:
			str = "OnPolyCornerMove";
			OnPolyCornerMove();
			break;
		case LOG_ON_POLY_DELETE:
			str = "OnPolyDelete";
			OnPolyDelete();
			break;
		case LOG_ON_POLY_DELETE_CUTOUT:
			str = "OnPolyDeleteCutout";
			OnPolyDeleteCutout();
			break;
		case LOG_ON_POLY_SIDE_ADD_CORNER:
			str = "OnPolySideAddCorner";
			OnPolySideAddCorner();
			break;
		case LOG_ON_POLY_SIDE_CONVERT:
			str.Format("OnPolySideConvert %d", param0);
			OnPolySideConvert(param0);
			break;
		case LOG_ON_RATLINE_CHANGE_END_PIN:
			str = "OnRatlineChangeEndPin";
			OnRatlineChangeEndPin();
			break;
		case LOG_ON_RATLINE_COMPLETE:
			str = "OnRatlineComplete";
			OnRatlineComplete();
			break;
		case LOG_ON_RATLINE_LOCK_CONNECTION:
			str = "OnRatlineLockConnection";
			OnRatlineLockConnection();
			break;
		case LOG_ON_RATLINE_ROUTE:
			str = "OnRatlineRoute";
			OnRatlineRoute();
			break;
		case LOG_ON_RATLINE_UNLOCK_CONNECTION:
			str = "OnRatlineUnlockConnection";
			OnRatlineUnlockConnection();
			break;
		case LOG_ON_REF_MOVE:
			str = "OnRefMove";
			OnRefMove();
			break;
		case LOG_ON_REF_PROPERTIES:
			str = "OnRefProperties";
			OnRefProperties();
			break;
		case LOG_ON_REF_ROTATE:
			str.Format("OnRefRotate %d", param0);
			OnRefRotate(param0);
			break;
		case LOG_ON_REPEAT_DRC:
			str = "OnToolsRepeatDrc";
			OnToolsRepeatDrc();
			break;
		case LOG_ON_SEGMENT_ADD_VERTEX:
			str = "OnSegmentAddVertex";
			OnSegmentAddVertex();
			break;
		case LOG_ON_SEGMENT_DELETE:
			str = "OnSegmentDelete";
			OnSegmentDelete();
			break;
		case LOG_ON_SEGMENT_MOVE:
			str = "OnSegmentMove";
			OnSegmentMove();
			break;
		case LOG_ON_SEGMENT_UNROUTE:
			str = "OnSegmentUnroute";
			OnSegmentUnroute();
			break;
		case LOG_ON_SM_EDIT:
			str = "OnSmEdit";
			OnSmEdit();
			break;
		case LOG_ON_TEE_DELETE:
			str = "OnTeeDelete";
			OnTeeDelete();
			break;
		case LOG_ON_TEE_MOVE:
			str = "OnTeeMove";
			OnTeeMove();
			break;
		case LOG_ON_TEE_PROPERTIES:
			str = "OnTeeProperties";
			OnTeeProperties();
			break;
		case LOG_ON_TEXT_ADD:
			str = "OnTextAdd";
			OnTextAdd();
			break;
		case LOG_ON_TEXT_DELETE:
			str = "OnTextDelete";
			OnTextDelete();
			break;
		case LOG_ON_TEXT_EDIT:
			str = "OnTextEdit";
			OnTextEdit();
			break;
		case LOG_ON_TEXT_MOVE:
			str = "OnTextMove";
			OnTextMove();
			break;
		case LOG_ON_TEXT_ROTATE:
			str.Format("OnTextRotate", param0);
			OnTextRotate(param0);
			break;
		case LOG_ON_TOOLS_MOVE_ORIGIN:
			str = "OnToolsMoveOrigin";
			OnToolsMoveOrigin();
			break;
		case LOG_ON_UNROUTE_TRACE:
			str = "OnUnrouteTrace";
			OnUnrouteTrace();
			break;
		case LOG_ON_VERTEX_ADD_VIA:
			str = "OnVertexAddVia";
			OnVertexAddVia();
			break;
		case LOG_ON_VERTEX_DELETE:
			str = "OnVertexDelete";
			OnVertexDelete();
			break;
		case LOG_ON_VERTEX_MOVE:
			str = "OnVertexMove";
			OnVertexMove();
			break;
		case LOG_ON_VERTEX_PROPERTIES:
			str = "OnVertexProperties";
			OnVertexProperties();
			break;
		case LOG_ON_VERTEX_REMOVE_VIA:
			str = "OnVertexRemoveVia";
			OnVertexRemoveVia();
			break;
		case LOG_ON_VERTEX_START_RATLINE:
			str = "OnVertexStartRatline";
			OnVertexStartRatline();
			break;
		case LOG_ON_VERTEX_START_TRACE:
			str = "OnVertexStartTrace";
			OnVertexStartTrace();
			break;
		case LOG_ON_EDIT_PASTE_FROM_FILE:
			str = "OnEditPasteFromFile";
			m_doc->OnEditPasteFromFile();
			break;
		case LOG_ON_EDIT_REDO:
			str = "OnEditRedo";
			m_doc->OnEditRedo();
			break;
		case LOG_ON_EDIT_UNDO:
			str = "OnEditUndo";
			m_doc->OnEditUndo();
			break;
		case LOG_ON_PROJECT_NETLIST:
			str = "OnProjectNetlist";
			m_doc->OnProjectNetlist();
			break;
		case LOG_ON_PROJECT_PARTLIST:
			str = "OnProjectPartlist";
			m_doc->OnProjectPartlist();
			break;
		case LOG_ON_TOOLS_CHECK_CONNECTIVITY:
			str = "OnToolsCheckConnectivity";
			m_doc->OnToolsCheckConnectivity();
			break;
		case LOG_ON_TOOLS_CHECK_COPPER_AREAS:
			str = "OnToolsCheckCopperAreas";
			m_doc->OnToolsCheckCopperAreas();
			break;
		case LOG_ON_TOOLS_CHECK_PARTS_AND_NETS:
			str = "OnToolsCheckPartsAndNets";
			m_doc->OnToolsCheckPartsAndNets();
			break;
		case LOG_ON_TOOLS_CHECK_TRACES:
			str = "OnToolsCheckTraces";
			m_doc->OnToolsCheckTraces();
			break;
		}
	}
	catch (CString *e)
	{
		// If the log file contains a "q" line, meaning the logger cancelled from a dialog box, we wind up here.  Also for
		// any other anomalies in the log file.
		str = *e;
		delete e;
	}

	m_log_status.Format("%d: %s", ++m_replay_ct, str);
	ShowSelectStatus();
	m_bReplaying = false;
}

void CFreePcbView::ReplayLoad()
{
	// When user hits F11, we come here:  load in a .log file, and start replay mode.  From now on, any key press (other than panning and
	// zooming) will launch the replay of a single step from the log file
	if( m_doc->FileClose() == IDCANCEL )
		return;

	CString s ((LPCSTR) IDS_LogFiles);
	CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_EXPLORER, 
		s, NULL, OPENFILENAME_SIZE_VERSION_500 );
	s.LoadStringA(IDS_ReplayLogFile);
	dlg.m_ofn.lpstrTitle = s; 
	int ret = dlg.DoModal();
	if( ret != IDOK )
		return;

	m_bReplayMode = true;
	CString pathname = dlg.GetPathName();
	int initialUid = CPcbItem::GetNextUid();
	if (!m_doc->FileOpen(pathname, false))
		{ m_bReplayMode = false; return; }

	// open log file (again), this time so that we can read the logging info (below the document info)
	if (!m_log_file.Open( pathname, CFile::modeRead, NULL ))
	{
		// bizarre...
		CString mess, s ((LPCSTR) IDS_UnableToOpenFile);
		mess.Format( s, pathname );
		AfxMessageBox( mess );
		m_bReplayMode = false;
		return;
	}

	// find beginning of logging section, marked by a "@@@" line
	CString line;
	do
	{
		if( !m_log_file.ReadString( line ) )
		{
			CString mess ((LPCSTR) IDS_UnableToFindLoggingDataInFile);
			AfxMessageBox( mess );
			m_bReplayMode = false;
			return;
		}
		line.Trim();
	}
	while( line != "@@@" );

	// find the first "u" line, which gives us an initial mapping of the logger's uid values to the replayer's
	do
	{
		if( !m_log_file.ReadString( line ) )
		{
			CString mess ((LPCSTR) IDS_UnableToFindLoggingDataInFile);
			AfxMessageBox( mess );
			m_bReplayMode = false;
			return;
		}
		line.Trim();
	}
	while( line[0] != 'u' );
	line.TrimLeft("u ");
	int initialUidLog = atoi(line);
	m_replay_uid_translate.RemoveAll();
	m_replay_uid_translate.Add( CPoint(initialUidLog, initialUid) );
	m_log_status.LoadStringA( IDS_HitAnyKeyToReplayLoggersFirstOperation );
	m_replay_ct = 0;
	m_bReplaying = true;					// Set this temporarily so that ShowSelectStatus will show the "Hit any key to replay..." msg
	ShowSelectStatus();
	m_bReplaying = false;
}

int CFreePcbView::ReplayUidTranslate(int logUid)
{
	// Given "logUid", a uid from the logger, translate it into a uid within the current replaying context.  Scans through m_replay_uid_translate,
	// and finds the last translation point (x,y) such that x<=logUid.  The correct translation is then logUid-x + y.
	if (logUid==-1)
		return -1;													// A uid of -1 is often used to signify a null item
	int lastX = -1, lastY = -1;
	for (int i = 0; i < m_replay_uid_translate.GetSize(); i++)
	{
		CPoint p = m_replay_uid_translate[i];
		if (p.x > logUid) break;
		lastX = p.x, lastY = p.y;
	}
	if (lastX<=0 || lastY<=0)
		throw new CString((LPCSTR) IDS_CorruptItemUidInLogFile);
	return logUid-lastX + lastY;
}

#endif // #ifdef CPT2_LOG