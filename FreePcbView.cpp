// FreePcbView.cpp : implementation of the CFreePcbView class
//

#include "stdafx.h"
#include "DlgAddText.h"
#include "DlgAssignNet.h"
#include "DlgSetSegmentWidth.h"
#include "DlgSetSegmentClearance.h"
#include "DlgEditBoardCorner.h"
#include "DlgAddArea.h"
#include "DlgRefText.h"
#include "MyToolBar.h"
#include <Mmsystem.h>
#include <sys/timeb.h>
#include <time.h>
#include <math.h>
#include "freepcbview.h"
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
#include "DlgRatWidth.h"
#include "DlgRatWidth.h"
#include "DlgSetPinClearance.h"


// globals
extern CFreePcbApp theApp;
//BOOL t_pressed = FALSE;
//BOOL n_pressed = FALSE;
BOOL gLastKeyWasArrow = FALSE;
int gTotalArrowMoveX = 0;
int gTotalArrowMoveY = 0;
BOOL gShiftKeyDown = FALSE;

BOOL gLastKeyWasGroupRotate = FALSE;
long long groupAverageX=0, groupAverageY=0;
int groupNumberItems=0;

HCURSOR my_cursor = LoadCursor( NULL, IDC_CROSS );

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ZOOM_RATIO 1.4

// constants for function key menu
#define FKEY_OFFSET_X 4
#define FKEY_OFFSET_Y 4
#define	FKEY_R_W 70
#define FKEY_R_H 30
#define FKEY_STEP (FKEY_R_W+5)
#define FKEY_GAP 20
#define FKEY_SEP_W 16

// constants for layer list
#define VSTEP 14

// macro for approximating angles to 1 degree accuracy
#define APPROX(angle,ref) ((angle > ref-M_PI/360) && (angle < ref+M_PI/360))

// these must be changed if context menu is edited
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
ON_COMMAND(ID_PAD_CONNECTTOPIN, OnPadConnectToPin)
ON_COMMAND(ID_SEGMENT_SETWIDTH, OnSegmentSetWidth)
ON_COMMAND(ID_SEGMENT_SETCLEARANCE, OnSegmentSetClearance)
ON_COMMAND(ID_SEGMENT_UNROUTE, OnSegmentUnroute)
ON_COMMAND(ID_RATLINE_ROUTE, OnRatlineRoute)
ON_COMMAND(ID_RATLINE_OPTIMIZE, OnRatlineOptimize)
ON_COMMAND(ID_VERTEX_MOVE, OnVertexMove)
ON_COMMAND(ID_VERTEX_DELETE, OnVertexDelete)
ON_COMMAND(ID_VERTEX_SETSIZE, OnVertexSize)
ON_COMMAND(ID_VERTEX_SETCLEARANCE, OnVertexClearance)
ON_COMMAND(ID_RATLINE_COMPLETE, OnRatlineComplete)
ON_COMMAND(ID_RATLINE_SETWIDTH, OnRatlineSetWidth)
ON_COMMAND(ID_RATLINE_SETRATLINEWIDTH, OnRatlineSetRatlineWidth)
ON_COMMAND(ID_RATLINE_SETCLEARANCE, OnRatlineSetClearance)
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
ON_COMMAND(ID_PAD_STARTSTUBTRACE, OnPadStartStubTrace)
ON_COMMAND(ID_PAD_SETCLEARANCE, OnPadSetClearance)
ON_COMMAND(ID_SEGMENT_DELETE, OnSegmentDelete)
ON_COMMAND(ID_ENDVERTEX_MOVE, OnEndVertexMove)
ON_COMMAND(ID_ENDVERTEX_ADDSEGMENTS, OnEndVertexAddSegments)
ON_COMMAND(ID_ENDVERTEX_ADDCONNECTION, OnEndVertexAddConnection)
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
ON_COMMAND(ID_ENDVERTEX_SETSIZE, OnVertexSize)
ON_COMMAND(ID_ENDVERTEX_SETVIACLEARANCE, OnVertexClearance)
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
ON_COMMAND(ID_NET_SETCLEARANCE, OnNetSetClearance)
ON_COMMAND(ID_CONNECT_SETWIDTH, OnConnectSetWidth)
ON_COMMAND(ID_CONNECT_SETCLEARANCE, OnConnectSetClearance)
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
ON_COMMAND(ID_GROUP_SAVETOFILE, OnGroupSaveToFile)
ON_COMMAND(ID_GROUP_COPY, OnGroupCopy)
ON_COMMAND(ID_GROUP_CUT, OnGroupCut)
ON_COMMAND(ID_GROUP_DELETE, OnGroupDelete)
ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
ON_COMMAND(ID_VERTEX_CONNECTTOPIN, OnVertexConnectToPin)
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
	// GetDocument() is not available at this point
	m_small_font.CreateFont( 14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial" );
#if 0
	m_small_font.CreateFont( 10, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "MS Sans Serif" );
#endif

	m_Doc = NULL;
	m_dlist = 0;
	m_last_mouse_point.x = 0;
	m_last_mouse_point.y = 0;
	m_last_cursor_point.x = 0;
	m_last_cursor_point.y = 0;
	m_left_pane_w = 110;	// the left pane on screen is this wide (pixels)
	m_bottom_pane_h = 40;	// the bottom pane on screen is this high (pixels)
	m_memDC_created = FALSE;
	m_dragging_new_item = FALSE;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	CalibrateTimer();
 }

// initialize the view object
// this code can't be placed in the constructor, because it depends on document
// don't try to draw window until this function has been called
// need only be called once
//
void CFreePcbView::InitInstance()
{
	// this should be called from InitInstance function of CApp,
	// after the document is created
	m_Doc = GetDocument();
	ASSERT_VALID(m_Doc);
	m_Doc->m_edit_footprint = FALSE;
	m_dlist = m_Doc->m_dlist;
	InitializeView();
	CRect screen_r;
	GetWindowRect( &screen_r );
	m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h,
		m_pcbu_per_pixel, m_org_x, m_org_y );
	for(int i=0; i<m_Doc->m_num_layers; i++ )
		m_dlist->SetLayerRGB( i, m_Doc->m_rgb[i][0], m_Doc->m_rgb[i][1], m_Doc->m_rgb[i][2] );
	ShowSelectStatus();
	ShowActiveLayer();
	m_Doc->m_view = this;
	// set up array of mask ids
	m_mask_id[SEL_MASK_PARTS].Set( ID_PART, ID_SEL_RECT );
	m_mask_id[SEL_MASK_REF].Set( ID_PART, ID_SEL_REF_TXT );
	m_mask_id[SEL_MASK_VALUE].Set( ID_PART, ID_SEL_VALUE_TXT );
	m_mask_id[SEL_MASK_PINS].Set( ID_PART, ID_SEL_PAD );
	m_mask_id[SEL_MASK_CON].Set( ID_NET, ID_CONNECT, 0, ID_SEL_SEG );
	m_mask_id[SEL_MASK_VIA].Set( ID_NET, ID_CONNECT, 0, ID_SEL_VERTEX );
	m_mask_id[SEL_MASK_AREAS].Set( ID_NET, ID_AREA );
	m_mask_id[SEL_MASK_TEXT].Set( ID_TEXT );
	m_mask_id[SEL_MASK_SM].Set( ID_SM_CUTOUT );
	m_mask_id[SEL_MASK_BOARD].Set( ID_BOARD );
	m_mask_id[SEL_MASK_DRC].Set( ID_DRC );
}

// initialize view with defaults for a new project
// should be called each time a new project is created
//
void CFreePcbView::InitializeView()
{
	if( !m_dlist )
		ASSERT(0);

	// set defaults
	SetCursorMode( CUR_NONE_SELECTED );
	m_sel_id.Clear();
	m_sel_layer = 0;
	m_dir = 0;
	m_debug_flag = 0;
	m_dragging_new_item = 0;
	m_active_layer = LAY_TOP_COPPER;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	m_sel_mask = 0xffff;
	SetSelMaskArray( m_sel_mask );
	m_inflection_mode = IM_90_45;
	m_snap_mode = SM_GRID_POINTS;

	// default screen coords in world units (i.e. display units)
	m_pcbu_per_pixel = 5.0*PCBU_PER_MIL;	// 5 mils per pixel
	m_org_x = -100.0*PCBU_PER_MIL;			// lower left corner of window
	m_org_y = -100.0*PCBU_PER_MIL;

	// grid defaults
	m_Doc->m_snap_angle = 45;

	m_left_pane_invalid = TRUE;
	Invalidate( FALSE );
}

// destructor
CFreePcbView::~CFreePcbView()
{
}

BOOL CFreePcbView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView drawing

void CFreePcbView::OnDraw(CDC* pDC)
{

	// get client rectangle
	GetClientRect( &m_client_r );

	// clear screen to black if no project open
	if( !m_Doc )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}
	if( !m_Doc->m_project_open )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}

	// draw stuff on left pane
	CRect r = m_client_r;
	int y_off = 10;
	int x_off = 10;
	if( m_left_pane_invalid )
	{
		// erase previous contents if changed
		CBrush brush( RGB(255, 255, 255) );
		CPen pen( PS_SOLID, 1, RGB(255, 255, 255) );
		CBrush * old_brush = pDC->SelectObject( &brush );
		CPen * old_pen = pDC->SelectObject( &pen );
		// erase left pane
		r.right = m_left_pane_w;
		r.bottom -= m_bottom_pane_h;
		pDC->Rectangle( &r );
		// erase bottom pane
		r = m_client_r;
		r.top = r.bottom - m_bottom_pane_h;
		pDC->Rectangle( &r );
		pDC->SelectObject( old_brush );
		pDC->SelectObject( old_pen );
		m_left_pane_invalid = FALSE;
	}
	CFont * old_font = pDC->SelectObject( &m_small_font );
	int index_to_active_layer;
	for( int i=0; i<m_Doc->m_num_layers; i++ )
	{
		// i = position index
		r.left = x_off;
		r.right = x_off+12;
		r.top = i*VSTEP+y_off;
		r.bottom = i*VSTEP+12+y_off;

		// il = layer index, since copper layers are displayed out of order
		int il = i;
		if( i == m_Doc->m_num_layers-1 && m_Doc->m_num_copper_layers > 1 )
			il = LAY_BOTTOM_COPPER;
		else if( i > LAY_TOP_COPPER )
			il = i+1;

		CBrush brush( RGB(m_Doc->m_rgb[il][0], m_Doc->m_rgb[il][1], m_Doc->m_rgb[il][2]) );
		if( m_Doc->m_vis[il] )
		{
			// if layer is visible, draw colored rectangle
			CBrush * old_brush = pDC->SelectObject( &brush );
			pDC->Rectangle( &r );
			pDC->SelectObject( old_brush );
		}
		else
		{
			// if layer is invisible, draw box with X
			pDC->Rectangle( &r );
			pDC->MoveTo( r.left, r.top );
			pDC->LineTo( r.right-1, r.bottom-1 );
			pDC->MoveTo( r.left, r.bottom-1 );
			pDC->LineTo( r.right-1, r.top );
		}
		r.left += 20;
		r.right += 120;
		r.bottom += 5;
		if( il == LAY_TOP_COPPER )
			pDC->DrawText( "top copper", -1, &r, DT_TOP );
		else if( il == LAY_BOTTOM_COPPER )
			pDC->DrawText( "bottom", -1, &r, 0 );
		else if( il == LAY_PAD_THRU )
			pDC->DrawText( "drilled hole", -1, &r, 0 );
		else
			pDC->DrawText( &layer_str[il][0], -1, &r, 0 );
		if( il >= LAY_TOP_COPPER )
		{
			CString num_str;
			num_str.Format( "[%c*]", layer_char[i-LAY_TOP_COPPER] );
			CRect nr = r;
			nr.left = nr.right - 55;
			pDC->DrawText( num_str, -1, &nr, DT_TOP );
		}
		CRect ar = r;
		ar.left = 2;
		ar.right = 8;
		ar.bottom -= 5;
		if( il == m_active_layer )
		{
			// draw arrowhead
			pDC->MoveTo( ar.left, ar.top+1 );
			pDC->LineTo( ar.right-1, (ar.top+ar.bottom)/2 );
			pDC->LineTo( ar.left, ar.bottom-1 );
			pDC->LineTo( ar.left, ar.top+1 );
		}
		else
		{
			// erase arrowhead
			pDC->FillSolidRect( &ar, RGB(255,255,255) );
		}
	}
	r.left = x_off;
	r.bottom += VSTEP*2;
	r.top += VSTEP*2;
	pDC->DrawText( "SELECTION MASK", -1, &r, DT_TOP );
	y_off = r.bottom;
	for( int i=0; i<NUM_SEL_MASKS; i++ )
	{
		// i = position index
		r.left = x_off;
		r.right = x_off+12;
		r.top = i*VSTEP+y_off;
		r.bottom = i*VSTEP+12+y_off;

		{
			CBrush green_brush( RGB(0, 255, 0) );
			CBrush red_brush( RGB(255, 0, 0) );
			CBrush * old_brush;

			if( m_sel_mask & (1<<i) )
			{
				// if mask is selected is visible, draw green rectangle
				old_brush = pDC->SelectObject( &green_brush );
			}
			else
			{
				// if mask not selected, draw red
				old_brush = pDC->SelectObject( &red_brush );
			}
			pDC->Rectangle( &r );
			pDC->SelectObject( old_brush );
		}

		r.left += 20;
		r.right += 120;
		r.bottom += 5;
		pDC->DrawText( sel_mask_str[i], -1, &r, DT_TOP );
	}
	r.left = x_off;
	r.bottom += VSTEP*2;
	r.top += VSTEP*2;
	pDC->DrawText( "* Use these", -1, &r, DT_TOP );
	r.bottom += VSTEP;
	r.top += VSTEP;
	pDC->DrawText( "keys to change", -1, &r, DT_TOP );
	r.bottom += VSTEP;
	r.top += VSTEP;
	pDC->DrawText( "routing layer", -1, &r, DT_TOP );

	// draw function keys on bottom pane
	DrawBottomPane();

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
// CFreePcbView diagnostics

#ifdef _DEBUG
void CFreePcbView::AssertValid() const
{
	CView::AssertValid();
}

void CFreePcbView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFreePcbDoc* CFreePcbView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFreePcbDoc)));
	return (CFreePcbDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView message handlers

// Window was resized
//
void CFreePcbView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	// update client rect and create clipping region
	GetClientRect( &m_client_r );
	m_pcb_rgn.DeleteObject();
	m_pcb_rgn.CreateRectRgn( m_left_pane_w, m_client_r.bottom-m_bottom_pane_h, m_client_r.right, m_client_r.top );

	// update display mapping for display list
	if( m_dlist )
	{
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
	}

	// create memory DC and DDB
	if( !m_memDC_created && m_client_r.right != 0 )
	{
		CDC * pDC = GetDC();
		m_memDC.CreateCompatibleDC( pDC );
		m_memDC_created = TRUE;
		m_bitmap.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
//		m_old_bitmap = m_memDC.SelectObject( &m_bitmap );
		m_old_bitmap = (HBITMAP)::SelectObject( m_memDC.m_hDC, m_bitmap.m_hObject );
		m_bitmap_rect = m_client_r;
		ReleaseDC( pDC );
	}
	else if( m_memDC_created && (m_bitmap_rect != m_client_r) )
	{
		CDC * pDC = GetDC();
//		m_memDC.SelectObject( m_old_bitmap );
		::SelectObject(m_memDC.m_hDC, m_old_bitmap );
		m_bitmap.DeleteObject();
		m_bitmap.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
//		m_old_bitmap = m_memDC.SelectObject( &m_bitmap );
		m_old_bitmap = (HBITMAP)::SelectObject( m_memDC.m_hDC, m_bitmap.m_hObject );
		m_bitmap_rect = m_client_r;
		ReleaseDC( pDC );
	}
}

// Left mouse button released, we should probably do something
//
void CFreePcbView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if( !m_bLButtonDown )
	{
		// this avoids problems with opening a project with the button held down
		CView::OnLButtonUp(nFlags, point);
		return;
	}

	CDC * pDC = NULL;
	CPoint tp = m_dlist->WindowToPCB( point );

	m_bLButtonDown = FALSE;
	gLastKeyWasArrow = FALSE;	// cancel series of arrow keys
	gLastKeyWasGroupRotate=false; // cancel series of group rotations
	if( m_bDraggingRect )
	{
		// we were dragging selection rect, handle it
		m_last_drag_rect.NormalizeRect();
		CPoint tl = m_dlist->WindowToPCB( m_last_drag_rect.TopLeft() );
		CPoint br = m_dlist->WindowToPCB( m_last_drag_rect.BottomRight() );
		m_sel_rect = CRect( tl, br );
		if( nFlags & MK_CONTROL )
		{
			// control key held down
			if( m_cursor_mode == CUR_PART_SELECTED )
			{
				if( m_sel_id.type != ID_PART && m_sel_id.st != ID_SEL_RECT )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_part );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			else if( m_cursor_mode == CUR_TEXT_SELECTED )
			{
				if( m_sel_id.type != ID_TEXT )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_text );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			else if( m_cursor_mode == CUR_SEG_SELECTED )
			{
				if( m_sel_id.type != ID_NET )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_net );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			if( m_cursor_mode == CUR_GROUP_SELECTED )
			{
				SelectItemsInRect( m_sel_rect, TRUE );
			}
			else
				SelectItemsInRect( m_sel_rect, FALSE );
		}
		else
		{
			SelectItemsInRect( m_sel_rect, FALSE );
		}
		m_bDraggingRect = FALSE;
		Invalidate( FALSE );
		CView::OnLButtonUp(nFlags, point);
		return;
	}

	if( point.y > (m_client_r.bottom-m_bottom_pane_h) )
	{
		// clicked in bottom pane, test for hit on function key rectangle
		for( int i=0; i<9; i++ )
		{
			CRect r( FKEY_OFFSET_X+i*FKEY_STEP+(i/4)*FKEY_GAP,
				m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H,
				FKEY_OFFSET_X+i*FKEY_STEP+(i/4)*FKEY_GAP+FKEY_R_W,
				m_client_r.bottom-FKEY_OFFSET_Y );
			if( r.PtInRect( point ) )
			{
				// fake function key pressed
				int nChar = i + 112;
				HandleKeyPress( nChar, 0, 0 );
			}
		}
	}
	else if( point.x < m_left_pane_w )
	{
		// clicked in left pane
		CRect r = m_client_r;
		int y_off = 10;
		int x_off = 10;
		for( int i=0; i<m_Doc->m_num_layers; i++ )
		{
			// i = position index
			// il = layer index, since copper layers are displayed out of order
			int il = i;
			if( i == m_Doc->m_num_layers-1 && m_Doc->m_num_copper_layers > 1 )
				il = LAY_BOTTOM_COPPER;
			else if( i > LAY_TOP_COPPER )
				il = i+1;
			// get color square
			r.left = x_off;
			r.right = x_off+12;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			if( r.PtInRect( point ) && il > LAY_BACKGND )
			{
				// clicked in color square
				m_Doc->m_vis[il] = !m_Doc->m_vis[il];
				m_dlist->SetLayerVisible( il, m_Doc->m_vis[il] );
				Invalidate( FALSE );
			}
			else
			{
				// get layer name rect
				r.left += 20;
				r.right += 120;
				r.bottom += 5;
				if( r.PtInRect( point ) )
				{
					// clicked on layer name
					if( i >= LAY_TOP_COPPER && i < LAY_TOP_COPPER + 8 )
					{
						int nChar = '1' + i - LAY_TOP_COPPER;
						HandleKeyPress( nChar, 0, 0 );
						Invalidate( FALSE );
					}
					else
					{
						switch( i )
						{
						case LAY_TOP_COPPER:    HandleKeyPress( '1', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+1:  HandleKeyPress( '2', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+2:  HandleKeyPress( '3', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+3:  HandleKeyPress( '4', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+4:  HandleKeyPress( '5', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+5:  HandleKeyPress( '6', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+6:  HandleKeyPress( '7', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+7:  HandleKeyPress( '8', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+8:  HandleKeyPress( 'Q', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+9:  HandleKeyPress( 'W', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+10: HandleKeyPress( 'E', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+11: HandleKeyPress( 'R', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+12: HandleKeyPress( 'T', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+13: HandleKeyPress( 'Y', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+14: HandleKeyPress( 'U', 0, 0 ); Invalidate( FALSE ); break;
						case LAY_TOP_COPPER+15: HandleKeyPress( 'I', 0, 0 ); Invalidate( FALSE ); break;
						}
					}
				}
			}
		}
		y_off = r.bottom + 2*VSTEP;
		for( int i=0; i<NUM_SEL_MASKS; i++ )
		{
			// get color square
			r.left = x_off;
			r.right = x_off+12+120;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			if( r.PtInRect( point ) )
			{
				// clicked in color square or name
				m_sel_mask = m_sel_mask ^ (1<<i);
				SetSelMaskArray( m_sel_mask );
				Invalidate( FALSE );
			}
		}
	}
	else
	{
		// clicked in PCB pane
		if(	CurNone() || CurSelected() )
		{
			// see if new item selected
			CPoint p = m_dlist->WindowToPCB( point );
			id sid;
			void * sel_ptr = NULL;
			if( m_sel_id.type == ID_PART )
				sel_ptr = m_sel_part;
			else if( m_sel_id.type == ID_NET )
				sel_ptr = m_sel_net;
			else if( m_sel_id.type == ID_TEXT )
				sel_ptr = m_sel_text;
			else if( m_sel_id.type == ID_DRC )
				sel_ptr = m_sel_dre;
			// save masks in case they are changed
			id old_mask_pins = m_mask_id[SEL_MASK_PINS];
			id old_mask_ref = m_mask_id[SEL_MASK_REF];
			if( nFlags & MK_CONTROL && m_mask_id[SEL_MASK_PARTS].ii == 0xfffe )
			{
				// if control key pressed and parts masked, also mask pins and ref
				m_mask_id[SEL_MASK_PINS].ii = 0xfffe;
				m_mask_id[SEL_MASK_REF].ii = 0xfffe;
			}
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sid, &m_sel_layer, &m_sel_id, sel_ptr,
				m_mask_id, NUM_SEL_MASKS );
			// restore mask
			m_mask_id[SEL_MASK_PINS] = old_mask_pins;
			m_mask_id[SEL_MASK_REF] = old_mask_ref;

			// check for second pad selected while holding down 's'
			SHORT kc = GetKeyState( 'S' );
			if( kc & 0x8000 && m_cursor_mode == CUR_PAD_SELECTED )
			{
				if( sid.type == ID_PART && sid.st == ID_SEL_PAD )
				{
					CString mess;
					// OK, now swap pads
					cpart * part1 = m_sel_part;
					CString pin_name1 = part1->shape->GetPinNameByIndex( m_sel_id.i );
					cnet * net1 = m_Doc->m_plist->GetPinNet(part1, &pin_name1);
					CString net_name1 = "unconnected";
					if( net1 )
						net_name1 = net1->name;
					cpart * part2 = (cpart*)ptr;
					CString pin_name2 = part2->shape->GetPinNameByIndex( sid.i );
					cnet * net2 = m_Doc->m_plist->GetPinNet(part2, &pin_name2);
					CString net_name2 = "unconnected";
					if( net2 )
						net_name2 = net2->name;
					if( net1 == NULL && net2 == NULL )
					{
						AfxMessageBox( "No connections to swap" );
						return;
					}
					mess.Format( "Swap %s.%s (\"%s\") and %s.%s (\"%s\") ?",
						part1->ref_des, pin_name1, net_name1,
						part2->ref_des, pin_name2, net_name2 );
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret == IDOK )
					{
						SaveUndoInfoFor2PartsAndNets( part1, part2, TRUE, m_Doc->m_undo_list );
						m_Doc->m_nlist->SwapPins( part1, &pin_name1, part2, &pin_name2 );
						m_Doc->ProjectModified( TRUE );
						ShowSelectStatus();
						Invalidate( FALSE );
					}
					return;
				}
			}

			// now handle new selection
			if( nFlags & MK_CONTROL )
			{
				// control key held down
				if(    sid.type == ID_PART
						&& m_mask_id[SEL_MASK_PARTS].ii != 0xfffe
					|| sid.type == ID_TEXT
						&& m_mask_id[SEL_MASK_TEXT].ii != 0xfffe
					|| (sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG
						&& ((cnet*)ptr)->connect[sid.i].seg[sid.ii].layer != LAY_RAT_LINE)
						&& m_mask_id[SEL_MASK_CON].ii != 0xfffe
					|| sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_VERTEX
						&& (((cnet*)ptr)->connect[sid.i].vtx[sid.ii].tee_ID
							|| ((cnet*)ptr)->connect[sid.i].vtx[sid.ii].force_via_flag )
						&& m_mask_id[SEL_MASK_VIA].ii != 0xfffe
					|| sid.type == ID_NET && sid.st == ID_AREA && sid.sst == ID_SEL_SIDE
						&& m_mask_id[SEL_MASK_AREAS].ii != 0xfffe
					|| sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT && sid.sst == ID_SEL_SIDE
						&& m_mask_id[SEL_MASK_SM].ii != 0xfffe
					|| sid.type == ID_BOARD && sid.st == ID_BOARD && sid.sst == ID_SEL_SIDE
						&& m_mask_id[SEL_MASK_BOARD].ii != 0xfffe
						)
				{
					// legal selection for group
					if( sid.type == ID_PART )
					{
						sid.st = ID_SEL_RECT;
						sid.i = 0;
						sid.sst = 0;
						sid.ii = 0;
					}
					// if previous single selection, convert to group
					if( m_cursor_mode == CUR_PART_SELECTED )
					{
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( m_sel_part );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					else if( m_cursor_mode == CUR_SEG_SELECTED )
					{
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( m_sel_net );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					else if( m_cursor_mode == CUR_AREA_SIDE_SELECTED )
					{
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( m_sel_net );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					else if( m_cursor_mode == CUR_SMCUTOUT_SIDE_SELECTED
						|| m_cursor_mode == CUR_BOARD_SIDE_SELECTED )
					{
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( NULL );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					else if( m_cursor_mode == CUR_TEXT_SELECTED )
					{
						if( m_sel_ids.GetSize() )
							ASSERT(0);
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( m_sel_text );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					// now add or remove from group
					if( m_cursor_mode == CUR_GROUP_SELECTED )
					{
						BOOL bFound = FALSE;
						for( int i=0; i<m_sel_ids.GetSize(); i++ )
						{
							id tid = m_sel_ids[i];
							void * tptr = m_sel_ptrs[i];
							if( tid == sid && m_sel_ptrs[i] == ptr )
							{
								bFound = TRUE;
								m_sel_ptrs.RemoveAt(i);
								m_sel_ids.RemoveAt(i);
							}
						}
						if( !bFound )
						{
							m_sel_ids.Add( sid );
							m_sel_ptrs.Add( ptr );
						}
						if( m_sel_ids.GetSize() == 0 )
						{
							CancelSelection();
						}
						else
						{
							HighlightGroup();
						}
						Invalidate( FALSE );
					}
				}
			}
			else if( sid.type == ID_DRC && sid.st == ID_SEL_DRE )
			{
				CancelSelection();
				DRError * dre = (DRError*)ptr;
				m_sel_id = sid;
				m_sel_dre = dre;
				m_Doc->m_drelist->HighLight( m_sel_dre );
				SetCursorMode( CUR_DRE_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_BOARD && sid.st == ID_BOARD_OUTLINE
				&& sid.sst == ID_SEL_CORNER )
			{
				CancelSelection();
				m_Doc->m_board_outline[sid.i].HighlightCorner( sid.ii );
				m_sel_id = sid;
				SetCursorMode( CUR_BOARD_CORNER_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_BOARD && sid.st == ID_BOARD_OUTLINE
				&& sid.sst == ID_SEL_SIDE )
			{
				CancelSelection();
				m_Doc->m_board_outline[sid.i].HighlightSide( sid.ii );
				m_sel_id = sid;
				SetCursorMode( CUR_BOARD_SIDE_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT
				&& sid.sst == ID_SEL_CORNER )
			{
				CancelSelection();
				m_Doc->m_sm_cutout[sid.i].HighlightCorner( sid.ii );
				m_sel_id = sid;
				SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT
				&& sid.sst == ID_SEL_SIDE )
			{
				CancelSelection();
				m_Doc->m_sm_cutout[sid.i].HighlightSide( sid.ii );
				m_sel_id = sid;
				SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_PART )
			{
				CancelSelection();
				m_sel_part = (cpart*)ptr;
				m_sel_id = sid;
				if( (GetKeyState('N') & 0x8000) && sid.st == ID_SEL_PAD )
				{
					// pad selected and if "n" held down, select net
					cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, sid.i );
					if( net )
					{
						m_sel_net = net;
						m_sel_id = net->id;
						m_sel_id.st = ID_ENTIRE_NET;
						m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
						m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
						SetCursorMode( CUR_NET_SELECTED );
					}
					else
					{
						m_Doc->m_plist->SelectPad( m_sel_part, sid.i );
						SetCursorMode( CUR_PAD_SELECTED );
						Invalidate( FALSE );
					}
				}
				else if( sid.st == ID_SEL_RECT )
				{
					SelectPart( m_sel_part );
					m_Doc->m_plist->SelectRefText( m_sel_part );
					m_Doc->m_plist->SelectValueText( m_sel_part );
				}
				else if( sid.st == ID_SEL_REF_TXT )
				{
					m_Doc->m_plist->SelectRefText( m_sel_part );
					SetCursorMode( CUR_REF_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_SEL_VALUE_TXT )
				{
					m_Doc->m_plist->SelectValueText( m_sel_part );
					SetCursorMode( CUR_VALUE_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_SEL_PAD )
				{
					m_Doc->m_plist->SelectPad( m_sel_part, sid.i );
					SetCursorMode( CUR_PAD_SELECTED );
					Invalidate( FALSE );
				}
			}
			else if( sid.type == ID_NET )
			{
				CancelSelection();
				m_sel_net = (cnet*)ptr;
				m_sel_id = sid;
				if( sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG )
				{
					// select segment
					m_Doc->m_nlist->HighlightSegment( m_sel_net, sid.i, sid.ii );
					if( m_sel_net->connect[sid.i].seg[sid.ii].layer != LAY_RAT_LINE )
						SetCursorMode( CUR_SEG_SELECTED );
					else
						SetCursorMode( CUR_RAT_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_CONNECT && sid.sst == ID_SEL_VERTEX )
				{
					// select vertex
					cconnect * c = &m_sel_net->connect[sid.i];
					if( c->end_pin == cconnect::NO_END && sid.ii == c->nsegs )
						SetCursorMode( CUR_END_VTX_SELECTED );
					else
						SetCursorMode( CUR_VTX_SELECTED );
					m_Doc->m_nlist->HighlightVertex( m_sel_net, sid.i, sid.ii );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_AREA && sid.sst == ID_SEL_SIDE )
				{
					// select copper area side
					m_Doc->m_nlist->SelectAreaSide( m_sel_net, sid.i, sid.ii );
					SetCursorMode( CUR_AREA_SIDE_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_AREA && sid.sst == ID_SEL_CORNER )
				{
					// select copper area corner
					m_Doc->m_nlist->SelectAreaCorner( m_sel_net, sid.i, sid.ii );
					SetCursorMode( CUR_AREA_CORNER_SELECTED );
					Invalidate( FALSE );
				}
				else
					ASSERT(0);
			}
			else if( sid.type == ID_TEXT )
			{
				CancelSelection();
				m_sel_text = (CText*)ptr;
				m_sel_id = sid;
				m_Doc->m_tlist->HighlightText( m_sel_text );
				SetCursorMode( CUR_TEXT_SELECTED );
				Invalidate( FALSE );
			}
			else
			{
				// nothing selected
				CancelSelection();
				m_sel_id.Clear();
				Invalidate( FALSE );
			}
		}
		else if( m_cursor_mode == CUR_DRAG_PART )
		{
			// complete move
			SetCursorMode( CUR_PART_SELECTED );
			CPoint p = m_dlist->WindowToPCB( point );
			m_Doc->m_plist->StopDragging();
			int old_angle = m_Doc->m_plist->GetAngle( m_sel_part );
			int angle = old_angle + m_dlist->GetDragAngle();
			angle = angle % 360;
			int old_side = m_sel_part->side;
			int side = old_side + m_dlist->GetDragSide();
			if( side > 1 )
				side = side - 2;

			// save undo info for part and attached nets
			if( !m_dragging_new_item )
				SaveUndoInfoForPartAndNets( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
			m_dragging_new_item = FALSE;

			// now move it
			m_sel_part->glued = 0;
			m_Doc->m_plist->Move( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
				angle, side );
			m_Doc->m_plist->HighlightPart( m_sel_part );
			m_Doc->m_nlist->PartMoved( m_sel_part );
			m_Doc->m_nlist->OptimizeConnections( m_sel_part );
			SetFKText( m_cursor_mode );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
#if 0
			//** for testing only
			CRect test_rect( 0, 0, 100, 100 );
			InvalidateRect( test_rect, FALSE );
			OnDraw( GetDC() );
#endif
		}
		else if( m_cursor_mode == CUR_DRAG_GROUP || m_cursor_mode == CUR_DRAG_GROUP_ADD )
		{
			// complete move
			m_Doc->m_dlist->StopDragging();
			if( m_cursor_mode == CUR_DRAG_GROUP )
				SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel_ptrs, &m_sel_ids, m_Doc->m_undo_list );
			MoveGroup( m_last_cursor_point.x - m_from_pt.x, m_last_cursor_point.y - m_from_pt.y );
			m_dlist->SetLayerVisible( LAY_HILITE, TRUE );
			HighlightGroup();
			if(m_cursor_mode == CUR_DRAG_GROUP_ADD)
				FindGroupCenter();
			SetCursorMode( CUR_GROUP_SELECTED );
			m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_MOVE_ORIGIN )
		{
			// complete move
			SetCursorMode( CUR_NONE_SELECTED );
			CPoint p = m_dlist->WindowToPCB( point );
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForMoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y, m_Doc->m_undo_list );
			MoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y );
			OnViewAllElements();
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_REF )
		{
			// complete move
			SetCursorMode( CUR_REF_SELECTED );
			CPoint p = m_dlist->WindowToPCB( point );
			m_Doc->m_plist->StopDragging();
			int drag_angle = m_dlist->GetDragAngle();
			// if part on bottom of board, drag angle is CCW instead of CW
			if( m_Doc->m_plist->GetSide( m_sel_part ) && drag_angle )
				drag_angle = 360 - drag_angle;
			int angle = m_Doc->m_plist->GetRefAngle( m_sel_part ) + drag_angle;
			if( angle>270 )
				angle = angle - 360;
			// save undo info
			SaveUndoInfoForPart( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
			// now move it
			m_Doc->m_plist->MoveRefText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
				angle, m_sel_part->m_ref_size, m_sel_part->m_ref_w );
			m_Doc->m_plist->SelectRefText( m_sel_part );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_VALUE )
		{
			// complete move
			SetCursorMode( CUR_VALUE_SELECTED );
			CPoint p = m_dlist->WindowToPCB( point );
			m_Doc->m_plist->StopDragging();
			int drag_angle = m_dlist->GetDragAngle();
			// if part on bottom of board, drag angle is CCW instead of CW
			if( m_Doc->m_plist->GetSide( m_sel_part ) && drag_angle )
				drag_angle = 360 - drag_angle;
			int angle = m_Doc->m_plist->GetValueAngle( m_sel_part ) + drag_angle;
			if( angle>270 )
				angle = angle - 360;
			// save undo info
			SaveUndoInfoForPart( m_sel_part,
				CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
			// now move it
			m_Doc->m_plist->MoveValueText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y,
				angle, m_sel_part->m_value_size, m_sel_part->m_value_w );
			m_Doc->m_plist->SelectValueText( m_sel_part );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_RAT )
		{
			// routing a ratline, add segment(s)
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
//			m_dlist->StopDragging();

			cconnect * c = &m_sel_net->connect[m_sel_ic];
			// test for termination of trace
			if( c->end_pin == cconnect::NO_END && m_sel_is == c->nsegs-1 && m_dir == 0
				&& c->vtx[c->nsegs].tee_ID )
			{
				// routing branch to tee-vertex, test for hit on tee-vertex
				cnet * hit_net;
				int hit_ic, hit_iv;
				BOOL bHit = m_Doc->m_nlist->TestForHitOnVertex( m_sel_net, 0,
					m_last_cursor_point.x, m_last_cursor_point.y,
					&hit_net, &hit_ic, &hit_iv );
				if( bHit && hit_net == m_sel_net )
				{
					int tee_ic, tee_iv;
					BOOL bTeeFound = m_Doc->m_nlist->FindTeeVertexInNet( m_sel_net, c->vtx[c->nsegs].tee_ID, &tee_ic, &tee_iv );
					if( bTeeFound && tee_ic == hit_ic && tee_iv == hit_iv )
					{
						// now route to tee-vertex
						SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

						CPoint pi = m_snap_angle_ref;
						CPoint pf = m_last_cursor_point;
						CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
						BOOL insert_flag = FALSE;
						if( pp != pi )
						{
							insert_flag = m_Doc->m_nlist->InsertSegment(
								m_sel_net,
								m_sel_ic, m_sel_is,
								pp.x, pp.y,
								m_active_layer,
								CConnectionWidthInfo(),
								m_dir
							);

							if( !insert_flag )
							{
								// hit end-vertex of segment, terminate routing
								goto cancel_selection_and_goodbye;
							}

							if( m_dir == 0 )
								m_sel_is++;
						}

						insert_flag = m_Doc->m_nlist->InsertSegment(
							m_sel_net,
							m_sel_ic, m_sel_is,
							m_last_cursor_point.x, m_last_cursor_point.y,
							m_active_layer,
							CConnectionWidthInfo(),
							m_dir
						);

						if( !insert_flag )
						{
							// hit end-vertex of segment, terminate routing
							goto cancel_selection_and_goodbye;
						}
						if( m_dir == 0 )
							m_sel_is++;

						// finish trace if necessary
						m_Doc->m_nlist->RouteSegment(
							m_sel_net,
							m_sel_ic, m_sel_is,
							m_active_layer,
							CConnectionWidthInfo()
						);

						m_Doc->m_nlist->ReconcileVia( m_sel_net, tee_ic, tee_iv );
						goto cancel_selection_and_goodbye;
					}
				}
			}
			else if( m_dir == 0 && c->vtx[m_sel_is+1].tee_ID || m_dir == 1 && c->vtx[m_sel_is].tee_ID )
			{
				// routing ratline to tee-vertex
				int tee_iv = m_sel_is + 1 - m_dir;
				cnet * hit_net;
				int hit_ic, hit_iv;

				BOOL bHit = m_Doc->m_nlist->TestForHitOnVertex( m_sel_net, 0,
					m_last_cursor_point.x, m_last_cursor_point.y,
					&hit_net, &hit_ic, &hit_iv );

				if( bHit && hit_net == m_sel_net && hit_ic == m_sel_ic && hit_iv == tee_iv )
				{
					// now route to tee-vertex
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

					CPoint pi = m_snap_angle_ref;
					CPoint pf = m_last_cursor_point;
					CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
					BOOL insert_flag = FALSE;
					if( pp != pi )
					{
						insert_flag = m_Doc->m_nlist->InsertSegment(
							m_sel_net,
							m_sel_ic, m_sel_is,
							pp.x, pp.y,
							m_active_layer,
							CConnectionWidthInfo(),
							m_dir
						);

						if( !insert_flag )
						{
							// hit end-vertex of segment, terminate routing
							goto cancel_selection_and_goodbye;
						}

						if( m_dir == 0 )
							m_sel_is++;
					}

					insert_flag = m_Doc->m_nlist->InsertSegment(
						m_sel_net,
						m_sel_ic, m_sel_is,
						m_last_cursor_point.x, m_last_cursor_point.y,
						m_active_layer,
						CConnectionWidthInfo(),
						CClearanceInfo(),
						m_dir );

					if( !insert_flag )
					{
						// hit end-vertex of segment, terminate routing
						goto cancel_selection_and_goodbye;
					}
					if( m_dir == 0 )
						m_sel_is++;

					// finish trace if necessary
					m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is, m_active_layer, CSegWidthInfo() );

					goto cancel_selection_and_goodbye;
				}
			}
			else if( m_sel_is == 0 && m_dir == 1 || m_sel_is == c->nsegs-1 && m_dir == 0 )
			{
				// routing ratline at end of trace, test for hit on any pad in net
				int ip = m_Doc->m_nlist->TestHitOnAnyPadInNet(
					m_last_cursor_point.x, m_last_cursor_point.y,
					m_active_layer,
					m_sel_net
				);

				int ns = m_sel_con.nsegs;
				if( ip != -1 )
				{
					// hit on pad in net, see if this is our starting pad
					if( ns < 3 && (m_dir == 0 && ip == m_sel_net->connect[m_sel_ic].start_pin
						|| m_dir == 1 && ip == m_sel_net->connect[m_sel_ic].end_pin) )
					{
						// starting pin with too few segments, don't route to pin
					}
					else
					{
						// route to pin
						// see if this is our destination
						if( !(m_dir == 0 && ip == m_sel_net->connect[m_sel_ic].end_pin
							|| m_dir == 1 && ip == m_sel_net->connect[m_sel_ic].start_pin) )
						{
							// no, change connection to this pin unless it is the starting pin
							cpart * hit_part = m_sel_net->pin[ip].part;
							CString * hit_pin_name = &m_sel_net->pin[ip].pin_name;
							m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, 1-m_dir, hit_part, hit_pin_name );
						}
						// now route to destination pin
						SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

						CPoint pi = m_snap_angle_ref;
						CPoint pf = m_last_cursor_point;
						CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
						BOOL insert_flag = FALSE;
						if( pp != pi )
						{
							insert_flag = m_Doc->m_nlist->InsertSegment(
								m_sel_net,
								m_sel_ic, m_sel_is,
								pp.x, pp.y,
								m_active_layer,
								CConnectionWidthInfo(),
								m_dir
							);

							if( !insert_flag )
							{
								// hit end-vertex of segment, terminate routing
								goto cancel_selection_and_goodbye;
							}
							if( m_dir == 0 )
								m_sel_is++;
						}

						insert_flag = m_Doc->m_nlist->InsertSegment(
							m_sel_net,
							m_sel_ic, m_sel_is,
							m_last_cursor_point.x, m_last_cursor_point.y,
							m_active_layer,
							CConnectionWidthInfo(),
							m_dir
						);

						if( !insert_flag )
						{
							// hit end-vertex of segment, terminate routing
							goto cancel_selection_and_goodbye;
						}
						if( m_dir == 0 )
							m_sel_is++;
						// finish trace to pad if necessary
						m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is, m_active_layer, CSegWidthInfo() );
						goto cancel_selection_and_goodbye;
					}
				}
			}
			// trace was not terminated, insert segment and continue routing
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

			CPoint pi = m_snap_angle_ref;
			CPoint pf = m_last_cursor_point;
			CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
			BOOL insert_flag = FALSE;
			if( pp != pi )
			{
				insert_flag = m_Doc->m_nlist->InsertSegment(
					m_sel_net,
					m_sel_ic, m_sel_is,
					pp.x, pp.y,
					m_active_layer,
					CConnectionWidthInfo(),
					m_dir
				);

				if( !insert_flag )
				{
					// hit end-vertex of segment, terminate routing
					goto cancel_selection_and_goodbye;
				}
				if( m_dir == 0 )
					m_sel_is++;
			}

			insert_flag = m_Doc->m_nlist->InsertSegment(
				m_sel_net,
				m_sel_ic, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y,
				m_active_layer,
				CConnectionWidthInfo(),
				m_dir );

			if( !insert_flag )
			{
				// hit end-vertex of segment, terminate routing
				goto cancel_selection_and_goodbye;
			}

			if( m_dir == 0 )
				m_sel_is++;

			m_Doc->m_nlist->StartDraggingSegment(
				pDC,
				m_sel_net,
				m_sel_id.i, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y,
				m_active_layer,
				LAY_SELECTION,
				m_active_layer,
				m_sel_net->def_width,
				m_dir,
				2
			);

			m_snap_angle_ref = m_last_cursor_point;
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
		{
			// add trace segment and vertex
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			m_dlist->StopDragging();

			// make undo record
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

			int layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is].layer;
			int insert_flag = m_Doc->m_nlist->InsertSegment(
				m_sel_net,
				m_sel_ic, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y,
				layer,
				m_sel_net->connect[m_sel_ic].seg[m_sel_is].seg_width,
				m_dir
			);

			CancelSelection();
			m_Doc->ProjectModified( TRUE );
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
			int ib = m_Doc->m_board_outline.GetSize();
			m_Doc->m_board_outline.SetSize( ib+1 );
			m_Doc->m_board_outline[ib].SetDisplayList( m_dlist );
			m_sel_id.i = ib;
			m_Doc->m_board_outline[ib].Start( LAY_BOARD_OUTLINE, 1, 20*NM_PER_MIL, p.x, p.y, 0, &m_sel_id, NULL );
			m_sel_id.sst = ID_SEL_CORNER;
			m_sel_id.ii = 0;
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );

			SetCursorMode( CUR_DRAG_BOARD_1 );

			m_Doc->ProjectModified( TRUE );
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
			m_Doc->m_board_outline[m_sel_id.i].AppendCorner( p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii++;
			SetCursorMode( CUR_DRAG_BOARD );
			m_Doc->ProjectModified( TRUE );
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
			if( p.x == m_Doc->m_board_outline[m_sel_id.i].GetX(0)
				&& p.y == m_Doc->m_board_outline[m_sel_id.i].GetY(0) )
			{
				// this point is the start point, close the polyline and quit
				SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
				m_Doc->m_board_outline[m_sel_id.i].Close( m_polyline_style );
				SetCursorMode( CUR_NONE_SELECTED );
				m_Doc->m_dlist->StopDragging();
			}
			else
			{
				// add corner to polyline
				m_Doc->m_board_outline[m_sel_id.i].AppendCorner( p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii++;
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
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
			SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
			m_Doc->m_board_outline[m_sel_id.i].InsertCorner( m_sel_id.ii+1, p.x, p.y );
			m_Doc->m_board_outline[m_sel_id.i].HighlightCorner( m_sel_id.ii+1 );
			SetCursorMode( CUR_BOARD_CORNER_SELECTED );
			m_sel_id.Set( ID_BOARD, ID_BOARD_OUTLINE, m_sel_id.i, ID_SEL_CORNER, m_sel_id.ii+1 );
			m_Doc->ProjectModified( TRUE );
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
			SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
			m_Doc->m_board_outline[m_sel_id.i].MoveCorner( m_sel_id.ii, p.x, p.y );
			m_Doc->m_board_outline[m_sel_id.i].HighlightCorner( m_sel_id.ii );
			SetCursorMode( CUR_BOARD_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int iarea = m_Doc->m_nlist->AddArea( m_sel_net, m_active_layer, p.x, p.y, m_polyline_hatch );
			m_sel_id.Set( m_sel_net->id.type, ID_AREA, iarea, ID_SEL_CORNER, 1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_1 );
			m_Doc->ProjectModified( TRUE );
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
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 2;
			SetCursorMode( CUR_DRAG_AREA );
			m_Doc->ProjectModified( TRUE );
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
			if( p.x == m_sel_net->area[m_sel_id.i].poly->GetX(0)
				&& p.y == m_sel_net->area[m_sel_id.i].poly->GetY(0) )
			{
				// cursor point is first point, close area
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
				m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
				m_Doc->m_dlist->StopDragging();
				m_Doc->m_nlist->OptimizeConnections( m_sel_net );
				CancelSelection();
			}
			else
			{
				// add cursor point
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii = m_sel_id.ii + 1;
				SetCursorMode( CUR_DRAG_AREA );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is, p.x, p.y );
			int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
			if( ret == -1 )
			{
				// error
				AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				TryToReselectAreaCorner( p.x, p.y );
				m_Doc->m_nlist->OptimizeConnections( m_sel_net );
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			m_Doc->m_nlist->InsertAreaCorner( m_sel_net, m_sel_ia, m_sel_is+1, p.x, p.y, CPolyLine::STRAIGHT );
			int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
			if( ret == -1 )
			{
				// error
				AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				TryToReselectAreaCorner( p.x, p.y );
				m_Doc->m_nlist->OptimizeConnections( m_sel_net );
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int ia = m_sel_id.i;
			carea * a = &m_sel_net->area[ia];
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
			m_sel_id.Set( m_sel_net->id.type, ID_AREA, ia, ID_SEL_CORNER, a->poly->GetNumCorners()-1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_CUTOUT_1 );
			m_Doc->ProjectModified( TRUE );
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
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 2;
			SetCursorMode( CUR_DRAG_AREA_CUTOUT );
			m_Doc->ProjectModified( TRUE );
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
			CPolyLine * poly = m_sel_net->area[m_sel_id.i].poly;
			int icontour = poly->GetContour( poly->GetNumCorners()-1 );
			int istart = poly->GetContourStart( icontour );
			if( p.x == poly->GetX(istart)
				&& p.y == poly->GetY(istart) )
			{
				// cursor point is first point, close area
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
				m_Doc->m_dlist->StopDragging();
				int n_old_areas = m_sel_net->area.GetSize();
				int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, FALSE );
				if( ret == -1 )
				{
					// error
					AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
					m_Doc->OnEditUndo();
				}
				else
					m_Doc->m_nlist->OptimizeConnections( m_sel_net );
				CancelSelection();
			}
			else
			{
				// add cursor point
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii = m_sel_id.ii + 1;
				SetCursorMode( CUR_DRAG_AREA_CUTOUT );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
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
			int ism = m_Doc->m_sm_cutout.GetSize();
			m_Doc->m_sm_cutout.SetSize( ism + 1 );
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[ism];
			p_sm->SetDisplayList( m_Doc->m_dlist );
			id id_sm( ID_SM_CUTOUT, ID_SM_CUTOUT, ism );
			m_sel_id = id_sm;
			p_sm->Start( m_polyline_layer, 0, 10*NM_PER_MIL, p.x, p.y, m_polyline_hatch, &m_sel_id, NULL );
			m_sel_id.sst = ID_SEL_CORNER;
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 1;
			SetCursorMode( CUR_DRAG_SMCUTOUT_1 );
			m_Doc->ProjectModified( TRUE );
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
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[m_sel_id.i];
			p_sm->AppendCorner( p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 2;
			SetCursorMode( CUR_DRAG_SMCUTOUT );
			m_Doc->ProjectModified( TRUE );
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
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( p.x == p_sm->GetX(0)
				&& p.y == p_sm->GetY(0) )
			{
				// cursor point is first point, close area
				SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
				p_sm->Close( m_polyline_style );
				SetCursorMode( CUR_NONE_SELECTED );
				m_Doc->m_dlist->StopDragging();
			}
			else
			{
				// add cursor point
				p_sm->AppendCorner( p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii = m_sel_id.ii + 1;
				SetCursorMode( CUR_DRAG_SMCUTOUT );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE )
		{
			SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			poly->MoveCorner( m_sel_id.ii, p.x, p.y );
			poly->HighlightCorner( m_sel_id.ii );
			SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT )
		{
			SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			poly->InsertCorner( m_sel_id.ii+1, p.x, p.y );
			poly->HighlightCorner( m_sel_id.ii+1 );
			m_sel_id.Set( ID_SM_CUTOUT, ID_SM_CUTOUT, m_sel_id.i, ID_SEL_CORNER, m_sel_id.ii+1 );
			SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int ia = m_sel_id.i;
			carea * a = &m_sel_net->area[ia];
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
			m_sel_id.Set( m_sel_net->id.type, ID_AREA, ia, ID_SEL_CORNER, a->poly->GetNumCorners()-1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_VTX )
		{
			// move vertex by modifying adjacent segments and reconciling via
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			CPoint p;
			p = m_last_cursor_point;
			int ic = m_sel_id.i;
			int ivtx = m_sel_id.ii;
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is, p.x, p.y );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, ic, ivtx );
			int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic );
			ReselectNetItemIfConnectionsChanged( new_ic );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_MOVE_SEGMENT )
		{
			// move vertex by modifying adjacent segments and reconciling via
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			CPoint cpi;
			CPoint cpf;
			m_Doc->m_dlist->Get_Endpoints(&cpi, &cpf);
			int ic = m_sel_id.i;
			int ivtx = m_sel_id.ii;
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,   cpi.x, cpi.y );
			ASSERT(cpi != cpf);			// Should be at least one grid snap apart.
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is+1, cpf.x, cpf.y );
			SetCursorMode( CUR_NONE_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_END_VTX )
		{
			// move end-vertex of stub trace
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			CPoint p;
			p = m_last_cursor_point;
			int ic = m_sel_id.i;
			int ivtx = m_sel_id.ii;
			m_Doc->m_nlist->MoveEndVertex( m_sel_net, ic, ivtx, p.x, p.y );
			SetCursorMode( CUR_END_VTX_SELECTED );
			int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic );
			ReselectNetItemIfConnectionsChanged( new_ic );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_CONNECT )
		{
			// dragging ratline to make a new connection
			// test for hit on pin
			CPoint p = m_dlist->WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, ID_SEL_PAD, 0, 0, 0 );	// force selection of pad
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
			if( ptr )
			{
				if( sel_id.type == ID_PART && sel_id.st == ID_SEL_PAD )
				{
					// hit on pin
					cpart * new_sel_part = (cpart*)ptr;
					cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
					if( m_sel_id.type == ID_NET  && m_sel_id.st == ID_CONNECT
						&& m_sel_id.sst == ID_SEL_VERTEX )
					{
						// dragging ratline from vertex of a trace
						if( new_sel_net && new_sel_net != m_sel_net )
						{
							// pin assigned to different net, can't connect it
							CString mess;
							mess.Format( "You are trying to connect a trace to a pin on a different net\nYou must detach the pin from the net first" );
							AfxMessageBox( mess );
						}
						else if( m_sel_con.end_pin == cconnect::NO_END
							&& m_sel_iv == m_sel_con.nsegs )
						{
							// dragging ratline from end of stub trace
							CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
							CPoint p = m_Doc->m_plist->GetPinPoint( new_sel_part, pin_name );
							int pin = m_Doc->m_nlist->GetNetPinIndex( m_sel_net, &new_sel_part->ref_des, &pin_name );
							if( pin == m_sel_con.start_pin )
							{
								// trying to connect pin to itself
								goto goodbye;
							}
							// convert to regular pin-pin trace
							if(  m_sel_con.vtx[m_sel_iv].tee_ID )
								ASSERT(0);	// error, should not be a branch end or a tee-vertex
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
							if( new_sel_net == 0 )
							{
								m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des,
									&pin_name );
							}
							m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic, p.x, p.y, LAY_RAT_LINE, CConnectionWidthInfo() );
							m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
							m_sel_con.vtx[m_sel_iv].force_via_flag = 0;
							m_sel_con.end_pin = pin;
							cvertex * end_v = &m_sel_con.vtx[m_sel_con.nsegs];
							end_v->pad_layer = m_Doc->m_plist->GetPinLayer( new_sel_part, sel_id.i );
							m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
							m_Doc->m_nlist->OptimizeConnections( m_sel_net );
							CancelSelection();
							m_Doc->m_dlist->StopDragging();
							Invalidate( FALSE );
							m_Doc->ProjectModified( TRUE );
						}
						else
						{
							// dragging ratline from any other vertex to pin
							// add new stub trace from pin to tee
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
							CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
							if( new_sel_net == 0 )
							{
								m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des,
									&pin_name );
							}
							int p1 = m_Doc->m_nlist->GetNetPinIndex( m_sel_net, &new_sel_part->ref_des, &pin_name );
							int ic = m_Doc->m_nlist->AddNetStub( m_sel_net, p1 );
							m_Doc->m_nlist->AppendSegment( m_sel_net, ic, m_sel_vtx.x, m_sel_vtx.y, LAY_RAT_LINE, CConnectionWidthInfo() );
							int id = m_sel_vtx.tee_ID;
							if( id == 0 )
							{
								id = m_Doc->m_nlist->GetNewTeeID();
								m_sel_vtx.tee_ID = id;
							}
							m_sel_net->connect[ic].vtx[1].tee_ID = m_sel_vtx.tee_ID;
							m_Doc->m_nlist->DrawConnection( m_sel_net, ic );
							m_Doc->m_nlist->OptimizeConnections( m_sel_net );
							CancelSelection();
							m_Doc->m_dlist->StopDragging();
							Invalidate( FALSE );
							m_Doc->ProjectModified( TRUE );
						}
					}
					else if( m_sel_id.type == ID_PART  && m_sel_id.st == ID_SEL_PAD )
					{
						// connecting pin to pin
						cnet * from_sel_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
						if( new_sel_net && from_sel_net && (new_sel_net != from_sel_net) )
						{
							// pins assigned to different nets, can't connect them
							CString mess;
							mess.Format( "You are trying to connect pins on different nets\nYou must detach one of them first" );
							AfxMessageBox( mess );
							m_Doc->m_dlist->StopDragging();
							SetCursorMode( CUR_PAD_SELECTED );
						}
						else
						{
							// see if we are trying to connect a pin to itself
							if( new_sel_part == m_sel_part && m_sel_id.i == sel_id.i )
							{
								// yes, forget it
								goto goodbye;
							}
							// we can connect these pins
							SaveUndoInfoForPart( m_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
							if( new_sel_net != from_sel_net )
							{
								// one pin is unassigned, assign it to net
								if( !new_sel_net )
								{
									// connecting to unassigned pin, assign it
									SaveUndoInfoForNetAndConnections( from_sel_net, CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
									CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
									m_Doc->m_nlist->AddNetPin( from_sel_net,
										&new_sel_part->ref_des,
										&pin_name );
									new_sel_net = from_sel_net;
								}
								else if( !from_sel_net )
								{
									// connecting from unassigned pin, assign it
									SaveUndoInfoForNetAndConnections( new_sel_net, CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
									CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&m_sel_part->ref_des,
										&pin_name );
									from_sel_net = new_sel_net;
								}
								else
									ASSERT(0);
							}
							else if( !new_sel_net && !m_sel_part->pin[m_sel_id.i].net )
							{
								// connecting 2 unassigned pins, select net
								DlgAssignNet assign_net_dlg;
								assign_net_dlg.m_map = &m_Doc->m_nlist->m_map;
								int ret = assign_net_dlg.DoModal();
								if( ret != IDOK )
								{
									m_Doc->m_dlist->StopDragging();
									SetCursorMode( CUR_PAD_SELECTED );
									goto goodbye;
								}
								CString name = assign_net_dlg.m_net_str;
								void * ptr;
								int test = m_Doc->m_nlist->m_map.Lookup( name, ptr );
								if( test )
								{
									// assign pins to existing net
									new_sel_net = (cnet*)ptr;
									SaveUndoInfoForNetAndConnections( new_sel_net,
										CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
									CString pin_name1 = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
									CString pin_name2 = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&m_sel_part->ref_des,
										&pin_name1 );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&new_sel_part->ref_des,
										&pin_name2 );
								}
								else
								{
									// make new net
									new_sel_net = m_Doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 10, CConnectionWidthInfo(), CClearanceInfo() );
									SaveUndoInfoForNetAndConnections( new_sel_net,
										CNetList::UNDO_NET_ADD, FALSE, m_Doc->m_undo_list );
									CString pin_name1 = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
									CString pin_name2 = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&m_sel_part->ref_des,
										&pin_name1 );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&new_sel_part->ref_des,
										&pin_name2 );
								}
							}
							// find pins in net and connect them
							int p1 = -1;
							int p2 = -1;
							for( int ip=0; ip<new_sel_net->npins; ip++ )
							{
								CString pin_name = new_sel_net->pin[ip].pin_name;
								if( new_sel_net->pin[ip].part == m_sel_part )
								{
									int pin_index = m_sel_part->shape->GetPinIndexByName( pin_name );
									if( pin_index == m_sel_id.i )
									{
										// found starting pin in net
										p1 = ip;
									}
								}
								if( new_sel_net->pin[ip].part == new_sel_part )
								{
									int pin_index = new_sel_part->shape->GetPinIndexByName( pin_name );
									if( pin_index == sel_id.i )
									{
										// found ending pin in net
										p2 = ip;
									}
								}
							}
							if( p1>=0 && p2>=0 )
								m_Doc->m_nlist->AddNetConnect( new_sel_net, p1, p2 );
							else
								ASSERT(0);	// couldn't find pins in net
							m_dlist->StopDragging();
							SetCursorMode( CUR_PAD_SELECTED );
						}
						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );
					}
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
		{
			// see if pad selected
			CPoint p = m_dlist->WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, ID_SEL_PAD, 0, 0, 0 );	// force selection of pad
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
			if( ptr )
			{
				if( sel_id.type == ID_PART )
				{
					if( sel_id.st == ID_SEL_PAD )
					{
						// see if we can connect to this pin
						cpart * new_sel_part = (cpart*)ptr;
						cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
						CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );

						if( new_sel_net && (new_sel_net != m_sel_net) )
						{
							// pin assigned to different net, can't connect it
							CString mess;
							mess.Format( "You are trying to connect to a pin on a different net" );
							AfxMessageBox( mess );
							return;
						}
						else if( new_sel_net == 0 )
						{
							// unassigned pin, assign it
							SaveUndoInfoForPart( new_sel_part,
								CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
							SaveUndoInfoForNetAndConnections( m_sel_net,
								CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
							m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des, &pin_name );
						}
						else
						{
							// pin already assigned to this net
							SaveUndoInfoForNetAndConnections( m_sel_net,
								CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
						}
						m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, m_sel_is,
							new_sel_part, &pin_name );
						m_dlist->Set_visible( m_sel_seg.dl_el, TRUE );
						m_dlist->StopDragging();
						m_dlist->CancelHighLight();
						SetCursorMode( CUR_RAT_SELECTED );
						m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
						m_Doc->m_nlist->SetAreaConnections( m_sel_net );
						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );
					}
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_STUB )
		{
			if( m_sel_is != 0 )
			{
				// if first vertex, we have already saved
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
			}

			// see if cursor on pad
			CPoint p = m_dlist->WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, ID_SEL_PAD, 0, 0, 0 );	// test for hit on pad
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
			if( ptr && sel_id.type == ID_PART && sel_id.st == ID_SEL_PAD )
			{
				// see if we can connect to this pin
				cpart * new_sel_part = (cpart*)ptr;
				cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
				CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
				if( m_Doc->m_plist->TestHitOnPad( new_sel_part, &pin_name, p.x, p.y, m_active_layer ) )
				{
					// check for starting pad of stub trace
					cpart * origin_part = m_sel_start_pin.part;
					CString * origin_pin_name = &m_sel_start_pin.pin_name;
					if( origin_part != new_sel_part || *origin_pin_name != *pin_name )
					{
						// not starting pad
						if( new_sel_net && (new_sel_net != m_sel_net) )
						{
							// pin assigned to different net, can't connect it
							CString mess;
							mess.Format( "You are trying to connect to a pin on a different net" );
							AfxMessageBox( mess );
							return;
						}
						else if( new_sel_net == 0 )
						{
							// unassigned pin, assign it
							SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );

							m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des, &pin_name );
						}
						else
						{
							// pin already assigned to this net
						}
						CPoint pi = m_snap_angle_ref;
						CPoint pf = m_last_cursor_point;
						CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
						if( pp != pi )
						{
							m_sel_id.ii = m_Doc->m_nlist->AppendSegment(
								m_sel_net,
								m_sel_ic,
								pp.x, pp.y,
								m_active_layer,
								CConnectionWidthInfo()
							);
						}

						m_sel_id.ii = m_Doc->m_nlist->AppendSegment(
							m_sel_net,
							m_sel_ic,
							m_last_cursor_point.x, m_last_cursor_point.y,
							m_active_layer,
							CConnectionWidthInfo()
						);

						CPoint pin_point = m_Doc->m_plist->GetPinPoint( new_sel_part, pin_name );
						if( m_last_cursor_point != pin_point )
						{
							m_sel_id.ii = m_Doc->m_nlist->AppendSegment(
								m_sel_net,
								m_sel_ic,
								pin_point.x, pin_point.y,
								m_active_layer,
								CConnectionWidthInfo()
							);
						}
						m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, TRUE, new_sel_part, &pin_name );

						m_dlist->StopDragging();
						SetCursorMode( CUR_NONE_SELECTED );
						m_Doc->m_nlist->SetAreaConnections( m_sel_net );

						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );

						return;
					}
				}
			}
			// test for tee-connection to vertex
			cnet * hit_net = NULL;
			int hit_ic, hit_iv;
			BOOL bHit = m_Doc->m_nlist->TestForHitOnVertex(
				m_sel_net,
				m_active_layer,
				m_last_cursor_point.x, m_last_cursor_point.y,
				&hit_net, &hit_ic, &hit_iv
			);
			if( bHit )
			{
				// hit on vertex
				cconnect * hit_c;
				cvertex * hit_v;
				hit_c = &m_sel_net->connect[hit_ic];
				hit_v = &hit_c->vtx[hit_iv];
				int ret = AfxMessageBox( "Branch created" );
				// first, see if already a tee
				int id;
				if( hit_v->tee_ID )
				{
					// yes
					id = hit_v->tee_ID;
				}
				else
				{
					// no
					id = m_Doc->m_nlist->GetNewTeeID();
					hit_v->tee_ID = id;
				}
				// now connect it (no via)
				CPoint pi = m_snap_angle_ref;
				CPoint pf = m_last_cursor_point;
				CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
				if( pp != pi )
				{
					m_sel_id.ii = m_Doc->m_nlist->AppendSegment(
						m_sel_net,
						m_sel_ic,
						pp.x, pp.y,
						m_active_layer,
						CConnectionWidthInfo()
					);
				}
				m_sel_id.ii = m_Doc->m_nlist->AppendSegment(
					m_sel_net,
					m_sel_ic,
					m_last_cursor_point.x, m_last_cursor_point.y,
					m_active_layer,
					CConnectionWidthInfo()
				);
				if( m_last_cursor_point.x != hit_v->x || m_last_cursor_point.y != hit_v->y )
				{
					m_sel_id.ii = m_Doc->m_nlist->AppendSegment(
						m_sel_net,
						m_sel_ic,
						hit_v->x, hit_v->y,
						m_active_layer,
						CConnectionWidthInfo()
					);
				}
				// set tee_ID for end-vertex and remove selector
				m_sel_con.vtx[m_sel_iv+1].tee_ID = id;

				m_Doc->m_nlist->ReconcileVia( m_sel_net, m_sel_ic, m_sel_iv+1 );

				m_dlist->StopDragging();

				m_Doc->m_nlist->OptimizeConnections( m_sel_net );

				CancelSelection();
				m_Doc->ProjectModified( TRUE );
				Invalidate( FALSE );

				return;
			}

			// come here if not connecting to anything
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			m_sel_vtx.force_via_flag = 0;
			CPoint pi = m_snap_angle_ref;
			CPoint pf = m_last_cursor_point;
			CPoint pp = GetInflectionPoint( pi, pf, m_inflection_mode );
			if( pp != pi )
			{
				m_sel_id.ii = m_Doc->m_nlist->AppendSegment(
					m_sel_net,
					m_sel_ic,
					pp.x, pp.y,
					m_active_layer,
					CConnectionWidthInfo()
				);
			}
			m_sel_id.ii = m_Doc->m_nlist->AppendSegment(
				m_sel_net, m_sel_ic,
				m_last_cursor_point.x, m_last_cursor_point.y,
				m_active_layer,
				CConnectionWidthInfo()
			);

			m_dlist->StopDragging();
			m_sel_id.ii++;
			m_Doc->m_nlist->StartDraggingStub(
				pDC,
				m_sel_net,
				m_sel_ic, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y,
				m_active_layer,
				m_sel_net->def_width.m_seg_width.m_val,
				m_active_layer,
				m_sel_net->def_width.m_via_width.m_val,
				m_sel_net->def_width.m_via_hole.m_val,
				2,
				m_inflection_mode
			);
			m_snap_angle_ref = m_last_cursor_point;
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_TEXT )
		{
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			if( !m_dragging_new_item )
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
			int old_angle = m_sel_text->m_angle;
			int angle = old_angle + m_dlist->GetDragAngle();
			if( angle>270 )
				angle = angle - 360;
			int old_mirror = m_sel_text->m_mirror;
			int mirror = (old_mirror + m_dlist->GetDragSide())%2;
			BOOL negative = m_sel_text->m_bNegative;;
			int layer = m_sel_text->m_layer;
			m_Doc->m_tlist->MoveText( m_sel_text, m_last_cursor_point.x, m_last_cursor_point.y,
				angle, mirror, negative, layer );
			if( m_dragging_new_item )
			{
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD, TRUE, m_Doc->m_undo_list );
				m_dragging_new_item = FALSE;
			}
			SetCursorMode( CUR_TEXT_SELECTED );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
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
		goto goodbye;

cancel_selection_and_goodbye:
		m_dlist->StopDragging();
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );

goodbye:
		ShowSelectStatus();
	}
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
	CView::OnLButtonDblClk(nFlags, point);
}

// right mouse button
//
void CFreePcbView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_disable_context_menu = 1;
	if( m_cursor_mode == CUR_DRAG_PART )
	{
		m_Doc->m_plist->CancelDraggingPart( m_sel_part );
		if( m_dragging_new_item )
		{
			CancelSelection();
			m_Doc->OnEditUndo();	// remove the part
		}
		else
		{
			SetCursorMode( CUR_PART_SELECTED );
			m_Doc->m_plist->HighlightPart( m_sel_part );
		}
		m_dragging_new_item = FALSE;
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_REF )
	{
		m_Doc->m_plist->CancelDraggingRefText( m_sel_part );
		SetCursorMode( CUR_REF_SELECTED );
		m_Doc->m_plist->SelectRefText( m_sel_part );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_VALUE )
	{
		m_Doc->m_plist->CancelDraggingValue( m_sel_part );
		SetCursorMode( CUR_VALUE_SELECTED );
		m_Doc->m_plist->SelectValueText( m_sel_part );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT )
	{
		m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_RAT_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
	{
		m_dlist->StopDragging();
		m_dlist->Set_visible( m_sel_seg.dl_el, TRUE );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_RAT_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX )
	{
		m_Doc->m_nlist->CancelDraggingVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_VTX_SELECTED );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_MOVE_SEGMENT )
	{
		m_Doc->m_nlist->CancelMovingSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_SEG_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX_INSERT )
	{
		m_Doc->m_nlist->CancelDraggingSegmentNewVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_SEG_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_END_VTX )
	{
		m_Doc->m_nlist->CancelDraggingEndVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_END_VTX_SELECTED );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_CONNECT )
	{
		m_Doc->m_dlist->StopDragging();
		if( m_sel_id.type == ID_PART )
			SetCursorMode( CUR_PAD_SELECTED );
		else
			SetCursorMode( CUR_VTX_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_TEXT )
	{
		m_Doc->m_tlist->CancelDraggingText( m_sel_text );
		if( m_dragging_new_item )
		{
			m_Doc->m_tlist->RemoveText( m_sel_text );
			CancelSelection();
			m_dragging_new_item = 0;
		}
		else
		{
			SetCursorMode( CUR_TEXT_SELECTED );
		}
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_SMCUTOUT
		  || m_cursor_mode == CUR_DRAG_SMCUTOUT_1
		  || (m_cursor_mode == CUR_DRAG_SMCUTOUT && m_sel_id.ii<3) )
	{
		// dragging first, second or third corner of solder mask cutout
		// delete it and cancel dragging
		m_dlist->StopDragging();
		if( m_cursor_mode != CUR_ADD_SMCUTOUT )
			m_Doc->m_sm_cutout.RemoveAt( m_sel_id.i );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT )
	{
		// dragging fourth or higher corner of solder mask cutout, close it
		m_dlist->StopDragging();
		SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
		m_Doc->m_sm_cutout[m_sel_id.i].Close( m_polyline_style );
		m_Doc->ProjectModified( TRUE );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
		poly->MakeVisible();
		poly->HighlightSide( m_sel_id.ii );
		SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
		poly->MakeVisible();
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
		poly->HighlightCorner( m_sel_id.ii );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_BOARD
		  || m_cursor_mode == CUR_DRAG_BOARD_1
		  || (m_cursor_mode == CUR_DRAG_BOARD && m_Doc->m_board_outline[m_sel_id.i].GetNumCorners()<3) )
	{
		// dragging first, second or third corner of board outline
		// just delete it (if necessary) and cancel
		m_dlist->StopDragging();
		if( m_cursor_mode != CUR_ADD_BOARD )
			m_Doc->m_board_outline.RemoveAt( m_sel_id.i );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD )
	{
		// dragging fourth or higher corner of board outline, close it
		m_dlist->StopDragging();
		SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
		m_Doc->m_board_outline[m_sel_id.i].Close( m_polyline_style );
		m_Doc->ProjectModified( TRUE );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_INSERT )
	{
		m_dlist->StopDragging();
		m_Doc->m_board_outline[m_sel_id.i].MakeVisible();
		m_Doc->m_board_outline[m_sel_id.i].HighlightSide( m_sel_id.i );
		SetCursorMode( CUR_BOARD_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_MOVE )
	{
		// get indexes for preceding and following corners
		m_dlist->StopDragging();
		m_Doc->m_board_outline[m_sel_id.i].MakeVisible();
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
		m_Doc->m_board_outline[m_sel_id.i].HighlightCorner( m_sel_id.i );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_1
		  || (m_cursor_mode == CUR_DRAG_AREA && m_sel_id.ii<3) )
	{
		m_dlist->StopDragging();
		m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA)
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
		m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
		CancelSelection();
		m_Doc->m_nlist->OptimizeConnections( m_sel_net );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
		  || (m_cursor_mode == CUR_DRAG_AREA_CUTOUT && m_sel_id.ii<3) )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = m_sel_net->area[m_sel_id.i].poly;
		int ncont = poly->GetNumContours();
		poly->RemoveContour(ncont-1);
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
		int icont = m_sel_net->area[m_sel_ia].poly->GetNumContours() - 1;
		int ic = m_sel_net->area[m_sel_ia].poly->GetContourStart(icont);
		CPoint p;
		p.x = m_sel_net->area[m_sel_ia].poly->GetX(ic);
		p.y = m_sel_net->area[m_sel_ia].poly->GetY(ic);
		int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, FALSE );
		if( ret == -1 )
		{
			// error
			AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
			m_Doc->OnEditUndo();
		}
		TryToReselectAreaCorner( p.x, p.y );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
	{
		m_Doc->m_nlist->CancelDraggingInsertedAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_Doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
	{
		m_Doc->m_nlist->CancelDraggingAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_Doc->m_nlist->SelectAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_STUB )
	{
		m_dlist->StopDragging();
		if( m_sel_id.ii > 0 )
		{
			m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
			int x = m_sel_net->connect[m_sel_ic].vtx[m_sel_iv].x;
			int y = m_sel_net->connect[m_sel_ic].vtx[m_sel_iv].y;
			BOOL test = m_Doc->m_nlist->TestPointInArea( m_sel_net, x, y, m_active_layer, NULL );
			if( !test )
			{
				// add a via and optimize
				OnEndVertexAddVia();	// this also optimizes and selects via
				SetCursorMode( CUR_END_VTX_SELECTED );
			}
			else
			{
				// just optimize
				int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic );
				ReselectNetItemIfConnectionsChanged( new_ic );
			}
		}
		else
		{
			m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
			CancelSelection();
		}
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP_ADD )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
		m_Doc->OnEditUndo();
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
	ShowSelectStatus();
	CView::OnRButtonDown(nFlags, point);
}

// System Key on keyboard pressed down
//
void CFreePcbView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 121 )
		OnKeyDown( nChar, nRepCnt, nFlags);
	else
		CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

// System Key on keyboard pressed up
//
void CFreePcbView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar != 121 )
		CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

// Key pressed up
//
void CFreePcbView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 16 )
	{
		gShiftKeyDown = FALSE;
	}
	if( nChar == 'D' )
	{
		// 'd'
		m_Doc->m_drelist->MakeHollowCircles();
		Invalidate( FALSE );
	}
	else if( nChar == 16 || nChar == 17 )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set mode
			if( nChar == 17 )
				m_snap_mode = SM_GRID_POINTS;
			if( nChar == 16 && m_Doc->m_snap_angle == 45 )
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
	if( nChar == 16 )
		gShiftKeyDown = TRUE;
	if( nChar == 'D' )
	{
		// 'd'
		m_Doc->m_drelist->MakeSolidCircles();
		Invalidate( FALSE );
	}
	else if( nChar == 16 || nChar == 17 )
	{
		if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing a trace segment, set mode
			if( nChar == 17 )	// ctrl
				m_snap_mode = SM_GRID_LINES;
			if( nChar == 16 && m_Doc->m_snap_angle == 45 )	// shift
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
//
void CFreePcbView::HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( m_bDraggingRect )
		return;

	if( nChar == 'F' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_END_VTX_SELECTED ) )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_iv, FALSE );
		int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic );
		ReselectNetItemIfConnectionsChanged( new_ic );
		ShowSelectStatus();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		return;
	}
	if( nChar == 'U' && ( m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_END_VTX_SELECTED ) )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->UnforceVia( m_sel_net, m_sel_ic, m_sel_iv );
		if( m_cursor_mode == CUR_END_VTX_SELECTED
			&& m_sel_con.seg[m_sel_iv-1].layer == LAY_RAT_LINE
			&& m_sel_vtx.tee_ID == 0 )
		{
			m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_iv-1, TRUE );
			CancelSelection();
		}
		else
		{
			m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
		}
		int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic );
		ReselectNetItemIfConnectionsChanged( new_ic );
		ShowSelectStatus();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
		return;
	}

	if( nChar == 'T' && (m_cursor_mode == CUR_VTX_SELECTED || m_cursor_mode == CUR_SEG_SELECTED ) )
	{
		// "t" pressed, select trace
		m_sel_id.sst = ID_ENTIRE_CONNECT;
		m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
		SetCursorMode( CUR_CONNECT_SELECTED );
		Invalidate( FALSE );
	}

	if( nChar == 'N' )
	{
		// "n" pressed, select net
		if( m_cursor_mode == CUR_VTX_SELECTED
			|| m_cursor_mode == CUR_SEG_SELECTED
			|| m_cursor_mode == CUR_CONNECT_SELECTED
			|| m_cursor_mode == CUR_AREA_CORNER_SELECTED
			|| m_cursor_mode == CUR_AREA_SIDE_SELECTED
			|| m_cursor_mode == CUR_RAT_SELECTED)
		{
			m_sel_id.st = ID_ENTIRE_NET;
			m_Doc->m_nlist->HighlightNet( m_sel_net );
			m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
			SetCursorMode( CUR_NET_SELECTED );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_PAD_SELECTED )
		{
			// pad selected and if "n" held down, select net
			cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, m_sel_id.i );
			if( net )
			{
				m_sel_net = net;
				m_sel_id = net->id;
				m_sel_id.st = ID_ENTIRE_NET;
				m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
				m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
				SetCursorMode( CUR_NET_SELECTED );
			}
		}
	}

	if( nChar == 27 )
	{
		// ESC key, if something selected, cancel it
		// otherwise, fake a right-click
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
			// backup, if possible, by unrouting preceding segment and changing active layer
			if( m_dir == 0 && m_sel_is > 0 )
			{
				// routing forward
				if( m_sel_net->connect[m_sel_ic].vtx[m_sel_is].tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is-1].layer;
					m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is-1 );
					m_sel_is--;
					ShowSelectStatus();
					m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].x;
					m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].y;
					CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
					SetCursorPos( p.x, p.y );
					OnRatlineRoute();
					m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
			}
			else if( m_dir == 1 && m_sel_is < m_sel_net->connect[m_sel_ic].nsegs-1
				&& !(m_sel_is == m_sel_net->connect[m_sel_ic].nsegs-2
				&& m_sel_net->connect[m_sel_ic].end_pin == cconnect::NO_END ) )
			{
				// routing backward, not at end of stub trace
				if( m_sel_net->connect[m_sel_ic].vtx[m_sel_is+1].tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
					int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is+1].layer;
					m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is+1 );
					ShowSelectStatus();
					m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is+1].x;
					m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is+1].y;
					CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
					SetCursorPos( p.x, p.y );
					OnRatlineRoute();
					m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing stub trace
			if( m_sel_is > 1 )
			{
				if( m_sel_net->connect[m_sel_ic].vtx[m_sel_is].tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1, FALSE );
					int ns = m_sel_con.nsegs;
					m_sel_is = ns;
					ShowSelectStatus();
					m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].x;
					m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].y;
					CPoint p = m_dlist->PCBToScreen( m_last_mouse_point );
					SetCursorPos( p.x, p.y );
					OnEndVertexAddSegments();
					int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is-1].layer;
					m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
			}
			else
			{
				if( m_sel_net->connect[m_sel_ic].vtx[m_sel_is].tee_ID )
				{
					AfxMessageBox( "tee-vertex reached" );
				}
				else
				{
					m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
					SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
					cpart * sel_part = m_Doc->m_plist->GetPart( m_sel_start_pin.ref_des );
					int i = sel_part->shape->GetPinIndexByName( m_sel_start_pin.pin_name );
					m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
					m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
					CancelSelection();
					m_sel_net = NULL;
					m_sel_part = sel_part;
					m_sel_id = sel_part->m_id;
					m_sel_id.st = ID_PAD;
					m_sel_id.i = i;
					m_Doc->m_plist->SelectPad( sel_part, i );
					OnPadStartStubTrace();
				}
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

	// test for pressing key to change layer
	char test_char = nChar;
	if( test_char >= 97 )
		test_char = '1' + nChar - 97;
	char * ch = strchr( layer_char, test_char );
	if( ch && m_Doc->m_num_copper_layers > 1 )
	{
		int ilayer = ch - layer_char;
		if( ilayer < m_Doc->m_num_copper_layers )
		{
			// OK, shortcut key to a copper layer
			int new_active_layer = ilayer + LAY_TOP_COPPER;
			if( ilayer == m_Doc->m_num_copper_layers-1 )
				new_active_layer = LAY_BOTTOM_COPPER;
			else if( new_active_layer > LAY_TOP_COPPER )
				new_active_layer++;
			if( !gShiftKeyDown )
			{
				// shift key not held down, change active layer for routing
				if( !m_Doc->m_vis[new_active_layer] )
				{
					PlaySound( TEXT("CriticalStop"), 0, 0 );
					AfxMessageBox( "Can't route on invisible layer" );
					ReleaseDC( pDC );
					return;
				}
				if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB)
				{
					// if we are routing, change layer
					pDC->SelectClipRgn( &m_pcb_rgn );
					SetDCToWorldCoords( pDC );
					if( m_sel_id.ii == 0 && m_dir == 0 )
					{
						// we are trying to change first segment from pad
						int p1 = m_sel_con.start_pin;
						CString pin_name = m_sel_net->pin[p1].pin_name;
						int pin_index = m_sel_net->pin[p1].part->shape->GetPinIndexByName( pin_name );
						if( m_sel_net->pin[p1].part->shape->m_padstack[pin_index].hole_size == 0)
						{
							// SMT pad, this is illegal;
							new_active_layer = -1;
							PlaySound( TEXT("CriticalStop"), 0, 0 );
						}
					}
					else if( m_sel_id.ii == (m_sel_con.nsegs-1) && m_dir == 1 )
					{
						// we are trying to change last segment to pad
						int p2 = m_sel_con.end_pin;
						if( p2 != -1 )
						{
							CString pin_name = m_sel_net->pin[p2].pin_name;
							int pin_index = m_sel_net->pin[p2].part->shape->GetPinIndexByName( pin_name );
							if( m_sel_net->pin[p2].part->shape->m_padstack[pin_index].hole_size == 0)
							{
								// SMT pad
								new_active_layer = -1;
								PlaySound( TEXT("CriticalStop"), 0, 0 );
							}
						}
					}
					if( new_active_layer != -1 )
					{
						m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
						m_active_layer = new_active_layer;
						ShowActiveLayer();
					}
				}
				else
				{
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
				return;
			}
			else
			{
				// shift key held down, change layer if item selected
				if( m_cursor_mode == CUR_SEG_SELECTED )
				{
					SaveUndoInfoForConnection( m_sel_net, m_sel_ic, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
					cconnect * c = &m_sel_net->connect[m_sel_ic];
					cseg * seg = &c->seg[m_sel_is];
					seg->layer = new_active_layer;
					m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
					m_Doc->ProjectModified( TRUE );
					Invalidate( FALSE );
				}
				else if( m_cursor_mode == CUR_CONNECT_SELECTED )
				{
					SaveUndoInfoForConnection( m_sel_net, m_sel_ic, TRUE, m_Doc->m_undo_list );
					m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
					cconnect * c = &m_sel_net->connect[m_sel_ic];
					for( int is=0; is<c->nsegs; is++ )
					{
						cseg * seg = &c->seg[is];
						seg->layer = new_active_layer;
					}
					m_Doc->m_nlist->DrawConnection( m_sel_net, m_sel_ic );
					m_Doc->ProjectModified( TRUE );
					Invalidate( FALSE );
				}
				else if( m_cursor_mode == CUR_AREA_CORNER_SELECTED
					|| m_cursor_mode == CUR_AREA_SIDE_SELECTED )
				{
					SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
					carea * a = &m_sel_net->area[m_sel_ia];
					a->poly->Undraw();
					a->poly->SetLayer( new_active_layer );
					a->poly->Draw( m_dlist );
					int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, TRUE, TRUE );
					if( ret == -1 )
					{
						// error
						AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
						m_Doc->OnEditUndo();
					}
					else
					{
						m_Doc->m_nlist->OptimizeConnections( m_sel_net );
					}
					CancelSelection();
					m_Doc->ProjectModified( TRUE );
					Invalidate( FALSE );
				}
				return;
			}
		}
	}

	// continue
	if( nChar >= 112 && nChar <= 123 )
	{
		// function key pressed
		fk = m_fkey_option[nChar-112];
	}
	if( nChar >= 37 && nChar <= 40 )
	{
		// arrow key
		BOOL bShift;
		SHORT kc = GetKeyState( VK_SHIFT );
		if( kc < 0 )
			bShift = TRUE;
		else
			bShift = FALSE;
		fk = FK_ARROW;
		int d;
		if( bShift && m_Doc->m_units == MM )
			d = 10000;		// 0.01 mm
		else if( bShift && m_Doc->m_units == MIL )
			d = 25400;		// 1 mil
		else if( m_sel_id.type == ID_NET )
			d = m_Doc->m_routing_grid_spacing;
		else
			d = m_Doc->m_part_grid_spacing;
		if( nChar == 37 )
			dx -= d;
		else if( nChar == 39 )
			dx += d;
		else if( nChar == 38 )
			dy += d;
		else if( nChar == 40 )
			dy -= d;
	}
	else
		gLastKeyWasArrow = FALSE;

	switch( m_cursor_mode )
	{
	case  CUR_NONE_SELECTED:
		if( fk == FK_ADD_AREA )
			OnAddArea();
		else if( fk == FK_ADD_TEXT )
			OnTextAdd();
		else if( fk == FK_ADD_PART )
			m_Doc->OnAddPart();
		else if( fk == FK_REDO_RATLINES )
		{
			SaveUndoInfoForAllNets( TRUE, m_Doc->m_undo_list );
			//			StartTimer();
			m_Doc->m_nlist->OptimizeConnections();
			//			double time = GetElapsedTime();
			Invalidate( FALSE );
		}
		break;

	case CUR_PART_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				if( m_sel_part->glued )
				{
					int ret = AfxMessageBox( "This part is glued, do you want to unglue it ?  ", MB_YESNO );
					if( ret == IDYES )
						m_sel_part->glued = 0;
					else
						return;
				}
				SaveUndoInfoForPartAndNets( m_sel_part,
					CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_plist->Move( m_sel_part,
				m_sel_part->x+dx,
				m_sel_part->y+dy,
				m_sel_part->angle,
				m_sel_part->side );
			m_Doc->m_nlist->PartMoved( m_sel_part );
			m_Doc->m_nlist->OptimizeConnections( m_sel_part );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->HighlightPart( m_sel_part );
			ShowRelativeDistance( m_sel_part->x, m_sel_part->y,
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_DELETE_PART || nChar == 46 )
			OnPartDelete();
		else if( fk == FK_EDIT_PART )
			m_Doc->OnPartProperties();
		else if( fk == FK_EDIT_FOOTPRINT )
		{
			m_Doc->m_edit_footprint = TRUE;
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
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForPart( m_sel_part,
					CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			CPoint ref_pt = m_Doc->m_plist->GetRefPoint( m_sel_part );
			m_Doc->m_plist->MoveRefText( m_sel_part,
										ref_pt.x + dx,
										ref_pt.y + dy,
										m_sel_part->m_ref_angle,
										m_sel_part->m_ref_size,
										m_sel_part->m_ref_w );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->SelectRefText( m_sel_part );
			ShowRelativeDistance( m_Doc->m_plist->GetRefPoint(m_sel_part).x,
				m_Doc->m_plist->GetRefPoint(m_sel_part).y,
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
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
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForPart( m_sel_part,
					CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			CPoint val_pt = m_Doc->m_plist->GetValuePoint( m_sel_part );
			m_Doc->m_plist->MoveValueText( m_sel_part,
										val_pt.x + dx,
										val_pt.y + dy,
										m_sel_part->m_value_angle,
										m_sel_part->m_value_size,
										m_sel_part->m_value_w );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->SelectValueText( m_sel_part );
			ShowRelativeDistance( m_Doc->m_plist->GetValuePoint(m_sel_part).x,
				m_Doc->m_plist->GetValuePoint(m_sel_part).y,
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
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
			if(!SegmentMovable())
			{
				PlaySound( TEXT("CriticalStop"), 0, 0 );
				break;
			}

			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();

			// 1. Move the line defined by the segment
			m_last_pt.x = m_sel_last_vtx.x;
			m_last_pt.y = m_sel_last_vtx.y;

			m_from_pt.x = m_sel_vtx.x;
			m_from_pt.y = m_sel_vtx.y;

			m_to_pt.x = m_sel_next_vtx.x;
			m_to_pt.y = m_sel_next_vtx.y;

			int nsegs = m_sel_con.nsegs;
			int use_third_segment = m_sel_is < nsegs - 1;
			if(use_third_segment)
			{
				m_next_pt.x = m_sel_next_next_vtx.x;	// Shouldn't really do this if we're off the edge?
				m_next_pt.y = m_sel_next_next_vtx.y;
			} else {
				m_next_pt.x = 0;
				m_next_pt.y = 0;
			}

			// 1. Move the endpoints of (xi, yi), (xf, yf) of the line by the mouse movement. This
			//		is just temporary, since the final ending position is determined by the intercept
			//		points with the leading and trailing segments:
			int new_from_x = m_from_pt.x + dx;
			int new_from_y = m_from_pt.y + dy;

			int new_to_x = m_to_pt.x + dx;
			int new_to_y = m_to_pt.y + dy;

			int old_x0_dir = sign(m_from_pt.x - m_last_pt.x);
			int old_y0_dir = sign(m_from_pt.y - m_last_pt.y);

			int old_x1_dir = sign(m_to_pt.x - m_from_pt.x);
			int old_y1_dir = sign(m_to_pt.y - m_from_pt.y);

			int old_x2_dir = sign(m_next_pt.x - m_to_pt.x);
			int old_y2_dir = sign(m_next_pt.y - m_to_pt.y);

			// 2. Find the intercept between the extended segment in motion and the leading segment.
			double d_new_from_x;
			double d_new_from_y;
			FindLineIntersection(m_last_pt.x, m_last_pt.y, m_from_pt.x, m_from_pt.y,
									new_from_x,    new_from_y,	   new_to_x,    new_to_y,
									&d_new_from_x, &d_new_from_y);
			int i_nudge_from_x = floor(d_new_from_x + .5);
			int i_nudge_from_y = floor(d_new_from_y + .5);

			// 3. Find the intercept between the extended segment in motion and the trailing segment:
			int i_nudge_to_x, i_nudge_to_y;
			if(use_third_segment)
			{
				double d_new_to_x;
				double d_new_to_y;
				FindLineIntersection(new_from_x,    new_from_y,	   new_to_x,    new_to_y,
									 m_to_pt.x,		m_to_pt.y,	m_next_pt.x, m_next_pt.y,
										&d_new_to_x, &d_new_to_y);

				i_nudge_to_x = floor(d_new_to_x + .5);
				i_nudge_to_y = floor(d_new_to_y + .5);
			} else {
				i_nudge_to_x = new_to_x;
				i_nudge_to_y = new_to_y;
			}

			// If we drag too far, the line segment can reverse itself causing a little triangle to form.
			//   That's a bad thing.
			if(    sign(i_nudge_to_x - i_nudge_from_x) == old_x1_dir
				&& sign(i_nudge_to_y - i_nudge_from_y) == old_y1_dir
				&& sign(i_nudge_from_x - m_last_pt.x) == old_x0_dir
				&& sign(i_nudge_from_y - m_last_pt.y) == old_y0_dir
				&& (!use_third_segment || (sign(m_next_pt.x - i_nudge_to_x) == old_x2_dir
										&& sign(m_next_pt.y - i_nudge_to_y) == old_y2_dir)))
			{
			//	Move both vetices to the new position:
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
											i_nudge_from_x, i_nudge_from_y );
				m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is+1,
											i_nudge_to_x, i_nudge_to_y );
			} else {
				break;
			}

			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_vtx.x, m_sel_vtx.y, gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
			m_Doc->ProjectModified( TRUE );
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
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
										m_sel_vtx.x + dx, m_sel_vtx.y + dy );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_vtx.x, m_sel_vtx.y, gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_VIA_SIZE )
			OnVertexSize();
		else if( fk == FK_MOVE_VERTEX )
			OnVertexMove();
		else if( fk == FK_ADD_CONNECT )
			OnVertexConnectToPin();
		else if( fk == FK_DELETE_VERTEX )
			OnVertexDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_END_VTX_SELECTED:
		if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_ADD_CONNECT )
			OnVertexConnectToPin();
		else if( fk == FK_MOVE_VERTEX )
			OnEndVertexMove();
		else if( fk == FK_DELETE_VERTEX )
			OnVertexDelete();
		else if( fk == FK_ADD_SEGMENT )
			OnEndVertexAddSegments();
		else if( fk == FK_ADD_VIA )
			OnEndVertexAddVia();
		else if( fk == FK_DELETE_VIA )
			OnEndVertexRemoveVia();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
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
			OnRatlineOptimize();	//**
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		break;

	case  CUR_NET_SELECTED:
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
		else if( fk == FK_START_STUB )
			OnPadStartStubTrace();
		else if( fk == FK_ADD_CONNECT )
			OnPadConnectToPin();
		else if( fk == FK_DETACH_NET )
			OnPadDetachFromNet();
		else if( fk == FK_REDO_RATLINES )
			OnPadOptimize();
		break;

	case CUR_TEXT_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_tlist->MoveText( m_sel_text,
						m_sel_text->m_x + dx, m_sel_text->m_y + dy,
						m_sel_text->m_angle, m_sel_text->m_mirror,
						m_sel_text->m_bNegative, m_sel_text->m_layer );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( m_sel_text->m_x, m_sel_text->m_y,
				gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_EDIT_TEXT )
			OnTextEdit();
		else if( fk == FK_MOVE_TEXT )
			OnTextMove();
		else if( fk == FK_DELETE_TEXT || nChar == 46 )
			OnTextDelete();
		break;

	case CUR_BOARD_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_Doc->m_board_outline[m_sel_id.i];
			poly->MoveCorner( m_sel_is,
				poly->GetX( m_sel_is ) + dx,
				poly->GetY( m_sel_is ) + dy );
			m_dlist->CancelHighLight();
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( poly->GetX( m_sel_is ), poly->GetY( m_sel_is ),
				gTotalArrowMoveX, gTotalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_Doc->ProjectModified( TRUE );
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
		if( fk == FK_POLY_STRAIGHT )
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
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_ic];
			poly->MoveCorner( m_sel_is,
				poly->GetX( m_sel_is ) + dx,
				poly->GetY( m_sel_is ) + dy );
			m_dlist->CancelHighLight();
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( poly->GetX( m_sel_is ), poly->GetY( m_sel_is ),
				gTotalArrowMoveX, gTotalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_Doc->ProjectModified( TRUE );
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
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( fk == FK_POLY_STRAIGHT )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::STRAIGHT;
				poly->SetSideStyle( m_sel_id.ii, m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.ii );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_POLY_ARC_CW )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::ARC_CW;
				poly->SetSideStyle( m_sel_id.ii, m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.ii );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_POLY_ARC_CCW )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::ARC_CCW;
				poly->SetSideStyle( m_sel_id.ii, m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.ii );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_ADD_CORNER )
				OnSmSideInsertCorner();
			else if( fk == FK_DELETE_CUTOUT || nChar == 46 )
				OnSmSideDeleteCutout();
		}
		break;

	case CUR_AREA_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = m_sel_net->area[m_sel_ic].poly;
			CPoint p;
			p.x = poly->GetX(m_sel_is)+dx;
			p.y = poly->GetY(m_sel_is)+dy;
			poly->MoveCorner( m_sel_is, p.x, p.y );
			int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
			if( ret == -1 )
			{
				// error
				AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				gTotalArrowMoveX += dx;
				gTotalArrowMoveY += dy;
				ShowRelativeDistance( p.x, p.y, gTotalArrowMoveX, gTotalArrowMoveY );
				TryToReselectAreaCorner( p.x, p.y );
			}
			m_Doc->ProjectModified( TRUE );
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
		if( fk == FK_SIDE_STYLE )
			OnAreaSideStyle();
		else if( fk == FK_EDIT_AREA )
			OnAreaEdit();
		else if( fk == FK_POLY_ARC_CCW )
		{
			SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
//			SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
			m_polyline_style = CPolyLine::ARC_CCW;
			m_Doc->m_nlist->SetAreaSideStyle( m_sel_net, m_sel_ia, m_sel_is, m_polyline_style );
			m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
			m_Doc->m_nlist->OptimizeConnections( m_sel_net );
			SetFKText( m_cursor_mode );
			Invalidate( FALSE );
			m_Doc->ProjectModified( TRUE );
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
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			if( poly->GetContour( m_sel_id.ii ) > 0 )
				OnAreaDeleteCutout();
			else
				OnAreaSideDeleteArea();
		}
		break;

	case CUR_DRE_SELECTED:
		if( nChar == 46 )
		{
			CancelSelection();
			m_Doc->m_drelist->Remove( m_sel_dre );
			Invalidate( FALSE );
		}
		break;

	case CUR_GROUP_SELECTED:
		if( fk == FK_ARROW )
		{
			m_dlist->CancelHighLight();
			if( !gLastKeyWasArrow && !gLastKeyWasGroupRotate)
			{
				if( GluedPartsInGroup() )
				{
					int ret = AfxMessageBox( "This group contains glued parts, do you want to unglue them ?  ", MB_YESNO );
					if( ret != IDYES )
						return;
				}
				SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel_ptrs, &m_sel_ids, m_Doc->m_undo_list );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			MoveGroup( dx, dy );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			HighlightGroup();
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_MOVE_GROUP )
		{
			OnGroupMove();
		}
		else if( fk == FK_DELETE_GROUP || nChar == 46 )
		{
			OnGroupDelete();
		}
		else if(fk == FK_ROTATE_GROUP)
		{
			OnGroupRotate();
		}
		break;

	case CUR_DRAG_RAT:
		if( fk == FK_COMPLETE )
		{
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

			int test = m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is, m_active_layer, CConnectionWidthInfo() );
			if( !test )
			{
				m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
				CancelSelection();
			}
			else
			{
				PlaySound( TEXT("CriticalStop"), 0, 0 );
			}

			Invalidate( FALSE );
			m_Doc->ProjectModified( TRUE );
		}
		break;

	case CUR_DRAG_STUB:
		break;

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

	if( nChar == ' ' )
	{
		// space bar pressed, center window on cursor then center cursor
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
		Invalidate( FALSE );
		p = m_dlist->PCBToScreen( p );
		SetCursorPos( p.x, p.y );
	}
	else if( nChar == VK_HOME )
	{
		// home key pressed, ViewAllElements
		OnViewAllElements();
	}
	else if( nChar == 33 )
	{
		// PgUp pressed, zoom in
		if( m_pcbu_per_pixel > 254 )
		{
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
			CRect screen_r;
			GetWindowRect( &screen_r );
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
				m_org_x, m_org_y );
			Invalidate( FALSE );
			p = m_dlist->PCBToScreen( p );
			SetCursorPos( p.x, p.y );
		}
	}
	else if( nChar == 34 )
	{
		// PgDn pressed, zoom out
		// first, make sure that window boundaries will be OK
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
			m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
				m_org_x, m_org_y );
			Invalidate( FALSE );
			p = m_dlist->PCBToScreen( p );
			SetCursorPos( p.x, p.y );
		}
	}
	ReleaseDC( pDC );
	if( gLastKeyWasArrow ==FALSE && gLastKeyWasGroupRotate==false )
		ShowSelectStatus();
}

// Mouse moved
//
void CFreePcbView::OnMouseMove(UINT nFlags, CPoint point)
{
	static BOOL bCursorOn = TRUE;

	if( (nFlags & MK_LBUTTON) && m_bLButtonDown )
	{
		double d = abs(point.x-m_start_pt.x) + abs(point.y-m_start_pt.y);
		if( m_bDraggingRect
			|| (d > 10 && !CurDragging() ) )
		{
			// we are dragging a selection rect
			SIZE s1;
			s1.cx = s1.cy = 1;
			m_drag_rect.TopLeft() = m_start_pt;
			m_drag_rect.BottomRight() = point;
			m_drag_rect.NormalizeRect();
			CDC * pDC = GetDC();
			if( !m_bDraggingRect )
			{
				//start dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, NULL, s1 );
			}
			else
			{
				// continue dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, &m_last_drag_rect, s1 );
			}
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
}


/////////////////////////////////////////////////////////////////////////
// Utility functions
//

// Set the device context to world coords
//
int CFreePcbView::SetDCToWorldCoords( CDC * pDC )
{
	m_dlist->SetDCToWorldCoords( pDC, &m_memDC, m_org_x, m_org_y );

	return 0;
}


// Set cursor mode, update function key menu if necessary
//
void CFreePcbView::SetCursorMode( int mode )
{
	if( mode != m_cursor_mode )
	{
		SetFKText( mode );
		m_cursor_mode = mode;
		ShowSelectStatus();
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
		if( CurDragging() )
			SetMainMenu( FALSE );
		else if( m_Doc->m_project_open )
			SetMainMenu( TRUE );
	}
}

// Set function key shortcut text
//
void CFreePcbView::SetFKText( int mode )
{
	for( int i=0; i<12; i++ )
	{
		m_fkey_option[i] = 0;
		m_fkey_command[i] = 0;
	}

	switch( mode )
	{
	case CUR_NONE_SELECTED:
		if( m_Doc->m_project_open )
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
		if( m_sel_part->glued )
			m_fkey_option[4] = FK_UNGLUE_PART;
		else
			m_fkey_option[4] = FK_GLUE_PART;
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
		if( m_sel_part->pin[m_sel_id.i].net )
			m_fkey_option[0] = FK_DETACH_NET;
		else
			m_fkey_option[0] = FK_ATTACH_NET;
		m_fkey_option[2] = FK_START_STUB;
		m_fkey_option[3] = FK_ADD_CONNECT;
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
			int style = m_Doc->m_sm_cutout[m_sel_id.i].GetSideStyle( m_sel_id.ii );
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
			int style = m_Doc->m_board_outline[m_sel_id.i].GetSideStyle( m_sel_id.ii );
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
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			if( poly->GetContour( m_sel_id.ii ) > 0 )
				m_fkey_option[6] = FK_DELETE_CUTOUT;
			else
				m_fkey_option[6] = FK_AREA_CUTOUT;
		}
		m_fkey_option[7] = FK_DELETE_AREA;
		break;

	case CUR_AREA_SIDE_SELECTED:
		m_fkey_option[0] = FK_SIDE_STYLE;
		m_fkey_option[1] = FK_EDIT_AREA;
		{
			int style = m_sel_net->area[m_sel_id.i].poly->GetSideStyle(m_sel_id.ii);
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			if( poly->GetContour( m_sel_id.ii ) > 0 )
				m_fkey_option[6] = FK_DELETE_CUTOUT;
			else
				m_fkey_option[6] = FK_AREA_CUTOUT;
		}
		m_fkey_option[7] = FK_DELETE_AREA;
		break;

	case CUR_SEG_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		if( m_sel_con.end_pin == cconnect::NO_END )
		{
			// stub trace
			if( m_sel_con.vtx[m_sel_con.nsegs].force_via_flag
				|| m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
			{
				m_fkey_option[5] = FK_UNROUTE_TRACE;
			}
			if( m_sel_con.nsegs == (m_sel_id.ii+1) )
			{
				// end segment of stub trace
				if( m_sel_con.vtx[m_sel_con.nsegs].force_via_flag
					|| m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
					m_fkey_option[4] = FK_UNROUTE;
				m_fkey_option[6] = FK_DELETE_SEGMENT;
			}
			else
			{
				// other segment of stub trace
				m_fkey_option[4] = FK_UNROUTE;
			}
		}
		else
		{
			// normal trace
			m_fkey_option[4] = FK_UNROUTE;
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		}
		m_fkey_option[2] = FK_ADD_VERTEX;
		if(SegmentMovable())
			m_fkey_option[3] = FK_MOVE_SEGMENT;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_RAT_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		if( m_sel_con.locked )
			m_fkey_option[2] = FK_UNLOCK_CONNECT;
		else
			m_fkey_option[2] = FK_LOCK_CONNECT;
		m_fkey_option[3] = FK_ROUTE;
		m_fkey_option[5] = FK_UNROUTE_TRACE;
		if( m_sel_con.end_pin == cconnect::NO_END )
		{
			// stub trace
			if( m_sel_con.nsegs == (m_sel_id.ii+1) )
			{
				// end segment of stub trace
				m_fkey_option[6] = FK_DELETE_SEGMENT;
			}
		}
		else if( m_sel_con.nsegs > 1
			&& ( m_sel_id.ii == 0 || m_sel_id.ii == (m_sel_con.nsegs-1) ) )
		{
			// pin-pin connection
			m_fkey_option[4] = FK_CHANGE_PIN;
		}
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_VTX_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		if( m_sel_vtx.viaExists() )
			m_fkey_option[1] = FK_VIA_SIZE;
		m_fkey_option[2] = FK_ADD_CONNECT;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		if( m_sel_con.end_pin != cconnect::NO_END
			|| m_sel_con.vtx[m_sel_con.nsegs].viaExists()
			|| m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_DELETE_VERTEX;
		m_fkey_option[7] = FK_DELETE_CONNECT;
		m_fkey_option[8] = FK_REDO_RATLINES;
		break;

	case CUR_END_VTX_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[1] = FK_ADD_SEGMENT;
		if( m_sel_vtx.viaExists() )
			m_fkey_option[4] = FK_DELETE_VIA;
		else
			m_fkey_option[4] = FK_ADD_VIA;
		m_fkey_option[2] = FK_ADD_CONNECT;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		if( m_sel_vtx.viaExists() )
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_DELETE_VERTEX;
		m_fkey_option[7] = FK_DELETE_CONNECT;
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
		break;

	case CUR_DRAG_RAT:
		m_fkey_option[3] = FK_COMPLETE;
		break;

	case CUR_DRAG_STUB:
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

	for( int i=0; i<12; i++ )
	{
		strcpy( m_fkey_str[2*i],   fk_str[2*m_fkey_option[i]] );
		strcpy( m_fkey_str[2*i+1], fk_str[2*m_fkey_option[i]+1] );
	}

	InvalidateLeftPane();
	Invalidate( FALSE );
}

int CFreePcbView::SegmentMovable(void)
{
	// see if this is the end of the road, if so, can't move it:
	{
		int x = m_sel_vtx.x;
		int y = m_sel_vtx.y;
		int layer = m_sel_seg.layer;
		BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
			m_sel_id.i, layer, 1 );
		if( test || m_sel_vtx.tee_ID )
		{
			return FALSE;
		}
	}

	// see if end vertex of this segment is in end pad of connection
	{
		int x = m_sel_next_vtx.x;
		int y = m_sel_next_vtx.y;
		int layer = m_sel_seg.layer;
		BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
			m_sel_id.i, layer, 0 );
		if( test || m_sel_next_vtx.tee_ID)
		{
			return FALSE;
		}
	}
	return TRUE;
}

// Draw bottom pane
//
void CFreePcbView::DrawBottomPane()
{
	CDC * pDC = GetDC();
	CFont * old_font = pDC->SelectObject( &m_small_font );

	// get client rectangle
	GetClientRect( &m_client_r );

	// draw labels for function keys at bottom of client area
	for( int j=0; j<3; j++ )
	{
		for( int i=0; i<4; i++ )
		{
			int ifn = j*4+i;
			if( ifn < 9 )
			{
				CRect r( FKEY_OFFSET_X+ifn*FKEY_STEP+j*FKEY_GAP,
					m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H,
					FKEY_OFFSET_X+ifn*FKEY_STEP+j*FKEY_GAP+FKEY_R_W,
					m_client_r.bottom-FKEY_OFFSET_Y );
				pDC->Rectangle( &r );
				pDC->MoveTo( r.left+FKEY_SEP_W, r.top );
				pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 + 1 );
				pDC->MoveTo( r.left, r.top + FKEY_R_H/2 );
				pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 );
				r.top += 1;
				r.left += 2;
				char fkstr[3] = "F1";
				fkstr[1] = '1' + j*4+i;
				pDC->DrawText( fkstr, -1, &r, 0 );
				r.left += FKEY_SEP_W;
				char * str1 = &m_fkey_str[2*ifn][0];
				char * str2 = &m_fkey_str[2*ifn+1][0];
				pDC->DrawText( str1, -1, &r, 0 );
				r.top += FKEY_R_H/2 - 2;
				pDC->DrawText( str2, -1, &r, 0 );
			}
		}
	}
	pDC->SelectObject( old_font );
	ReleaseDC( pDC );
}

void CFreePcbView::ShowRelativeDistance( int dx, int dy )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	double d = sqrt( (double)dx*dx + (double)dy*dy );
	if( m_Doc->m_units == MIL )
		str.Format( "dx = %.1f, dy = %.1f, d = %.2f",
		(double)dx/NM_PER_MIL, (double)dy/NM_PER_MIL, d/NM_PER_MIL );
	else
		str.Format( "dx = %.3f, dy = %.3f, d = %.3f", dx/1000000.0, dy/1000000.0, d/1000000.0 );
	pMain->DrawStatus( 3, &str );
}

void CFreePcbView::ShowRelativeDistance( int x, int y, int dx, int dy )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	double d = sqrt( (double)dx*dx + (double)dy*dy );
	if( m_Doc->m_units == MIL )
		str.Format( "x = %.1f, y = %.1f, dx = %.1f, dy = %.1f, d = %.2f",
		(double)x/NM_PER_MIL, (double)y/NM_PER_MIL,
		(double)dx/NM_PER_MIL, (double)dy/NM_PER_MIL, d/NM_PER_MIL );
	else
		str.Format( "x = %.3f, y = %.3f, dx = %.3f, dy = %.3f, d = %.3f",
		x/1000000.0, y/1000000.0,
		dx/1000000.0, dy/1000000.0, d/1000000.0 );
	pMain->DrawStatus( 3, &str );
}

// display selected item in status bar
//
int CFreePcbView::ShowSelectStatus()
{
	CString x_str, y_str, w_str, hole_str, via_w_str, via_hole_str;
	int u = m_Doc->m_units;

	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;

	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		str.Format( "No selection" );
		break;

	case CUR_DRE_SELECTED:
		str.Format( "DRE %s", m_sel_dre->str );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		{
			CString lay_str;
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( poly->GetLayer() == LAY_SM_TOP )
				lay_str = "Top";
			else
				lay_str = "Bottom";
			::MakeCStringFromDimension( &x_str, poly->GetX(m_sel_id.ii), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, poly->GetY(m_sel_id.ii), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str.Format( "Solder mask cutout %d: %s, corner %d, x %s, y %s",
				m_sel_id.i+1, lay_str, m_sel_id.ii+1, x_str, y_str );
		}
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		{
			CString style_str;
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( poly->GetSideStyle( m_sel_id.ii ) == CPolyLine::STRAIGHT )
				style_str = "straight";
			else if( poly->GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CW )
				style_str = "arc(cw)";
			else if( poly->GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CCW )
				style_str = "arc(ccw)";
			CString lay_str;
			if( poly->GetLayer() == LAY_SM_TOP )
				lay_str = "Top";
			else
				lay_str = "Bottom";
			str.Format( "Solder mask cutout %d: %s, side %d of %d, %s",
				m_sel_id.i+1, lay_str, m_sel_id.ii+1,
				poly->GetNumCorners(), style_str );
		}
		break;

	case CUR_BOARD_CORNER_SELECTED:
		::MakeCStringFromDimension( &x_str, m_Doc->m_board_outline[m_sel_id.i].GetX(m_sel_id.ii), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &y_str, m_Doc->m_board_outline[m_sel_id.i].GetY(m_sel_id.ii), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		str.Format( "board outline %d, corner %d, x %s, y %s",
			m_sel_id.i+1, m_sel_id.ii+1, x_str, y_str );
		break;

	case CUR_BOARD_SIDE_SELECTED:
		{
			CString style_str;
			if( m_Doc->m_board_outline[m_sel_id.i].GetSideStyle( m_sel_id.ii ) == CPolyLine::STRAIGHT )
				style_str = "straight";
			else if( m_Doc->m_board_outline[m_sel_id.i].GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CW )
				style_str = "arc(cw)";
			else if( m_Doc->m_board_outline[m_sel_id.i].GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CCW )
				style_str = "arc(ccw)";
			str.Format( "board outline %d, side %d of %d, %s", m_sel_id.i+1, m_sel_id.ii+1,
				m_Doc->m_board_outline[m_sel_id.i].GetNumCorners(), style_str );
		}
		break;

	case CUR_PART_SELECTED:
		{
			CString side = "top";
			if( m_sel_part->side )
				side = "bottom";
			::MakeCStringFromDimension( &x_str, m_sel_part->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_part->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			int rep_angle = ::GetReportedAngleForPart( m_sel_part->angle,
				m_sel_part->shape->m_centroid_angle, m_sel_part->side );
			str.Format( "part %s \"%s\", x %s, y %s, angle %d, %s",
				m_sel_part->ref_des, m_sel_part->shape->m_name,
				x_str, y_str,
				rep_angle,
				side );
			int a = ::GetPartAngleForReportedAngle( rep_angle,
				m_sel_part->shape->m_centroid_angle, m_sel_part->side );
			if( a != (m_sel_part->angle) )
				ASSERT(0);
		}
		break;

	case CUR_REF_SELECTED:
		str.Format( "ref text: %s", m_sel_part->ref_des );
		break;

	case CUR_VALUE_SELECTED:
		str.Format( "value: %s", m_sel_part->value );
		break;

	case CUR_PAD_SELECTED:
		{
			cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
			::MakeCStringFromDimension( &x_str, m_sel_part->pin[m_sel_id.i].x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_part->pin[m_sel_id.i].y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			if( pin_net )
			{
				// pad attached to net
				str.Format( "pin %s.%s on net \"%s\", x %s, y %s",
					m_sel_part->ref_des,
					m_sel_part->shape->GetPinNameByIndex(m_sel_id.i),
					pin_net->name, x_str, y_str );
			}
			else
			{
				// pad not attached to a net
				str.Format( "pin %s.%s unconnected, x %s, y %s",
					m_sel_part->ref_des,
					m_sel_part->shape->GetPinNameByIndex(m_sel_id.i),
					x_str, y_str );
			}
		}
		break;

	case CUR_SEG_SELECTED:
	case CUR_RAT_SELECTED:
	case CUR_DRAG_STUB:
	case CUR_DRAG_RAT:
		{
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				if( m_cursor_mode == CUR_DRAG_STUB )
				{
					// stub trace segment
					str.Format( "net \"%s\" stub(%d) from %s.%s, seg %d",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_id.ii+1
						);
				}
				else
				{
					// stub trace segment
					if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
						str.Format( "net \"%s\" branch(%d) to %s.%s, seg %d, width %d (T%d), clearance %d",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii+1,
							m_sel_seg.width()/NM_PER_MIL,
							m_sel_con.vtx[m_sel_con.nsegs].tee_ID,
							m_sel_seg.clearance()/NM_PER_MIL
						);
					else
						str.Format( "net \"%s\" stub(%d) from %s.%s, seg %d, width %d, clearance %d",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii+1,
							m_sel_seg.width()/NM_PER_MIL,
							m_sel_seg.clearance()/NM_PER_MIL
						);
				}
			}
			else
			{
				// normal connected trace segment
				CString locked_flag = "";
				if( m_sel_con.locked )
					locked_flag = " (L)";
				if( m_sel_con.nsegs == 1 && m_sel_seg.layer == LAY_RAT_LINE )
				{
					str.Format( "net \"%s\" connection(%d) %s.%s-%s.%s%s, seg %d, width %d, clearance %d",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag, m_sel_id.ii+1,
						m_sel_seg.width()/NM_PER_MIL,
						m_sel_seg.clearance()/NM_PER_MIL
					);
				}
				else
				{
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, seg %d, width %d, clearance %d",
						m_sel_net->name,  m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag, m_sel_id.ii+1,
						m_sel_seg.width()/NM_PER_MIL,
                        m_sel_seg.clearance()/NM_PER_MIL
					);
				}
			}
		}
		break;

	case CUR_VTX_SELECTED:
		{
			CString locked_flag = "";
			if( m_sel_con.locked )
				locked_flag = " (L)";
			CString tee_flag = "";
			if( int id = m_sel_vtx.tee_ID )
				tee_flag.Format( " (T%d)", id );
			if( m_sel_vtx.force_via_flag )
				tee_flag = "(F)" + tee_flag;
			int via_w = m_sel_vtx.via_w();
			::MakeCStringFromDimension( &x_str, m_sel_vtx.x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_vtx.y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &via_w_str, m_sel_vtx.via_w(), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &via_hole_str, m_sel_vtx.via_hole_w(), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				// vertex of stub trace
				if( via_w )
				{
					// via
					if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
						str.Format( "net \"%s\" branch(%d) to %s.%s, vertex %d, x %s, y %s, via %s/%s %s",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							via_w_str,
							via_hole_str,
							tee_flag
							);
					else
						str.Format( "net \"%s\" stub(%d) from %s.%s, vertex %d, x %s, y %s, via %s/%s %s",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							via_w_str,
							via_hole_str,
							tee_flag
							);
				}
				else
				{
					// no via
					if( m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
						str.Format( "net \"%s\" branch(%d) to %s.%s, vertex %d, x %s, y %s %s",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							tee_flag
							);
					else
						str.Format( "net \"%s\" stub(%d) from %s.%s, vertex %d, x %s, y %s %s",
							m_sel_net->name, m_sel_id.i+1,
							m_sel_start_pin.ref_des,
							m_sel_start_pin.pin_name,
							m_sel_id.ii,
							x_str,
							y_str,
							tee_flag
							);
				}
			}
			else
			{
				// vertex of normal connected trace
				if( via_w )
				{
					// with via
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, vertex %d, x %s, y %s, via %s/%s %s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_id.ii,
						x_str,
						y_str,
						via_w_str,
						via_hole_str,
						tee_flag
						);
				}
				else
				{
					// no via
					str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s, vertex %d, x %s, y %s %s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						m_sel_end_pin.ref_des,
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_id.ii,
						x_str,
						y_str,
						tee_flag
						);
				}
			}
		}
		break;

	case CUR_END_VTX_SELECTED:
		{
			CString tee_flag = "";
			if( int id = m_sel_vtx.tee_ID )
				tee_flag.Format( " (T%d)", id );
			if( m_sel_vtx.force_via_flag )
				tee_flag = "(F)" + tee_flag;
			::MakeCStringFromDimension( &x_str, m_sel_vtx.x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, m_sel_vtx.y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &via_w_str, m_sel_vtx.via_w(), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &via_hole_str, m_sel_vtx.via_hole_w(), u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str.Format( "net \"%s\" stub(%d) end, x %s, y %s, via %s/%s %s",
				m_sel_net->name, m_sel_id.i+1,
				x_str,
				y_str,
				via_w_str,
				via_hole_str,
				tee_flag
				);
		}
		break;

	case CUR_CONNECT_SELECTED:
		{
			CString locked_flag = "";
			if( m_sel_con.locked )
				locked_flag = " (L)";
			// get length of trace
			CString len_str;
			double len = 0;
			double last_x = m_sel_con.vtx[0].x;
			double last_y = m_sel_con.vtx[0].y;
			for( int iv=0; iv<=m_sel_con.nsegs; iv++ )
			{
				double x = m_sel_con.vtx[iv].x;
				double y = m_sel_con.vtx[iv].y;
				len += sqrt( (x-last_x)*(x-last_x) + (y-last_y)*(y-last_y) );
				last_x = x;
				last_y = y;
			}
			::MakeCStringFromDimension( &len_str, (int)len, u, TRUE, TRUE, FALSE, u==MIL?1:3 );
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				// stub or branch trace
				if( int id = m_sel_con.vtx[m_sel_con.nsegs].tee_ID )
				{
					CString tee_flag = "";
					tee_flag.Format( "(T%d)", id );
					str.Format( "net \"%s\" branch(%d) from %s.%s%s %s len=%s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						locked_flag, tee_flag, len_str );
				}
				else
				{
					str.Format( "net \"%s\" stub(%d) from %s.%s%s len=%s",
						m_sel_net->name, m_sel_id.i+1,
						m_sel_start_pin.ref_des,
						m_sel_start_pin.pin_name,
						locked_flag, len_str );
				}
			}
			else
			{
				// normal trace
				str.Format( "net \"%s\" trace(%d) %s.%s-%s.%s%s len=%s",
					m_sel_net->name, m_sel_id.i+1,
					m_sel_start_pin.ref_des,
					m_sel_start_pin.pin_name,
					m_sel_end_pin.ref_des,
					m_sel_end_pin.pin_name,
					locked_flag, len_str );
			}
		}
		break;

	case CUR_NET_SELECTED:
		str.Format( "net \"%s\": clearance %d", m_sel_net->name, m_sel_net->def_clearance.m_ca_clearance.m_val/NM_PER_MIL );
		break;

	case CUR_TEXT_SELECTED:
		{
			CString neg_str = "";
			if( m_sel_text->m_bNegative )
				neg_str = "(NEG)";
			str.Format( "Text \"%s\" %s", m_sel_text->m_str, neg_str );
			break;
		}

	case CUR_AREA_CORNER_SELECTED:
		{
			CPoint p = m_Doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
			::MakeCStringFromDimension( &x_str, p.x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			::MakeCStringFromDimension( &y_str, p.y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
			str.Format( "\"%s\" copper area %d corner %d, x %s, y %s",
				m_sel_net->name, m_sel_id.i+1, m_sel_id.ii+1,
				x_str, y_str );
		}
		break;

	case CUR_AREA_SIDE_SELECTED:
		{
			int ic = m_sel_id.ii;
			int ia = m_sel_id.i;
			CPolyLine * p = m_sel_net->area[ia].poly;
			int ncont = p->GetContour(ic);
			if( ncont == 0 )
				str.Format( "\"%s\" copper area %d edge %d", m_sel_net->name, ia+1, ic+1 );
			else
			{
				str.Format( "\"%s\" copper area %d cutout %d edge %d",
					m_sel_net->name, ia+1, ncont, ic+1-p->GetContourStart(ncont) );
			}
		}
		break;

	case CUR_GROUP_SELECTED:
		str.Format( "Group selected" );
		break;

	case CUR_ADD_BOARD:
		str.Format( "Placing first corner of board outline" );
		break;

	case CUR_DRAG_BOARD_1:
		str.Format( "Placing second corner of board outline" );
		break;

	case CUR_DRAG_BOARD:
		str.Format( "Placing corner %d of board outline", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_BOARD_INSERT:
		str.Format( "Inserting corner %d of board outline", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_BOARD_MOVE:
		str.Format( "Moving corner %d of board outline", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_PART:
		str.Format( "Moving part %s", m_sel_part->ref_des );
		break;

	case CUR_DRAG_REF:
		str.Format( "Moving ref text for part %s", m_sel_part->ref_des );
		break;

	case CUR_DRAG_VTX:
		str.Format( "Routing net \"%s\"", m_sel_net->name );
		break;

	case CUR_DRAG_END_VTX:
		str.Format( "Routing net \"%s\"", m_sel_net->name );
		break;

	case CUR_DRAG_TEXT:
		str.Format( "Moving text \"%s\"", m_sel_text->m_str );
		break;

	case CUR_ADD_AREA:
		str.Format( "Placing first corner of copper area" );
		break;

	case CUR_DRAG_AREA_1:
		str.Format( "Placing second corner of copper area" );
		break;

	case CUR_DRAG_AREA:
		str.Format( "Placing corner %d of copper area", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_AREA_INSERT:
		str.Format( "Inserting corner %d of copper area", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_AREA_MOVE:
		str.Format( "Moving corner %d of copper area", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_CONNECT:
		if( m_sel_id.type == ID_PART )
			str.Format( "Adding connection to pin \"%s.%s",
			m_sel_part->ref_des,
			m_sel_part->shape->GetPinNameByIndex(m_sel_id.i) );
		else if( m_sel_id.type == ID_NET )
			str.Format( "Adding branch to trace \"%s.%d",
			m_sel_net->name,
			m_sel_id.i );
		break;

	case CUR_DRAG_MEASURE_1:
		str = "Measurement mode: left-click to start";
		break;

	}
	pMain->DrawStatus( 3, &str );
	return 0;
}

// display cursor coords in status bar
//
int CFreePcbView::ShowCursor()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	CPoint p;
	p = m_last_cursor_point;
	if( m_Doc->m_units == MIL )
	{
		str.Format( "X: %8.1f", (double)m_last_cursor_point.x/PCBU_PER_MIL );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %8.1f", (double)m_last_cursor_point.y/PCBU_PER_MIL );
		pMain->DrawStatus( 2, &str );
	}
	else
	{
		str.Format( "X: %8.3f", m_last_cursor_point.x/1000000.0 );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %8.3f", m_last_cursor_point.y/1000000.0 );
		pMain->DrawStatus( 2, &str );
	}
	return 0;
}

// display active layer in status bar and change layer order for DisplayList
//
int CFreePcbView::ShowActiveLayer()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	if( m_active_layer == LAY_TOP_COPPER )
		str.Format( "Top" );
	else if( m_active_layer == LAY_BOTTOM_COPPER )
		str.Format( "Bottom" );
	else if( m_active_layer > LAY_BOTTOM_COPPER )
		str.Format( "Inner %d", m_active_layer - LAY_BOTTOM_COPPER );
	pMain->DrawStatus( 4, &str );
	for( int order=LAY_TOP_COPPER; order<LAY_TOP_COPPER+m_Doc->m_num_copper_layers; order++ )
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

// SelectPart...this is called from FreePcbDoc when a new part is added
// selects the new part as long as the cursor is not dragging something
//
int CFreePcbView::SelectPart( cpart * part )
{
	if(	!CurDragging() )
	{
		// deselect previously selected item
		if( m_sel_id.type )
			CancelSelection();

		// select part
		m_sel_part = part;
		m_sel_id = part->m_id;
		m_sel_id.st = ID_SEL_RECT;
		m_Doc->m_plist->SelectPart( m_sel_part );
		SetCursorMode( CUR_PART_SELECTED );
	}
	gLastKeyWasArrow = FALSE;
	gLastKeyWasGroupRotate=false;
	Invalidate( FALSE );
	return 0;
}

// cancel selection
//
void CFreePcbView::CancelSelection()
{
	m_Doc->m_dlist->CancelHighLight();
	m_sel_ids.RemoveAll();
	m_sel_ptrs.RemoveAll();
	m_sel_id.Clear();
	SetCursorMode( CUR_NONE_SELECTED );
}

// attempt to reselect area corner based on position
// should be used after areas are modified
void CFreePcbView::TryToReselectAreaCorner( int x, int y )
{
	m_dlist->CancelHighLight();
	for( int ia=0; ia<m_sel_net->nareas; ia++ )
	{
		for( int ic=0; ic<m_sel_net->area[ia].poly->GetNumCorners(); ic++ )
		{
			if( x == m_sel_net->area[ia].poly->GetX(ic)
				&& y == m_sel_net->area[ia].poly->GetY(ic) )
			{
				// found matching corner
				m_sel_id = id(ID_NET,ID_AREA,ia,ID_SEL_CORNER,ic);
				SetCursorMode( CUR_AREA_CORNER_SELECTED );
				m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, ia, ic );
				return;
			}
		}
	}
	CancelSelection();
}


// set trace width using dialog
// enter with:
//	mode = 0 if called with segment selected
//	mode = 1 if called with connection selected
//	mode = 2 if called with net selected
//
int CFreePcbView::SetWidth( int mode )
{
	// set parameters for dialog
	DlgSetSegmentWidth dlg;

	dlg.m_w = &m_Doc->m_w;
	dlg.m_v_w = &m_Doc->m_v_w;
	dlg.m_v_h_w = &m_Doc->m_v_h_w;

	if( mode == 0 )
	{
		dlg.m_init_width = m_sel_seg.seg_width;
	}
	else
	{
		dlg.m_init_width = m_sel_net->def_width;
	}
	dlg.m_init_width.Update();

	// launch dialog
	dlg.m_mode = mode;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// returned with "OK"
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

		// set default values for net or connection
		if( dlg.m_def == 2 )
		{
			// set default for net
			m_sel_net->def_width = dlg.m_width;

			m_Doc->m_nlist->UpdateNetAttributes( m_sel_net );
		}

		// apply new widths to net, connection or segment
		if( dlg.m_apply == 3 )
		{
			// apply to net
			m_Doc->m_nlist->SetNetWidth( m_sel_net, dlg.m_width );
		}
		else if( dlg.m_apply == 2 )
		{
			// apply to connection
			m_Doc->m_nlist->SetConnectionWidth( m_sel_net, m_sel_ic, dlg.m_width );
		}
		else if( dlg.m_apply == 1 )
		{
			// apply to segment
			m_Doc->m_nlist->SetSegmentWidth( m_sel_net, m_sel_ic, m_sel_id.ii, dlg.m_width );
		}
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
	return 0;
}


// set trace width using dialog
// enter with:
//	mode = 0 if called with segment selected
//	mode = 1 if called with connection selected
//	mode = 2 if called with net selected
//
int CFreePcbView::SetClearance( int mode )
{
	// set parameters for dialog
	DlgSetSegmentClearance dlg;

	if( mode == 0 )
	{
		cseg *s = &m_sel_seg;
		dlg.m_clearance = s->seg_clearance;
	}
	else
	{
		dlg.m_clearance = m_sel_net->def_clearance;

		// Since dlg.m_clearance is supposed to be a segment clearance,
		// make sure dlg.m_clearance's parent is the net's clearance,
		// not the net's parent - which results from the assignment
		// statement above.
		dlg.m_clearance.SetParent(m_sel_net->def_clearance);
	}

	// launch dialog
	dlg.m_mode = mode;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

		CClearanceInfo clearance;

		clearance = dlg.m_clearance;

		// set default values for net or connection
		if( dlg.m_def == 2 )
		{
			// set default for net
			m_sel_net->def_clearance = clearance;

			m_Doc->m_nlist->UpdateNetAttributes( m_sel_net );

			// Now if the "default for net" clearance is applied below
			// to a net/trace/segment, apply the "use parent" clearance.
			clearance = CClearanceInfo(CClearanceInfo::E_USE_PARENT);
		}

		// apply new widths to net, connection or segment
		if( dlg.m_apply == 3 )
		{
			// apply to net
			m_Doc->m_nlist->SetNetClearance( m_sel_net, clearance );
		}
		else if( dlg.m_apply == 2 )
		{
			// apply to connection
			m_Doc->m_nlist->SetConnectionClearance( m_sel_net, m_sel_ic, clearance );
		}
		else if( dlg.m_apply == 1 )
		{
			// apply to segment
			m_Doc->m_nlist->SetSegmentClearance( m_sel_net, m_sel_ic, m_sel_id.ii, clearance );
		}
	}

	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
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
	if( !m_Doc->m_project_open )	// no project open
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
		if( m_Doc->m_board_outline[m_sel_id.i].GetNumCorners() < 4 )
				pPopup->EnableMenuItem( ID_BOARDCORNER_DELETECORNER, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_BOARD_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_BOARD_SIDE);
		ASSERT(pPopup != NULL);
		style = m_Doc->m_board_outline[m_sel_id.i].GetSideStyle( m_sel_id.ii );
		if( style == CPolyLine::STRAIGHT )
		{
			int xi = m_Doc->m_board_outline[m_sel_id.i].GetX( m_sel_id.ii );
			int yi = m_Doc->m_board_outline[m_sel_id.i].GetY( m_sel_id.ii );
			int xf, yf;
			if( m_sel_id.ii != (m_Doc->m_board_outline[m_sel_id.i].GetNumCorners()-1) )
			{
				xf = m_Doc->m_board_outline[m_sel_id.i].GetX( m_sel_id.ii+1 );
				yf = m_Doc->m_board_outline[m_sel_id.i].GetY( m_sel_id.ii+1 );
			}
			else
			{
				xf = m_Doc->m_board_outline[m_sel_id.i].GetX( 0 );
				yf = m_Doc->m_board_outline[m_sel_id.i].GetY( 0 );
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
		if( m_sel_part->pin[m_sel_id.i].net )
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

		if( m_sel_con.end_pin == cconnect::NO_END
			&& m_sel_con.vtx[m_sel_con.nsegs].tee_ID == 0
			&& m_sel_con.vtx[m_sel_con.nsegs].force_via_flag == 0
			)
		{
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		}
		if( m_sel_con.end_pin == cconnect::NO_END
			&& m_sel_con.nsegs == (m_sel_id.ii+1)
			&& m_sel_con.vtx[m_sel_con.nsegs].tee_ID == 0
			&& m_sel_con.vtx[m_sel_con.nsegs].force_via_flag == 0
			)
		{
			// last segment of stub trace unless a tee or via
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTE, MF_GRAYED );
		}
		if( m_sel_con.end_pin != cconnect::NO_END
			|| m_sel_con.nsegs > (m_sel_id.ii+1)
			)
		{
			pPopup->EnableMenuItem( ID_SEGMENT_DELETE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_RAT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_RATLINE);
		ASSERT(pPopup != NULL);
		if( m_sel_con.locked )
			pPopup->EnableMenuItem( ID_RATLINE_LOCKCONNECTION, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_RATLINE_UNLOCKCONNECTION, MF_GRAYED );
		if( m_sel_con.end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		if( m_sel_con.nsegs == 1
			|| !(m_sel_id.ii == 0 || m_sel_id.ii == (m_sel_con.nsegs-1) ) )
			pPopup->EnableMenuItem( ID_RATLINE_CHANGEPIN, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VERTEX);
		ASSERT(pPopup != NULL);

		if( !m_sel_vtx.viaExists() )
		{
			pPopup->EnableMenuItem( ID_VERTEX_SETSIZE,      MF_GRAYED );
			pPopup->EnableMenuItem( ID_VERTEX_SETCLEARANCE, MF_GRAYED );
		}

		if( m_sel_con.end_pin == cconnect::NO_END
			&& m_sel_con.vtx[m_sel_con.nsegs].tee_ID == 0
			&& m_sel_con.vtx[m_sel_con.nsegs].force_via_flag == 0
			)
		{
			pPopup->EnableMenuItem( ID_VERTEX_UNROUTETRACE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_END_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_END_VERTEX);
		ASSERT(pPopup != NULL);
		if( m_sel_vtx.viaExists() )
		{
			pPopup->EnableMenuItem( ID_ENDVERTEX_ADDVIA, MF_GRAYED );
		}
		else
		{
			pPopup->EnableMenuItem( ID_ENDVERTEX_REMOVEVIA, MF_GRAYED );

			pPopup->EnableMenuItem( ID_ENDVERTEX_SETSIZE, MF_GRAYED );
			pPopup->EnableMenuItem( ID_ENDVERTEX_SETVIACLEARANCE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_CONNECT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_CONNECT);
		ASSERT(pPopup != NULL);
		if( m_sel_con.end_pin == cconnect::NO_END )
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
			if( area->poly->GetContour( m_sel_id.ii ) == 0 )
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
			if( area->poly->GetContour( m_sel_id.ii ) == 0 )
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
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( poly->GetNumCorners() < 4 )
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
	CDlgAddArea dlg;
	dlg.Initialize( m_Doc->m_nlist, m_Doc->m_num_layers, NULL, m_active_layer, -1 );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( !dlg.m_net )
		{
			CString str;
			str.Format( "Net \"%s\" not found", dlg.m_net_name );
			AfxMessageBox( str, MB_OK );
		}
		else
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_ADD_AREA );
			// make layer visible
			m_active_layer = dlg.m_layer;
			m_Doc->m_vis[m_active_layer] = TRUE;
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
}

// add copper area cutout
//
void CFreePcbView::OnAreaAddCutout()
{
	// check if any non-straight sides
	BOOL bArcs = FALSE;
	CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
	int ns = poly->GetNumCorners();
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	SetCursorMode( CUR_ADD_AREA_CUTOUT );
	// make layer visible
	m_active_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();
	m_Doc->m_vis[m_active_layer] = TRUE;
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
	CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
	int icont = poly->GetContour( m_sel_id.ii );
	if( icont < 1 )
		ASSERT(0);
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
	poly->RemoveContour( icont );
	CancelSelection();
	m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	Invalidate( FALSE );
}

// move part
//
void CFreePcbView::OnPartMove()
{
	// check for glue
	if( m_sel_part->glued )
	{
		int ret = AfxMessageBox( "This part is glued, do you want to unglue it ?  ", MB_YESNO );
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
	m_Doc->m_plist->StartDraggingPart( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_PART );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// add text string
//
void CFreePcbView::OnTextAdd()
{
	// create, initialize and show dialog
	CDlgAddText add_text_dlg;
	CString str = "";
	add_text_dlg.Initialize( 0, m_Doc->m_num_layers, 1, &str, m_Doc->m_units,
			LAY_SILK_TOP, 0, 0, 0, 0, 0, 0, 0 );
	add_text_dlg.m_num_layers = m_Doc->m_num_layers;
	add_text_dlg.m_drag_flag = 1;
	// defaults for dialog
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;
	else
	{
		int x = add_text_dlg.m_x;
		int y = add_text_dlg.m_y;
		int mirror = add_text_dlg.m_mirror;
		BOOL bNegative = add_text_dlg.m_bNegative;
		int angle = add_text_dlg.m_angle;
		int font_size = add_text_dlg.m_height;
		int stroke_width = add_text_dlg.m_width;
		int layer = add_text_dlg.m_layer;
		CString str = add_text_dlg.m_str;
		m_Doc->m_vis[layer] = TRUE;
		m_dlist->SetLayerVisible( layer, TRUE );

		// get cursor position and convert to PCB coords
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = m_dlist->ScreenToPCB( p );	// convert to PCB coords
		// set pDC to PCB coords
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		if( add_text_dlg.m_drag_flag )
		{
			m_sel_text = m_Doc->m_tlist->AddText( p.x, p.y, angle, mirror, bNegative,
				layer, font_size, stroke_width, &str );
			m_dragging_new_item = 1;
			m_Doc->m_tlist->StartDraggingText( pDC, m_sel_text );
			SetCursorMode( CUR_DRAG_TEXT );
		}
		else
		{
			m_sel_text = m_Doc->m_tlist->AddText( x, y, angle, mirror, bNegative,
				layer, font_size,  stroke_width, &str );
			SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD, TRUE, m_Doc->m_undo_list );
			m_Doc->m_tlist->HighlightText( m_sel_text );
		}
		ReleaseDC( pDC );
		Invalidate( FALSE );
	}
}

// delete text ... enter with text selected
//
void CFreePcbView::OnTextDelete()
{
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_DELETE, TRUE, m_Doc->m_undo_list );
	m_Doc->m_tlist->RemoveText( m_sel_text );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move text, enter with text selected
//
void CFreePcbView::OnTextMove()
{
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
	m_dlist->CancelHighLight();
	m_dragging_new_item = 0;
	m_Doc->m_tlist->StartDraggingText( pDC, m_sel_text );
	SetCursorMode( CUR_DRAG_TEXT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// glue part
//
void CFreePcbView::OnPartGlue()
{
	SaveUndoInfoForPart( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_sel_part->glued = 1;
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// unglue part
//
void CFreePcbView::OnPartUnglue()
{
	SaveUndoInfoForPart( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_sel_part->glued = 0;
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// delete part
//
void CFreePcbView::OnPartDelete()
{
	// delete part
	CString mess;
	mess.Format( "Deleting part %s\nDo you wish to remove all references\nto this part from netlist ?",
		m_sel_part->ref_des );
	int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
	if( ret == IDCANCEL )
		return;
	// save undo info
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_DELETE, NULL, TRUE, m_Doc->m_undo_list );
	// now do it
	if( ret == IDYES )
		m_Doc->m_nlist->PartDeleted( m_sel_part );
	else if( ret == IDNO )
		m_Doc->m_nlist->PartDisconnected( m_sel_part );
	m_Doc->m_plist->Remove( m_sel_part );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// optimize all nets to part
//
void CFreePcbView::OnPartOptimize()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move ref. designator text for part
//
void CFreePcbView::OnRefMove()
{
	// move reference ID
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint cur_p = m_dlist->PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	m_Doc->m_plist->StartDraggingRefText( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_REF );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// optimize net for this pad
//
void CFreePcbView::OnPadOptimize()
{
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	if( pin_net )
	{
		m_Doc->m_nlist->OptimizeConnections( pin_net );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// start stub trace from this pad
//
void CFreePcbView::OnPadStartStubTrace()
{
	cnet * net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	if( net == NULL )
	{
		AfxMessageBox( "Pad must be assigned to a net before adding trace", MB_OK );
		return;
	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint pi = m_last_cursor_point;
	CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
	CPoint p = m_Doc->m_plist->GetPinPoint( m_sel_part, pin_name );

	// force to layer of pad if SMT
	if( m_sel_part->shape->m_padstack[m_sel_id.i].hole_size == 0 )
	{
		m_active_layer = m_Doc->m_plist->GetPinLayer( m_sel_part, &pin_name );
		ShowActiveLayer();
	}

	// find starting pin in net
	int p1 = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].part == m_sel_part )
		{
			if( net->pin[ip].pin_name == m_sel_part->shape->GetPinNameByIndex( m_sel_id.i ) )
			{
				// found starting pin in net
				p1 = ip;
			}
		}
	}
	if( p1 == -1 )
		ASSERT(0);		// starting pin not found in net

	// add connection for stub trace
	SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_sel_net = net;
	m_sel_id.Set( ID_NET, ID_CONNECT, 0, ID_SEL_SEG, 0 );
	m_sel_id.i = m_Doc->m_nlist->AddNetStub( net, p1 );

	// start dragging line
	m_Doc->m_nlist->StartDraggingStub(
		pDC,
		net,
		m_sel_id.i, m_sel_id.ii,
		pi.x, pi.y,
		m_active_layer,
		net->def_width.m_seg_width.m_val,
		m_active_layer,
		net->def_width.m_via_width.m_val,
		net->def_width.m_via_hole.m_val,
		2,
		m_inflection_mode
	);

	m_snap_angle_ref = p;

	SetCursorMode( CUR_DRAG_STUB );
	m_dlist->CancelHighLight();
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );

	ReleaseDC( pDC );

	Invalidate( FALSE );
}


// attach this pad to a net
//
void CFreePcbView::OnPadAddToNet()
{
	DlgAssignNet assign_net_dlg;
	assign_net_dlg.m_map = &m_Doc->m_nlist->m_map;
	int ret = assign_net_dlg.DoModal();
	if( ret == IDOK )
	{
		CString name = assign_net_dlg.m_net_str;
		void * ptr;
		cnet * new_net = 0;
		int test = m_Doc->m_nlist->m_map.Lookup( name, ptr );
		if( !test )
		{
			// create new net if legal string
			name.Trim();
			if( name.GetLength() )
			{
				new_net = m_Doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 10, CConnectionWidthInfo(), CClearanceInfo() );
				SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_ADD, TRUE, m_Doc->m_undo_list );
			}
			else
			{
				// blank net name
				AfxMessageBox( "Illegal net name" );
				return;
			}
		}
		else
		{
			// use selected net
			new_net = (cnet*)ptr;
			SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		}
		// assign pin to net
		if( new_net )
		{
			SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, FALSE, m_Doc->m_undo_list );
			CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
			m_Doc->m_nlist->AddNetPin( new_net,
				&m_sel_part->ref_des,
				&pin_name );
			m_Doc->m_nlist->OptimizeConnections( new_net );
			SetFKText( m_cursor_mode );
		}
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// remove this pad from net
//
void CFreePcbView::OnPadDetachFromNet()
{
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	CString pin_name = m_sel_part->shape->GetPinNameByIndex(m_sel_id.i);
	cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, m_sel_id.i );
	m_Doc->m_nlist->RemoveNetPin( m_sel_part, &pin_name );
	m_Doc->m_nlist->RemoveOrphanBranches( net, 0 );
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// connect this pad to another pad
//
void CFreePcbView::OnPadConnectToPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
	CPoint p = m_Doc->m_plist->GetPinPoint( m_sel_part, pin_name );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, p.x, p.y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_CONNECT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// connect this vertex to another pad with a tee connection
//
void CFreePcbView::OnVertexConnectToPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, m_sel_vtx.x, m_sel_vtx.y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_CONNECT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// set width for this segment (not a ratline)
//
void CFreePcbView::OnSegmentSetWidth()
{
	SetWidth( 0 );
	m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	Invalidate( FALSE );
}

// set clearance for this segment (not a ratline)
//
void CFreePcbView::OnSegmentSetClearance()
{
	SetClearance( 0 );
	m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	Invalidate( FALSE );
}

// unroute this segment, convert to a ratline
//
void CFreePcbView::OnSegmentUnroute()
{
	// save undo info for connection
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

	// edit connection segment
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	// see if segments to pin also need to be unrouted
	// see if start vertex of this segment is in start pad of connection
	int x = m_sel_vtx.x;
	int y = m_sel_vtx.y;
	int layer = m_sel_seg.layer;
	BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,	m_sel_id.i, layer, 1 );
	if( test )
	{
		// unroute preceding segments
		for( int is=m_sel_id.ii-1; is>=0; is-- )
			m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );	}
	// see if end vertex of this segment is in end pad of connection
	x = m_sel_next_vtx.x;
	y = m_sel_next_vtx.y;
	test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net, m_sel_id.i, layer, 0 );
	if( test )
	{
		// unroute following segments
		for( int is=m_sel_con.nsegs-1; is>m_sel_id.ii; is-- )
			m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );
	}

	// unroute segment
	id id = m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, id.i, id.ii );
	m_sel_id = id;
	SetCursorMode( CUR_RAT_SELECTED );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// delete this segment, only used for the last segment of a stub trace
//
void CFreePcbView::OnSegmentDelete()
{
	int id = m_sel_con.vtx[m_sel_con.nsegs].tee_ID;
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	if( id )
		m_Doc->m_nlist->DisconnectBranch( m_sel_net, m_sel_ic );
	m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is, TRUE );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// route this ratline
//
void CFreePcbView::OnRatlineRoute()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	int last_seg_layer = 0;
	int n_segs = m_sel_con.nsegs;
	// get direction for routing, based on closest end of selected segment to cursor
	double d1x = p.x - m_sel_vtx.x;
	double d1y = p.y - m_sel_vtx.y;
	double d2x = p.x - m_sel_next_vtx.x;
	double d2y = p.y - m_sel_next_vtx.y;
	double d1 = d1x*d1x + d1y*d1y;
	double d2 = d2x*d2x + d2y*d2y;
	if( d1<d2 )
	{
		// route forward
		m_dir = 0;
		if( m_sel_id.ii > 0 )
			last_seg_layer = m_sel_con.seg[m_sel_id.ii-1].layer;
		m_snap_angle_ref.x = m_sel_vtx.x;
		m_snap_angle_ref.y = m_sel_vtx.y;
	}
	else
	{
		// route backward
		m_dir = 1;
		if( m_sel_id.ii < (m_sel_con.nsegs-1) )
			last_seg_layer = m_sel_con.seg[m_sel_id.ii+1].layer;
		m_snap_angle_ref.x = m_sel_next_vtx.x;
		m_snap_angle_ref.y = m_sel_next_vtx.y;
	}
	if( m_sel_id.ii == 0 && m_dir == 0)
	{
		// first segment, force to layer of starting pad if SMT
		int p1 = m_sel_con.start_pin;
		cpart * p = m_sel_net->pin[p1].part;
		CString pin_name = m_sel_net->pin[p1].pin_name;
		int pin_index = p->shape->GetPinIndexByName( pin_name );
		if( p->shape->m_padstack[pin_index].hole_size == 0)
		{
			m_active_layer = m_Doc->m_plist->GetPinLayer( p, &pin_name );
			ShowActiveLayer();
		}
	}
	else if( m_sel_id.ii == (n_segs-1) && m_dir == 1 )
	{
		// last segment, force to layer of starting pad if SMT
		int p1 = m_sel_con.end_pin;
		if( p1 != cconnect::NO_END )
		{
			cpart * p = m_sel_net->pin[p1].part;
			CString pin_name = m_sel_net->pin[p1].pin_name;
			int pin_index = p->shape->GetPinIndexByName( pin_name );
			if( p1 != cconnect::NO_END )
			{
				if( p->shape->m_padstack[pin_index].hole_size == 0)
				{
					m_active_layer = m_Doc->m_plist->GetPinLayer( p, &pin_name );
					ShowActiveLayer();
				}
			}
		}
	}

	// now start dragging new segment
	m_dragging_new_item = 0;

	m_Doc->m_nlist->StartDraggingSegment(
		pDC,
		m_sel_net,
		m_sel_ic, m_sel_is,
		p.x, p.y,
		m_active_layer,
		LAY_SELECTION,
		last_seg_layer,
		m_sel_net->def_width,
		m_dir,
		2
	);

	SetCursorMode( CUR_DRAG_RAT );
	ReleaseDC( pDC );
}

// optimize this connection
//
void CFreePcbView::OnRatlineOptimize()
{
	int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic );
	ReselectNetItemIfConnectionsChanged( new_ic );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// optimize this connection
//
void CFreePcbView::OnRatlineChangeEndPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	cconnect * c = &m_sel_con;
	m_dlist->Set_visible( c->seg[m_sel_id.ii].dl_el, FALSE );
	int x, y;
	if( m_sel_id.ii == 0 )
	{
		// ratline is first segment of connection
		x = c->vtx[1].x;
		y = c->vtx[1].y;
	}
	else
	{
		// ratline is last segment of connection
		x = c->vtx[m_sel_id.ii].x;
		y = c->vtx[m_sel_id.ii].y;
	}
	m_dlist->StartDraggingRatLine( pDC, 0, 0, x, y, LAY_RAT_LINE, 1, 1 );
	SetCursorMode( CUR_DRAG_RAT_PIN );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// change vertex size vertex
//
void CFreePcbView::OnVertexSize()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	CDlgVia dlg = new CDlgVia;
	dlg.Initialize( m_sel_vtx.via_width );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_sel_vtx.via_width = dlg.m_via_width;

		m_dlist->CancelHighLight();
		m_Doc->m_nlist->DrawVia( m_sel_net, m_sel_ic, m_sel_is );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
	}
	Invalidate( FALSE );
}


void CFreePcbView::OnPadSetClearance()
{
	CDlgSetPinClearance dlg;

	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
	}
}

void CFreePcbView::OnVertexClearance()
{
	CDlgSetPinClearance dlg;

	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
	}
}


// move this vertex
//
void CFreePcbView::OnVertexMove()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	id id = m_sel_id;
	int ic = m_sel_id.i;
	int ivtx = m_sel_id.ii;
	m_dragging_new_item = 0;
	m_from_pt.x = m_sel_vtx.x;
	m_from_pt.y = m_sel_vtx.y;
	m_Doc->m_nlist->StartDraggingVertex( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_VTX );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete this vertex
// i.e. unroute adjacent segments and reroute if on same layer
// stub trace needs some special handling
//
void CFreePcbView::OnVertexDelete()
{
	int end_via_w, end_via_hole_w;
	int ic = m_sel_id.i;
	int iv = m_sel_id.ii;
	cconnect * c = &m_sel_net->connect[ic];
	cvertex * v = &c->vtx[iv];

	// deal with tee-connections
	int tee_id = v->tee_ID;
	if( tee_id )
	{
		// this is a tee-connection
		int ret = AfxMessageBox( "You are deleting a tee-vertex\nContinue ?",
									MB_OKCANCEL );
		if( ret == IDCANCEL )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	v->tee_ID = 0;
	v->force_via_flag = 0;
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );

	if( c->end_pin == cconnect::NO_END && iv == c->nsegs )
	{
		// last vertex of stub trace, just delete last segment
		m_Doc->m_nlist->RemoveSegment( m_sel_net, ic, iv-1, TRUE );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	}
	else if( c->seg[iv-1].layer != c->seg[iv].layer
				|| c->seg[iv-1].layer == LAY_RAT_LINE )
	{
		// deleting vertex between two disimilar segments
		v->force_via_flag = 0;
		v->tee_ID = 0;
		m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, iv-1 );
		m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, iv );
		m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	}
	else
	{
		// deleting vertex between two similar segments, combine them
		// start by unrouting both adjacent segments
		// NOTE: this creates a problem if the segments pre or post
		// the adjacent segments are unrouted, since they will be combined
		// with the adjacent segments when unrouted
		// the solution is to temporarily put them on a different layer,
		// then restore them
		int pre_pre_layer = -1;
		int pre_pre_width = -1;
		int post_post_layer = -1;
		int post_post_width = -1;
		if( iv >= 2 )
		{
			// temporarily put pre_pre_segment on top copper layer
			pre_pre_layer = c->seg[iv-2].layer;
			pre_pre_width = c->seg[iv-2].width();
			c->seg[iv-2].layer = LAY_TOP_COPPER-1;
			c->seg[iv-2].seg_width.m_seg_width = 1;
		}
		if( iv < c->nsegs-1 )
		{
			// temporarily put post_post_segment on top copper layer
			post_post_layer = c->seg[iv+1].layer;
			post_post_width = c->seg[iv+1].width();
			c->seg[iv+1].layer = LAY_TOP_COPPER-1;
			c->seg[iv+1].seg_width.m_seg_width = 1;
		}

		// save preceding vertex parameters
		CConnectionWidthInfo pre_via_width = c->vtx[iv-1].via_width;
		int pre_force_via_flag = c->vtx[iv-1].force_via_flag;
		int pre_tee_ID = c->vtx[iv-1].tee_ID;

		// save following vertex parameters
		CConnectionWidthInfo post_via_width = c->vtx[iv+1].via_width;
		int post_force_via_flag = c->vtx[iv+1].force_via_flag;
		int post_tee_ID = c->vtx[iv+1].tee_ID;

		// get adjacent segment width and layers
		int w = max( c->seg[iv-1].width(), c->seg[iv-1].width() );
		int pre_layer = c->seg[iv-1].layer;
		int post_layer = c->seg[iv].layer;
		m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, ic, iv );
		m_Doc->m_nlist->UnrouteSegment( m_sel_net, ic, iv-1 );
		m_dlist->CancelHighLight();
		// now reroute if they were on the same layer
		BOOL bReroute = !m_Doc->m_nlist->RouteSegment( m_sel_net, ic, iv-1, pre_layer, CSegWidthInfo(w) );
		ASSERT( bReroute );		// failed
		// reconstruct adjacent vias
		c->vtx[iv-1].via_width = pre_via_width;
		c->vtx[iv-1].force_via_flag = pre_force_via_flag;
		c->vtx[iv-1].tee_ID = pre_tee_ID;

		c->vtx[iv].via_width = post_via_width;
		c->vtx[iv].force_via_flag = post_force_via_flag;
		c->vtx[iv].tee_ID = post_tee_ID;

		m_Doc->m_nlist->ReconcileVia( m_sel_net, ic, iv-1 );
		m_Doc->m_nlist->ReconcileVia( m_sel_net, ic, iv );

		//BAF FIX
		// reconstruct segments next to adjacent segments
		if( pre_pre_layer != -1 )
		{
			c->seg[iv-2].layer = pre_pre_layer;
			c->seg[iv-2].seg_width.m_seg_width = pre_pre_width;
		}
		if( post_post_layer != -1 )
		{
			c->seg[iv].layer = post_post_layer;
			c->seg[iv].seg_width.m_seg_width = post_post_width;
		}
		m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, ic );
	}
	if( tee_id )
	{
		// if tee, remove it and any stubs
		m_Doc->m_nlist->RemoveTee( m_sel_net, tee_id );
		m_Doc->m_nlist->RemoveOrphanBranches( m_sel_net, tee_id, TRUE );
	}
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move the end vertex of a stub trace
//
void CFreePcbView::OnEndVertexMove()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	SetCursorMode( CUR_DRAG_END_VTX );
	m_Doc->m_nlist->StartDraggingEndVertex( pDC, m_sel_net, m_sel_ic, m_sel_is, 2 );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}


// force a via on end vertex
//
void CFreePcbView::OnEndVertexAddVia()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_is );
	SetFKText( m_cursor_mode );
	int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic );
	ReselectNetItemIfConnectionsChanged( new_ic );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// remove forced via on end vertex
//
void CFreePcbView::OnEndVertexRemoveVia()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->UnforceVia( m_sel_net, m_sel_ic, m_sel_is, FALSE );
	if( m_sel_con.seg[m_sel_is-1].layer == LAY_RAT_LINE )
	{
		m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1, TRUE );
		CancelSelection();
	}
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	int new_ic = m_Doc->m_nlist->OptimizeConnections( m_sel_net, m_sel_ic );
	ReselectNetItemIfConnectionsChanged( new_ic );
	Invalidate( FALSE );
}

// append more segments to this stub trace
//
void CFreePcbView::OnEndVertexAddSegments()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	m_sel_id.sst = ID_SEL_SEG;
	m_snap_angle_ref.x = m_sel_vtx.x;
	m_snap_angle_ref.y = m_sel_vtx.y;

	m_Doc->m_nlist->StartDraggingStub( 
		pDC, 
		m_sel_net, 
		m_sel_ic, m_sel_is,
		p.x, p.y, 
		m_active_layer, 
		m_sel_net->def_width.m_seg_width.m_val, 
		m_active_layer, 
		m_sel_net->def_width.m_via_width.m_val, 
		m_sel_net->def_width.m_via_hole.m_val,
		2, 
		m_inflection_mode 
	);

	SetCursorMode( CUR_DRAG_STUB );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// convert stub trace to regular connection by adding ratline to a pad
//
void CFreePcbView::OnEndVertexAddConnection()
{
	OnVertexConnectToPin();
}

// end vertex selected, delete it and the adjacent segment
//
void CFreePcbView::OnEndVertexDelete()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1, TRUE );
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// edit the position of an end vertex
//
void CFreePcbView::OnEndVertexEdit()
{
	DlgEditBoardCorner dlg;
	CString str = "Edit End Vertex Position";
	int x = m_sel_vtx.x;
	int y = m_sel_vtx.y;
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->MoveEndVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.GetX(), dlg.GetY() );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
}

// finish routing a connection by making a segment to the destination pad
//
void CFreePcbView::OnRatlineComplete()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );

	// complete routing to pin
	int test = m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is, m_active_layer, CSegWidthInfo() );
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
	m_Doc->ProjectModified( TRUE );
}

// set width of a connection
//
void CFreePcbView::OnRatlineSetWidth()
{
	if( m_sel_con.nsegs == 1 )
		SetWidth( 2 );
	else
		SetWidth( 1 );

	Invalidate( FALSE );
}

// set clearance of a connection
//
void CFreePcbView::OnRatlineSetClearance()
{
	if( m_sel_con.nsegs == 1 )
		SetClearance( 2 );
	else
		SetClearance( 1 );

	Invalidate( FALSE );
}

// delete a connection
//
void CFreePcbView::OnRatlineDeleteConnection()
{
	if( m_sel_con.locked )
	{
		int ret = AfxMessageBox( "You are trying to delete a locked connection.\nAre you sure ? ",
			MB_YESNO );
		if( ret == IDNO )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic, FALSE );
	m_Doc->m_nlist->RemoveOrphanBranches( m_sel_net, 0, TRUE );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// Set the width of the ratlines themselves
void CFreePcbView::OnRatlineSetRatlineWidth()
{
	CRatWidth dlg;
	dlg.m_ratline_w = m_Doc->m_ratline_w / NM_PER_MIL;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CancelSelection();

		m_Doc->SetRatlineWidth(dlg.m_ratline_w * NM_PER_MIL);

		Invalidate( FALSE );
	}
}

// lock a connection
//
void CFreePcbView::OnRatlineLockConnection()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_sel_con.locked = 1;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// unlock a connection
//
void CFreePcbView::OnRatlineUnlockConnection()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	m_sel_con.locked = 0;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
}

// edit a text string
//
void CFreePcbView::OnTextEdit()
{
	// create dialog and pass parameters
	CDlgAddText add_text_dlg;
	add_text_dlg.Initialize( 0, m_Doc->m_num_layers, 0, &m_sel_text->m_str,
		m_Doc->m_units, m_sel_text->m_layer, m_sel_text->m_mirror,
			m_sel_text->m_bNegative, m_sel_text->m_angle, m_sel_text->m_font_size,
			m_sel_text->m_stroke_width, m_sel_text->m_x, m_sel_text->m_y );
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;

	// now replace old text with new one
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY, TRUE, m_Doc->m_undo_list );
	int x = add_text_dlg.m_x;
	int y = add_text_dlg.m_y;
	int mirror = add_text_dlg.m_mirror;
	BOOL bNegative = add_text_dlg.m_bNegative;
	int angle = add_text_dlg.m_angle;
	int font_size = add_text_dlg.m_height;
	int stroke_width = add_text_dlg.m_width;
	int layer = add_text_dlg.m_layer;
	CString test_str = add_text_dlg.m_str;
	m_dlist->CancelHighLight();
	CText * new_text = m_Doc->m_tlist->AddText( x, y, angle, mirror, bNegative,
		layer, font_size, stroke_width, &test_str );
	new_text->m_guid = m_sel_text->m_guid;
	m_Doc->m_tlist->RemoveText( m_sel_text );
	m_sel_text = new_text;
	m_Doc->m_tlist->HighlightText( m_sel_text );

	// start dragging if requested in dialog
	if( add_text_dlg.m_drag_flag )
		OnTextMove();
	else
		Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
}

// start adding board outline by dragging line for first side
//
void CFreePcbView::OnAddBoardOutline()
{
	m_Doc->m_vis[LAY_BOARD_OUTLINE] = TRUE;
	m_dlist->SetLayerVisible( LAY_BOARD_OUTLINE, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_dlist->CancelHighLight();
	int ib = m_Doc->m_board_outline.GetSize() - 1;
	m_sel_id.Set( ID_BOARD, ID_BOARD_OUTLINE, ib, ID_SEL_CORNER, 0 );
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
	m_from_pt.x = m_Doc->m_board_outline[m_sel_id.i].GetX( m_sel_id.ii );
	m_from_pt.y = m_Doc->m_board_outline[m_sel_id.i].GetY( m_sel_id.ii );
	m_Doc->m_board_outline[m_sel_id.i].StartDraggingToMoveCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_BOARD_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// edit a board outline corner
//
void CFreePcbView::OnBoardCornerEdit()
{
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	int x = m_Doc->m_board_outline[m_sel_id.i].GetX(m_sel_id.ii);
	int y = m_Doc->m_board_outline[m_sel_id.i].GetY(m_sel_id.ii);
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
		m_Doc->m_board_outline[m_sel_id.i].MoveCorner( m_sel_id.ii,
			dlg.GetX(), dlg.GetY() );
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// delete a board corner
//
void CFreePcbView::OnBoardCornerDelete()
{
	if( m_Doc->m_board_outline[m_sel_id.i].GetNumCorners() < 4 )
	{
		AfxMessageBox( "Board outline has too few corners" );
		return;
	}
	SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
	m_Doc->m_board_outline[m_sel_id.i].DeleteCorner( m_sel_id.ii );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
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
	m_Doc->m_board_outline[m_sel_id.i].StartDraggingToInsertCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_BOARD_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete entire board outline
//
void CFreePcbView::OnBoardDeleteOutline()
{
	SaveUndoInfoForBoardOutlines( TRUE, m_Doc->m_undo_list );
	m_Doc->m_board_outline.RemoveAt( m_sel_id.i );
	id new_id = m_sel_id;
	for( int i=m_sel_id.i; i<m_Doc->m_board_outline.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[i];
		new_id.i = i;
		poly->SetId( &new_id );
	}
	m_Doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

// move a copper area corner
//
void CFreePcbView::OnAreaCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_sel_net->area[m_sel_id.i].poly->GetX( m_sel_id.ii );
	m_from_pt.y = m_sel_net->area[m_sel_id.i].poly->GetY( m_sel_id.ii );
	m_Doc->m_nlist->StartDraggingAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete a copper area corner
//
void CFreePcbView::OnAreaCornerDelete()
{
	carea * area;
	area = &m_sel_net->area[m_sel_id.i];
	if( area->poly->GetNumCorners() > 3 )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
//		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		area->poly->DeleteCorner( m_sel_id.ii );
		m_dlist->CancelHighLight();
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		m_Doc->m_nlist->OptimizeConnections( m_sel_net );
		m_Doc->ProjectModified( TRUE );
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else
		OnAreaCornerDeleteArea();
}

// delete entire area
//
void CFreePcbView::OnAreaCornerDeleteArea()
{
	OnAreaSideDeleteArea();
	m_Doc->ProjectModified( TRUE );
}

//insert a new corner in a side of a copper area
//
void CFreePcbView::OnAreaSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_Doc->m_nlist->StartDraggingInsertedAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete entire area
//
void CFreePcbView::OnAreaSideDeleteArea()
{
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_DELETE, TRUE, m_Doc->m_undo_list );
//	SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_DELETE, TRUE, m_Doc->m_undo_list );
	m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
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
				if( nFlags & MK_CONTROL )
				{
					// control key held down
					m_snap_mode = SM_GRID_LINES;
					m_inflection_mode = IM_NONE;
				}
				else
				{
					m_snap_mode = SM_GRID_POINTS;
					if( m_Doc->m_snap_angle == 45 )
					{
						if( nFlags & MK_SHIFT )
							m_inflection_mode = IM_45_90;
						else
							m_inflection_mode = IM_90_45;
					}
					else if( m_Doc->m_snap_angle == 90 )
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
			grid_spacing = m_Doc->m_part_grid_spacing;
		}
		else if( CurDraggingRouting() )
		{
			grid_spacing = m_Doc->m_routing_grid_spacing;
		}
		else if( m_Doc->m_units == MIL )
		{
			grid_spacing = m_Doc->m_pcbu_per_wu;
		}
		else if( m_Doc->m_units == MM )
		{
			grid_spacing = m_Doc->m_pcbu_per_wu;
		}
		else
			ASSERT(0);
		// see if we need to snap to angle
		if( m_Doc->m_snap_angle && (wp != m_snap_angle_ref)
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
			{
				// patch to snap to grid lines, contributed by ???
				// modified by AMW to work when cursor x,y are < 0
				// offset cursor and ref positions by integral number of grid spaces
				// to make all values positive
				double offset_grid_spaces;
				modf( (double)INT_MAX/grid_spacing, &offset_grid_spaces );
				double offset = offset_grid_spaces*grid_spacing;
				double off_x = wp.x + offset;
				double off_y = wp.y + offset;
				double ref_x = m_snap_angle_ref.x + offset;
				double ref_y = m_snap_angle_ref.y + offset;
				//find nearest snap angle to an integer division of 90
				int snap_angle = m_Doc->m_snap_angle;
				if(90 % snap_angle != 0)
				{
					int snap_pos = snap_angle;
					int snap_neg = snap_angle;
					while(90 % snap_angle != 0)
					{
						snap_pos++;
						snap_neg--;
						if(snap_pos >= 90)
							snap_pos = 90;
						if(snap_neg <= 1)
							snap_neg = 1;
						if(90 % snap_pos == 0)
							snap_angle = snap_pos;
						if(90 % snap_neg == 0)
							snap_angle = snap_neg;
					}
				}

				//snap the x and y coordinates to the appropriate angle
				double angle_grid = snap_angle*M_PI/180.0;
				double dx = off_x - ref_x;
				double dy = off_y - ref_y;
				double angle = atan2(dy,dx) + 2*M_PI; //make it a positive angle
				if(angle > 2*M_PI)
					angle -= 2*M_PI;
				double angle_angle_grid = angle/angle_grid;
				int sel_snap_angle = (int)angle_angle_grid + ((angle_angle_grid - (double)((int)angle_angle_grid)) > 0.5 ? 1 : 0);
				double point_angle = angle_grid*sel_snap_angle; //result of angle calculation
				CString test, test_grid;
				test.Format("point_angle: %f\r\n",point_angle);
				test_grid.Format("grid_spacing: %d\r\n",grid_spacing);


				//find the distance along that angle
				//match the distance the actual point is from the start point
				//double dist = sqrt(dx*dx + dy*dy);
				double dist = dx*cos(point_angle)+dy*sin(point_angle);

				double distx = dist*cos(point_angle);
				double disty = dist*sin(point_angle);

				double xpos = ref_x + distx;
				double ypos = ref_y + disty;


				//special case horizontal lines and vertical lines
				// just to make sure floating point error doesn't cause any problems
				if(APPROX(point_angle,0) || APPROX(point_angle,2*M_PI) || APPROX(point_angle,M_PI))
				{
					//horizontal line
					//snap x component to nearest grid
					off_y = (int)(ypos + 0.5);
					double modval = fmod(xpos,(double)grid_spacing);
					if(modval > grid_spacing/2.0)
					{
						//round up to nearest grid space
						off_x = xpos + ((double)grid_spacing - modval);
					}
					else
					{
						//round down to nearest grid space
						off_x = xpos - modval;
					}
				}
				else if(APPROX(point_angle,M_PI/2) || APPROX(point_angle,3*M_PI/2))
				{
					//vertical line
					//snap y component to nearest grid
					off_x = (int)(xpos + 0.5);
					double modval = fabs(fmod(ypos,(double)grid_spacing));
					int test = modval * grid_spacing - offset;
					if(modval > grid_spacing/2.0)
					{
						off_y = ypos + ((double)grid_spacing - modval);
					}
					else
					{
						off_y = ypos - modval;
					}
				}
				else
				{
					//normal case
					//snap x and y components to nearest grid line along the same angle
					//calculate grid lines surrounding point
					int minx = ((int)(xpos/(double)grid_spacing))*grid_spacing - (xpos < 0);
					//int minx = (int)fmod(xpos,(double)grid_spacing);
					int maxx = minx + grid_spacing;
					int miny = ((int)(ypos/(double)grid_spacing))*grid_spacing - (ypos < 0);
					//int miny = (int)fmod(ypos,(double)grid_spacing);
					int maxy = miny + grid_spacing;

					//calculate the relative distances to each of those grid lines from the ref point
					int rminx = minx - ref_x;
					int rmaxx = maxx - ref_x;
					int rminy = miny - ref_y;
					int rmaxy = maxy - ref_y;

					//calculate the length of the hypotenuse of the triangle
					double maxxh = dist*(double)rmaxx/distx;
					double minxh = dist*(double)rminx/distx;
					double maxyh = dist*(double)rmaxy/disty;
					double minyh = dist*(double)rminy/disty;

					//some stupidly large value.  One of the results MUST be smaller than this unless the grid is this large (unlikely)
					double mindist = 1e100;
					int select = 0;

					if(fabs(maxxh - dist) < mindist)
					{
						select = 1;
						mindist = fabs(maxxh - dist);
					}
					if(fabs(minxh - dist) < mindist)
					{
						select = 2;
						mindist = fabs(minxh - dist);
					}
					if(fabs(maxyh - dist) < mindist)
					{
						select = 3;
						mindist = fabs(maxyh - dist);
					}
					if(fabs(minyh - dist) < mindist)
					{
						select = 4;
						mindist = fabs(minyh - dist);
					}

					switch(select)
					{
					case 1:
						//snap to line right of point
						off_x = maxx;
						off_y = (int)(disty*(double)rmaxx/distx + (double)(ref_y) + 0.5);
						break;
					case 2:
						//snap to line left of point
						off_x = minx;
						off_y = (int)(disty*(double)rminx/distx + (double)(ref_y) + 0.5);
						break;
					case 3:
						//snap to line above point
						off_x = (int)(distx*(double)rmaxy/disty + (double)(ref_x) + 0.5);
						off_y = maxy;
						break;
					case 4:
						//snap to line below point
						off_x = (int)(distx*(double)rminy/disty + (double)(ref_x) + 0.5);
						off_y = miny;
						break;
					default:
						ASSERT(0);//error
					}

				}
				// remove offset and correct for round-off
				double ttest = off_x - offset;
				if( ttest >= 0.0 )
					wp.x = ttest + 0.5;
				else
					wp.x = ttest - 0.5;
				ttest = off_y - offset;
				if( ttest >= 0.0 )
					wp.y = ttest + 0.5;
				else
					wp.y = ttest - 0.5;
			}
			else
			{
				// old code
				// snap to angle only if the starting point is on-grid
				double ddx = fmod( (double)(m_snap_angle_ref.x), grid_spacing );
				double ddy = fmod( (double)(m_snap_angle_ref.y), grid_spacing );
				if( fabs(ddx) < 0.5 && fabs(ddy) < 0.5 )
				{
					// starting point is on-grid, snap to angle
					// snap to n*45 degree angle
					const double pi = 3.14159265359;
					double dx = wp.x - m_snap_angle_ref.x;
					double dy = wp.y - m_snap_angle_ref.y;
					double dist = sqrt( dx*dx + dy*dy );
					double dist45 = dist/sqrt(2.0);
					{
						int d;
						d = (int)(dist/grid_spacing+0.5);
						dist = d*grid_spacing;
						d = (int)(dist45/grid_spacing+0.5);
						dist45 = d*grid_spacing;
					}
					if( m_Doc->m_snap_angle == 45 )
					{
						// snap angle = 45 degrees, divide circle into 8 octants
						double angle = atan2( dy, dx );
						if( angle < 0.0 )
							angle = 2.0*pi + angle;
						angle += pi/8.0;
						double d_quad = angle/(pi/4.0);
						int oct = d_quad;
						switch( oct )
						{
						case 0:
							wp.x = m_snap_angle_ref.x + dist;
							wp.y = m_snap_angle_ref.y;
							break;
						case 1:
							wp.x = m_snap_angle_ref.x + dist45;
							wp.y = m_snap_angle_ref.y + dist45;
							break;
						case 2:
							wp.x = m_snap_angle_ref.x;
							wp.y = m_snap_angle_ref.y + dist;
							break;
						case 3:
							wp.x = m_snap_angle_ref.x - dist45;
							wp.y = m_snap_angle_ref.y + dist45;
							break;
						case 4:
							wp.x = m_snap_angle_ref.x - dist;
							wp.y = m_snap_angle_ref.y;
							break;
						case 5:
							wp.x = m_snap_angle_ref.x - dist45;
							wp.y = m_snap_angle_ref.y - dist45;
							break;
						case 6:
							wp.x = m_snap_angle_ref.x;
							wp.y = m_snap_angle_ref.y - dist;
							break;
						case 7:
							wp.x = m_snap_angle_ref.x + dist45;
							wp.y = m_snap_angle_ref.y - dist45;
							break;
						case 8:
							wp.x = m_snap_angle_ref.x + dist;
							wp.y = m_snap_angle_ref.y;
							break;
						default:
							ASSERT(0);
							break;
						}
					}
					else
					{
						// snap angle is 90 degrees, divide into 4 quadrants
						double angle = atan2( dy, dx );
						if( angle < 0.0 )
							angle = 2.0*pi + angle;
						angle += pi/4.0;
						double d_quad = angle/(pi/2.0);
						int quad = d_quad;
						switch( quad )
						{
						case 0:
							wp.x = m_snap_angle_ref.x + dist;
							wp.y = m_snap_angle_ref.y;
							break;
						case 1:
							wp.x = m_snap_angle_ref.x;
							wp.y = m_snap_angle_ref.y + dist;
							break;
						case 2:
							wp.x = m_snap_angle_ref.x - dist;
							wp.y = m_snap_angle_ref.y;
							break;
						case 3:
							wp.x = m_snap_angle_ref.x;
							wp.y = m_snap_angle_ref.y - dist;
							break;
						case 4:
							wp.x = m_snap_angle_ref.x + dist;
							wp.y = m_snap_angle_ref.y;
							break;
						default:
							ASSERT(0);
							break;
						}
					}
				}
			}
		}
		else
			m_snap_mode = SM_GRID_POINTS;

		// snap to grid points if needed
		if( m_snap_mode == SM_GRID_POINTS )
		{
			// snap to grid
			// get position in integral units of grid_spacing
			if( wp.x > 0 )
				wp.x = (wp.x + grid_spacing/2)/grid_spacing;
			else
				wp.x = (wp.x - grid_spacing/2)/grid_spacing;
			if( wp.y > 0 )
				wp.y = (wp.y + grid_spacing/2)/grid_spacing;
			else
				wp.y = (wp.y - grid_spacing/2)/grid_spacing;
			// multiply by grid spacing, adding or subracting 0.5 to prevent round-off
			// when using a fractional grid
			double test = wp.x * grid_spacing;
			if( test > 0.0 )
				test += 0.5;
			else
				test -= 0.5;
			wp.x = test;
			test = wp.y * grid_spacing;
			if( test > 0.0 )
				test += 0.5;
			else
				test -= 0.5;
			wp.y = test;
		}
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
		m_Doc->m_visual_grid_spacing = fabs( m_Doc->m_visible_grid[lp] );
	else
		ASSERT(0);
	m_dlist->SetVisibleGrid( TRUE, m_Doc->m_visual_grid_spacing );
	Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangePlacementGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_part_grid_spacing = fabs( m_Doc->m_part_grid[lp] );
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangeRoutingGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_routing_grid_spacing = fabs( m_Doc->m_routing_grid[lp] );
	else
		ASSERT(0);
	SetFocus();
	m_Doc->ProjectModified( TRUE );
	return 0;
}

LONG CFreePcbView::OnChangeSnapAngle( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
		{
			m_Doc->m_snap_angle = 45;
			m_inflection_mode = IM_90_45;
		}
		else if( lp == 1 )
		{
			m_Doc->m_snap_angle = 90;
			m_inflection_mode = IM_90;
		}
		else
		{
			m_Doc->m_snap_angle = 0;
			m_inflection_mode = IM_NONE;
		}
	}
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangeUnits( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
			m_Doc->m_units = MIL;
		else
			m_Doc->m_units = MM;
	}
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	ShowSelectStatus();
	return 0;
}


void CFreePcbView::OnSegmentDeleteTrace()
{
	OnRatlineDeleteConnection();
}

void CFreePcbView::OnAreaCornerProperties()
{
	// reuse board corner dialog
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	CPoint pt = m_Doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
	dlg.Init( &str, m_Doc->m_units, pt.x, pt.y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
//		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is,
			dlg.GetX(), dlg.GetY() );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		m_Doc->m_nlist->OptimizeConnections( m_sel_net );
		m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnRefProperties()
{
	CDlgRefText dlg;
	dlg.Initialize( m_Doc->m_plist, m_sel_part, &m_Doc->m_footprint_cache_map );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// edit this part
		SaveUndoInfoForPart( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
		m_Doc->m_plist->ResizeRefText( m_sel_part, dlg.m_height, dlg.m_width, dlg.m_vis );
		m_Doc->ProjectModified( TRUE );
		m_dlist->CancelHighLight();
		if( m_cursor_mode == CUR_PART_SELECTED )
			m_Doc->m_plist->SelectPart( m_sel_part );
		else if( m_cursor_mode == CUR_REF_SELECTED
			&& m_sel_part->m_ref_size && m_sel_part->m_ref_vis )
			m_Doc->m_plist->SelectRefText( m_sel_part );
		else
			CancelSelection();
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnVertexProperties()
{
	DlgEditBoardCorner dlg;
	CString str = "Vertex Position";
	int x = m_sel_vtx.x;
	int y = m_sel_vtx.y;
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY,
			TRUE, m_Doc->m_undo_list );
		m_dlist->CancelHighLight();
		m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.GetX(), dlg.GetY() );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
}


BOOL CFreePcbView::OnEraseBkgnd(CDC* pDC)
{
	// Erase the left and bottom panes, the PCB area is always redrawn
	m_left_pane_invalid = TRUE;
	return FALSE;
}

void CFreePcbView::OnBoardSideConvertToStraightLine()
{
	m_Doc->m_board_outline[m_sel_id.i].SetSideStyle( m_sel_id.ii, CPolyLine::STRAIGHT );
	m_Doc->m_board_outline[m_sel_id.i].HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnBoardSideConvertToArcCw()
{
	m_Doc->m_board_outline[m_sel_id.i].SetSideStyle( m_sel_id.ii, CPolyLine::ARC_CW );
	m_Doc->m_board_outline[m_sel_id.i].HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnBoardSideConvertToArcCcw()
{
	m_Doc->m_board_outline[m_sel_id.i].SetSideStyle( m_sel_id.ii, CPolyLine::ARC_CCW );
	m_Doc->m_board_outline[m_sel_id.i].HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// unroute entire connection
//
void CFreePcbView::OnUnrouteTrace()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
	cconnect * c = &m_sel_con;
	for( int is=0; is<c->nsegs; is++ )
		m_Doc->m_nlist->UnrouteSegmentWithoutMerge( m_sel_net, m_sel_ic, is );
	m_Doc->m_nlist->MergeUnroutedSegments( m_sel_net, m_sel_ic );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, 0 );
	m_sel_id.st = ID_SEL_SEG;
	m_sel_id.ii = 0;
	SetCursorMode( CUR_RAT_SELECTED );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// save undo info for a group, for UNDO_GROUP_MODIFY or UNDO_GROUP_DELETE
// creates
//
void CFreePcbView::SaveUndoInfoForGroup( int type, CArray<void*> * ptrs, CArray<id> * ids, CUndoList * list )
{
	if( type != UNDO_GROUP_MODIFY && type != UNDO_GROUP_DELETE )
		ASSERT(0);

	void * ptr;

	// first, mark all nets as unselected and set new_event flag
	m_Doc->m_nlist->MarkAllNets(0);
	list->NewEvent();

	// save info for all relevant nets
	for( int i=0; i<ids->GetSize(); i++ )
	{
		id sid = (*ids)[i];
		if( sid.type == ID_PART )
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
		else if( sid.type == ID_NET )
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
		if( sid.type == ID_PART )
		{
			cpart * part = (cpart*)(*ptrs)[i];
			SaveUndoInfoForPart( part,
				CPartList::UNDO_PART_MODIFY, NULL, FALSE, list );
		}
		else if( sid.type == ID_TEXT )
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
}

// save undo info for two existing parts and all nets connected to them,
// assuming that the parts will be modified (not deleted or added)
//
void CFreePcbView::SaveUndoInfoFor2PartsAndNets( cpart * part1, cpart * part2, BOOL new_event, CUndoList * list )
{
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
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
						list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
							&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
					}
					undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
					list->Push( CNetList::UNDO_NET_MODIFY, u_net,
						&m_Doc->m_nlist->NetUndoCallback, u_net->size );
					net->utility = 1;
				}
			}
		}
	}
	// now save undo info for parts
	undo_part * u_part1 = m_Doc->m_plist->CreatePartUndoRecord( part1, NULL );
	list->Push( CPartList::UNDO_PART_MODIFY, u_part1,
		&m_Doc->m_plist->PartUndoCallback, u_part1->size );
	undo_part * u_part2 = m_Doc->m_plist->CreatePartUndoRecord( part2, NULL );
	list->Push( CPartList::UNDO_PART_MODIFY, u_part2,
		&m_Doc->m_plist->PartUndoCallback, u_part2->size );
	// now save undo descriptor
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, 0, &part1->ref_des, &part2->ref_des, 0, 0, NULL, NULL );
		list->Push( UNDO_2_PARTS_AND_NETS, ptr, &UndoCallback );
	}
}

// save undo info for net, all connections and all areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndAreas( cnet * net, BOOL new_event, CUndoList * list )
{
	if( new_event )
		ASSERT( 0 );
	SaveUndoInfoForNetAndConnections( net,
		CNetList::UNDO_NET_MODIFY, new_event, list );
	for( int ia=0; ia<net->nareas; ia++ )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_MODIFY, FALSE, list );
}

// save undo info for net, all connections and
// a single copper area
// type may be:
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndArea( cnet * net, int iarea,
														   int type, BOOL new_event, CUndoList * list )
{
	if( new_event )
	{
		list->NewEvent();
	}
	SaveUndoInfoForArea( net, iarea, type, FALSE, m_Doc->m_undo_list );
	SaveUndoInfoForNetAndConnections( net,
		CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, &net->name, NULL, iarea, 0, NULL, NULL );
		list->Push( UNDO_NET_AND_CONNECTIONS_AND_AREA, ptr, &UndoCallback );
	}
}

// save undo info for a copper area to be modified or deleted
// type may be:
//	CNetList::UNDO_AREA_ADD	if area will be added
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForArea( cnet * net, int iarea, int type, BOOL new_event, CUndoList * list )
{
	void *ptr;
	if( new_event )
	{
		list->NewEvent();
		SaveUndoInfoForNet( net, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	int nc = 1;
	if( type != CNetList::UNDO_AREA_ADD )
	{
		nc = net->area[iarea].poly->GetNumContours();
		if( !net->area[iarea].poly->GetClosed() )
			nc--;
	}
	if( nc > 0 )
	{
		undo_area * undo = m_Doc->m_nlist->CreateAreaUndoRecord( net, iarea, type );
		list->Push( type, (void*)undo, &m_Doc->m_nlist->AreaUndoCallback, undo->size );
	}
	// now save undo descriptor
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, type, &net->name, NULL, iarea, 0, NULL, NULL );
		list->Push( UNDO_AREA, ptr, &UndoCallback );
	}
}

// save undo info for all of the areas in a net
//
void CFreePcbView::SaveUndoInfoForAllAreasInNet( cnet * net, BOOL new_event, CUndoList * list )
{
	if( new_event )
	{
		list->NewEvent();		// flag new undo event
		SaveUndoInfoForNet( net, CNetList::UNDO_NET_OPTIMIZE, FALSE, list );
	}
	for( int ia=net->area.GetSize()-1; ia>=0; ia-- )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_DELETE, FALSE, list );
	undo_area * u_area = m_Doc->m_nlist->CreateAreaUndoRecord( net, 0, CNetList::UNDO_AREA_CLEAR_ALL );
	list->Push( CNetList::UNDO_AREA_CLEAR_ALL, u_area, &m_Doc->m_nlist->AreaUndoCallback, u_area->size );
	// now save undo descriptor
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, &net->name, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_AREAS_IN_NET, ptr, &UndoCallback );
	}
}

// save undo info for all of the areas in two nets
//
void CFreePcbView::SaveUndoInfoForAllAreasIn2Nets( cnet * net1, cnet * net2, BOOL new_event, CUndoList * list )
{
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
}

// save undo info for all nets (but not areas)
//
void CFreePcbView::SaveUndoInfoForAllNets( BOOL new_event, CUndoList * list )
{
	POSITION pos;
	CString name;
	CMapStringToPtr * m_map = &m_Doc->m_nlist->m_map;
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
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
			list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
				&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
		}
		undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
		list->Push( CNetList::UNDO_NET_MODIFY, u_net,
			&m_Doc->m_nlist->NetUndoCallback, u_net->size );
	}
}

// save undo info for all nets including areas
//
void CFreePcbView::SaveUndoInfoForAllNetsAndConnectionsAndAreas( BOOL new_event, CUndoList * list )
{
	POSITION pos;
	CString name;
	CMapStringToPtr * m_map = &m_Doc->m_nlist->m_map;
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
}

void CFreePcbView::SaveUndoInfoForMoveOrigin( int x_off, int y_off, CUndoList * list )
{
	// now push onto undo list
	undo_move_origin * undo = m_Doc->CreateMoveOriginUndoRecord( x_off, y_off );
	list->NewEvent();
	list->Push( 0, (void*)undo, &m_Doc->MoveOriginUndoCallback );
	// save top-level descriptor
	void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, x_off, y_off, NULL, NULL );
	list->Push( UNDO_MOVE_ORIGIN, ptr, &UndoCallback );
}

void CFreePcbView::SaveUndoInfoForBoardOutlines( BOOL new_event, CUndoList * list )
{
	// now push onto undo list
	if( new_event )
		list->NewEvent();		// flag new undo event

	// get number of closed board outlines
	int n_closed = 0;
	for( int i=0; i<m_Doc->m_board_outline.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[i];
		if( poly->GetClosed() )
			n_closed = i+1;
	}
	// push all board outlines onto undo list
	for( int i=0; i<n_closed; i++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[i];
		undo_board_outline * undo = m_Doc->CreateBoardOutlineUndoRecord( poly );
		list->Push( UNDO_BOARD, (void*)undo, &m_Doc->BoardOutlineUndoCallback );
	}
	list->Push( UNDO_BOARD_OUTLINE_CLEAR_ALL, NULL, &m_Doc->BoardOutlineUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_BOARD_OUTLINES, ptr, &UndoCallback );
	}
}

void CFreePcbView::SaveUndoInfoForSMCutouts( BOOL new_event, CUndoList * list )
{
	// push undo info onto list
	if( new_event )
		list->NewEvent();		// flag new undo event
	// get last closed cutout
	int i;
	int n_closed = 0;
	for( i=0; i<m_Doc->m_sm_cutout.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[i];
		if( poly->GetClosed() )
			n_closed = i+1;
		else
			break;
	}
	// push all closed cutouts onto undo list
	for( i=0; i<n_closed; i++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[i];
		undo_sm_cutout * undo = m_Doc->CreateSMCutoutUndoRecord( poly );
		list->Push( UNDO_SM_CUTOUT, (void*)undo, &m_Doc->SMCutoutUndoCallback );
	}
	// create UNDO_SM_CUTOUT_CLEAR_ALL record and push it
	list->Push( UNDO_SM_CUTOUT_CLEAR_ALL, NULL, &m_Doc->SMCutoutUndoCallback );
	// now push top-level callback for redoing
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, 0, NULL, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_ALL_SM_CUTOUTS, ptr, &UndoCallback );
	}
}

void CFreePcbView::SaveUndoInfoForText( CText * text, int type, BOOL new_event, CUndoList * list )
{
	// create new undo record and push onto undo list
	undo_text * undo = m_Doc->m_tlist->CreateUndoRecord( text );
	if( new_event )
		list->NewEvent();		// flag new undo event
	list->Push( type, (void*)undo, &m_Doc->m_tlist->TextUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, NULL, NULL, 0, 0, NULL, (void *)undo );
		list->Push( UNDO_TEXT, ptr, &UndoCallback );
	}
}

void CFreePcbView::SaveUndoInfoForText( undo_text * u_text, int type, BOOL new_event, CUndoList * list )
{
	// copy undo record and push onto undo list
	undo_text * undo = new undo_text;
	*undo = *u_text;
	if( new_event )
		list->NewEvent();		// flag new undo event
	list->Push( type, (void*)undo, &m_Doc->m_tlist->TextUndoCallback );
	if( new_event )
	{
		void * ptr = CreateUndoDescriptor( list, type, NULL, NULL, 0, 0, NULL, (void*)undo );
		list->Push( UNDO_TEXT, ptr, &UndoCallback );
	}
}


void CFreePcbView::OnViewEntireBoard()
{
	if( m_Doc->m_board_outline.GetSize() )
	{
		// get boundaries of board outline
		int max_x = INT_MIN;
		int min_x = INT_MAX;
		int max_y = INT_MIN;
		int min_y = INT_MAX;
		for( int ib=0; ib<m_Doc->m_board_outline.GetSize(); ib++ )
		{
			for( int ic=0; ic<m_Doc->m_board_outline[0].GetNumCorners(); ic++ )
			{
				if( m_Doc->m_board_outline[ib].GetX( ic ) > max_x )
					max_x = m_Doc->m_board_outline[ib].GetX( ic );
				if( m_Doc->m_board_outline[ib].GetX( ic ) < min_x )
					min_x = m_Doc->m_board_outline[ib].GetX( ic );
				if( m_Doc->m_board_outline[ib].GetY( ic ) > max_y )
					max_y = m_Doc->m_board_outline[ib].GetY( ic );
				if( m_Doc->m_board_outline[ib].GetY( ic ) < min_y )
					min_y = m_Doc->m_board_outline[ib].GetY( ic );
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
		AfxMessageBox( "Board outline does not exist" );
	}
}

void CFreePcbView::OnViewAllElements()
{
	// reset window to enclose all elements
	BOOL bOK = FALSE;
	CRect r;
	// parts
	int test = m_Doc->m_plist->GetPartBoundaries( &r );
	if( test != 0 )
		bOK = TRUE;
	int max_x = r.right;
	int min_x = r.left;
	int max_y = r.top;
	int min_y = r.bottom;
	// board outline
	for( int ib=0; ib<m_Doc->m_board_outline.GetSize(); ib++ )
	{
		r = m_Doc->m_board_outline[ib].GetBounds();
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	// nets
	if( m_Doc->m_nlist->GetNetBoundaries( &r ) )
	{
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	// texts
	if( m_Doc->m_tlist->GetTextBoundaries( &r ) )
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
		CRect screen_r;
		GetWindowRect( &screen_r );
		m_dlist->SetMapping( &m_client_r, &screen_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel,
			m_org_x, m_org_y );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnAreaEdgeHatchStyle()
{
	CDlgSetAreaHatch dlg;
	dlg.Init( m_sel_net->area[m_sel_id.i].poly->GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int hatch = dlg.GetHatch();
		m_sel_net->area[m_sel_id.i].poly->SetHatch( hatch );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnPartEditFootprint()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnPartEditThisFootprint()
{
	m_Doc->m_edit_footprint = TRUE;
	theApp.OnViewFootprint();
}

// Offer new footprint from the Footprint Editor
//
void CFreePcbView::OnExternalChangeFootprint( CShape * fp )
{
	CString str;
	str.Format( "Do you wish to replace the footprint of part \"%s\"\nwith the new footprint \"%s\" ?",
		m_sel_part->ref_des, fp->m_name );
	int ret = AfxMessageBox( str, MB_YESNO );
	if( ret == IDYES )
	{
		// OK, see if a footprint of the same name is already in the cache
		void * ptr;
		BOOL found = m_Doc->m_footprint_cache_map.Lookup( fp->m_name, ptr );
		if( found )
		{
			// see how many parts are using it, not counting the current one
			CShape * old_fp = (CShape*)ptr;
			int num = m_Doc->m_plist->GetNumFootprintInstances( old_fp );
			if( m_sel_part->shape == old_fp )
				num--;
			if( num <= 0 )
			{
				// go ahead and replace it
				m_Doc->m_plist->UndrawPart( m_sel_part );
				old_fp->Copy( fp );
				m_Doc->m_plist->PartFootprintChanged( m_sel_part, old_fp );
				m_Doc->ResetUndoState();
			}
			else
			{
				// offer to overwrite or rename it
				CDlgDupFootprintName dlg;
				CString mess;
				mess.Format( "Warning: A footprint named \"%s\"\r\nis already in use by other parts.\r\n", fp->m_name );
				mess += "Loading this new footprint will overwrite the old one\r\nunless you change its name\r\n";
				dlg.Initialize( &mess, &m_Doc->m_footprint_cache_map );
				int ret = dlg.DoModal();
				if( ret == IDOK )
				{
					// clicked "OK"
					if( dlg.m_replace_all_flag )
					{
						// replace all instances of footprint
						old_fp->Copy( fp );
						m_Doc->m_plist->FootprintChanged( old_fp );
						m_Doc->ResetUndoState();
					}
					else
					{
						// assign new name to footprint and put in cache
						CShape * shape = new CShape;
						shape->Copy( fp );
						shape->m_name = *dlg.GetNewName();
						m_Doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
						m_Doc->m_plist->PartFootprintChanged( m_sel_part, shape );
						m_Doc->ResetUndoState();
					}
				}
			}
		}
		else
		{
			// footprint name not found in cache, add the new footprint
			CShape * shape = new CShape;
			shape->Copy( fp );
			m_Doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
			m_Doc->m_plist->PartFootprintChanged( m_sel_part, shape );
			m_Doc->ResetUndoState();
		}
		m_Doc->ProjectModified( TRUE );
		m_dlist->CancelHighLight();
		m_Doc->m_plist->SelectRefText( m_sel_part );
		m_Doc->m_plist->HighlightPart( m_sel_part );
		Invalidate( FALSE );
	}
}

// find a part in the layout, center window on it and select it
//
void CFreePcbView::OnViewFindpart()
{
	CDlgFindPart dlg;
	dlg.Initialize( m_Doc->m_plist );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString * ref_des = &dlg.sel_ref_des;
		cpart * part = m_Doc->m_plist->GetPart( *ref_des );
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
				AfxMessageBox( "Sorry, this part doesn't have a footprint" );
			}
		}
		else
		{
			AfxMessageBox( "Sorry, this part doesn't exist" );
		}
	}
}

void CFreePcbView::OnFootprintWizard()
{
	m_Doc->OnToolsFootprintwizard();
}

void CFreePcbView::OnFootprintEditor()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnCheckPartsAndNets()
{
	m_Doc->OnToolsCheckPartsAndNets();
}

void CFreePcbView::OnDrc()
{
	m_Doc->OnToolsDrc();
}

void CFreePcbView::OnClearDRC()
{
	m_Doc->OnToolsClearDrc();
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
		m_Doc->m_vis[il] = TRUE;
		m_dlist->SetLayerVisible( il, TRUE );
		// start dragging rectangle
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		m_dlist->CancelHighLight();
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
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = poly->GetX( m_sel_id.ii );
	m_from_pt.y = poly->GetY( m_sel_id.ii );
	poly->StartDraggingToMoveCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_SMCUTOUT_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnSmCornerSetPosition()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	int x = poly->GetX(m_sel_id.ii);
	int y = poly->GetY(m_sel_id.ii);
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
		poly->MoveCorner( m_sel_id.ii,
			dlg.GetX(), dlg.GetY() );
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnSmCornerDeleteCorner()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	if( poly->GetNumCorners() < 4 )
	{
		AfxMessageBox( "Solder mask cutout has too few corners" );
		return;
	}
	SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
	poly->DeleteCorner( m_sel_id.ii );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnSmCornerDeleteCutout()
{
	SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
	m_Doc->m_sm_cutout.RemoveAt( m_sel_id.i );
	id new_id = m_sel_id;
	for( int i=m_sel_id.i; i<m_Doc->m_sm_cutout.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[i];
		new_id.i = i;
		poly->SetId( &new_id );
	}
	m_Doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

// insert corner into solder mask cutout side and start dragging
void CFreePcbView::OnSmSideInsertCorner()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	poly->StartDraggingToInsertCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_SMCUTOUT_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );

}

// change hatch style for solder mask cutout
void CFreePcbView::OnSmSideHatchStyle()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	CDlgSetAreaHatch dlg;
	dlg.Init( poly->GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForSMCutouts( TRUE, m_Doc->m_undo_list );
		int hatch = dlg.GetHatch();
		poly->SetHatch( hatch );
		m_Doc->ProjectModified( TRUE );
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
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->side = 1 - m_sel_part->side;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// rotate part clockwise 90 degrees clockwise
//
void CFreePcbView::OnPartRotate()
{
	SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 90)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnPartRotateCCW()
{
	SaveUndoInfoForPartAndNets( m_sel_part,
		CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 270)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}


void CFreePcbView::OnNetSetWidth()
{
	SetWidth( 2 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
}

void CFreePcbView::OnNetSetClearance()
{
	SetClearance( 2 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightNet( m_sel_net );
}


void CFreePcbView::OnConnectSetWidth()
{
	SetWidth( 1 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
}

void CFreePcbView::OnConnectSetClearance()
{
	SetClearance( 1 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
}

void CFreePcbView::OnSegmentAddVertex()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	SetCursorMode( CUR_DRAG_VTX_INSERT );
	m_Doc->m_nlist->StartDraggingSegmentNewVertex(
		pDC,
		m_sel_net,
		m_sel_ic, m_sel_is,
		p.x, p.y,
		m_sel_net->connect[m_sel_ic].seg[m_sel_is].layer,
		m_sel_net->connect[m_sel_ic].seg[m_sel_is].width(),
		2
	);
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
	ChangeTraceLayer( 0, m_sel_seg.layer );
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
	CDlgChangeLayer dlg;
	dlg.Initialize( mode, old_layer, m_Doc->m_num_copper_layers );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int err = 0;
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE, m_Doc->m_undo_list );
		cconnect * c = &m_sel_con;
		if( dlg.m_apply_to == 0 )
		{
			err = m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
						m_sel_id.i, m_sel_id.ii, dlg.m_new_layer );
			if( err )
			{
				AfxMessageBox( "Unable to change layer for this segment" );
			}
		}
		else if( dlg.m_apply_to == 1 )
		{
			for( int is=0; is<c->nsegs; is++ )
			{
				if( c->seg[is].layer >= LAY_TOP_COPPER )
				{
					err += m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
						m_sel_id.i, is, dlg.m_new_layer );
				}
			}
			if( err )
			{
				AfxMessageBox( "Unable to change layer for all segments" );
			}
		}
		else if( dlg.m_apply_to == 2 )
		{
			for( int ic=0; ic<m_sel_net->nconnects; ic++ )
			{
				cconnect * c = &m_sel_net->connect[ic];
				for( int is=0; is<c->nsegs; is++ )
				{
					if( c->seg[is].layer >= LAY_TOP_COPPER )
					{
						err += m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net,
							ic, is, dlg.m_new_layer );
					}
				}
			}
			if( err )
			{
				AfxMessageBox( "Unable to change layer for all segments" );
			}
		}
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnNetEditnet()
{
	CDlgEditNet dlg;
	netlist_info nl;
	m_Doc->m_nlist->ExportNetListInfo( &nl );
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
	dlg.Initialize( m_Doc->m_nlist, &nl, inet, m_Doc->m_plist, FALSE, TRUE, m_Doc->m_units,
		&m_Doc->m_w, &m_Doc->m_v_w, &m_Doc->m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_Doc->ResetUndoState();
		CancelSelection();
		m_Doc->m_nlist->ImportNetListInfo( &nl, 0, NULL, m_Doc->m_def_width );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnToolsMoveOrigin()
{
	CDlgMoveOrigin dlg;
	dlg.Initialize( m_Doc->m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( dlg.m_drag )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_MOVE_ORIGIN );
			m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
				m_last_cursor_point.y, 0, LAY_SELECTION, 2 );
			Invalidate( FALSE );
			ReleaseDC( pDC );
		}
		else
		{
			SaveUndoInfoForMoveOrigin( -dlg.m_x, -dlg.m_x, m_Doc->m_undo_list );
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
	for( int ib=0; ib<m_Doc->m_board_outline.GetSize(); ib++ )
		m_Doc->m_board_outline[ib].MoveOrigin( x_off, y_off );
	m_Doc->m_plist->MoveOrigin( x_off, y_off );
	m_Doc->m_nlist->MoveOrigin( x_off, y_off );
	m_Doc->m_tlist->MoveOrigin( x_off, y_off );
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
		m_Doc->m_sm_cutout[ism].MoveOrigin( x_off, y_off );
}

void CFreePcbView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// save starting position in pixels
	m_bLButtonDown = TRUE;
	m_bDraggingRect = FALSE;
	m_start_pt = point;
	CView::OnLButtonDown(nFlags, point);
}

// Select all parts and trace segments in rectangle
// Fill arrays m_sel_ids[] and m_sel_ptrs[]
// Set utility flags for selected parts and segments
//
void CFreePcbView::SelectItemsInRect( CRect r, BOOL bAddToGroup )
{
	if( !bAddToGroup )
		CancelSelection();
	r.NormalizeRect();

	// find parts in rect
	if( m_sel_mask & (1<<SEL_MASK_PARTS ) )
	{
		cpart * part = m_Doc->m_plist->GetFirstPart();
		while( part )
		{
			CRect p_r;
			if( m_Doc->m_plist->GetPartBoundingRect( part, &p_r ) )
			{
				p_r.NormalizeRect();
				if( InRange( p_r.top, r.top, r.bottom )
					&& InRange( p_r.bottom, r.top, r.bottom )
					&& InRange( p_r.left, r.left, r.right )
					&& InRange( p_r.right, r.left, r.right ) )
				{
					// add part to selection list and highlight it
					id pid( ID_PART, ID_SEL_RECT, 0, 0, 0 );
					if( FindItemInGroup( part, &pid ) == -1 )
					{
						m_sel_ids.Add( pid );
						m_sel_ptrs.Add( part );
					}
				}
			}
			part = m_Doc->m_plist->GetNextPart( part );
		}
	}

	// find trace segments and vertices contained in rect
	if( m_sel_mask & (1<<SEL_MASK_CON ) )
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				for( int is=0; is<c->nsegs; is++ )
				{
					cvertex * pre_v = &c->vtx[is];
					cvertex * post_v = &c->vtx[is+1];
					cseg * s = &c->seg[is];
					BOOL bPreV = InRange( pre_v->x, r.left, r.right )
						&& InRange( pre_v->y, r.top, r.bottom );
					BOOL bPostV = InRange( post_v->x, r.left, r.right )
						&& InRange( post_v->y, r.top, r.bottom );
//					if( bPreV && bPostV && s->layer >= LAY_TOP_COPPER
					if( bPreV && bPostV
						&& m_Doc->m_vis[s->layer] )
					{
						// add segment to selection list and highlight it
						id sid( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, is );
						if( FindItemInGroup( net, &sid ) == -1 )
						{
							m_sel_ids.Add( sid );
							m_sel_ptrs.Add( net );
						}
					}
					if( bPreV && ( pre_v->tee_ID || pre_v->via_w() ) )
					{
						id vid( ID_NET, ID_CONNECT, ic, ID_SEL_VERTEX, is );
						if( FindItemInGroup( net, &vid ) == -1 )
						{
							m_sel_ids.Add( vid );
							m_sel_ptrs.Add( net );
						}
					}
					if( bPostV && ( post_v->tee_ID || post_v->via_w() ) )
					{
						id vid( ID_NET, ID_CONNECT, ic, ID_SEL_VERTEX, is+1 );
						if( FindItemInGroup( net, &vid ) == -1 )
						{
							m_sel_ids.Add( vid );
							m_sel_ptrs.Add( net );
						}
					}
				}
			}
			net = m_Doc->m_nlist->GetNextNet();
		}
	}

	// find texts in rect
	if( m_sel_mask & (1<<SEL_MASK_TEXT ) )
	{
		CText * t = m_Doc->m_tlist->GetFirstText();
		while( t )
		{
			if( InRange( m_dlist->Get_x( t->dl_sel ), r.left, r.right )
				&& InRange( m_dlist->Get_xf( t->dl_sel ), r.left, r.right )
				&& InRange( m_dlist->Get_y( t->dl_sel ), r.top, r.bottom )
				&& InRange( m_dlist->Get_yf( t->dl_sel ), r.top, r.bottom )
				&& m_Doc->m_vis[t->m_layer] )
			{
				// add text to selection list and highlight it
				id sid( ID_TEXT, ID_SEL_TXT, 0, 0, 0 );
				if( FindItemInGroup( t, &sid ) == -1 )
				{
					m_sel_ids.Add( sid );
					m_sel_ptrs.Add( t );
				}
			}
			t = m_Doc->m_tlist->GetNextText();
		}
	}

	// find copper area sides in rect
	if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			if( net->nareas )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * poly = a->poly;
					for( int ic=0; ic<poly->GetNumContours(); ic++ )
					{
						int istart = poly->GetContourStart(ic);
						int iend = poly->GetContourEnd(ic);
						for( int is=istart; is<=iend; is++ )
						{
							int ic1, ic2;
							ic1 = is;
							if( is < iend )
								ic2 = is+1;
							else
								ic2 = istart;
							int x1 = poly->GetX(ic1);
							int y1 = poly->GetY(ic1);
							int x2 = poly->GetX(ic2);
							int y2 = poly->GetY(ic2);
							if( InRange( x1, r.left, r.right )
								&& InRange( x2, r.left, r.right )
								&& InRange( y1, r.top, r.bottom )
								&& InRange( y2, r.top, r.bottom )
								&& m_Doc->m_vis[poly->GetLayer()] )
							{
								id aid( ID_NET, ID_AREA, ia, ID_SEL_SIDE, is );
								if( FindItemInGroup( net, &aid ) == -1 )
								{
									m_sel_ids.Add( aid );
									m_sel_ptrs.Add( net );
								}
							}
						}
					}
				}
			}
			net = m_Doc->m_nlist->GetNextNet();
		}
	}

	// find solder mask cutout sides in rect
	if( m_sel_mask & (1<<SEL_MASK_SM ) )
	{
		for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[im];
			for( int ic=0; ic<poly->GetNumContours(); ic++ )
			{
				int istart = poly->GetContourStart(ic);
				int iend = poly->GetContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->GetX(ic1);
					int y1 = poly->GetY(ic1);
					int x2 = poly->GetX(ic2);
					int y2 = poly->GetY(ic2);
					if( InRange( x1, r.left, r.right )
						&& InRange( x2, r.left, r.right )
						&& InRange( y1, r.top, r.bottom )
						&& InRange( y2, r.top, r.bottom )
						&& m_Doc->m_vis[poly->GetLayer()] )
					{
						id smid( ID_SM_CUTOUT, ID_SM_CUTOUT, im, ID_SEL_SIDE, is );
						if( FindItemInGroup( poly, &smid ) == -1 )
						{
							m_sel_ids.Add( smid );
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
		for( int im=0; im<m_Doc->m_board_outline.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[im];
			for( int ic=0; ic<poly->GetNumContours(); ic++ )
			{
				int istart = poly->GetContourStart(ic);
				int iend = poly->GetContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->GetX(ic1);
					int y1 = poly->GetY(ic1);
					int x2 = poly->GetX(ic2);
					int y2 = poly->GetY(ic2);
					if( InRange( x1, r.left, r.right )
						&& InRange( x2, r.left, r.right )
						&& InRange( y1, r.top, r.bottom )
						&& InRange( y2, r.top, r.bottom )
						&& m_Doc->m_vis[poly->GetLayer()] )
					{
						id bd_id( ID_BOARD, ID_BOARD, im, ID_SEL_SIDE, is );
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

	// now highlight selected items
	if( m_sel_ids.GetSize() == 0 )
		CancelSelection();
	else
	{
		HighlightGroup();
		SetCursorMode( CUR_GROUP_SELECTED );
	}
	gLastKeyWasArrow = FALSE;
	gLastKeyWasGroupRotate=false;
	FindGroupCenter();
}

// Start dragging group being moved or added
// If group is being added (i.e. pasted):
//	bAdd = TRUE;
//	x, y = coords for cursor point for dragging
//
void CFreePcbView::StartDraggingGroup( BOOL bAdd, int x, int y )
{
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
		if( sid.type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			m_Doc->m_plist->MakePartVisible( part, FALSE );
			n_parts++;
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			dl_element * dl = net->connect[sid.i].seg[sid.ii].dl_el;
			m_dlist->Set_visible( dl, FALSE );
			m_Doc->m_nlist->SetViaVisible( net, sid.i, sid.ii, FALSE );
			m_Doc->m_nlist->SetViaVisible( net, sid.i, sid.ii+1, FALSE );
			n_segs++;
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA
			&& sid.sst == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.i];
//			a->poly->SetSideVisible( sid.ii, FALSE );
			a->poly->MakeVisible( FALSE );
			n_area_sides++;
		}
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
//			poly->SetSideVisible( sid.ii, FALSE );
			poly->MakeVisible( FALSE );
			n_sm_sides++;
		}
		else if( sid.type == ID_BOARD && sid.st == ID_BOARD
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.i];
//			poly->SetSideVisible( sid.ii, FALSE );
			poly->MakeVisible( FALSE );
			n_sm_sides++;
		}
		else if( sid.type == ID_TEXT )
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
		if( sid.type == ID_PART )
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
		else if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			cconnect * c = &net->connect[sid.i];
			cseg * s = &c->seg[sid.ii];
			cvertex * v1 = &c->vtx[sid.ii];
			cvertex * v2 = &c->vtx[sid.ii+1];
			CPoint p1( v1->x - m_from_pt.x, v1->y - m_from_pt.y );
			CPoint p2( v2->x - m_from_pt.x, v2->y - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA
			&& sid.sst == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.i];
			CPolyLine * poly = a->poly;
			int icontour = poly->GetContour(sid.ii);
			int ic1 = sid.ii;
			int ic2 = sid.ii+1;
			if( ic2 > poly->GetContourEnd(icontour) )
				ic2 = poly->GetContourStart(icontour);
			CPoint p1( poly->GetX(ic1) - m_from_pt.x, poly->GetY(ic1) - m_from_pt.y );
			CPoint p2( poly->GetX(ic2) - m_from_pt.x, poly->GetY(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
			int icontour = poly->GetContour(sid.ii);
			int ic1 = sid.ii;
			int ic2 = sid.ii+1;
			if( ic2 > poly->GetContourEnd(icontour) )
				ic2 = poly->GetContourStart(icontour);
			CPoint p1( poly->GetX(ic1) - m_from_pt.x, poly->GetY(ic1) - m_from_pt.y );
			CPoint p2( poly->GetX(ic2) - m_from_pt.x, poly->GetY(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.type == ID_BOARD && sid.st == ID_BOARD
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.i];
			int icontour = poly->GetContour(sid.ii);
			int ic1 = sid.ii;
			int ic2 = sid.ii+1;
			if( ic2 > poly->GetContourEnd(icontour) )
				ic2 = poly->GetContourStart(icontour);
			CPoint p1( poly->GetX(ic1) - m_from_pt.x, poly->GetY(ic1) - m_from_pt.y );
			CPoint p2( poly->GetX(ic2) - m_from_pt.x, poly->GetY(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 );
		}
		else if( sid.type == ID_TEXT )
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
}

void CFreePcbView::CancelDraggingGroup()
{
	m_dlist->StopDragging();
	// make elements visible again
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			m_Doc->m_plist->MakePartVisible( part, TRUE );
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			dl_element * dl = net->connect[sid.i].seg[sid.ii].dl_el;
			m_dlist->Set_visible( dl, TRUE );
			m_Doc->m_nlist->SetViaVisible( net, sid.i, sid.ii, TRUE );
			m_Doc->m_nlist->SetViaVisible( net, sid.i, sid.ii+1, TRUE );
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA
			&& sid.sst == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.i];
//			a->poly->SetSideVisible( sid.ii, TRUE );
			a->poly->MakeVisible( TRUE );
		}
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
//			poly->SetSideVisible( sid.ii, TRUE );
			poly->MakeVisible( TRUE );
		}
		else if( sid.type == ID_BOARD && sid.st == ID_BOARD
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.i];
//			poly->SetSideVisible( sid.ii, TRUE );
			poly->MakeVisible( TRUE );
		}
		else if( sid.type == ID_TEXT )
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
}

void CFreePcbView::OnGroupMove()
{
	if( GluedPartsInGroup() )
	{
		int ret = AfxMessageBox( "This group contains glued parts, unglue and move them ?", MB_OKCANCEL );
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
	UngluePartsInGroup();

	// mark all parts and nets as unselected
	m_Doc->m_nlist->MarkAllNets(0);
	m_Doc->m_plist->MarkAllParts(0);

	// mark connections, segments and vertices of selected nets as unselected
	// and vertices and area corners as unmoved
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			if( net->utility == FALSE )
			{
				// first time for this net,
				net->utility = TRUE;	// mark as selected
				// mark all connections, segments and vertices as unselected
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					net->connect[ic].utility = FALSE;
					for( int is=0; is<net->connect[ic].nsegs; is++ )
					{
						net->connect[ic].seg[is].utility = FALSE;
						net->connect[ic].vtx[is].utility = FALSE;
						net->connect[ic].vtx[is].utility2 = FALSE;
						net->connect[ic].vtx[is+1].utility = FALSE;
						net->connect[ic].vtx[is+1].utility2 = FALSE;
					}
				}
				// mark all area corners as unmoved
				for( int ia=0; ia<net->nareas; ia++ )
				{
					for( int is=0; is<net->area[ia].poly->GetNumCorners(); is++ )
					{
						net->area[ia].poly->SetUtility( is, 0 );	// unmoved
					}
				}
			}
		}
	}
	// mark all corners of solder mask cutouts as unmoved
	for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[im];
		for( int ic=0; ic<poly->GetNumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}
	// mark all corners of board outlines as unmoved
	for( int im=0; im<m_Doc->m_board_outline.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[im];
		for( int ic=0; ic<poly->GetNumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}

	// mark all relevant parts, nets, connections and segments as selected
	// and move text and copper area corners
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_SEG )
		{
			// segment
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.i;
			int is = sid.ii;
			cconnect * c = &net->connect[ic];	// this connection
			cseg * s = &c->seg[is];				// this segment
			cvertex * pre_v = &c->vtx[is];
			cvertex * post_v = &c->vtx[is+1];
			c->utility = TRUE;					// mark connection selected
			s->utility = TRUE;					// mark segment selected
			pre_v->utility =  TRUE;				// mark adjacent vertices as selected
			post_v->utility =  TRUE;
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_VERTEX )
		{
			// vertex
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.i;
			int iv = sid.ii;
			cconnect * c = &net->connect[ic];	// this connection
			cvertex * v = &c->vtx[iv];
			c->utility = TRUE;					// mark connection selected
			v->utility = TRUE;					// mark vertex selected
		}
		else if( sid.type == ID_PART && sid.st == ID_SEL_RECT )
		{
			// part
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->utility = TRUE;	// mark part selected
		}
		else if( sid.type == ID_TEXT && sid.st == ID_SEL_TXT )
		{
			// text
			CText * t = (CText*)m_sel_ptrs[i];
			m_Doc->m_tlist->MoveText( t, t->m_x+dx, t->m_y+dy, t->m_angle,
				t->m_mirror, t->m_bNegative, t->m_layer );
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA && sid.sst == ID_SEL_SIDE )
		{
			// area side
			cnet * net = (cnet*)m_sel_ptrs[i];
			CPolyLine * poly = net->area[sid.i].poly;
			int icontour = poly->GetContour(sid.ii);
			int istart = poly->GetContourStart(icontour);
			int iend = poly->GetContourEnd(icontour);
			int ic1 = sid.ii;
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			poly->Undraw();
			if( !poly->GetUtility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->GetX(ic1) + dx );
				poly->SetY( ic1, poly->GetY(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->GetUtility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->GetX(ic2) + dx );
				poly->SetY( ic2, poly->GetY(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
			poly->Draw();
		}
		// sm_cutout side
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT && sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
			int icontour = poly->GetContour(0);
			int istart = poly->GetContourStart(icontour);
			int iend = poly->GetContourEnd(icontour);
			int ic1 = sid.ii;
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			poly->Undraw();
			if( !poly->GetUtility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->GetX(ic1) + dx );
				poly->SetY( ic1, poly->GetY(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->GetUtility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->GetX(ic2) + dx );
				poly->SetY( ic2, poly->GetY(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
			poly->Draw();
		}
		// board outline side
		else if( sid.type == ID_BOARD && sid.st == ID_BOARD && sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.i];
			int icontour = poly->GetContour(0);
			int istart = poly->GetContourStart(icontour);
			int iend = poly->GetContourEnd(icontour);
			int ic1 = sid.ii;
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			poly->Undraw();
			if( !poly->GetUtility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->GetX(ic1) + dx );
				poly->SetY( ic1, poly->GetY(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->GetUtility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->GetX(ic2) + dx );
				poly->SetY( ic2, poly->GetY(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
			poly->Draw();
		}
		else
			ASSERT(0);
	}

	// assume utility flags have been set on selected parts,
	// nets, connections, segments and vertices

	// move parts in group
	cpart * part = m_Doc->m_plist->GetFirstPart();
	while( part != NULL )
	{
		if( part->utility )
		{
			// move part
			m_Doc->m_plist->Move( part, part->x+dx, part->y+dy, part->angle, part->side );
			// find segments which connect to this part and move them
			cnet * net;
			for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
			{
				net = (cnet*)part->pin[ip].net;
				if( net )
				{
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						cconnect * c = &net->connect[ic];
						int nsegs = c->nsegs;
						if( nsegs )
						{
							int p1 = c->start_pin;
							CString pin_name1 = net->pin[p1].pin_name;
							int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
							int p2 = c->end_pin;
							if( net->pin[p1].part == part )
							{
								// starting pin is on part
								if( !c->seg[0].utility && c->seg[0].layer != LAY_RAT_LINE )
								{
									// first segment is not selected, unroute it
									if( !c->seg[0].utility )
										m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, 0 );
								}
								// move vertex if not selected
								if( !c->vtx[0].utility )
								{
									m_Doc->m_nlist->MoveVertex( net, ic, 0,
										part->pin[pin_index1].x, part->pin[pin_index1].y );
									c->vtx[0].utility2 = 1; // moved
								}
							}
							if( p2 != cconnect::NO_END )
							{
								if( net->pin[p2].part == part )
								{
									// ending pin is on part
									if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
									{
										// unroute it if not selected
										if( !c->seg[nsegs-1].utility )
											m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, nsegs-1 );
									}
									// modify vertex position if necessary
									if( !c->vtx[nsegs].utility )
									{
										// move vertex if unselected
										CString pin_name2 = net->pin[p2].pin_name;
										int pin_index2 = part->shape->GetPinIndexByName( pin_name2 );
										m_Doc->m_nlist->MoveVertex( net, ic, nsegs,
											part->pin[pin_index2].x, part->pin[pin_index2].y );
										c->vtx[nsegs].utility2 =  1;	// moved

									}
								}
							}
						}
					}
				}
			}
		}
		part = m_Doc->m_plist->GetNextPart( part );
	}

	// get selected segments
	cnet * net = m_Doc->m_nlist->GetFirstNet();
	while( net != NULL )
	{
		if( net->utility )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->utility )
				{
					// undraw entire trace
					m_Doc->m_nlist->UndrawConnection( net, ic );
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].utility )
						{
							// move trace segment by flagging adjacent vertices
							cseg * s = &c->seg[is];				// this segment
							cvertex * pre_v = &c->vtx[is];		// pre vertex
							cvertex * post_v = &c->vtx[is+1];	// post vertex
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
								if( !c->seg[is-1].utility )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is-1 );
							}
							if( is < c->nsegs-1 )
							{
								// test for following segment and not end of stub trace
								if( !c->seg[is+1].utility && (part2 || is < c->nsegs-2) )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is+1 );
							}
						}
					}
					// now move vertices
					for( int iv=0; iv<=c->nsegs; iv++ )
					{
						cvertex * v = &c->vtx[iv];
						if( v->utility && !v->utility2 )
						{
							// selected and not already moved
							v->x += dx;
							v->y += dy;
							v->utility2 = TRUE;	// moved
							// if adjacent segments were not selected, unroute them
							if( iv>0 )
							{
								cseg * pre_s = &c->seg[iv-1];
								if( pre_s->utility == 0 )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv-1 );
							}
							if( iv<c->nsegs )
							{
								cseg * post_s = &c->seg[iv];
								if( post_s->utility == 0 )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv );
							}
						}
					}

					// now some special cases
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].utility )
						{
							// move trace segment
							cseg * s = &c->seg[is];				// this segment
							cvertex * pre_v = &c->vtx[is];		// pre vertex
							cvertex * post_v = &c->vtx[is+1];	// post vertex
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// special case, first segment of trace selected but part not selected
							if( part1->utility == FALSE && is == 0 )
							{
								// insert ratline as new first segment, unselected
								CPoint new_v_pt( pre_v->x, pre_v->y );
								CPoint old_v_pt = m_Doc->m_plist->GetPinPoint( part1, net->pin[c->start_pin].pin_name );		// pre vertex coords
								m_Doc->m_nlist->MoveVertex( net, ic, 0, old_v_pt.x, old_v_pt.y );
								m_Doc->m_nlist->InsertSegment( net, ic, 0, new_v_pt.x, new_v_pt.y, LAY_RAT_LINE, CConnectionWidthInfo(1, 0, 0), 0 );
								c->seg[0].utility = 0;
								c->vtx[0].utility = 0;
								is++;
							}

							// special case, last segment of trace selected but part not selected
							if( part2 )
							{
								if( part2->utility == FALSE && is == c->nsegs-1 )
								{
									// insert ratline as new last segment
									CConnectionWidthInfo old_width( c->seg[c->nsegs-1].seg_width );
									old_width.m_via_width = c->vtx[c->nsegs-1].via_width.m_via_width;
									old_width.m_via_hole  = c->vtx[c->nsegs-1].via_width.m_via_hole;

									CClearanceInfo old_clearance( c->seg[c->nsegs-1].seg_clearance );
									int old_layer = c->seg[c->nsegs-1].layer;

									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->nsegs-1 );
									CPoint new_v_pt( c->vtx[c->nsegs].x, c->vtx[c->nsegs].y );
									CPoint old_v_pt = m_Doc->m_plist->GetPinPoint( part2, net->pin[c->end_pin].pin_name );
									m_Doc->m_nlist->MoveVertex( net, ic, c->nsegs, old_v_pt.x, old_v_pt.y );

									BOOL bInserted = m_Doc->m_nlist->InsertSegment( net, ic, c->nsegs-1,
										new_v_pt.x, new_v_pt.y, old_layer, old_width, old_clearance, 0 );

									c->seg[c->nsegs-2].utility = 1;
									c->seg[c->nsegs-1].utility = 0;
								}
							}
						}
					}
					m_Doc->m_nlist->MergeUnroutedSegments( net, ic );	// this also redraws connection
				}
			}

			// now deal with tees that have been moved
			// requiring that stubs attached to tees have to move as well
			// if attached segments have not been selected, they must be unrouted
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->end_pin == cconnect::NO_END )
				{
					cvertex * end_vtx = &c->vtx[c->nsegs];
					cseg * end_seg = &c->seg[c->nsegs-1];
					if( int id = end_vtx->tee_ID )
					{
						// stub tee
						int tee_ic;
						int tee_iv;
						BOOL bFound = m_Doc->m_nlist->FindTeeVertexInNet( net, id, &tee_ic, &tee_iv );
						if ( !bFound )
						{
							end_vtx->tee_ID = 0;
						}
						else
						{
							cvertex * tee_vtx;
							tee_vtx = &net->connect[tee_ic].vtx[tee_iv];
							if( tee_vtx->utility2 )
							{
								// tee-vertex was moved
								end_vtx->x = tee_vtx->x;
								end_vtx->y = tee_vtx->y;
								if( !end_seg->utility )
								{
									// attached segment not selected
									m_Doc->m_nlist->UndrawConnection( net, ic );
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->nsegs-1 );
									m_Doc->m_nlist->DrawConnection( net, ic );
								}
							}
						}
					}
				}
			}
		}
		net = m_Doc->m_nlist->GetNextNet();
	}

	// merge unrouted segments for all traces
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.i;
			m_Doc->m_nlist->MergeUnroutedSegments( net, ic );
		}
	}

	//** this shouldn't be necessary
	CNetList * nl = m_Doc->m_nlist;
	for( cnet * n=nl->GetFirstNet(); n; n=nl->GetNextNet() )
		nl->RehookPartsToNet( n );
	//**
	m_Doc->m_nlist->SetAreaConnections();

	// regenerate selection list from utility flags
	// first, remove all segments and vertices
	for( int i=m_sel_ids.GetSize()-1; i>=0; i-- )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_VERTEX )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
	}
	// add segments and vertices back into group
	net = m_Doc->m_nlist->GetFirstNet();
	while( net )
	{
		if( net->utility )
		{
			// selected net
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->utility )
				{
					// selected connection
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].utility )
						{
							m_sel_ptrs.Add( net );
							id sid( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, is );
							m_sel_ids.Add( sid );
							c->seg[is].dl_el->visible = 1;	// restore visibility
						}
					}
					for( int iv=0; iv<c->nsegs+1; iv++ )
					{
						if( c->vtx[iv].utility )
						{
							if( c->vtx[iv].viaExists() || c->vtx[iv].tee_ID )
							{
								m_sel_ptrs.Add( net );
								id vid( ID_NET, ID_CONNECT, ic, ID_SEL_VERTEX, iv );
								m_sel_ids.Add( vid );
								if( c->vtx[iv].viaExists() )
								{
									int n_el = c->vtx[iv].dl_el.GetSize();
									for( int il=0; il<n_el; il++ )
										c->vtx[iv].dl_el[il]->visible = 1;
								}
							}
						}
					}
				}
			}
		}
		net = m_Doc->m_nlist->GetNextNet();
	}

	groupAverageX+=dx;
	groupAverageY+=dy;
}

// Highlight group selection
// the only legal members are parts, texts, trace segments and
// copper area, solder mask cutout and board outline sides
//
void CFreePcbView::HighlightGroup()
{
	m_dlist->CancelHighLight();
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART && sid.st == ID_SEL_RECT )
			m_Doc->m_plist->HighlightPart( (cpart*)m_sel_ptrs[i] );
		else if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG )
			m_Doc->m_nlist->HighlightSegment( (cnet*)m_sel_ptrs[i], sid.i, sid.ii );
		else if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_VERTEX )
		{
			cvertex * v = &((cnet*)m_sel_ptrs[i])->connect[sid.i].vtx[sid.ii];
			if( v->tee_ID || v->force_via_flag )
				m_Doc->m_nlist->HighlightVertex( (cnet*)m_sel_ptrs[i], sid.i, sid.ii );
		}
		else if( sid.type == ID_TEXT && sid.st == ID_SEL_TXT )
			m_Doc->m_tlist->HighlightText( (CText*)m_sel_ptrs[i] );
		else if( sid.type == ID_NET && sid.st == ID_AREA && sid.sst == ID_SEL_SIDE )
			((cnet*)m_sel_ptrs[i])->area[sid.i].poly->HighlightSide(sid.ii);
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT && sid.sst == ID_SEL_SIDE )
			m_Doc->m_sm_cutout[sid.i].HighlightSide(sid.ii);
		else if( sid.type == ID_BOARD && sid.st == ID_BOARD && sid.sst == ID_SEL_SIDE )
			m_Doc->m_board_outline[sid.i].HighlightSide(sid.ii);
		else
			ASSERT(0);
	}
}

// Find item in group by id
// returns index of item if found, otherwise -1
//
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
		if( m_sel_ids[i].type == ID_PART )
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
		if( m_sel_ids[i].type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->glued = FALSE;
		}
	}
}

// Set array of selection mask ids
//
void CFreePcbView::SetSelMaskArray( int mask )
{
	for( int i=0; i<NUM_SEL_MASKS; i++ )
	{
		if( mask & (1<<i) )
			m_mask_id[i].ii = 0;
		else
			m_mask_id[i].ii = 0xfffe;	// guaranteed not to exist
	}
}


void CFreePcbView::OnAddSimilarArea()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	SetCursorMode( CUR_ADD_AREA );
	m_active_layer = m_sel_net->area[m_sel_ia].poly->GetLayer();
	m_Doc->m_vis[m_active_layer] = TRUE;
	m_dlist->SetLayerVisible( m_active_layer, TRUE );
	ShowActiveLayer();
	m_dlist->StartDraggingArray( pDC, m_last_cursor_point.x,
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPolyLine::STRAIGHT;
	m_polyline_hatch = m_sel_net->area[m_sel_ia].poly->GetHatch();
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnAreaEdit()
{
	CDlgAddArea dlg;
	int layer = m_sel_net->area[m_sel_id.i].poly->GetLayer();
	int hatch = m_sel_net->area[m_sel_id.i].poly->GetHatch();
	dlg.Initialize( m_Doc->m_nlist, m_Doc->m_num_layers, m_sel_net, layer, hatch );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		cnet * net = dlg.m_net;
		if( m_sel_net == net )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net, TRUE, m_Doc->m_undo_list );
		}
		else
		{
			// move area to new net
			SaveUndoInfoForAllAreasIn2Nets( m_sel_net, net, TRUE, m_Doc->m_undo_list );
			int ia = m_Doc->m_nlist->AddArea( net, dlg.m_layer, 0, 0, 0 );
			net->area[ia].poly->Copy( m_sel_net->area[m_sel_ia].poly );
			net->area[ia].poly->SetPtr( net );
			id new_id = net->area[ia].poly->GetId();
			new_id.i = ia;
			net->area[ia].poly->SetId( &new_id );
			m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
			m_Doc->m_nlist->OptimizeConnections( m_sel_net );
			m_Doc->m_nlist->SetAreaConnections( net, ia );
			m_Doc->m_nlist->OptimizeConnections( net );
			CancelSelection();
			m_sel_net = net;
			m_sel_ia = ia;
		}
		m_sel_net->area[m_sel_ia].poly->Undraw();
		m_sel_net->area[m_sel_ia].poly->SetLayer( dlg.m_layer );
		m_sel_net->area[m_sel_ia].poly->SetHatch( dlg.m_hatch );
		m_sel_net->area[m_sel_ia].poly->Draw();
		int ret = m_Doc->m_nlist->AreaPolygonModified( m_sel_net, m_sel_ia, FALSE, TRUE );
		if( ret == -1 )
		{
			// error
			AfxMessageBox( "Error: Unable to clip polygon due to intersecting arc" );
			CancelSelection();
			m_Doc->OnEditUndo();
		}
		m_Doc->m_nlist->OptimizeConnections( m_sel_net );
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}


void CFreePcbView::ReselectNetItemIfConnectionsChanged( int new_ic )
{
	if( new_ic >= 0 && new_ic < m_sel_net->nconnects
		&& (m_cursor_mode == CUR_SEG_SELECTED
		|| m_cursor_mode == CUR_RAT_SELECTED
		|| m_cursor_mode == CUR_VTX_SELECTED
		|| m_cursor_mode == CUR_END_VTX_SELECTED
		|| m_cursor_mode == CUR_CONNECT_SELECTED
		|| m_cursor_mode == CUR_NET_SELECTED ) )
	{
		m_Doc->m_dlist->CancelHighLight();
		m_sel_ic = new_ic;
		if( m_cursor_mode == CUR_SEG_SELECTED )
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		else if( m_cursor_mode == CUR_RAT_SELECTED )
			m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		else if( m_cursor_mode == CUR_VTX_SELECTED )
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
		else if( m_cursor_mode == CUR_END_VTX_SELECTED )
			m_Doc->m_nlist->HighlightVertex( m_sel_net, m_sel_ic, m_sel_iv );
		else if( m_cursor_mode == CUR_CONNECT_SELECTED )
			m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
		else if( m_cursor_mode == CUR_NET_SELECTED )
			m_Doc->m_nlist->HighlightNetConnections( m_sel_net );
	}
	else
		CancelSelection();
}

void CFreePcbView::OnGroupCopy()
{
	// clear clipboard
	m_Doc->clip_sm_cutout.SetSize(0);
	m_Doc->clip_board_outline.SetSize(0);
	m_Doc->clip_tlist->RemoveAllTexts();
	m_Doc->clip_nlist->RemoveAllNets();
	m_Doc->clip_plist->RemoveAllParts();

	// set pointers
	CArray<CPolyLine> * g_sm = &m_Doc->clip_sm_cutout;
	CArray<CPolyLine> * g_bd = &m_Doc->clip_board_outline;
	CPartList * g_pl = m_Doc->clip_plist;
	CNetList * g_nl = m_Doc->clip_nlist;
	CTextList * g_tl = m_Doc->clip_tlist;

	// add all parts and text from group
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART && sid.st == ID_SEL_RECT )
		{
			// add part to group partlist
			cpart * part = (cpart*)m_sel_ptrs[i];
			CShape * shape = part->shape;
			cpart * g_part = g_pl->Add( part->shape, &part->ref_des, &part->package, part->x, part->y,
				part->side, part->angle, 1, 0 );
			// set ref text parameters
			g_part->m_ref_angle = part->m_ref_angle;
			g_part->m_ref_size = part->m_ref_size;
			g_part->m_ref_w = part->m_ref_w;
			g_part->m_ref_xi = part->m_ref_xi;
			g_part->m_ref_yi = part->m_ref_yi;
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
						g_net = g_nl->AddNet( net->name, net->npins, net->def_width, net->def_clearance );
					}
					// add pin to net
					CString pin_name = shape->GetPinNameByIndex( ip );
					g_nl->AddNetPin( g_net, &part->ref_des, &pin_name, FALSE );
				}
			}
		}
		else if( sid.type == ID_TEXT && sid.st == ID_SEL_TXT )
		{
			// add text string to group textlist
			CText * t = (CText*)m_sel_ptrs[i];
			g_tl->AddText( t->m_x, t->m_y, t->m_angle, t->m_mirror,  t->m_bNegative,
				t->m_layer, t->m_font_size, t->m_stroke_width, &t->m_str, FALSE );
		}
	}

	// mark all connections and areas as unchecked
	cnet * net = m_Doc->m_nlist->GetFirstNet();
	while( net )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
			net->connect[ic].utility = FALSE;
		for( int ia=0; ia<net->nareas; ia++ )
			net->area[ia].utility = FALSE;
		net = m_Doc->m_nlist->GetNextNet();
	}

	// check all selected areas and connections
	g_nl->ClearTeeIDs();
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT )
		{
			// connection, only add if between parts in group
			cnet * net = (cnet*)m_sel_ptrs[i];
			cconnect * c = &net->connect[sid.i];
			if( c->utility == FALSE )
			{
				cnet * g_net = g_nl->GetNetPtrByName( &net->name );
				if( g_net == NULL )
				{
					g_net = g_nl->AddNet( net->name, net->npins, net->def_width, net->def_clearance );
				}
				// test start and end pins
				BOOL bStartPinInGroup = FALSE;
				BOOL bEndPinInGroup = FALSE;
				BOOL bStubTrace = FALSE;
				if( c->end_pin == cconnect::NO_END )
					bStubTrace = TRUE;
				cpin * pin1 = &net->pin[c->start_pin];
				cpart * part1 = pin1->part;
				cpin * pin2 = NULL;
				cpart * part2 = NULL;
				if( !bStubTrace )
				{
					pin2 = &net->pin[c->end_pin];
					part2 = pin2->part;
				}
				// loop through all group parts
				cpart * g_part = g_pl->GetFirstPart();
				while( g_part )
				{
					if( part1->ref_des == g_part->ref_des )
						bStartPinInGroup = TRUE;
					if( !bStubTrace )
						if( part2->ref_des == g_part->ref_des )
							bEndPinInGroup = TRUE;
					g_part = g_pl->GetNextPart( g_part );
				}
				if( bStartPinInGroup && (bEndPinInGroup || bStubTrace) )
				{
					// add connection to group net, and copy all segments and vertices
					int p1 = g_nl->GetNetPinIndex( g_net, &pin1->ref_des, &pin1->pin_name );
					int g_ic;
					if( !bStubTrace )
					{
						int p2 = g_nl->GetNetPinIndex( g_net, &pin2->ref_des, &pin2->pin_name );
						g_ic = g_nl->AddNetConnect( g_net, p1, p2 );
					}
					else
					{
						g_ic = g_nl->AddNetStub( g_net, p1 );
					}
					cconnect * g_c = &g_net->connect[g_ic];
					g_c->nsegs = c->nsegs;
					g_c->seg.SetSize( c->nsegs );
					g_c->vtx.SetSize( c->nsegs + 1 );
					for( int is=0; is<c->nsegs; is++ )
					{
						g_c->seg[is] = c->seg[is];
						g_c->seg[is].m_dlist = NULL;
						g_c->seg[is].dl_el = NULL;
						g_c->seg[is].dl_sel = NULL;
						g_c->vtx[is] = c->vtx[is];	// this zeros graphics elements
						c->vtx[is] = g_c->vtx[is];	// this restores them
						g_c->vtx[is].m_dlist = NULL;
						g_nl->AddTeeID( g_c->vtx[is].tee_ID );
					}
					g_c->vtx[c->nsegs] = c->vtx[c->nsegs];
					c->vtx[c->nsegs] = g_c->vtx[c->nsegs];
					g_c->vtx[c->nsegs].m_dlist = NULL;
					g_nl->AddTeeID( g_c->vtx[c->nsegs].tee_ID );
					// remove any routed segments that are not in group
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].layer != LAY_RAT_LINE )
						{
							// routed segment, is this in group ?
							id search_id = sid;
							search_id.sst = ID_SEL_SEG;
							search_id.ii = is;
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
					for( int iv=1; iv<c->nsegs; iv++ )
					{
						cvertex * v = &c->vtx[iv];
						if( v->tee_ID || v->viaExists() )
						{
							// is this in group?
							id search_id = sid;
							search_id.sst = ID_SEL_VERTEX;
							search_id.ii = iv;
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
								cvertex * g_v = &g_c->vtx[iv];
								g_v->tee_ID = 0;
								g_v->force_via_flag = 0;
								g_v->SetNoVia();
								if( c->end_pin == cconnect::NO_END && iv == c->nsegs )
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
		else if( sid.type == ID_NET && sid.st == ID_AREA
			&& sid.sst == ID_SEL_SIDE )
		{
			// area side selected
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.i];
			CPolyLine * p = a->poly;
			if( a->utility == 0 )
			{
				// first area side found, mark area as selected and
				// all other sides as unselected
				for( int is=0; is<p->GetNumSides(); is++ )
					p->SetUtility( is, 0 );
				a->utility = 1;
			}
			p->SetUtility( sid.ii, 1 );	// mark this side as selected
		}
	}

	// remove any unrouted segments at end of stub unless a branch
	cnet * g_net = g_nl->GetFirstNet();
	while( g_net )
	{
		for( int ic=g_net->nconnects-1; ic>=0; ic-- )
		{
			cconnect * c = &g_net->connect[ic];
			if( c->end_pin == cconnect::NO_END )
			{
				if( c->vtx[c->nsegs].tee_ID != 0 )
				{
					// if tee-vertex not found, clear tee_id
					if( !g_nl->FindTeeVertexInNet( g_net, c->vtx[c->nsegs].tee_ID, NULL, NULL ) )
						c->vtx[c->nsegs].tee_ID = 0;
				}
				if( c->vtx[c->nsegs].tee_ID == 0 )
				{
					for( int is=c->nsegs-1; is>=0; is-- )
					{
						cseg * s = &c->seg[is];
						if( s->layer == LAY_RAT_LINE )
						{
							g_nl->RemoveSegment( g_net, ic, is, FALSE );
							is = c->nsegs;	// in case more than 1 segment is removed
						}
						else
							break;
					}
				}
			}
		}
		g_net = g_nl->GetNextNet();
	}

	// loop through connections again, removing any unused tees
	g_net = g_nl->GetFirstNet();
	while( g_net )
	{
		m_Doc->m_nlist->RemoveOrphanBranches( g_net, 0, TRUE );
		for( int ic=0; ic<g_net->nconnects; ic++ )
		{
			cconnect * c = &g_net->connect[ic];
			if( c->end_pin == cconnect::NO_END && c->vtx[c->nsegs].tee_ID )
			{
				// branch
				int id = c->vtx[c->nsegs].tee_ID;
				if( !g_nl->FindTeeVertexInNet( g_net, id, NULL, NULL ) )
				{
					// the original tee is not in the group
					// remove last segment and vertex
					g_nl->DisconnectBranch( g_net, ic );
				}
			}
			else
			{
				// check for tee-vertices
				for( int iv=1; iv<c->nsegs; iv++ )
				{
					cvertex * vtx = &c->vtx[iv];
					if( int id = vtx->tee_ID )
					{
						// tee-vertex, see if we need to add any unselected branches
						cnet * net = m_Doc->m_nlist->GetNetPtrByName( &g_net->name );
						if( net )
						{
							for( int bic=0; bic<net->nconnects; bic++ )
							{
								cconnect * bc = &net->connect[bic];
								cvertex * bv = &bc->vtx[bc->nsegs];
								if( bc->end_pin == cconnect::NO_END
									&& bv->tee_ID == id
									&& bc->utility == FALSE )
								{
									// found a branch that is not included in group
									// test start pin
									BOOL bStartPinInGroup = FALSE;
									cpin * pin = &net->pin[bc->start_pin];
									cpart * part = pin->part;
									// see if start pin is on a group part
									cpart * g_part = g_pl->GetPart( pin->ref_des );
									if( g_part )
									{
										// add branch
										int g_pin = g_nl->GetNetPinIndex( g_net, &pin->ref_des, &pin->pin_name );
										if( g_pin == -1 )
										{
											g_nl->AddNetPin( g_net, &pin->ref_des, &pin->pin_name, 0 );
											g_pin = g_nl->GetNetPinIndex( g_net, &pin->ref_des, &pin->pin_name );
										}
										int g_ic = g_nl->AddNetStub( g_net, g_pin );
										g_nl->AppendSegment( g_net, g_ic, vtx->x, vtx->y, LAY_RAT_LINE, CSegWidthInfo() );
										g_net->connect[g_ic].vtx[1].tee_ID = id;
									}

								}
							}
						}
						g_nl->RemoveTeeIfNoBranches( g_net, id );
					}
				}
			}
		}
		g_net = g_nl->GetNextNet();
	}

	// now check areas, only copy areas if all sides are selected
	net = m_Doc->m_nlist->GetFirstNet();
	while( net )
	{
		for( int ia=0; ia<net->nareas; ia++ )
		{
			carea * a = &net->area[ia];
			if( a->utility )
			{
				BOOL bAllSides = TRUE;
				CPolyLine * p = a->poly;
				for( int is=0; is<p->GetNumSides(); is++ )
				{
					if( p->GetUtility( is ) == 0 )
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
						g_net = g_nl->AddNet( net->name, net->npins, net->def_width, net->def_clearance );
					}
					int g_ia = g_nl->AddArea( g_net, p->GetLayer(), p->GetX(0), p->GetY(0),
						p->GetHatch() );
					CPolyLine * g_p = g_net->area[g_ia].poly;
					g_p->Copy( p );
					id g_id;
					g_id = g_p->GetId();
					g_id.i = g_ia;
					g_p->SetId( &g_id );
				}
			}
		}
		net = m_Doc->m_nlist->GetNextNet();
	}

	// now remove any nets with zero pins, connections and areas
	net = g_nl->GetFirstNet();
	while( net )
	{
		cnet * next_net = g_nl->GetNextNet();
		if( net->npins == 0 && net->nconnects == 0 && net->nareas == 0 )
			g_nl->RemoveNet( net );
		net = next_net;
	}

	// mark all sm_cutouts as unselected
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
	{
		for( int is=0; is<m_Doc->m_sm_cutout[ism].GetNumSides(); is++ )
			m_Doc->m_sm_cutout[ism].SetUtility( is, 0 );
	}
	// find selected sides
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT && sid.sst == ID_SEL_SIDE )
		{
			m_Doc->m_sm_cutout[sid.i].SetUtility( sid.ii, 1 );
		}
	}
	// copy to group
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
	{
		CPolyLine * p = &m_Doc->m_sm_cutout[ism];
		BOOL bAllSides = TRUE;
		for( int is=0; is<p->GetNumSides(); is++ )
		{
			if( p->GetUtility( is ) == 0 )
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
			id sid = g_p->GetId();
			sid.i = g_ism;
			g_p->SetId( &sid );
		}
	}

	// mark all board outlines as unselected
	for( int ibd=0; ibd<m_Doc->m_board_outline.GetSize(); ibd++ )
	{
		for( int is=0; is<m_Doc->m_board_outline[ibd].GetNumSides(); is++ )
			m_Doc->m_board_outline[ibd].SetUtility( is, 0 );
	}
	// find selected sides
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_BOARD && sid.st == ID_BOARD && sid.sst == ID_SEL_SIDE )
		{
			m_Doc->m_board_outline[sid.i].SetUtility( sid.ii, 1 );
		}
	}
	// copy to group
	for( int ibd=0; ibd<m_Doc->m_board_outline.GetSize(); ibd++ )
	{
		CPolyLine * p = &m_Doc->m_board_outline[ibd];
		BOOL bAllSides = TRUE;
		for( int is=0; is<p->GetNumSides(); is++ )
		{
			if( p->GetUtility( is ) == 0 )
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
			id sid = g_p->GetId();
			sid.i = g_ibd;
			g_p->SetId( &sid );
		}
	}

	// see if anything copied
	if( !g_nl->GetFirstNet() && !g_pl->GetFirstPart() && !g_sm->GetSize()
		&& !g_bd->GetSize() && !g_tl->GetNumTexts() )
	{
		AfxMessageBox( "Nothing copied !\nRemember that traces must be connected\nto a part in the group to be copied" );
		CWnd* pMain = AfxGetMainWnd();
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			CMenu* submenu = pMenu->GetSubMenu(1);	// "Edit" submenu
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
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
			submenu->EnableMenuItem( ID_EDIT_PASTE, MF_BYCOMMAND | MF_ENABLED );
			pMain->DrawMenuBar();
		}
	}
}

// function to find all stub traces ending on tee and mark them for removal,
// then looks for any tees on that stub and recurses
//
void MarkStubsForRemoval( cnet * net, int tee_ID )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		cvertex * end_v = &c->vtx[c->nsegs];
		if( c->end_pin == cconnect::NO_END && end_v->tee_ID == tee_ID )
		{
			// flag this stub for removal, and search for other tees in stub
			c->utility = 2;
			net->utility = 1;
			for( int iv=1; iv<c->nsegs; iv++ )
			{
				cvertex * v = &c->vtx[iv];
				if( v->tee_ID )
				{
					MarkStubsForRemoval( net, v->tee_ID );
				}
			}
		}
	}
}

void CFreePcbView::OnGroupCut()
{
	OnGroupCopy();
	OnGroupDelete();
}

// Remove all elements in group from project
//
void CFreePcbView::OnGroupDelete()
{
	DeleteGroup( &m_sel_ptrs, &m_sel_ids );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
}

void CFreePcbView::DeleteGroup( CArray<void*> * grp_ptr, CArray<id> * grp_id )
{
	CPartList * pl =  m_Doc->m_plist;
	CNetList * nl = m_Doc->m_nlist;
	cpart * part;
	cnet * net;

	// create undo descriptor before deletion
	undo_group_descriptor * undo = (undo_group_descriptor*)CreateGroupDescriptor( m_Doc->m_undo_list,
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
		if( this_id.type == ID_PART && this_id.st == ID_SEL_RECT )
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
		if( this_id.type == ID_NET && this_id.st == ID_CONNECT )
		{
			cnet * net = (cnet *) (*grp_ptr)[i];
			net->utility = 1;		// this net will be modified
		}
	}
	// save undo info
	m_Doc->m_undo_list->NewEvent();
	for( net=nl->GetFirstNet(); net; net=nl->GetNextNet() )
		if( net->utility )
			SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE, m_Doc->m_undo_list );
	for( part=pl->GetFirstPart(); part; part=pl->GetNextPart(part) )
		if( part->utility )
			SaveUndoInfoForPart( part,
			CPartList::UNDO_PART_DELETE, NULL, FALSE, m_Doc->m_undo_list );

	// mark all nets as unmodified (again)
	nl->MarkAllNets( 0 );
	// mark all sm_cutout sides as unselected
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
		for( int is=0; is<m_Doc->m_sm_cutout[ism].GetNumSides(); is++ )
			m_Doc->m_sm_cutout[ism].SetUtility( is, 0 );
	// mark all board outline sides as unselected
	for( int ibd=0; ibd<m_Doc->m_board_outline.GetSize(); ibd++ )
		for( int is=0; is<m_Doc->m_board_outline[ibd].GetNumSides(); is++ )
			m_Doc->m_board_outline[ibd].SetUtility( is, 0 );

	// first pass
	// identify selected tees and mark dependent stub traces for removal
	for( int i=0; i<grp_id->GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		void * ptr = (*grp_ptr)[i];
		if( this_id.type == ID_NET )
		{
			cnet * net = (cnet*)ptr;
			if( this_id.st == ID_CONNECT )
			{
				int ic = this_id.i;
				cconnect * c = &net->connect[ic];
				if( this_id.sst == ID_SEL_VERTEX )
				{
					// mark tee for removal
					int iv = this_id.ii;
					cvertex * v = &c->vtx[iv];
					if( v->tee_ID && iv != c->nsegs )
					{
						// tee, not at end of stub trace
						MarkStubsForRemoval( net, v->tee_ID );
					}
				}
			}
		}
	}
	// second pass
	// unroute selected trace segments
	for( int i=0; i<(*grp_id).GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		void * ptr = (*grp_ptr)[i];
		if( this_id.type == ID_NET )
		{
			cnet * net = (cnet*)ptr;
			if( this_id.st == ID_CONNECT )
			{
				// don't actually delete connections, just unroute
				int ic = this_id.i;
				cconnect * c = &net->connect[ic];
				if( this_id.sst == ID_SEL_SEG )
				{
					// unroute segment
					int is = this_id.ii;
					nl->UnrouteSegmentWithoutMerge( net, ic, is );
					net->utility = 1;	// flag net as modified
					if( c->utility == 0 )
						c->utility = 1;		// flag connection as modified
				}
				else if( this_id.sst == ID_SEL_VERTEX )
				{
					// mark vertex for unrouting
					int iv = this_id.ii;
				}
				else
					ASSERT(0);
			}
		}
	}
	// third pass
	// remove connections marked for deletion and merge unrouted segments
	net = nl->GetFirstNet();
	while( net )
	{
		if( net->utility )
		{
			for( int ic=net->nconnects-1; ic>=0; ic-- )
			{
				cconnect * c = &net->connect[ic];
				if( c->utility == 2 )
					nl->RemoveNetConnect( net, ic, FALSE );
				else if( c->utility == 1 )
					nl->MergeUnroutedSegments( net, ic );
			}
		}
		net = nl->GetNextNet();
	}
	// fourth pass
	// remove board outlines, copper areas, parts, texts and sm_cutouts
	for( int i=0; i<(*grp_id).GetSize(); i++ )
	{
		id this_id = (*grp_id)[i];
		if( this_id.type == ID_NET && this_id.st == ID_AREA && this_id.sst == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)(*grp_ptr)[i];
			carea * a = &net->area[this_id.i];
			a->poly->SetUtility( this_id.ii, 1 );	// mark for deletion
			a->utility = 1;
			net->utility = 1;						// mark as modified
		}
		if( this_id.type == ID_PART && this_id.st == ID_SEL_RECT )
		{
			cpart * part = (cpart *) (*grp_ptr)[i];
			nl->PartDeleted( part );
			pl->Remove( part );
		}
		else if( this_id.type == ID_TEXT )
		{
			CText * text = (CText *)(*grp_ptr)[i];
			SaveUndoInfoForText( text, CTextList::UNDO_TEXT_DELETE, FALSE, m_Doc->m_undo_list );
			m_Doc->m_tlist->RemoveText( text );
		}
		else if( this_id.type == ID_SM_CUTOUT && this_id.sst == ID_SEL_SIDE )
			m_Doc->m_sm_cutout[this_id.i].SetUtility( this_id.ii, 1 );	// mark for deletion
		else if( this_id.type == ID_BOARD && this_id.sst == ID_SEL_SIDE )
			m_Doc->m_board_outline[this_id.i].SetUtility( this_id.ii, 1 );	// mark for deletion
	}
	// delete sm_cutouts and renumber them
	BOOL bUndoSaved = FALSE;
	for( int ism=m_Doc->m_sm_cutout.GetSize()-1; ism>=0; ism-- )
	{
		BOOL bDelete = TRUE;
		for( int is=0; is<m_Doc->m_sm_cutout[ism].GetNumSides(); is++ )
			if( m_Doc->m_sm_cutout[ism].GetUtility( is ) == 0 )
				bDelete = FALSE;
		if( bDelete )
		{
			if( !bUndoSaved )
				SaveUndoInfoForSMCutouts( FALSE, m_Doc->m_undo_list );
			m_Doc->m_sm_cutout.RemoveAt( ism );
			bUndoSaved = TRUE;
		}
	}
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
	{
			id new_id = m_Doc->m_sm_cutout[ism].GetId();
			new_id.i = ism;
			m_Doc->m_sm_cutout[ism].SetId( &new_id );
	}
	// delete board outlines and renumber them
	bUndoSaved = FALSE;
	for( int ibd=m_Doc->m_board_outline.GetSize()-1; ibd>=0; ibd-- )
	{
		BOOL bDelete = TRUE;
		for( int is=0; is<m_Doc->m_board_outline[ibd].GetNumSides(); is++ )
			if( m_Doc->m_board_outline[ibd].GetUtility( is ) == 0 )
				bDelete = FALSE;
		if( bDelete )
		{
			if( !bUndoSaved )
				SaveUndoInfoForBoardOutlines( FALSE, m_Doc->m_undo_list );
			m_Doc->m_board_outline.RemoveAt( ibd );
			bUndoSaved = TRUE;
		}
	}
	for( int ibd=0; ibd<m_Doc->m_board_outline.GetSize(); ibd++ )
	{
			id new_id = m_Doc->m_board_outline[ibd].GetId();
			new_id.i = ibd;
			m_Doc->m_board_outline[ibd].SetId( &new_id );
	}
	// delete copper areas or cutouts if all sides are in group
	net = nl->GetFirstNet();
	while( net )
	{
		for( int ia=net->nareas-1; ia>=0; ia-- )
		{
			carea * a = &net->area[ia];
			if( a->utility )
			{
				// see if entire area can be deleted
				BOOL bDelete = TRUE;
				for( int is=0; is<a->poly->GetContourEnd(0); is++ )
					if( a->poly->GetUtility( is ) == 0 )
						bDelete = FALSE;
				if( bDelete )
				{
					SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_DELETE, FALSE, m_Doc->m_undo_list );
					nl->RemoveArea( net, ia );
				}
				else if( a->poly->GetNumContours() > 1 )
				{
					// see if cutouts can be deleted
					BOOL bCutoutsDeleted = FALSE;
					for( int icont=a->poly->GetNumContours()-1; icont>0; icont-- )
					{
						int istart = a->poly->GetContourStart( icont );
						int iend = a->poly->GetContourEnd( icont );
						bDelete = TRUE;
						for( int is=istart; is<=iend; is++ )
							if( a->poly->GetUtility( is ) == 0 )
								bDelete = FALSE;
						if( bDelete )
						{
							a->poly->RemoveContour( icont );
							bCutoutsDeleted = TRUE;
						}
					}
					if( bCutoutsDeleted )
						SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_MODIFY, FALSE, m_Doc->m_undo_list );
				}
			}
		}
		nl->SetAreaConnections( net );
		net = nl->GetNextNet();
	}
	// clean up
	m_Doc->m_undo_list->Push( UNDO_GROUP, (void*)undo, &UndoGroupCallback );
}

void CFreePcbView::OnGroupPaste()
{
	void * vp;
	// pointers to group lists
	CPartList * g_pl = m_Doc->clip_plist;
	CTextList * g_tl = m_Doc->clip_tlist;
	CNetList * g_nl = new CNetList( NULL, g_pl );	// make copy to modify
	g_nl->Copy( m_Doc->clip_nlist );
	g_nl->MarkAllNets( 0 );
	CArray<CPolyLine> * g_sm = &m_Doc->clip_sm_cutout;
	CArray<CPolyLine> * g_bd = &m_Doc->clip_board_outline;
	// pointers to project lists
	CPartList * pl = m_Doc->m_plist;
	CNetList * nl = m_Doc->m_nlist;
	CTextList * tl = m_Doc->m_tlist;
	CArray<CPolyLine> * sm = &m_Doc->m_sm_cutout;
	CArray<CPolyLine> * bd = &m_Doc->m_board_outline;

	// get paste options
	CDlgGroupPaste dlg;
	dlg.Initialize( g_nl );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// start pasting
		CancelSelection();
		SetCursorMode( CUR_GROUP_SELECTED );
		m_sel_id.type = ID_MULTI;
		m_Doc->m_undo_list->NewEvent();
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
		cnet * net = nl->GetFirstNet();
		while( net )
		{
			for( int ip=0; ip<net->npins; ip++ )
			{
				cpin * p = &net->pin[ip];
				if( !ref_des_map.Lookup( p->ref_des, vp ) )
				{
					ref_des_map.SetAt( p->ref_des, NULL );
				}
			}
			net = nl->GetNextNet();
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
				CString mess = "Part \"";
				mess += conflicted_ref;
				mess += "\" already exists in project.\nIt will be renamed \" ";
				mess += new_ref;
				mess += " \"";
				AfxMessageBox( mess );
				bConflict = TRUE;
			}
			// now change part refs in group netlist
			net = g_nl->GetFirstNet();
			while( net )
			{
				for( int ip=0; ip<net->npins; ip++ )
				{
					cpin * pin = &net->pin[ip];
					if( pin->utility == 0 && pin->ref_des == g_part->ref_des )
					{
						pin->ref_des = new_ref;
						pin->part = NULL;
						pin->utility = 1;	// only change it once
					}
				}
				net = g_nl->GetNextNet();
			}
			// add new part
			cpart * prj_part = pl->Add( g_part->shape, &new_ref, &g_part->package,
				g_part->x + dlg.m_dx, g_part->y + dlg.m_dy,
				g_part->side, g_part->angle, 1, 0 );
			ref_des_map.SetAt( new_ref, NULL );
			SaveUndoInfoForPart( prj_part,
				CPartList::UNDO_PART_ADD, &prj_part->ref_des, FALSE, m_Doc->m_undo_list );
			// set ref text parameters
			pl->UndrawPart( prj_part );
			prj_part->m_ref_angle = g_part->m_ref_angle;
			prj_part->m_ref_size = g_part->m_ref_size;
			prj_part->m_ref_w = g_part->m_ref_w;
			prj_part->m_ref_xi = g_part->m_ref_xi;
			prj_part->m_ref_yi = g_part->m_ref_yi;
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
			m_sel_ids[i].st = ID_SEL_RECT;
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
			cnet * net = nl->GetFirstNet();
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
				net = nl->GetNextNet();
			}
			g_suffix.Format( "_$G%d", max_g_num + 1 );
		}
		// now loop through all nets in group and add or merge with project
		cnet * prj_net = NULL;	// project net
		cnet * g_net = g_nl->GetFirstNet();	// group net
		while( g_net )
		{
			// see if there are routed segments in this net
			BOOL bRouted = FALSE;
			if( dlg.m_pin_net_option == 1 )
			{
				for( int ic=0; ic<g_net->nconnects; ic++ )
				{
					cconnect * c = &g_net->connect[ic];
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].width() > 0 )
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
			if( (dlg.m_pin_net_option == 0 || bRouted) || g_net->nareas > 0 )
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
						cnet * net = nl->GetFirstNet();
						int max_num = 0;
						CString prefix;
						while( net )
						{
							int num = ParseRef( &net->name, &prefix );
							if( prefix == "N" && num > max_num )
								max_num = num;
							net = nl->GetNextNet();
						}
						new_name.Format( "N%05d", max_num+1 );
					}
					else
					{
						// add group suffix
						new_name = g_net->name + g_suffix;
					}
					// add new net
					prj_net = nl->AddNet( new_name, g_net->npins, g_net->def_width, g_net->def_clearance );
					SaveUndoInfoForNet( prj_net, CNetList::UNDO_NET_ADD, FALSE, m_Doc->m_undo_list );
					prj_net->utility = 1;	// mark as saved
				}
				else
				{
					// merge group net with project net of same name
					prj_net = nl->GetNetPtrByName( &g_net->name );
					if( !prj_net )
					{
						// no project net with the same name
						prj_net = nl->AddNet( g_net->name, g_net->npins, g_net->def_width, g_net->def_clearance );
						SaveUndoInfoForNet( prj_net, CNetList::UNDO_NET_ADD, FALSE, m_Doc->m_undo_list );
						prj_net->utility = 1;	// mark as saved
					}
					else if( prj_net->utility == 0 )
					{
						SaveUndoInfoForNetAndConnectionsAndAreas( prj_net, FALSE, m_Doc->m_undo_list );
						prj_net->utility = 1;	// mark as saved
					}
				}
				if( !prj_net )
					ASSERT(0);
				// now create map for renaming tees
				CMap<int,int,int,int> tee_map;
				// connect group part pins to project net
				for( int ip=0; ip<g_net->npins; ip++ )
				{
					cpin * pin = &g_net->pin[ip];
					BOOL bAdd = TRUE;
					if( dlg.m_pin_net_option == 1 )
					{
						// only add pin if connected to a routed trace
						bAdd = FALSE;
						for( int ic=0; ic<g_net->nconnects; ic++ )
						{
							cconnect * c = &g_net->connect[ic];
							if( c->start_pin == ip || c->end_pin == ip )
							{
								for( int is=0; is<c->nsegs; is++ )
								{
									if( c->seg[is].width() > 0 )
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
						nl->AddNetPin( prj_net, &pin->ref_des, &pin->pin_name, FALSE );
				}
				// create new traces
				for( int g_ic=0; g_ic<g_net->nconnects; g_ic++ )
				{
					cconnect * g_c = &g_net->connect[g_ic];
					// get start pin of connection in new net
					CString g_start_ref_des = g_net->pin[g_c->start_pin].ref_des;
					CString g_start_pin_name = g_net->pin[g_c->start_pin].pin_name;
					int new_start_pin = nl->GetNetPinIndex( prj_net, &g_start_ref_des, &g_start_pin_name );
					// get end pin of connection in new net
					CString g_end_ref_des;
					CString g_end_pin_name;
					int new_end_pin = cconnect::NO_END;
					if( g_c->end_pin != cconnect::NO_END )
					{
						g_end_ref_des = g_net->pin[g_c->end_pin].ref_des;
						g_end_pin_name = g_net->pin[g_c->end_pin].pin_name;
						new_end_pin = nl->GetNetPinIndex( prj_net, &g_end_ref_des, &g_end_pin_name );
					}
					if( new_start_pin != -1 && (new_end_pin != -1 || g_c->end_pin == cconnect::NO_END) )
					{
						// add connection to new net
						int ic;
						if( new_end_pin != cconnect::NO_END )
						{
							ic = nl->AddNetConnect( prj_net, new_start_pin, new_end_pin );
						}
						else
						{
							ic = nl->AddNetStub( prj_net, new_start_pin );
						}
						nl->UndrawConnection( prj_net, ic );
						// copy it and draw it
						if( ic < 0 )
							ASSERT(0);
						else
						{
							// copy connection
							cconnect * c = &prj_net->connect[ic];
							c->nsegs = g_c->nsegs;
							c->seg.SetSize( g_c->nsegs );
							c->vtx.SetSize( g_c->nsegs + 1 );
							for( int is=0; is<c->nsegs; is++ )
							{
								c->seg[is] = g_c->seg[is];
								c->seg[is].m_dlist = m_dlist;
								c->seg[is].dl_el = NULL;
								c->seg[is].dl_sel = NULL;
								c->vtx[is] = g_c->vtx[is];
								c->vtx[is].m_dlist = m_dlist;
								c->vtx[is].dl_sel = NULL;
								c->vtx[is].dl_hole = NULL;
								c->vtx[is].dl_el.SetSize(0);
								id seg_id( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, is );
								m_sel_ptrs.Add( prj_net );
								m_sel_ids.Add( seg_id );
								id vtx_id( ID_NET, ID_CONNECT, ic, ID_SEL_VERTEX, is );
								m_sel_ptrs.Add( prj_net );
								m_sel_ids.Add( vtx_id );
							}
							c->vtx[c->nsegs] = g_c->vtx[g_c->nsegs];
							c->vtx[c->nsegs].m_dlist = m_dlist;
							c->vtx[c->nsegs].dl_sel = NULL;
							c->vtx[c->nsegs].dl_hole = NULL;
							c->vtx[c->nsegs].dl_el.SetSize(0);
							if( c->end_pin != cconnect::NO_END )
							{
								id vtx_id( ID_NET, ID_CONNECT, ic, ID_SEL_VERTEX, c->nsegs );
								m_sel_ptrs.Add( prj_net );
								m_sel_ids.Add( vtx_id );
							}
							for( int iv=0; iv<c->vtx.GetSize(); iv++ )
							{
								c->vtx[iv].x += dlg.m_dx;
								c->vtx[iv].y += dlg.m_dy;
								if( int g_id = c->vtx[iv].tee_ID )
								{
									// assign new tee_ID
									int new_id;
									BOOL bFound = tee_map.Lookup( g_id, new_id );
									if( !bFound )
									{
										new_id = nl->GetNewTeeID();
										tee_map.SetAt( g_id, new_id );
									}
									c->vtx[iv].tee_ID = new_id;
								}
								// update lower-left corner
								double d = c->vtx[iv].x + c->vtx[iv].y;
								if( d < min_d )
								{
									min_d = d;
									min_x = c->vtx[iv].x;
									min_y = c->vtx[iv].y;
								}
							}
							nl->DrawConnection( prj_net, ic );
						}
					}
				}
				// add copper areas
				for( int g_ia=0; g_ia<g_net->nareas; g_ia++ )
				{
					carea * ga = &g_net->area[g_ia];
					CPolyLine * gp = ga->poly;
					int ia = nl->AddArea( prj_net, gp->GetLayer(),
						gp->GetX(0), gp->GetY(0), gp->GetHatch() );
					CPolyLine * p = prj_net->area[ia].poly;
					id p_id = p->GetId();
					p->Copy( gp );
					p->SetId( &p_id );
					p->SetPtr( prj_net );
					for( int is=0; is<p->GetNumSides(); is++ )
					{
						int x = p->GetX(is);
						int y = p->GetY(is);
						p->SetX( is, x + dlg.m_dx );
						p->SetY( is, y + dlg.m_dy );
						p_id.i = ia;
						p_id.sst = ID_SEL_SIDE;
						p_id.ii = is;
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
			g_net = g_nl->GetNextNet();
		}
		// now destroy modified g_nl and restore links in g_pl
		delete g_nl;

		// add sm_cutouts
		int grp_size = g_sm->GetSize();
		int old_size = sm->GetSize();
		if( grp_size > 0 )
		{
			SaveUndoInfoForSMCutouts( FALSE, m_Doc->m_undo_list );
			sm->SetSize( old_size + grp_size );
			for( int g_ism=0; g_ism<grp_size; g_ism++ )
			{
				int ism = g_ism + old_size;
				CPolyLine * g_p = &(*g_sm)[g_ism];
				CPolyLine * p = &(*sm)[ism];
				p->Copy( g_p );
				id p_id = p->GetId();
				p_id.i = ism;
				p->SetId( &p_id );
				for( int is=0; is<p->GetNumSides(); is++ )
				{
					int x = p->GetX(is);
					int y = p->GetY(is);
					p->SetX( is, x + dlg.m_dx );
					p->SetY( is, y + dlg.m_dy );
					p_id.i = ism;
					p_id.sst = ID_SEL_SIDE;
					p_id.ii = is;
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
			SaveUndoInfoForBoardOutlines( FALSE, m_Doc->m_undo_list );
			bd->SetSize( old_size + grp_size );
			for( int g_ibd=0; g_ibd<grp_size; g_ibd++ )
			{
				int ibd = g_ibd + old_size;
				CPolyLine * g_p = &(*g_bd)[g_ibd];	// group poly
				CPolyLine * p = &(*bd)[ibd];		// project poly
				p->Copy( g_p );
				id p_id = p->GetId();
				p_id.i = ibd;
				p->SetId( &p_id );
				for( int is=0; is<p->GetNumSides(); is++ )
				{
					int x = p->GetX(is);
					int y = p->GetY(is);
					p->SetX( is, x + dlg.m_dx );
					p->SetY( is, y + dlg.m_dy );
					p_id.sst = ID_SEL_SIDE;
					p_id.ii = is;
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

		// add text
		CText * t = g_tl->GetFirstText();
		while( t )
		{
			CText * new_text = m_Doc->m_tlist->AddText( t->m_x+dlg.m_dx, t->m_y+dlg.m_dy, t->m_angle,
				t->m_mirror, t->m_bNegative, t->m_layer, t->m_font_size, t->m_stroke_width,
				&t->m_str, TRUE );
			SaveUndoInfoForText( new_text, CTextList::UNDO_TEXT_ADD, FALSE, m_Doc->m_undo_list );
			id t_id( ID_TEXT, ID_SEL_TXT, 0, 0, 0 );
			m_sel_ids.Add( t_id );
			m_sel_ptrs.Add( new_text );
			CRect text_bounds;
			m_Doc->m_tlist->GetTextRectOnPCB( new_text, &text_bounds );
			double d = text_bounds.left + text_bounds.bottom;
			if( d < min_d )
			{
				min_d = d;
				min_x = text_bounds.left;
				min_y = text_bounds.bottom;
			}
			t = g_tl->GetNextText();
		}

		HighlightGroup();
		if( bDragGroup )
		{
			if( min_x == INT_MAX || min_y == INT_MAX )
				AfxMessageBox( "No items to drag" );
			else
				StartDraggingGroup( TRUE, min_x, min_y );
		}
		else
			FindGroupCenter();
		m_Doc->ProjectModified( TRUE );
	}
}

void CFreePcbView::OnGroupSaveToFile()
{
	// Copy group to pseudo-clipboard
	OnGroupCopy();

	// force old-style file dialog by setting size of OPENFILENAME struct
	CFileDialog dlg( 0, "fpc", NULL, 0,
		"PCB files (*.fpc)|*.fpc|All Files (*.*)|*.*||",
		NULL, OPENFILENAME_SIZE_VERSION_400 );
	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_Doc->m_parent_folder;
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
			CString mess;
			mess.Format( "Unable to open file %s", pathname );
			AfxMessageBox( mess );
		}
		else
		{
			// write clipboard to file
			try
			{
				// make map of all footprints used by group
				CMapStringToPtr clip_cache_map;
				cpart * part = m_Doc->clip_plist->GetFirstPart();
				while( part )
				{
					void * vp;
					if( part->shape )
						if( !clip_cache_map.Lookup( part->shape->m_name, vp ) )
							clip_cache_map.SetAt( part->shape->m_name, part->shape );
					part = m_Doc->clip_plist->GetNextPart( part );
				}
				m_Doc->WriteOptions( &pcb_file );
				m_Doc->WriteFootprints( &pcb_file, &clip_cache_map );
				m_Doc->WriteBoardOutline( &pcb_file, &m_Doc->clip_board_outline );
				m_Doc->WriteSolderMaskCutouts( &pcb_file, &m_Doc->clip_sm_cutout );
				m_Doc->clip_plist->WriteParts( &pcb_file );
				m_Doc->clip_nlist->WriteNets( &pcb_file );
				m_Doc->clip_tlist->WriteTexts( &pcb_file );
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
	if( !m_Doc->m_project_open )
		return;
	if( m_cursor_mode == CUR_GROUP_SELECTED )
		OnGroupCopy();
}

void CFreePcbView::OnEditPaste()
{
	if( !m_Doc->m_project_open )
		return;
	OnGroupPaste();
}


void CFreePcbView::OnEditCut()
{
	if( !m_Doc->m_project_open )
		return;
	if( m_cursor_mode == CUR_GROUP_SELECTED )
	{
		OnGroupCopy();
		OnGroupDelete();
	}
}

void CFreePcbView::RotateGroup()
{
	UngluePartsInGroup();

	// mark all parts and nets as unselected
	m_Doc->m_nlist->MarkAllNets(0);
	m_Doc->m_plist->MarkAllParts(0);

	// mark connections, segments and vertices of selected nets as unselected
	// and vertices and area corners as unmoved
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			if( net->utility == FALSE )
			{
				// first time for this net,
				net->utility = TRUE;	// mark as selected
				// mark all connections, segments and vertices as unselected
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					net->connect[ic].utility = FALSE;
					for( int is=0; is<net->connect[ic].nsegs; is++ )
					{
						net->connect[ic].seg[is].utility = FALSE;
						net->connect[ic].vtx[is].utility = FALSE;
						net->connect[ic].vtx[is].utility2 = FALSE;
						net->connect[ic].vtx[is+1].utility = FALSE;
						net->connect[ic].vtx[is+1].utility2 = FALSE;
					}
				}
				// mark all area corners as unmoved
				for( int ia=0; ia<net->nareas; ia++ )
				{
					for( int is=0; is<net->area[ia].poly->GetNumCorners(); is++ )
					{
						net->area[ia].poly->SetUtility( is, 0 );	// unmoved
					}
				}
			}
		}
	}
	// mark all corners of solder mask cutouts as unmoved
	for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[im];
		for( int ic=0; ic<poly->GetNumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}
	// mark all corners of board outlines as unmoved
	for( int im=0; im<m_Doc->m_board_outline.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_board_outline[im];
		for( int ic=0; ic<poly->GetNumCorners(); ic++ )
			poly->SetUtility( ic, 0 );	// unmoved
	}

	int tempx;
	// mark all relevant parts, nets, connections and segments as selected
	// and move text and copper area corners
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_SEG )
		{
			// segment
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.i;
			int is = sid.ii;
			cconnect * c = &net->connect[ic];	// this connection
			cseg * s = &c->seg[is];				// this segment
			cvertex * pre_v = &c->vtx[is];
			cvertex * post_v = &c->vtx[is+1];
			c->utility = TRUE;					// mark connection selected
			s->utility = TRUE;					// mark segment selected
			pre_v->utility =  TRUE;				// mark adjacent vertices as selected
			post_v->utility =  TRUE;
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_VERTEX )
		{
			// vertex
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.i;
			int iv = sid.ii;
			cconnect * c = &net->connect[ic];	// this connection
			cvertex * v = &c->vtx[iv];
			c->utility = TRUE;					// mark connection selected
			v->utility = TRUE;					// mark vertex selected
		}
		else if( sid.type == ID_PART && sid.st == ID_SEL_RECT )
		{
			// part
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->utility = TRUE;	// mark part selected
		}
		else if( sid.type == ID_TEXT && sid.st == ID_SEL_TXT )
		{
			// text
			CText * t = (CText*)m_sel_ptrs[i];
			m_Doc->m_tlist->MoveText( t, groupAverageX + t->m_y - groupAverageY,
				groupAverageY - t->m_x + groupAverageX, (t->m_angle+90)%360,
				t->m_mirror, t->m_bNegative, t->m_layer );
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA && sid.sst == ID_SEL_SIDE )
		{
			// area side
			cnet * net = (cnet*)m_sel_ptrs[i];
			CPolyLine * poly = net->area[sid.i].poly;
			int icontour = poly->GetContour(sid.ii);
			int istart = poly->GetContourStart(icontour);
			int iend = poly->GetContourEnd(icontour);
			int ic1 = sid.ii;
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			poly->Undraw();
			if( !poly->GetUtility(ic1) )
			{
				// unmoved, move it
				tempx=poly->GetX(ic1);
				poly->SetX( ic1, groupAverageX + poly->GetY(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY -tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->GetUtility(ic2) )
			{
				// unmoved, move it
				tempx=poly->GetX(ic2);
				poly->SetX( ic2, groupAverageX + poly->GetY(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY -tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
			poly->Draw();
		}
		// sm_cutout side
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT && sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
			int icontour = poly->GetContour(0);
			int istart = poly->GetContourStart(icontour);
			int iend = poly->GetContourEnd(icontour);
			int ic1 = sid.ii;
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			poly->Undraw();
			if( !poly->GetUtility(ic1) )
			{
				// unmoved, move it
				tempx=poly->GetX(ic1);
				poly->SetX( ic1, groupAverageX + poly->GetY(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY -tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->GetUtility(ic2) )
			{
				// unmoved, move it
				tempx=poly->GetX(ic2);
				poly->SetX( ic2, groupAverageX + poly->GetY(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY -tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
			poly->Draw();
		}
		// board outline side
		else if( sid.type == ID_BOARD && sid.st == ID_BOARD && sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[sid.i];
			int icontour = poly->GetContour(0);
			int istart = poly->GetContourStart(icontour);
			int iend = poly->GetContourEnd(icontour);
			int ic1 = sid.ii;
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			poly->Undraw();
			if( !poly->GetUtility(ic1) )
			{
				// unmoved, move it
				tempx=poly->GetX(ic1);
				tempx=poly->GetX(ic1);
				poly->SetX( ic1, groupAverageX + poly->GetY(ic1)- groupAverageY );
				poly->SetY( ic1, groupAverageY -tempx + groupAverageX );
				poly->SetUtility(ic1,1);
			}
			if( !poly->GetUtility(ic2) )
			{
				// unmoved, move it
				tempx=poly->GetX(ic2);
				poly->SetX( ic2, groupAverageX + poly->GetY(ic2)- groupAverageY );
				poly->SetY( ic2, groupAverageY -tempx + groupAverageX );
				poly->SetUtility(ic2,1);
			}
			poly->Draw();
		}
		else
			ASSERT(0);
	}

	// assume utility flags have been set on selected parts,
	// nets, connections, segments and vertices

	// move parts in group
	cpart * part = m_Doc->m_plist->GetFirstPart();
	while( part != NULL )
	{
		if( part->utility )
		{
			// move part
			m_Doc->m_plist->Move( part, groupAverageX + part->y - groupAverageY,
				groupAverageY -part->x + groupAverageX, (part->angle+90)%360, part->side );
			// find segments which connect to this part and move them
			cnet * net;
			for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
			{
				net = (cnet*)part->pin[ip].net;
				if( net )
				{
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						cconnect * c = &net->connect[ic];
						int nsegs = c->nsegs;
						if( nsegs )
						{
							int p1 = c->start_pin;
							CString pin_name1 = net->pin[p1].pin_name;
							int pin_index1 = part->shape->GetPinIndexByName( pin_name1 );
							int p2 = c->end_pin;
							if( net->pin[p1].part == part )
							{
								// starting pin is on part
								if( !c->seg[0].utility && c->seg[0].layer != LAY_RAT_LINE )
								{
									// first segment is not selected, unroute it
									if( !c->seg[0].utility )
										m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, 0 );
								}
								// move vertex if not selected
								if( !c->vtx[0].utility )
								{
									m_Doc->m_nlist->MoveVertex( net, ic, 0,
										part->pin[pin_index1].x, part->pin[pin_index1].y );
									c->vtx[0].utility2 = 1; // moved
								}
							}
							if( p2 != cconnect::NO_END )
							{
								if( net->pin[p2].part == part )
								{
									// ending pin is on part
									if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
									{
										// unroute it if not selected
										if( !c->seg[nsegs-1].utility )
											m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, nsegs-1 );
									}
									// modify vertex position if necessary
									if( !c->vtx[nsegs].utility )
									{
										// move vertex if unselected
										CString pin_name2 = net->pin[p2].pin_name;
										int pin_index2 = part->shape->GetPinIndexByName( pin_name2 );
										m_Doc->m_nlist->MoveVertex( net, ic, nsegs,
											part->pin[pin_index2].x, part->pin[pin_index2].y );
										c->vtx[nsegs].utility2 =  1;	// moved

									}
								}
							}
						}
					}
				}
			}
		}
		part = m_Doc->m_plist->GetNextPart( part );
	}

	// get selected segments
	cnet * net = m_Doc->m_nlist->GetFirstNet();
	while( net != NULL )
	{
		if( net->utility )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->utility )
				{
					// undraw entire trace
					m_Doc->m_nlist->UndrawConnection( net, ic );
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].utility )
						{
							// move trace segment by flagging adjacent vertices
							cseg * s = &c->seg[is];				// this segment
							cvertex * pre_v = &c->vtx[is];		// pre vertex
							cvertex * post_v = &c->vtx[is+1];	// post vertex
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
								if( !c->seg[is-1].utility )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is-1 );
							}
							if( is < c->nsegs-1 )
							{
								// test for following segment and not end of stub trace
								if( !c->seg[is+1].utility && (part2 || is < c->nsegs-2) )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is+1 );
							}
						}
					}
					// now move vertices
					for( int iv=0; iv<=c->nsegs; iv++ )
					{
						cvertex * v = &c->vtx[iv];
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
								cseg * pre_s = &c->seg[iv-1];
								if( pre_s->utility == 0 )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv-1 );
							}
							if( iv<c->nsegs )
							{
								cseg * post_s = &c->seg[iv];
								if( post_s->utility == 0 )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, iv );
							}
						}
					}

					// now some special cases
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].utility )
						{
							// move trace segment
							cseg * s = &c->seg[is];				// this segment
							cvertex * pre_v = &c->vtx[is];		// pre vertex
							cvertex * post_v = &c->vtx[is+1];	// post vertex
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;

							// special case, first segment of trace selected but part not selected
							if( part1->utility == FALSE && is == 0 )
							{
								// insert ratline as new first segment, unselected
								CPoint new_v_pt( pre_v->x, pre_v->y );
								CPoint old_v_pt = m_Doc->m_plist->GetPinPoint( part1, net->pin[c->start_pin].pin_name );		// pre vertex coords
								m_Doc->m_nlist->MoveVertex( net, ic, 0, old_v_pt.x, old_v_pt.y );
								m_Doc->m_nlist->InsertSegment( net, ic, 0, new_v_pt.x, new_v_pt.y, LAY_RAT_LINE, CConnectionWidthInfo(1, 0, 0), 0 );
								c->seg[0].utility = 0;
								c->vtx[0].utility = 0;
								is++;
							}

							// special case, last segment of trace selected but part not selected
							if( part2 )
							{
								if( part2->utility == FALSE && is == c->nsegs-1 )
								{
									// insert ratline as new last segment
									CConnectionWidthInfo old_width( c->seg[c->nsegs-1].seg_width );
									old_width.m_via_width = c->vtx[c->nsegs-1].via_width.m_via_width;
									old_width.m_via_hole  = c->vtx[c->nsegs-1].via_width.m_via_hole;

									CClearanceInfo old_clearance( c->seg[c->nsegs-1].seg_clearance );
									int old_layer = c->seg[c->nsegs-1].layer;

									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->nsegs-1 );
									CPoint new_v_pt( c->vtx[c->nsegs].x, c->vtx[c->nsegs].y );
									CPoint old_v_pt = m_Doc->m_plist->GetPinPoint( part2, net->pin[c->end_pin].pin_name );
									m_Doc->m_nlist->MoveVertex( net, ic, c->nsegs, old_v_pt.x, old_v_pt.y );
									BOOL bInserted = m_Doc->m_nlist->InsertSegment( net, ic, c->nsegs-1,
										new_v_pt.x, new_v_pt.y, old_layer, old_width, old_clearance, 0 );
									c->seg[c->nsegs-2].utility = 1;
									c->seg[c->nsegs-1].utility = 0;
								}
							}
						}
					}
					m_Doc->m_nlist->MergeUnroutedSegments( net, ic );	// this also redraws connection
				}
			}

			// now deal with tees that have been moved
			// requiring that stubs attached to tees have to move as well
			// if attached segments have not been selected, they must be unrouted
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->end_pin == cconnect::NO_END )
				{
					cvertex * end_vtx = &c->vtx[c->nsegs];
					cseg * end_seg = &c->seg[c->nsegs-1];
					if( int id = end_vtx->tee_ID )
					{
						// stub tee
						int tee_ic;
						int tee_iv;
						BOOL bFound = m_Doc->m_nlist->FindTeeVertexInNet( net, id, &tee_ic, &tee_iv );
						if ( !bFound )
						{
							end_vtx->tee_ID = 0;
						}
						else
						{
							cvertex * tee_vtx;
							tee_vtx = &net->connect[tee_ic].vtx[tee_iv];
							if( tee_vtx->utility2 )
							{
								// tee-vertex was moved
								end_vtx->x = tee_vtx->x;
								end_vtx->y = tee_vtx->y;
								if( !end_seg->utility )
								{
									// attached segment not selected
									m_Doc->m_nlist->UndrawConnection( net, ic );
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->nsegs-1 );
									m_Doc->m_nlist->DrawConnection( net, ic );
								}
							}
						}
					}
				}
			}
		}
		net = m_Doc->m_nlist->GetNextNet();
	}

	// merge unrouted segments for all traces
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.i;
			m_Doc->m_nlist->MergeUnroutedSegments( net, ic );
		}
	}

	//** this shouldn't be necessary
	CNetList * nl = m_Doc->m_nlist;
	for( cnet * n=nl->GetFirstNet(); n; n=nl->GetNextNet() )
		nl->RehookPartsToNet( n );
	//**
	m_Doc->m_nlist->SetAreaConnections();

	// regenerate selection list from utility flags
	// first, remove all segments and vertices
	for( int i=m_sel_ids.GetSize()-1; i>=0; i-- )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_VERTEX )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
	}
	// add segments and vertices back into group
	net = m_Doc->m_nlist->GetFirstNet();
	while( net )
	{
		if( net->utility )
		{
			// selected net
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->utility )
				{
					// selected connection
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].utility )
						{
							m_sel_ptrs.Add( net );
							id sid( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, is );
							m_sel_ids.Add( sid );
							c->seg[is].dl_el->visible = 1;	// restore visibility
						}
					}
					for( int iv=0; iv<c->nsegs+1; iv++ )
					{
						if( c->vtx[iv].utility )
						{
							if( c->vtx[iv].viaExists() || c->vtx[iv].tee_ID )
							{
								m_sel_ptrs.Add( net );
								id vid( ID_NET, ID_CONNECT, ic, ID_SEL_VERTEX, iv );
								m_sel_ids.Add( vid );
								if( c->vtx[iv].viaExists() )
								{
									int n_el = c->vtx[iv].dl_el.GetSize();
									for( int il=0; il<n_el; il++ )
										c->vtx[iv].dl_el[il]->visible = 1;
								}
							}
						}
					}
				}
			}
		}
		net = m_Doc->m_nlist->GetNextNet();
	}
}

void CFreePcbView::FindGroupCenter()
{
	int groupNumberItems=0;
	groupAverageX=groupAverageY=0;

	// find parts
	if( m_sel_mask & (1<<SEL_MASK_PARTS ) )  // may not be necessary??
	{
		cpart * part = m_Doc->m_plist->GetFirstPart();
		while( part )
		{
			id pid( ID_PART, ID_SEL_RECT, 0, 0, 0 );
			if( FindItemInGroup( part, &pid ) != -1 )
			{
				groupAverageX+=part->x;
				groupAverageY+=part->y;
				groupNumberItems++;
			}
			part = m_Doc->m_plist->GetNextPart( part );
		}
	}

	// find trace segments and vertices contained in rect
	if( m_sel_mask & (1<<SEL_MASK_CON ) )
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				for( int is=0; is<c->nsegs; is++ )
				{
					cvertex * pre_v = &c->vtx[is];
					cvertex * post_v = &c->vtx[is+1];
					cseg * s = &c->seg[is];

					if( m_Doc->m_vis[s->layer] )
					{
						// add segment to selection list and highlight it
						id sid( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, is );
						if( FindItemInGroup( net, &sid ) != -1 )
						{
							groupAverageX+=pre_v->x+post_v->x;
							groupAverageY+=pre_v->y+post_v->y;
							groupNumberItems+=2;
						}
					}
				}
			}
			net = m_Doc->m_nlist->GetNextNet();
		}
	}

	// find texts in group
	if( m_sel_mask & (1<<SEL_MASK_TEXT ) )
	{
		CText * t = m_Doc->m_tlist->GetFirstText();
		while( t )
		{
			if( m_Doc->m_vis[t->m_layer] )
			{
				// add text to selection list and highlight it
				id sid( ID_TEXT, ID_SEL_TXT, 0, 0, 0 );
				if( FindItemInGroup( t, &sid ) != -1 )
				{
					groupAverageX+=m_dlist->Get_x( t->dl_sel );
					groupAverageY+=m_dlist->Get_y( t->dl_sel );
					groupNumberItems++;
				}
			}
			t = m_Doc->m_tlist->GetNextText();
		}
	}

	// find copper area sides in rect
	if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			if( net->nareas )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * poly = a->poly;
					for( int ic=0; ic<poly->GetNumContours(); ic++ )
					{
						int istart = poly->GetContourStart(ic);
						int iend = poly->GetContourEnd(ic);
						for( int is=istart; is<=iend; is++ )
						{
							int ic1, ic2;
							ic1 = is;
							if( is < iend )
								ic2 = is+1;
							else
								ic2 = istart;
							int x1 = poly->GetX(ic1);
							int y1 = poly->GetY(ic1);
							int x2 = poly->GetX(ic2);
							int y2 = poly->GetY(ic2);
							if( m_Doc->m_vis[poly->GetLayer()] )
							{
								id aid( ID_NET, ID_AREA, ia, ID_SEL_SIDE, is );
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
			net = m_Doc->m_nlist->GetNextNet();
		}
	}

	// find solder mask cutout sides in rect
	if( m_sel_mask & (1<<SEL_MASK_SM ) )
	{
		for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[im];
			for( int ic=0; ic<poly->GetNumContours(); ic++ )
			{
				int istart = poly->GetContourStart(ic);
				int iend = poly->GetContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->GetX(ic1);
					int y1 = poly->GetY(ic1);
					int x2 = poly->GetX(ic2);
					int y2 = poly->GetY(ic2);
					if( m_Doc->m_vis[poly->GetLayer()] )
					{
						id smid( ID_SM_CUTOUT, ID_SM_CUTOUT, im, ID_SEL_SIDE, is );
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
		for( int im=0; im<m_Doc->m_board_outline.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_board_outline[im];
			for( int ic=0; ic<poly->GetNumContours(); ic++ )
			{
				int istart = poly->GetContourStart(ic);
				int iend = poly->GetContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->GetX(ic1);
					int y1 = poly->GetY(ic1);
					int x2 = poly->GetX(ic2);
					int y2 = poly->GetY(ic2);
					if( m_Doc->m_vis[poly->GetLayer()] )
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

		double x=floor(groupAverageX/m_Doc->m_part_grid_spacing +.5);
		groupAverageX=(long long)(m_Doc->m_part_grid_spacing*x);
		x=floor(groupAverageY/m_Doc->m_part_grid_spacing +.5);
		groupAverageY=(long long)(m_Doc->m_part_grid_spacing*x);
	}
}


// save undo info for part, prior to editing operation
// type may be:
//	UNDO_PART_DELETE	if part will be deleted
//	UNDO_PART_MODIFY	if part will be modified (e.g. moved)
//	UNDO_PART_ADD		if part will be added
// for UNDO_PART_ADD, use reference designator to identify part, ignore cpart * part
// on callback, ref_des will be used to find part, then name will be changed to part->ref_des
//
void CFreePcbView::SaveUndoInfoForPart( cpart * part, int type, CString * ref_des, BOOL new_event, CUndoList * list )
{
	undo_part * u_part;
	if( new_event )
		list->NewEvent();
	if( type == CPartList::UNDO_PART_ADD )
		u_part = m_Doc->m_plist->CreatePartUndoRecord( NULL, ref_des );
	else if( ref_des )
		u_part = m_Doc->m_plist->CreatePartUndoRecord( part, ref_des );
	else
		u_part = m_Doc->m_plist->CreatePartUndoRecord( part, &part->ref_des );

	list->Push( type, u_part, &m_Doc->m_plist->PartUndoCallback, u_part->size );

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
}

// save undo info for a part and all nets connected to it
// type may be:
//	UNDO_PART_DELETE	if part will be deleted
//	UNDO_PART_MODIFY	if part will be modified (e.g. moved or ref_des changed)
// note that the ref_des may be different than the part->ref_des
// on callback, ref_des will be used to find part, then name will be changed to part->ref_des
//
void CFreePcbView::SaveUndoInfoForPartAndNets( cpart * part, int type, CString * ref_des, BOOL new_event, CUndoList * list )
{
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
}

// save undo info for a net (not connections or areas)
//
void CFreePcbView::SaveUndoInfoForNet( cnet * net, int type, BOOL new_event, CUndoList * list )
{
	void * ptr;
	if( new_event )
		list->NewEvent();
	undo_net * u_net = m_Doc->m_nlist->CreateNetUndoRecord( net );
	list->Push( type, u_net, &m_Doc->m_nlist->NetUndoCallback, u_net->size );
}

// save undo info for a net and connections, not areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnections( cnet * net, int type, BOOL new_event, CUndoList * list )
{
	void * ptr;
	if( new_event )
		list->NewEvent();
	if( type != CNetList::UNDO_NET_ADD )
		for( int ic=net->nconnects-1; ic>=0; ic-- )
		{
			cconnect * c = &net->connect[ic];
			SaveUndoInfoForConnection( net, ic, FALSE, list );
		}
	SaveUndoInfoForNet( net, type, FALSE, list );
	if( new_event )
	{
		ptr = CreateUndoDescriptor( list, type, &net->name, NULL, 0, 0, NULL, NULL );
		list->Push( UNDO_NET_AND_CONNECTIONS, ptr, &UndoCallback );
	}
}

// save undo info for a connection
//
void CFreePcbView::SaveUndoInfoForConnection( cnet * net, int ic, BOOL new_event, CUndoList * list )
{
	if( new_event )
		list->NewEvent();
	undo_con * u_con = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
	list->Push( CNetList::UNDO_CONNECT_MODIFY, u_con,
		&m_Doc->m_nlist->ConnectUndoCallback, u_con->size );
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
	undo_descriptor * u_d = (undo_descriptor*)ptr;
	if( undo )
	{
		CFreePcbView * view = u_d->view;
		// if callback was from undo_list, push info to redo list, and vice versa
		CUndoList * redo_list;
		if( u_d->list == view->m_Doc->m_undo_list )
			redo_list = view->m_Doc->m_redo_list;
		else
			redo_list = view->m_Doc->m_undo_list;
		undo_text * u_text = (undo_text *)u_d->ptr;
		// save undo/redo info
		if( type == UNDO_PART )
		{
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->str1 );	//use new ref des
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
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->str1 );
			if(u_d->type == CPartList::UNDO_PART_DELETE )
				view->SaveUndoInfoForPart( NULL, CPartList::UNDO_PART_ADD, &u_d->name1, TRUE, redo_list );
			else if( u_d->type == CPartList::UNDO_PART_MODIFY )
				view->SaveUndoInfoForPartAndNets( part, CPartList::UNDO_PART_MODIFY, &u_d->name1, TRUE, redo_list );
		}
		else if( type == UNDO_2_PARTS_AND_NETS )
		{
			cpart * part = view->m_Doc->m_plist->GetPart( u_d->name1 );
			cpart * part2 = view->m_Doc->m_plist->GetPart( u_d->name2 );
			view->SaveUndoInfoFor2PartsAndNets( part, part2, TRUE, redo_list );
		}
		else if( type == UNDO_NET_AND_CONNECTIONS )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			view->SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, TRUE, redo_list );
		}
		else if( type == UNDO_AREA )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			if( u_d->type == CNetList::UNDO_AREA_ADD )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_DELETE, TRUE, redo_list );
			else if( u_d->type == CNetList::UNDO_AREA_DELETE )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_ADD, TRUE, redo_list );
			else if( type == UNDO_AREA )
				view->SaveUndoInfoForArea( net, u_d->int1, CNetList::UNDO_AREA_MODIFY, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_AREAS_IN_NET )
		{
			cnet * net = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			view->SaveUndoInfoForAllAreasInNet( net, TRUE, redo_list );
		}
		else if( type == UNDO_ALL_AREAS_IN_2_NETS )
		{
			cnet * net1 = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name1 );
			cnet * net2 = view->m_Doc->m_nlist->GetNetPtrByName( &u_d->name2 );
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
				GUID guid = u_text->m_guid;
				CText * text = view->m_Doc->m_tlist->GetFirstText();
				while( text && text->m_guid != guid )
				{
					text = view->m_Doc->m_tlist->GetNextText();
				}
				if( !text )
					ASSERT(0);	// guid not found
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
}

// callback for undoing group operations
// note this is a static function (i.e. global)
//
void CFreePcbView::UndoGroupCallback( int type, void * ptr, BOOL undo )
{
	undo_group_descriptor * u_d = (undo_group_descriptor*)ptr;
	if( undo )
	{
		CFreePcbView * view = u_d->view;
		CFreePcbDoc * doc = view->m_Doc;
		// if callback was from undo_list, push info to redo list, and vice versa
		CUndoList * redo_list;
		if( u_d->list == view->m_Doc->m_undo_list )
			redo_list = view->m_Doc->m_redo_list;
		else
			redo_list = view->m_Doc->m_undo_list;
		if( u_d->type == UNDO_GROUP_MODIFY || u_d->type == UNDO_GROUP_ADD )
		{
			// reconstruct pointers from names of items (since they may have changed)
			// and save the current status of the group
			int n_items = u_d->m_id.GetSize();
			CArray<void*> ptrs;
			ptrs.SetSize( n_items );
			for( int i=0; i<n_items; i++ )
			{
				CString * str_ptr = &u_d->str[i];
				id this_id = u_d->m_id[i];
				if( this_id.type == ID_PART )
				{
					cpart * part = doc->m_plist->GetPart( *str_ptr );
					if( part )
						ptrs[i] = (void*)part;
					else
						ASSERT(0);	// couldn't find part
				}
				else if( this_id.type == ID_NET )
				{
					cnet * net = doc->m_nlist->GetNetPtrByName( str_ptr );
					if( net )
						ptrs[i] = (void*)net;
					else
						ASSERT(0);	// couldn't find net
				}
				else if( this_id.type == ID_TEXT )
				{
					GUID guid;
					::SetGuidFromString( str_ptr, &guid );
					CText * text = doc->m_tlist->GetText( &guid );
					if( text )
						ptrs[i] = (void*)text;
					else
						ASSERT(0);	// couldn't find text
				}
			}
			if( u_d->type == UNDO_GROUP_MODIFY )
				view->SaveUndoInfoForGroup( u_d->type, &ptrs, &u_d->m_id, redo_list );
			else if( u_d->type == UNDO_GROUP_ADD )
			{
				// delete group
				view->DeleteGroup( &ptrs, &u_d->m_id );
			}
		}
		else if( u_d->type == UNDO_GROUP_DELETE )
		{
			// just copy the undo record with type UNDO_GROUP_ADD
			undo_group_descriptor * new_u_d = new undo_group_descriptor;
			new_u_d->list = redo_list;
			new_u_d->type = UNDO_GROUP_ADD;
			new_u_d->view = u_d->view;
			int n_items = u_d->m_id.GetSize();
			new_u_d->str.SetSize( n_items );
			new_u_d->m_id.SetSize( n_items );
			for( int i=0; i<n_items; i++ )
			{
				new_u_d->m_id[i] = u_d->m_id[i];
				new_u_d->str[i] = u_d->str[i];
			}
			redo_list->NewEvent();
			redo_list->Push( UNDO_GROUP, (void*)new_u_d, &view->UndoGroupCallback );
		}
	}
	delete(u_d);	// delete the undo record
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
	undo->m_id.SetSize( n_items );
	for( int i=0; i<n_items; i++ )
	{
		id this_id = (*ids)[i];
		undo->m_id[i] = this_id;
		if( this_id.type == ID_PART )
		{
			cpart * part = (cpart*)(*ptrs)[i];
			undo->str[i] = part->ref_des;
		}
		else if( this_id.type == ID_NET )
		{
			cnet * net = (cnet*)(*ptrs)[i];
			undo->str[i] = net->name;
		}
		else if( this_id.type == ID_TEXT )
		{
			CText * text = (CText*)(*ptrs)[i];
			::GetStringFromGuid( &text->m_guid, &undo->str[i] );
		}
	}
	return undo;
}


void CFreePcbView::OnGroupRotate()
{
	m_dlist->CancelHighLight();
	if( !gLastKeyWasArrow && !gLastKeyWasGroupRotate)
	{
		if( GluedPartsInGroup() )
		{
			int ret = AfxMessageBox( "This group contains glued parts, unglue and rotate them ?  ", MB_YESNO );
			if( ret != IDYES )
				return;
		}
		SaveUndoInfoForGroup( UNDO_GROUP_MODIFY, &m_sel_ptrs, &m_sel_ids, m_Doc->m_undo_list );
		gLastKeyWasGroupRotate=true;
	}
	RotateGroup( );
	HighlightGroup();
	m_Doc->ProjectModified( TRUE );
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
		if( m_Doc->m_project_modified )
			m_Doc->ProjectModified( TRUE, FALSE );
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
	CDlgSideStyle dlg;
	int style = m_sel_net->area[m_sel_ia].poly->GetSideStyle( m_sel_id.ii );
	dlg.Initialize( style );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY, TRUE, m_Doc->m_undo_list );
		m_dlist->CancelHighLight();
		m_sel_net->area[m_sel_ia].poly->SetSideStyle( m_sel_id.ii, dlg.m_style );
		m_Doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_id.ii );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
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
	m_Doc->m_plist->StartDraggingValue( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_VALUE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnValueProperties()
{
	CDlgValueText dlg;
	dlg.Initialize( m_Doc->m_plist, m_sel_part );
	int ret =  dlg.DoModal();
	if( ret == IDOK )
	{
		// edit this part
		SaveUndoInfoForPart( m_sel_part,
			CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
		m_Doc->m_plist->SetValue( m_sel_part, &m_sel_part->value,
			m_sel_part->m_value_xi, m_sel_part->m_value_yi,
			m_sel_part->m_value_angle,
			dlg.m_height, dlg.m_width, dlg.m_vis );
		m_Doc->ProjectModified( TRUE );
		m_dlist->CancelHighLight();
		if( m_cursor_mode == CUR_PART_SELECTED )
			m_Doc->m_plist->SelectPart( m_sel_part );
		else if( m_cursor_mode == CUR_VALUE_SELECTED
			&& m_sel_part->m_value_size && m_sel_part->m_value_vis )
			m_Doc->m_plist->SelectValueText( m_sel_part );
		else
			CancelSelection();
		Invalidate( FALSE );
	}
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
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_ref_angle = (m_sel_part->m_ref_angle + 90)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectRefText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnRefRotateCCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_ref_angle = (m_sel_part->m_ref_angle + 270)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectRefText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnValueRotateCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_value_angle = (m_sel_part->m_value_angle + 90)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectValueText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnValueRotateCCW()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, NULL, TRUE, m_Doc->m_undo_list );
	m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->m_value_angle = (m_sel_part->m_value_angle + 270)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_plist->SelectValueText( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}


void CFreePcbView::OnSegmentMove()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	id id = m_sel_id;
	int ic = m_sel_id.i;
	int ivtx = m_sel_id.ii;
	m_dragging_new_item = 0;

	m_last_pt.x = m_sel_last_vtx.x;
	m_last_pt.y = m_sel_last_vtx.y;

	m_from_pt.x = m_sel_vtx.x;
	m_from_pt.y = m_sel_vtx.y;

	m_to_pt.x = m_sel_next_vtx.x;
	m_to_pt.y = m_sel_next_vtx.y;

	int nsegs = m_sel_con.nsegs;
	int use_third_segment = ivtx < nsegs - 1;
	if(use_third_segment)
	{
		m_next_pt.x = m_sel_next_next_vtx.x;	// Shouldn't really do this if we're off the edge?
		m_next_pt.y = m_sel_next_next_vtx.y;
	}

	CPoint p;
//	p = m_last_mouse_point;

	p.x = (m_to_pt.x - m_from_pt.x) / 2 + m_from_pt.x;
	p.y = (m_to_pt.y - m_from_pt.y) / 2 + m_from_pt.y;


	m_Doc->m_nlist->StartMovingSegment( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2, use_third_segment );
	SetCursorMode( CUR_MOVE_SEGMENT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}
