// Shape.cpp : implementation of CShape class.  Also includes classes CPad, stroke, CPadstack, CCentroid and CGlue (the
// last 3 descendants of CPcbItem)
//
#include "stdafx.h"
#include "afx.h"
#include "SMFontUtil.h"
#include "shape.h"
#include <math.h> 

#define NO_MM	// this restores backward compatibility for project files

// utility function make strings for dimensions
//
CString ws( int n, int units )
{
	CString str;
	MakeCStringFromDimension( &str, n, units, FALSE, FALSE, FALSE, 6 ); 
	return str;
}
 

// class CPad  (Renamed by CPT2, was "pad")
CPad::CPad()
{
	shape = PAD_NONE;
	size_l = size_r = size_h = radius = 0;
	connect_flag = 0;
	dl_el = NULL;
}

BOOL CPad::operator==(CPad p)
{ 
	return shape==p.shape 
			&& size_l==p.size_l 
			&& size_r==p.size_r
			&& size_h==p.size_h
			&& (shape!=PAD_RRECT || radius==p.radius)
			&& connect_flag == p.connect_flag;
}

// class CPadstack  (Renamed by CPT2, was "padstack")
CPadstack::CPadstack(CShape *_shape)
	: CPcbItem (_shape->doc)
{ 
	shape = _shape;
	shape->m_padstacks.Add(this);
	exists = false;
	top.shape = PAD_NONE;
	top_mask.shape = PAD_DEFAULT;
	top_paste.shape = PAD_DEFAULT;
	inner.shape = PAD_NONE;
	bottom.shape = PAD_NONE;
	bottom_mask.shape = PAD_DEFAULT;
	bottom_paste.shape = PAD_DEFAULT;
	dl_el = dl_sel = top.dl_el = inner.dl_el = bottom.dl_el = top_mask.dl_el = 
		top_paste.dl_el = bottom_mask.dl_el = bottom_paste.dl_el = NULL;
}

CPadstack::CPadstack( CPadstack *src, CShape *_shape )
	: CPcbItem (src->doc)
{ 
	Copy(src); 
	shape = _shape;
	shape->m_padstacks.Add(this);
}

CPadstack::CPadstack(CFreePcbDoc *_doc, int _uid)
	: CPcbItem (_doc, _uid)
{ 
	shape = NULL;
	dl_el = dl_sel = top.dl_el = inner.dl_el = bottom.dl_el = top_mask.dl_el = 
		top_paste.dl_el = bottom_mask.dl_el = bottom_paste.dl_el = NULL;
}


bool CPadstack::SameAs( CPadstack *p )
{ 
	return( name == p->name
			&& angle==p->angle 
			&& hole_size==p->hole_size 
			&& x_rel==p->x_rel 
			&& y_rel==p->y_rel
			&& top==p->top
			&& top_mask==p->top_mask
			&& top_paste==p->top_paste
			&& bottom==p->bottom
			&& bottom_mask==p->bottom_mask
			&& bottom_paste==p->bottom_paste
			&& inner==p->inner				
			); 
}

void CPadstack::Highlight()
{
	// select it by making its selection rectangle visible
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !dl_sel) return;
	dl->Highlight( DL_RECT_X, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 
		1 );
}


// Get bounding rectangle of pad
//
CRect CPadstack::GetBounds()
{
	CRect r;
	int dx=0, dy=0;
	CPad * p = &top;
	if( top.shape == PAD_NONE && bottom.shape != PAD_NONE )
		p = &bottom;
	if( p->shape == PAD_NONE )
	{
		{
			dx = hole_size/2;
			dy = dx;
		}
	}
	else if( p->shape == PAD_SQUARE || p->shape == PAD_ROUND || p->shape == PAD_OCTAGON )
	{
		dx = p->size_h/2;
		dy = dx;
	}
	else if( p->shape == PAD_RECT || p->shape == PAD_RRECT || p->shape == PAD_OVAL )
	{
		if( angle == 0 || angle == 180 )
		{
			dx = p->size_l;
			dy = p->size_h/2;
		}
		else if( angle == 90 || angle == 270 )
		{
			dx = p->size_h/2;
			dy = p->size_l;
		}
		else
			ASSERT(0);	// illegal angle
	}
	r.left = x_rel - dx;
	r.right = x_rel + dx;
	r.bottom = y_rel - dy;
	r.top = y_rel + dy;
	return r;
}

bool CPadstack::IsOnPcb() 
	{ return shape->IsOnPcb() && shape->m_padstacks.Contains(this); }

void CPadstack::Copy( CPadstack *src, bool bCopyName )
{
	// Copies the padstack.  Doesn't touch the "shape" pointer (caller must deal with it)
	if (bCopyName)
		name = src->name;
	hole_size = src->hole_size;	
	x_rel = src->x_rel, y_rel = src->y_rel;
	angle = src->angle;
	exists = src->exists;
	top = src->top;
	top_mask = src->top_mask;
	top_paste = src->top_paste;
	bottom = src->bottom;
	bottom_mask = src->bottom_mask;
	bottom_paste = src->bottom_paste;
	inner = src->inner;
	dl_el = dl_sel = top.dl_el = inner.dl_el = bottom.dl_el = top_mask.dl_el = 
		top_paste.dl_el = bottom_mask.dl_el = bottom_paste.dl_el = NULL;
	utility = src->utility;
}

void CPadstack::SetVisible(bool bVis)
{
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible(top.dl_el, bVis);
	dl->Set_visible(top_mask.dl_el, bVis);
	dl->Set_visible(top_paste.dl_el, bVis);
	dl->Set_visible(inner.dl_el, bVis);
	dl->Set_visible(bottom.dl_el, bVis);
	dl->Set_visible(bottom_mask.dl_el, bVis);
	dl->Set_visible(bottom_paste.dl_el, bVis);
	dl->Set_visible(dl_el, bVis);						// This is the hole (if any)
}

//** AMW3 returns the copper layer of the padstack
//** if side = 0, assumes footprint is on top of PCB, if side == 1, on bottom
//** possible return values are LAY_PAD_THRU, LAY_TOP_COPPER, LAY_BOTTOM_COPPER
//
int CPadstack::CopperLayer( int side )
{
	if( hole_size )
		return LAY_PAD_THRU;
	else if( top.shape != PAD_NONE )
		return FlipLayer( side, LAY_TOP_COPPER );
	else if( bottom.shape != PAD_NONE )
		return FlipLayer( side, LAY_BOTTOM_COPPER );
	else
		ASSERT(0);	// oops
}


CCentroid::CCentroid(CShape *s)
	: CPcbItem (s->doc)
{
	m_shape = s;
	m_shape->m_centroid = this; 
}

bool CCentroid::IsOnPcb()
	{ return m_shape->IsOnPcb() && m_shape->m_centroid == this; }

int CCentroid::Draw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	int axis_offset_x = 0;
	int axis_offset_y = 0;
	if( m_angle == 0 )
		axis_offset_x = CENTROID_WIDTH;
	else if( m_angle == 90 )
		axis_offset_y = CENTROID_WIDTH;
	else if( m_angle == 180 )
		axis_offset_x = -CENTROID_WIDTH;
	else if( m_angle == 270 )
		axis_offset_y = -CENTROID_WIDTH;
	dl_el = dl->AddMain( this, LAY_FP_CENTROID, DL_CENTROID, TRUE, 
		CENTROID_WIDTH, 0, 0, m_x, m_y, 
		m_x+axis_offset_x, m_y + axis_offset_y, 0, 0, 0 ); 
	dl_sel = dl->AddSelector( this, LAY_FP_CENTROID, DL_HOLLOW_RECT, TRUE, 0, 0, 
		m_x-CENTROID_WIDTH/2, m_y-CENTROID_WIDTH/2, m_x+CENTROID_WIDTH/2, m_y+CENTROID_WIDTH/2, 0, 0, 0 );
	bDrawn = true;
	return NOERR;
}

void CCentroid::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !dl_sel) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 1 );
}

void CCentroid::StartDragging( CDC * pDC )
{
	// CPT2 Derived from old CEditShape::StartDraggingCentroid()
	CDisplayList *dl = doc->m_dlist;
	// make centroid invisible
	dl->Set_visible( dl_el, 0 );
	dl->CancelHighlight();
	dl->StartDraggingRectangle( pDC, m_x, m_y,
						-CENTROID_WIDTH/2, -CENTROID_WIDTH/2,
						CENTROID_WIDTH/2, CENTROID_WIDTH/2,
						0, LAY_FP_SELECTION );
}

// Cancel dragging centroid
//
void CCentroid::CancelDragging()
{
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible( dl_el, 1 );
	// stop dragging
	dl->StopDragging();
}

void CCentroid::SetVisible(bool bVis)
{
	if (dl_el)
		dl_el->visible = bVis;
}


CGlue::CGlue(CShape *_shape, GLUE_POS_TYPE _type, int _w, int _x, int _y) 
	: CPcbItem (_shape->doc)
{ 
	type = _type; 
	w = _w; x = _x; y = _y; 
	shape = _shape; 
	shape->m_glues.Add(this);
}

CGlue::CGlue(CGlue *src, CShape *_shape )
	: CPcbItem (src->doc)
{ 
	type = src->type; 
	w = src->w; x = src->x; y = src->y;
	utility = src->utility;
	shape = _shape;
	shape->m_glues.Add(this);
}

bool CGlue::IsOnPcb()
	{ return shape->IsOnPcb() && shape->m_glues.Contains(this); }

int CGlue::Draw()
{
	// Draw the glue.  Note that this routine is responsible for placing centroid-positioned glues properly.
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	int w0 = w;
	if( w0 == 0 )
		w0 = DEFAULT_GLUE_WIDTH;
	int x0 = x, y0 = y;
	if (type==GLUE_POS_CENTROID)
	{
		CCentroid *c = doc->m_edit_footprint->m_centroid;
		x0 = c->m_x, y0 = c->m_y;
	}
	dl_el = dl->AddMain( this, LAY_FP_DOT, DL_CIRC, TRUE, w0, 0, 0, x0, y0, 0, 0, 0, 0 );
	dl_sel = dl->AddSelector( this, LAY_FP_DOT, DL_HOLLOW_RECT, TRUE, 0, 0, 
		x0 - w0/2, y0 - w0/2, x0 + w0/2, y0 + w0/2, 0, 0, 0 );
	bDrawn = true;
	return NOERR;
}


void CGlue::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !dl_sel) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 1 );
}


// Start dragging glue spot
//
void CGlue::StartDragging( CDC * pDC )
{
	// make glue spot invisible
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible( dl_el, 0 );
	dl->CancelHighlight();
	int w0 = w;
	if( w0 == 0 )
		w0 = DEFAULT_GLUE_WIDTH;
	dl->StartDraggingRectangle( pDC, x, y, -w0/2, -w0/2, w0/2, w0/2, 0, LAY_FP_SELECTION );
}

// Cancel dragging glue spot
//
void CGlue::CancelDragging()
{
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible( dl_el, 1 );
	// stop dragging
	dl->StopDragging();
}

void CGlue::SetVisible(bool bVis)
{
	if (dl_el)
		dl_el->visible = bVis;
}

// class CShape
// this constructor creates an empty shape
// CPT2 converted
//
CShape::CShape(CFreePcbDoc *_doc)
	: CPcbItem (_doc)
{
	m_tl = NULL;	//** AMW3 see Clear();
	Clear();
} 

CShape::CShape( CShape *src )
	: CPcbItem (src->doc)
{
	m_tl = NULL;	//** AMW3 see Clear();
	Clear();
	Copy(src);
}

CShape::CShape(CFreePcbDoc *_doc, CString *_name)
	: CPcbItem (_doc)
{
	m_tl = NULL;	//** AMW3 see Clear();
	Clear();
	m_name = *_name;
} 

CShape::CShape(CFreePcbDoc *_doc, int uid)
	: CPcbItem (_doc, uid)
{
	m_ref = NULL;
	m_value = NULL;
	m_tl = new CTextList(doc);
	m_centroid = NULL;
}

CShape::~CShape()
{
	delete m_tl;
}


void CShape::Clear()
{
	// CPT2 converted
	CString strRef ("REF");
	m_ref = new CRefText(doc, 100*NM_PER_MIL, 200*NM_PER_MIL, 0, false, false, LAY_FP_SILK_TOP, 100*NM_PER_MIL, 10*NM_PER_MIL, 0, &strRef, true);
	m_ref->m_shape = this;
	CString strValue ("VALUE");
	m_value = new CValueText(doc, 100*NM_PER_MIL, 0*NM_PER_MIL, 0, false, false, LAY_FP_SILK_TOP, 100*NM_PER_MIL, 10*NM_PER_MIL, 0, &strValue, true);
	m_value->m_shape = this;
	if( m_tl )		//** AMW3 fixes memory leak
		delete m_tl;
	m_tl = new CTextList(doc);
	m_centroid = new CCentroid(this);
	m_author = "";
	m_source = "";
	m_desc = "";
	m_name = "EMPTY_SHAPE";
	m_units = MIL;
	m_sel_xi = m_sel_yi = 0;
	m_sel_xf = m_sel_yf = 500*NM_PER_MIL;
	m_centroid->m_type = CENTROID_DEFAULT;
	m_centroid->m_x = 0;
	m_centroid->m_y = 0;
	m_centroid->m_angle = 0;
	m_padstacks.RemoveAll();
	m_outlines.RemoveAll();
	m_tl->texts.RemoveAll();
	m_glues.RemoveAll();
}

void CShape::MakeDummy()
{
	// CPT2 experimental.  Create a dummy shape --- will be used when importing shapes without their own proper f.p.
	// At this point, just a circle outline.
	Clear();
	COutline *o = new COutline(this, LAY_FP_SILK_TOP, 10*NM_PER_MIL);
	CContour *c = new CContour(o, true);
	c->AppendCorner(0, 50*NM_PER_MIL);
	c->AppendCorner(50*NM_PER_MIL, 100*NM_PER_MIL, CPolyline::ARC_CW);
	c->AppendCorner(100*NM_PER_MIL, 50*NM_PER_MIL, CPolyline::ARC_CW);
	c->AppendCorner(50*NM_PER_MIL, 0, CPolyline::ARC_CW);
	c->Close(CPolyline::ARC_CW);
	m_name = "Dummy";
	m_sel_xi = m_sel_yi = 0;
	m_sel_xf = m_sel_yf = 100*NM_PER_MIL;
	m_ref->m_y = 110*NM_PER_MIL;
}

void CShape::MarkConstituents(int util)
{
	utility = util;
	m_ref->utility = util;
	m_value->utility = util;
	m_centroid->utility = util;
	m_padstacks.SetUtility(util);
	CIter<COutline> io (&m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
		o->MarkConstituents(util);
	m_tl->texts.SetUtility(util);
	m_glues.SetUtility(util);
}

bool CShape::IsOnPcb()
	// A shape is "on the pcb" if it's either the current focus of the fp editor, or if it's in the local cache (doc->m_slist).  Analogously 
	// for footprint items like centroids within a shape.
	{ return this == doc->m_edit_footprint || doc->m_slist->shapes.Contains(this); }


void CShape::SaveUndoInfo()
{
	if (bUndoInfoSaved) return;
	bUndoInfoSaved = true;
	doc->m_undo_items.Add( new CUShape(this) );
	CIter<CPadstack> ips (&m_padstacks);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
		ps->SaveUndoInfo();
	m_ref->SaveUndoInfo();
	m_value->SaveUndoInfo();
	CIter<CText> it (&m_tl->texts);
	for (CText *t = it.First(); t; t = it.Next())
		t->SaveUndoInfo();
	CIter<COutline> io (&m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
		o->SaveUndoInfo();
	CIter<CGlue> ig (&m_glues);
	for (CGlue *g = ig.First(); g; g = ig.Next())
		g->SaveUndoInfo();
	m_centroid->SaveUndoInfo();
}

// function to create shape from definition string
// returns 0 if successful
// CPT2 converted
//
int CShape::MakeFromString( CString name, CString str )
{
	enum {	
		MAX_PARAMS = 40,
		MAX_CHARS_PER_PARAM = 20
	};

	enum { P_UNDEFINED = -1 };

	// packages
	enum {
		HOLE,
		DIP,
		SIP,
		SOIC,
		PLCC,
		QFP,
		HDR,
		CHIP
	};

	// pin patterns
	enum {
		EDGE,
		GRID
	};

	// pin 1 position for EDGE patterns
	enum {
		P1BL,	// pin 1 at bottom-left corner
		P1BR,	// pin 1 at bottom-right corner	
		P1TL,	// pin 1 at top-left corner
		P1TR,	// pin 1 at top-right corner
		P1TC,	// pin 1 at top center
		P1LC,	// pin 1 at left center
		P1BC,	// pin 1 at bottom center
		P1RC	// pin 1 at right center
	};

	// pin numbering direction
	enum {
		// for EDGE pattern
		CCW,	// number pins counter-clockwise around edge of part
		CW,		// number pins clockwise around edge
		// for GRID pattern
		LRBT,	// number pins left->right, bottom->top
		LRTB,	// number pins left->right, top->bottom
		RLBT,	// number pins right->left, bottom->top
		RLTB,	// number pins right->left, top->bottom
		BTLR,	// number pins bottom->top, left->right
		TBLR,	// number pins top->bottom, left->right
		BTRL,	// number pins bottom->top, right->left
		TBRL 	// number pins top->bottom, right->left
	};

	// through-hole or SMT pads
	enum {
		TH,		// through-hole pads
		SMT		// surface-mount pads
	};

	// pad shape
	enum {
		NONE,
		RNDSQ1,		// pin 1 square, others round
		ROUND,		// round
		SQUARE,		// square
		RECT,		// rectangular
		RRECT,		// rounded-rectangle
		OVAL,		// oval,
		OCTAGON		// octagonal
	};

	// delete original shape info
	Clear();

	char param[MAX_PARAMS][MAX_CHARS_PER_PARAM];
	char * pstr;
	int np = 0;

	// parse string, separator is "_"
	pstr = mystrtok( str , "_" );
	while( pstr )
	{
		if( strlen( pstr ) < MAX_CHARS_PER_PARAM )
			strcpy( param[np++], pstr );
		else 
			return 1;
		if( np >= MAX_PARAMS )
			return 1;
		pstr = mystrtok( NULL , "_" );
	}

	// now parse parameters
	// defaults
	int package = P_UNDEFINED;			// package type, must be first param
	int npins = P_UNDEFINED;			// number of pins, must be second param
	int pattern = P_UNDEFINED;			// EDGE of GRID	
	int pin_dir = P_UNDEFINED;			// CW,CCW,RLTB, etc.
	int pin_start = P_UNDEFINED;		// (for EDGE only) P1BL, etc.	
	int npins_x = P_UNDEFINED;			// length of horizontal pin rows, or hor. grid size		
	int npins_y = P_UNDEFINED;			// length of vertical pin rows, or vert. grid size	
	int pin_spacing_x = P_UNDEFINED;	// spacing between pins in hor. rows	
	int pin_spacing_y = P_UNDEFINED;	// spacing between pins in vert. rows
	int offset_right = P_UNDEFINED;		// offset from part origin to right vert. row	
	int offset_top = P_UNDEFINED;		// offset from origin to top hor. row
	int offset_left = P_UNDEFINED;		// offset from origin to left vert. row
	int offset_bottom = P_UNDEFINED;	// offset from origin to bottom hor. row
	int pad_w = P_UNDEFINED;			// pad width
	int pad_len_int = P_UNDEFINED;		// pad length external to pin
	int pad_len_ext = P_UNDEFINED;		// pad length internal to pin
	int pad_radius = P_UNDEFINED;		// pad corner radius
	int ref_text_size = P_UNDEFINED;	// ref. text character height
	int pad_shape = P_UNDEFINED;		// pad shape (RNDSQ1, ROUND, RECT)
	int pad_hole_w = P_UNDEFINED;		// pad hole diameter, or 0 for SMT pad
	int units = MIL;
	
	// now loop through all substrings
	for( int ip=0; ip<np; ip++ )
	{	
		CString p = param[ip];	// param 
		CString p1 = "";		// param minus left-most char
		CString p2 = "";		// param minus left-most 2 characters
		int len = p.GetLength();
		if( len > 0 )
			p1 = p.Right( len-1 );
		if( len > 1 )
			p2 = p.Right( len-2 );

		// package type, must be first param
		if( ip == 0 )
		{
			if( p == "HOLE" )
			{
				package = HOLE;
				pattern = GRID;
				npins = 1;
				npins_x = 1;
				npins_y = 1;
				pin_start = P1BL;
				pin_dir = LRTB;
				pin_spacing_x = PCBU_PER_MIL*100;
				pin_spacing_y = PCBU_PER_MIL*100;
				pad_shape = NONE;
				pad_hole_w = 100*PCBU_PER_MIL;
			}
			else if( p == "DIP" )
			{
				package = DIP;
				pattern = EDGE;
				pin_start = P1BL;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*100;
				pad_shape = RNDSQ1;
				pad_hole_w = 24*PCBU_PER_MIL;
			}
			else if( p == "SIP" )
			{
				package = SIP;
				pattern = EDGE;
				pin_start = P1BL;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*100;
				pad_shape = RNDSQ1;
				pad_hole_w = 24*PCBU_PER_MIL;
			}
			else if( p == "SOIC" )
			{
				package = SOIC;
				pattern = EDGE;
				pin_start = P1BL;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*50;
				pad_shape = RECT;
				pad_hole_w = 0;
			}
			else if( p == "PLCC" )
			{
				package = PLCC;
				pattern = EDGE;
				pin_start = P1TC;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*50;
				pad_shape = RECT;
				pad_hole_w = 0;
			}
			else if( p == "QFP" )
			{
				package = QFP;
				pattern = EDGE;
				pin_start = P1TL;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*50;
				pad_shape = RECT;
				pad_hole_w = 0;
			}
			else if( p == "HDR" )
			{
				package = HDR;
				pattern = GRID;
				pin_start = P1BL;
				pin_dir = LRTB;
				pin_spacing_x = PCBU_PER_MIL*100;
				pin_spacing_y = PCBU_PER_MIL*100;
				pad_shape = RNDSQ1;
				pad_hole_w = PCBU_PER_MIL*24;
			}
			else if( p == "CHIP" )
			{
				package = CHIP;
				pattern = GRID;
				pin_start = P1BL;
				pin_dir = LRTB;
				pin_spacing_x = PCBU_PER_MIL*100;
				pin_spacing_y = PCBU_PER_MIL*100;
				pad_shape = RECT;
				pad_hole_w = 0;
			}
			else
				return 1;
			pad_w = pin_spacing_x/2;
		}
		
		// number of pins, must be second param
		else if( ip == 1 )
		{
			if (p[0]<'0' || p[0]>'9')
				return 1;
			npins = atoi( p );
			if( npins<0 || npins>10000 )
				return 1;										// out of range
			if( package == DIP || package == SOIC )
			{
				npins_x = npins/2;
				npins_y = 0;	// not used for DIP or SOIC
			}
			else if( package == PLCC || package == QFP )
			{
				npins_x = npins/4;
				npins_y = npins/4;
			}
			else if( package == HDR || package == CHIP )
			{
				if( npins == 1 )
				{
					npins_x = 1;
					npins_y = 1;
				}
				else
				{
					npins_x = npins/2;
					npins_y = 2;
				}
			}
			else if( package == HOLE )
			{
				npins = 1;
				npins_x = 1;
				npins_y = 1;
			}
			else if( package == SIP )
			{
				npins_x = npins;
				npins_y = 1;
			}
		}

		// units
		else if( p[0] == 'U' )
		{
			if( p1 == "MM" )
				units = MM;
			else if( p1 == "MIL" )
				units = MIL;
			else if( p1 == "NM" )
				units = NM;
		}
		
		// pins on edge of part or in grid pattern
		else if( p == "GRID" )
			pattern = GRID;
		else if( p == "EDGE" )
			pattern = EDGE;

		// # pins horizontal (ignore if DIP, SOIC )
		else if( p[0] == 'H' && p[1]>='0' && p[1]<='9' )
		{
			npins_x = atoi( p1 );
			if( package == PLCC || package == QFP )
				npins_y = (npins-2*npins_x)/2;
			else if( package == HDR || package == CHIP )
				npins_y = npins/npins_x;
		}

		// starting corner
		else if( p == "P1BL" )
			pin_start = P1BL;
		else if( p == "P1BR" )
			pin_start = P1BR;
		else if( p == "P1TR" )
			pin_start = P1TR;
		else if( p == "P1TL" )
			pin_start = P1TL;
		else if( p == "P1TC" )
			pin_start = P1TC;
		else if( p == "P1BC" )
			pin_start = P1BC;
		else if( p == "P1LC" )
			pin_start = P1LC;
		else if( p == "P1RC" )
			pin_start = P1RC;
		else if( p == "P1BL" ) 
			pin_start = P1BL;	// default
		
		// direction to increment pins (0=CCW, 1=CW)
		else if( p == "CCW" )
			pin_dir = CCW;
		else if( p == "CW" )
			pin_dir = CW;
		else if( p == "CW" )
			pin_dir = CW;
		else if( p == "LRBT" )
			pin_dir = LRBT;
		else if( p == "LRTB" )
			pin_dir = LRTB;
		else if( p == "RLBT" )
			pin_dir = RLBT;
		else if( p == "RLTB" )
			pin_dir = RLTB;
		else if( p == "BTLR" )
			pin_dir = BTLR;
		else if( p == "TBLR" )
			pin_dir = TBLR;
		else if( p == "BTRL" )
			pin_dir = BTRL;
		else if( p == "TBRL" )
			pin_dir = TBRL;

		// horizontal pin spacing
		else if( p[0] == 'P' && p[1] == 'H' )
		{
			pin_spacing_x = GetDimensionFromString( &p2 );
			if( pin_spacing_x == 0 && npins_x > 1 )
				return 1;
			if( pin_spacing_y == P_UNDEFINED )
				pin_spacing_y = pin_spacing_x;
		}
		
		// vertical pin spacing
		else if( p[0] == 'P' && p[1] == 'V' )
		{
			pin_spacing_y = GetDimensionFromString( &p2 );
			if( pin_spacing_y == 0 && npins_y > 1 )
				return 1;
		}
		
		// pin row offsets (or use defaults)
		else if( p[0] == 'R' && p[1] == 'L' )
			offset_left = GetDimensionFromString( &p2 );
		else if( p[0] == 'R' && p[1] == 'R' )
		{
			offset_right = GetDimensionFromString( &p2 );
			if( offset_right == 0 )
				return 1;
		}
		else if( p[0] == 'R' && p[1] == 'B' )
			offset_bottom = GetDimensionFromString( &p2 );
		else if( p[0] == 'R' && p[1] == 'T' )
		{
			offset_top = GetDimensionFromString( &p2 );
			if( offset_top == 0 )
				return 1;
		}
		
		// pad shape 
		else if( p == "RNDSQ1" )
			pad_shape = RNDSQ1;
		else if( p == "ROUND" )
			pad_shape = ROUND;
		else if( p == "SQUARE" )
			pad_shape = SQUARE;
		else if( p == "RECT" )
			pad_shape = RECT;
		else if( p == "RNDRECT" )
			pad_shape = RRECT;
		else if( p == "OVAL" )
			pad_shape = OVAL;
		else if( p == "OCTAGON" )
			pad_shape = OCTAGON;
		else if( p == "NONE" )
			pad_shape = NONE;
		
		// pad width
		if( p[0] == 'P' && p[1] == 'W' )
		{
			pad_w = GetDimensionFromString( &p2 );
			if( pad_w == 0 && pad_shape != NONE )
				return 1;
		}
		
		// pad lengths
		else if( p[0] == 'P' && p[1] == 'E' )
			pad_len_ext = GetDimensionFromString( &p2 );
		else if( p[0] == 'P' && param[ip][1] == 'I' )
			pad_len_int = GetDimensionFromString( &p2 );
		
		// pad radius
		else if( p[0] == 'P' && p[1] == 'R' )
			pad_radius = GetDimensionFromString( &p2 );
		
		// hole width
		else if( param[ip][0] == 'H' && param[ip][1] == 'W' )
			pad_hole_w = GetDimensionFromString( &p2 );
		
		// text size
		else if( param[ip][0] == 'T' && param[ip][1] == 'S' )
		{
			ref_text_size = GetDimensionFromString( &p2 );
			if( ref_text_size == 0 )
				return 1;
		}
	}
	
	// now set any undefined parameters using defaults
	// pin spacing y
	if( pin_spacing_y == P_UNDEFINED )
		pin_spacing_y = pin_spacing_x;
	
	// offsets for edge part rows
	if( offset_left == P_UNDEFINED )
		offset_left = 0;
	if( offset_bottom == P_UNDEFINED )
		offset_bottom = 0;
	if( offset_right == P_UNDEFINED )
		offset_right = offset_left + (npins_x-1)*pin_spacing_x;
	if( offset_top == P_UNDEFINED )
		offset_top = offset_bottom + (npins_y-1)*pin_spacing_y;
	
	// pad width
	if( pad_w == P_UNDEFINED )
		pad_w = pin_spacing_x/2;
	if( pad_len_ext == P_UNDEFINED )
		pad_len_ext = pad_w;
	if( pad_len_int == P_UNDEFINED )
		pad_len_int = pad_w;

	// text size
	if( ref_text_size == P_UNDEFINED )
		ref_text_size = 50*PCBU_PER_MIL;

#undef MAX_PARAMS
#undef MAX_CHARS_PER_PARAM

	int pad_len = pad_w/2;
	int pad_wid = pad_w/2;
	if( pad_shape == NONE )
	{
		pad_wid = pad_hole_w/2;
		pad_len = pad_hole_w/2;
	}
	else if( pad_shape == RECT )
		pad_len = pad_len_ext;
	if( pad_wid == 0 || pad_len == 0 )
		return 1;

	// CREATE ARRAY OF PADSTACKS
	int pin1x = 0, pin1y = 0;									// CPT2.  In the case of GRID patterns, may have to do an offset relative to pin 1 at the end
	if( pattern == EDGE )
	{
		// edge pattern, such as DIP, SOIC, PLCC, etc.
		for( int ip=0; ip<npins; ip++ )
		{
			// i is the actual pin number
			int i = ip;
			if( pin_start == P1LC )
				i = ip + npins_y/2 + 1;
			else if( pin_start == P1TL )
				i = ip + npins_y;
			else if( pin_start == P1TC )
				i = ip + npins_y + npins_x/2 + 1;
			else if( pin_start == P1TR )
				i = ip + npins_y + npins_x;
			else if( pin_start == P1RC )
				i = ip + npins_y + npins_x + npins_y/2 + 1;
			else if( pin_start == P1BR )
				i = ip + 2*npins_y + npins_x;
			else if( pin_start == P1BC )
				i = ip + 2*npins_y + npins_x + npins_x/2 + 1;
			if( i >= npins )
				i = i - npins;
			if( pattern == EDGE && pin_dir == CW )
			{
				if( pin_start == P1TL || pin_start == P1TR 
					|| pin_start == P1BR || pin_start == P1BL )
					i = (npins-1) - i;
				else
					i = npins - i;
			}
			if( i >= npins )
				i = i - npins;

			CPadstack *ps = new CPadstack(this);
			// set pin name, and store the 0-based pin number in utility field
			ps->name.Format( "%d", i+1 );
			ps->utility = i;

			// set hole width and determine TH or SMT
			int pad_type = pad_hole_w? TH: SMT;
			ps->hole_size = pad_hole_w;

			// now set pin positions for bottom, left, top and right rows
			// and create padstack array
			// note that pin numbers are not necessarily in sequence
			if( ip<npins_x )
			{
				// bottom row of pins, left->right
				ps->angle = 270;
				ps->y_rel = -offset_bottom;
				ps->x_rel = ip*pin_spacing_x;
			}
			else if( ip>=npins_x && ip<(npins_x+npins_y) )
			{
				// right row of pins
				ps->angle = 180;
				ps->y_rel = (ip-npins_x)*pin_spacing_y;
				ps->x_rel = offset_right;
			}
			else if( ip>=(npins_x+npins_y) && ip<(2*npins_x+npins_y) )
			{
				// top row of pins
				ps->angle = 90;
				ps->y_rel = offset_top;
				ps->x_rel = (2*npins_x+npins_y-ip-1)*pin_spacing_x;
			}
			else
			{
				// left row of pins
				ps->angle = 0;
				ps->y_rel = (2*npins_x+2*npins_y-ip-1)*pin_spacing_y;
				ps->x_rel = -offset_left;
			}
			if( pad_shape == RNDSQ1 || pad_shape == SQUARE
				|| pad_shape == ROUND || pad_shape == OCTAGON )
			{
				int type = PAD_ROUND;
				if( pad_shape == SQUARE || (pad_shape == RNDSQ1 && ip == 0) )
					type = PAD_SQUARE;
				else if( pad_shape == OCTAGON )
					type = PAD_OCTAGON;
				ps->top.shape = type;
				ps->top.size_h = pad_w;
				if( pad_type == SMT )
				{
					ps->bottom.shape = PAD_NONE;
					ps->inner.shape = PAD_NONE;
				}
				else
				{
					ps->bottom = ps->top;
					ps->inner.shape = PAD_ROUND;
					ps->inner.size_h = pad_w;
				}
			}
			else if( pad_shape == ROUND || pad_shape == RNDSQ1 )
			{
				ps->top.shape = PAD_ROUND;
				ps->top.size_h = pad_w;
				if( pad_type == SMT )
				{
					ps->bottom.shape = PAD_NONE;
					ps->inner.shape = PAD_NONE;
				}
				else
				{
					ps->bottom = ps->top;
					ps->inner.shape = PAD_ROUND;
					ps->inner.size_h = pad_w;
				}
			}
			else if( pad_shape == RECT || pad_shape == OVAL )
			{
				int type = PAD_RECT;
				if( pad_shape == OVAL )
					type = PAD_OVAL;
				if( pad_type == SMT )
				{
					ps->top.shape = type;
					ps->top.size_h = pad_w;
					ps->top.size_l = pad_len_ext;
					ps->top.size_r = pad_len_int;
					ps->bottom.shape = PAD_NONE;
					ps->inner.shape = PAD_NONE;
				}
				else
				{
					ps->top.shape = type;
					ps->top.size_h = pad_w;
					ps->top.size_l = pad_len_ext;
					ps->top.size_r = pad_len_int;
					ps->bottom = ps->top;
					ps->inner.shape = PAD_ROUND;
					ps->inner.size_h = min(pad_w,pad_len_ext+pad_len_int);
				}
			}
			else if( pad_shape == RRECT )
			{
				if( pad_type == SMT )
				{
					ps->top.shape = PAD_RRECT;
					ps->top.size_h = pad_w;
					ps->top.size_l = pad_len_ext;
					ps->top.size_r = pad_len_int;
					ps->top.radius = pad_radius;
					ps->bottom.shape = PAD_NONE;
					ps->inner.shape = PAD_NONE;
				}
				else
				{
					ps->top.shape = PAD_RRECT;
					ps->top.size_h = pad_w;
					ps->top.size_l = pad_len_ext;
					ps->top.size_r = pad_len_int;
					ps->top.radius = pad_radius;
					ps->bottom = ps->top;
					ps->inner.shape = PAD_ROUND;
					ps->inner.size_h = min(pad_w,pad_len_ext+pad_len_int);
				}
			}
			else if( pad_shape == NONE )
			{
				if( pad_type == SMT )
				{
					ps->top.shape = PAD_NONE;
					ps->top.size_h = 0;
					ps->top.size_l = 0;
					ps->top.size_r = 0;
					ps->bottom.shape = PAD_NONE;
					ps->inner.shape = PAD_NONE;
				}
				else
				{
					ps->top.shape = PAD_NONE;
					ps->top.size_h = 0;
					ps->top.size_l = 0;
					ps->top.size_r = 0;
					ps->inner = ps->top;
					ps->bottom = ps->top;
				}
			}
		}
	}

	else if( pattern == GRID )
	{
		// grid pattern, such as HDR
		int pad_type = pad_hole_w? TH: SMT;
		for( int ih=0; ih<npins_x; ih++ )
			for( int iv=0; iv<npins_y; iv++ )
			{
				// get actual pin number, and [CPT2] create padstack object
				int ip;
				if( pin_dir == LRBT )
					ip = ih + iv*npins_x; 
				else if( pin_dir == LRTB )
					ip = ih + (npins_y-iv-1)*npins_x; 
				else if( pin_dir == RLBT )
					ip = (npins_x-ih-1) + iv*npins_x; 
				else if( pin_dir == RLTB )
					ip = (npins_x-ih-1) + (npins_y-iv-1)*npins_x; 
				else if( pin_dir == BTLR )
					ip = iv + ih*npins_y; 
				else if( pin_dir == BTRL )
					ip = iv + (npins_x-ih-1)*npins_y; 
				else if( pin_dir == TBLR )
					ip = (npins_y-iv-1) + ih*npins_y; 
				else if( pin_dir == TBRL )
					ip = (npins_y-iv-1) + (npins_x-ih-1)*npins_y;
				CPadstack *ps = new CPadstack(this);
				// CPT2 Name the padstack (used to be done later, but why not now?).  Also store 0-based pin # in utility field.
				ps->name.Format( "%d", ip+1 );
				ps->utility = ip;

				ps->x_rel = ih * pin_spacing_x;
				ps->y_rel = iv * pin_spacing_y;
				// if RECT pads are used, assume that there are 2 rows
				ps->angle = 0;
				if( pad_shape == RECT )
				{
					if( npins_x == 2 )
						if( ih == 0 )
							ps->angle = 0;
						else
							ps->angle = 180;
					else if( npins_y == 2 )
						if( iv == 0 )
							ps->angle = 270;
						else
							ps->angle = 90;
				}
				if (ip==0)
					pin1x = ps->x_rel,
					pin1y = ps->y_rel;
				ps->hole_size = pad_hole_w;
				if( pad_shape == RNDSQ1 || pad_shape == ROUND || pad_shape == SQUARE || pad_shape == OCTAGON )
				{
					if( (ip==0 && pad_shape == RNDSQ1) || pad_shape == SQUARE )
						ps->top.shape = PAD_SQUARE;
					else if( pad_shape == OCTAGON )
						ps->top.shape = PAD_OCTAGON;
					else
						ps->top.shape = PAD_ROUND;
					ps->top.size_h = pad_w;
					if( pad_type == SMT )
					{
						ps->bottom.shape = PAD_NONE;
						ps->inner.shape = PAD_NONE;
					}
					else
					{
						ps->bottom = ps->top;
						ps->inner.shape = PAD_NONE;
//						ps->inner.size_h = pad_w;
					}
				}
				else if( pad_shape == RECT || pad_shape == OVAL )
				{
					if( pad_shape == RECT )
						ps->top.shape = PAD_RECT;
					else
						ps->top.shape = PAD_OVAL;
					ps->top.size_h = pad_w;
					ps->top.size_l = pad_len_ext;
					ps->top.size_r = pad_len_int;
					if( pad_type == SMT )
					{
						ps->bottom.shape = PAD_NONE;
						ps->inner.shape = PAD_NONE;
					}
					else
					{
						ps->bottom = ps->top;
						ps->inner.shape = PAD_NONE;
//						ps->inner.size_h = pad_w;
					}
				}
				else if( pad_shape == RRECT )
				{
					ps->top.shape = PAD_RRECT;
					ps->top.size_h = pad_w;
					ps->top.size_l = pad_len_ext;
					ps->top.size_r = pad_len_int;
					ps->top.radius = pad_radius;
					if( pad_type == SMT )
					{
						ps->bottom.shape = PAD_NONE;
						ps->inner.shape = PAD_NONE;
					}
					else
					{
						ps->bottom = ps->top;
						ps->inner.shape = PAD_NONE;
//						ps->inner.size_h = pad_w;
					}
				}
			}
	}
	else
		ASSERT(0);	// no pattern

	// CREATE REF-TEXT BOXES AND OUTLINE.  CPT2 TODO better positioning of ref-text
	CRect pad_rect = GetBounds();
	// Certain ref-text params are common to all patterns:
	m_ref->m_angle = 0;
	m_ref->m_font_size = ref_text_size;
	m_ref->m_stroke_width = m_ref->m_font_size/10;
	if( package == HOLE )
		m_ref->m_x = m_ref->m_y = pin_spacing_x;
	else if( package == DIP || package == SOIC )
	{
		m_ref->m_x = m_ref->m_y = pin_spacing_x;
		COutline *o = new COutline(this, LAY_FP_SILK_TOP, 7*PCBU_PER_MIL);			// CPT2 changed layer
		m_outlines.Add(o);
		CContour *ctr = new CContour(o, true);
		ctr->AppendCorner( 0, pad_len_int+offset_top/12 );
		ctr->AppendCorner( (npins_x-1)*pin_spacing_x, pad_len_int+offset_top/12 );
		ctr->AppendCorner( (npins_x-1)*pin_spacing_x, 11*offset_top/12-pad_len_int );
		ctr->AppendCorner( 0, 11*offset_top/12-pad_len_int );
		ctr->AppendCorner( 0, 7*offset_top/12 );
		ctr->AppendCorner( pin_spacing_x/4, 7*offset_top/12 );
		ctr->AppendCorner( pin_spacing_x/4, 5*offset_top/12 );
		ctr->AppendCorner( 0, 5*offset_top/12 );
		ctr->Close();
	}
	else if( package == PLCC || package == QFP )
	{
		m_ref->m_x = offset_right/4;
		m_ref->m_y = offset_top/2;
		COutline *o = new COutline(this, LAY_FP_SILK_TOP, 7*PCBU_PER_MIL);
		m_outlines.Add(o);
		CContour *ctr = new CContour(o, true);
		int xf = (npins_x-1)*pin_spacing_x;
		int yf = (npins_y-1)*pin_spacing_y;
		int corner = pin_spacing_x;
		int notch = pin_spacing_x/2;
		int notch_w = pin_spacing_x/4;
		if( pin_start == P1BL )
		{
			// notch bottom-left corner
			ctr->AppendCorner( 0, corner );
			ctr->AppendCorner( corner, 0 );
		}
		else
			ctr->AppendCorner( 0, 0 );
		if( pin_start == P1BC )
		{
			// notch bottom side
			ctr->AppendCorner( xf/2-notch_w, 0 );
			ctr->AppendCorner( xf/2-notch_w, notch );
			ctr->AppendCorner( xf/2+notch_w, notch );
			ctr->AppendCorner( xf/2+notch_w, 0 );
		}
		if( pin_start == P1BR )
		{
			// notch bottom-right corner
			ctr->AppendCorner( xf-corner, 0 );
			ctr->AppendCorner( xf, corner );
		}
		else
			ctr->AppendCorner( xf, 0 );
		if( pin_start == P1RC )
		{
			// notch right side
			ctr->AppendCorner( xf, yf/2-notch_w ); 
			ctr->AppendCorner( xf-notch, yf/2-notch_w ); 
			ctr->AppendCorner( xf-notch, yf/2+notch_w ); 
			ctr->AppendCorner( xf, yf/2+notch_w ); 
		}
		if( pin_start == P1TR )
		{
			// notch top-right corner
			ctr->AppendCorner( xf, yf-corner ); 
			ctr->AppendCorner( xf-corner, yf ); 
		}
		else
			ctr->AppendCorner( xf, yf ); 
		if( pin_start == P1TC )
		{
			// notch top side
			ctr->AppendCorner( xf/2+notch_w, yf ); 
			ctr->AppendCorner( xf/2+notch_w, yf-notch ); 
			ctr->AppendCorner( xf/2-notch_w, yf-notch ); 
			ctr->AppendCorner( xf/2-notch_w, yf ); 
		}
		if( pin_start == P1TL )
		{
			// notch top-left corner
			ctr->AppendCorner( 0+corner, yf ); 
			ctr->AppendCorner( 0, yf-corner ); 
		}
		else
			ctr->AppendCorner( 0, yf); 
		if( pin_start == P1LC )
		{
			// notch left side
			ctr->AppendCorner( 0, yf/2+notch_w );
			ctr->AppendCorner( notch, yf/2+notch_w );
			ctr->AppendCorner( notch, yf/2-notch_w );
			ctr->AppendCorner( 0, yf/2-notch_w );
		}
		ctr->Close();
	}
	else if( package == HDR || package == CHIP || package == SIP )
	{
		m_ref->m_x = offset_right/4;
		m_ref->m_y = offset_top/2;
		// create part outline with 7 mil line width and 13 mil clearance
		COutline *o = new COutline(this, LAY_FP_SILK_TOP, 7*PCBU_PER_MIL);				// CPT2 changed layer
		m_outlines.Add(o);
		CContour *ctr = new CContour(o, true);
		int clear = 13*PCBU_PER_MIL;
		ctr->AppendCorner( pad_rect.left-clear, pad_rect.bottom-clear ); 
		ctr->AppendCorner( pad_rect.right+clear, pad_rect.bottom-clear ); 
		ctr->AppendCorner( pad_rect.right+clear, pad_rect.top+clear ); 
		ctr->AppendCorner( pad_rect.left-clear, pad_rect.top+clear ); 
		ctr->Close();
	}

	// If necessary, adjust offsets for pin 1 position, which may not be 0,0.  CPT2 bug fix:  used to be done _before_ the outline poly was created.
	if (pin1x || pin1y)
	{
		CIter<CPadstack> ips (&m_padstacks);
		for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
			ps->x_rel -= pin1x,
			ps->y_rel -= pin1y;
		CIter<COutline> io (&m_outlines);
		for (COutline *o = io.First(); o; o = io.Next())
			o->Offset(-pin1x, -pin1y);
		m_ref->m_x -= pin1x;
		m_ref->m_y -= pin1y;
	}

	// now set selection rectangle
	CRect br = GetBounds();
	m_sel_xi = br.left - 10*NM_PER_MIL;
	m_sel_yi = br.bottom - 10*NM_PER_MIL;
	m_sel_xf = br.right + 10*NM_PER_MIL;
	m_sel_yf = br.top + 10*NM_PER_MIL;

	m_name = name.Left(MAX_NAME_SIZE);	// this is the limit
	m_units = units;
	CPoint c = GetDefaultCentroid();
	m_centroid->m_type = CENTROID_DEFAULT;
	m_centroid->m_x = c.x;
	m_centroid->m_y = c.y;
	m_centroid->m_angle = 0;
	GenerateValueParams();				// CPT2 added.
	return 0;
}

// MakeFromFile() ... create a footprint from a description in a text file
//
// enter with:
//	file = pointer to an open CStdioFile, or NULL if file not already open
//	name = name of footprint (if known), or "" if unknown
//	file_path = full pathname to file if not already open
//	pos = position in file if not already open (i.e. position of "name: xxx" line)
//
// returns 0 if successful
// returns 1 if unable to open file
// returns 2 if file i/o error
// returns 3 if unable to parse file
// returns 4 if name doesn't match (if name is known)
//
int CShape::MakeFromFile( CStdioFile * in_file, CString name, 
						 CString file_path, int pos )
{
	CString key_str;
	int err;
	CArray<CString> p;
	int ipin, file_pos;
	int num_pins;
	BOOL bValue = FALSE;
	BOOL bRef = FALSE;
	int n_glue = 0;
	COutline *currPoly = NULL;			// CPT2
	CPadstack *currPs = NULL;
	extern CFreePcbApp theApp;
	doc = theApp.m_doc;					// CPT2.  REALLY want m_doc to be a legit value from the get-go.

	Clear();
	p.SetSize( 10 );
	// if in_file exists, use it, otherwise open file
	CStdioFile * file;
	if( !in_file )
	{
		file = new CStdioFile;
		if (!file->Open( file_path, CFile::modeRead ))
			return 1;
	}
	else
		file = in_file;

	// delete any original shape info and set defaults
	Clear();
	m_units = NM;
	int mult = 1;
	BOOL bCentroidFound = FALSE;

	// now read lines from file and make footprint
	try
	{
		if( !in_file )
			file->Seek( pos, CFile::begin );	// set starting position
		int line_num = 0;
		CString name_str;
		int ok = file->ReadString( name_str );	// get first line from file
		if( !ok )
		{
			if( !in_file )
				delete file;
			return 2;
		}
		int np = ParseKeyString( &name_str, &key_str, &p );	// parse it
		if( np < 2 || key_str != "name" )
		{
			if( !in_file )
				delete file;
			return 4;	// first line should be "name: xxx"
		}
		if( name != "" && p[0] != name )
		{
			if( !in_file )
				delete file;
			return 4;	// if name defined, it should match
		}
		if( p[0].GetLength() > MAX_NAME_SIZE )
		{
			CString mess, s ((LPCSTR) IDS_FootprintNameTooLong);
			mess.Format( s, p[0], p[0].Left(MAX_NAME_SIZE) );
			AfxMessageBox( mess );
		}
		m_name = p[0].Left(MAX_NAME_SIZE);

		// now get the rest of the lines from file
		while( 1 )
		{
			CString instr;
			file_pos = file->GetPosition();		// remember position
			int err = file->ReadString( instr );
			if( !err )
			{
				// EOF, this is not an error
				if( !in_file )
					delete file;
				goto normal_return;
			}
			np = ParseKeyString( &instr, &key_str, &p );	// parse line
			if( np == 0 )
				continue;	// blank line or comment, skip it
			if( key_str == "end" )
			{
				// end of shape definition
				if( !in_file )
					delete file;
				break;	
			}
			else if( key_str == "name" || key_str[0] == '[' )
			{
				// beginning of next shape or end of shapes section
				file->Seek( file_pos, CFile::begin );	// back up
				if( !in_file )
					delete file;
				break;	
			}
			else if( key_str == "str" && np >= 2 )
			{
				// keyword = "str", make footprint from string
				int err = MakeFromString( name, p[0] );
				if( !in_file )
					delete file;
				if( err )
					return 3;
				else
					goto normal_return;
			}
			else if( key_str == "author" && np >= 2 )
			{
				m_author = p[0];
			}
			else if( key_str == "source" && np >= 2 )
			{
				m_source = p[0];
			}
			else if( key_str == "description" && np >= 2 )
			{
				m_desc = p[0];
			}
			else if( key_str == "units" && np >= 2 )
			{
				if( p[0] == "MIL" )
				{
					m_units = MIL;
					mult = 25400;
				}
				else if( p[0] == "MM" )
				{
					m_units = MM;
					mult = 1000000;
				}
				else if( p[0] == "NM" )
				{
					m_units = NM;
					mult = 1;
				}
			}
			else if( key_str == "sel_rect" && np >= 5 )
			{
				m_sel_xi = GetDimensionFromString( &p[0], m_units );
				m_sel_yi = GetDimensionFromString( &p[1], m_units);
				m_sel_xf = GetDimensionFromString( &p[2], m_units);
				m_sel_yf = GetDimensionFromString( &p[3], m_units);
			}
			else if( key_str == "ref_text" && np >= 6 )
			{
				int size = GetDimensionFromString( &p[0], m_units);
				int xi = GetDimensionFromString( &p[1], m_units);
				int yi = GetDimensionFromString( &p[2], m_units);
				int angle = my_atoi( &p[3] ); 
				int w = GetDimensionFromString( &p[4], m_units);
				int layer = np>=7? my_atoi( &p[5] ): 4;
				if( layer < 4 )
					layer = 4;
				bool bMirror = layer==LAY_FP_SILK_BOTTOM || layer==LAY_FP_BOTTOM_COPPER;
				m_ref->Move(xi, yi, angle, bMirror, false, layer, size, w);
				bRef = TRUE;
			}
			else if( key_str == "value_text" && np >= 6 )
			{
				int size = GetDimensionFromString( &p[0], m_units);
				int xi = GetDimensionFromString( &p[1], m_units);
				int yi = GetDimensionFromString( &p[2], m_units);
				int angle = my_atoi( &p[3] ); 
				int w = GetDimensionFromString( &p[4], m_units);
				int layer = np>=7? my_atoi( &p[5] ): 4;
				if( layer < 4 )
					layer = 4;
				bool bMirror = layer==LAY_FP_SILK_BOTTOM || layer==LAY_FP_BOTTOM_COPPER;
				m_value->Move(xi, yi, angle, bMirror, false, layer, size, w);
				bValue = TRUE;
			}
			else if( key_str == "centroid" && np >= 4 )
			{
				m_centroid->m_type = (CENTROID_TYPE)my_atoi( &p[0] );
				m_centroid->m_x = GetDimensionFromString( &p[1], m_units);
				m_centroid->m_y = GetDimensionFromString( &p[2], m_units);
				m_centroid->m_angle = 0;
				if( np >= 5 )
					m_centroid->m_angle = my_atoi( &p[3] );
				bCentroidFound = TRUE;
			}
			else if( key_str == "adhesive" && np >= 5 )
			{
				GLUE_POS_TYPE type = (GLUE_POS_TYPE)my_atoi( &p[0] );
				int w = GetDimensionFromString( &p[1], m_units);
				int x = GetDimensionFromString( &p[2], m_units);
				int y = GetDimensionFromString( &p[3], m_units);
				CGlue *g = new CGlue(this, type, w, x, y);
			}
			else if( key_str == "text" && np >= 7 )
			{
				int font_size = GetDimensionFromString( &p[1], m_units);
				int x = GetDimensionFromString( &p[2], m_units);
				int y = GetDimensionFromString( &p[3], m_units);
				int angle = my_atoi( &p[4] ); 
				int stroke_w = GetDimensionFromString( &p[5], m_units);
				int mirror = 0;
				int layer = LAY_FP_SILK_TOP;
				BOOL bNegative = FALSE;
				if( np >= 9 )
				{
					mirror = my_atoi( &p[6] );
					layer = my_atoi( &p[7] );
				}
				if( np >= 10 )
					bNegative = my_atoi( &p[8] );
				m_tl->AddText(x, y, angle, mirror, bNegative, layer, font_size, stroke_w, &p[0], NULL, this);
			}
			else if( (key_str == "outline_polygon" || key_str == "outline_polyline")
				&& np >= 4 )
			{
				int poly_layer = LAY_FP_SILK_TOP;
				int w = GetDimensionFromString( &p[0], m_units);
				int x = GetDimensionFromString( &p[1], m_units);
				int y = GetDimensionFromString( &p[2], m_units);
				if( np >= 5 )
					poly_layer = my_atoi( &p[3] );
				if( poly_layer < LAY_FP_SILK_TOP )
					poly_layer = LAY_FP_SILK_TOP;
				currPoly = new COutline (this, poly_layer, w);
				m_outlines.Add(currPoly);
				CContour *ctr = new CContour(currPoly, true);			// Adds ctr as poly's main contour
				CCorner *c = new CCorner(ctr, x, y);					// Constructor adds corner to ctr->corners and sets ctr->head/tail					{
			}
			else if( key_str == "next_corner" && np >= 3 )
			{
				int x = GetDimensionFromString( &p[0], m_units);
				int y = GetDimensionFromString( &p[1], m_units);
				int style = CPolyline::STRAIGHT;
				if( np >= 4 )
					style = my_atoi( &p[2] );
				CContour *ctr = currPoly->main;
				CCorner *c = new CCorner(ctr, x, y);
				CSide *s = new CSide(ctr, style);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
			else if( key_str == "close_polyline" && np >= 2 )
			{
				int style = CPolyline::STRAIGHT;
				if( np >= 2 )
					style = my_atoi( &p[0] );
				currPoly->main->Close(style);
			}
			else if( key_str == "n_pins" && np >= 2 )
			{
				num_pins = my_atoi( &p[0] );
			}
			else if( key_str == "pin" && np >= 6 )
			{
				CString pin_name = p[0];
				if( pin_name.GetLength() > MAX_PIN_NAME_SIZE )
				{
					CString mess, s ((LPCSTR) IDS_FootprintPinNameTooLong);
					mess.Format( s, m_name, pin_name, pin_name.Left(MAX_PIN_NAME_SIZE) );
					AfxMessageBox( mess );
					pin_name = pin_name.Left(MAX_PIN_NAME_SIZE);
				}
				CPadstack *ps = new CPadstack(this);
				ps->name = pin_name; 
				ps->hole_size = GetDimensionFromString( &p[1], m_units); 
				ps->x_rel = GetDimensionFromString( &p[2], m_units); 
				ps->y_rel = GetDimensionFromString( &p[3], m_units); 
				ps->angle = my_atoi( &p[4] );
				currPs = ps;
			}
			else if( key_str == "top_pad" && np >= 5 )
			{
				CPad * pad = &currPs->top;
				ASSERT( pad->connect_flag == 0 );
				pad->shape = my_atoi( &p[0] ); 
				pad->size_h = GetDimensionFromString( &p[1], m_units); 
				pad->size_l = GetDimensionFromString( &p[2], m_units); 
				pad->size_r = GetDimensionFromString( &p[3], m_units);
				if( np >= 6 )
				{
					pad->radius = GetDimensionFromString( &p[4], m_units);
					int rad_min = min( pad->size_h/2, pad->size_l );
					pad->radius = max( pad->radius, rad_min );
				}
				if( np >= 7 )
					pad->connect_flag = my_atoi( &p[5] );
			}
			else if( key_str == "top_mask" && np >= 6 )
			{
				CPad *pad = &currPs->top_mask;
				pad->shape = my_atoi( &p[0] ); 
				if( pad->shape > (PAD_DEFAULT - 50) )
					pad->shape = PAD_DEFAULT; 
				pad->size_h = GetDimensionFromString( &p[1], m_units); 
				pad->size_l = GetDimensionFromString( &p[2], m_units); 
				pad->size_r = GetDimensionFromString( &p[3], m_units);
				pad->radius = GetDimensionFromString( &p[4], m_units);
				int rad_min = min( pad->size_h/2, pad->size_l );
				pad->radius = max( pad->radius, rad_min );
			}
			else if( key_str == "top_paste" && np >= 6 )
			{
				CPad *pad = &currPs->top_paste;
				pad->shape = my_atoi( &p[0] ); 
				if( pad->shape > (PAD_DEFAULT - 50) )
					pad->shape = PAD_DEFAULT; 
				pad->size_h = GetDimensionFromString( &p[1], m_units); 
				pad->size_l = GetDimensionFromString( &p[2], m_units); 
				pad->size_r = GetDimensionFromString( &p[3], m_units);
				pad->radius = GetDimensionFromString( &p[4], m_units);
				int rad_min = min( pad->size_h/2, pad->size_l );
				pad->radius = max( pad->radius, rad_min );
			}
			else if( key_str == "inner_pad" && np >= 7 )
			{
				CPad *pad = &currPs->inner;
				pad->shape = my_atoi( &p[0] ); 
				if( pad->shape > (PAD_DEFAULT - 50) )
					pad->shape = PAD_DEFAULT; 
				pad->size_h = GetDimensionFromString( &p[1], m_units); 
				pad->size_l = GetDimensionFromString( &p[2], m_units); 
				pad->size_r = GetDimensionFromString( &p[3], m_units);
				pad->radius = GetDimensionFromString( &p[4], m_units);
				int rad_min = min( pad->size_h/2, pad->size_l );
				pad->radius = max( pad->radius, rad_min );
			}
			else if( key_str == "bottom_pad" && np >= 5 )
			{
				CPad * pad = &currPs->bottom;
				ASSERT( pad->connect_flag == 0 );
				pad->shape = my_atoi( &p[0] ); 
				pad->size_h = GetDimensionFromString( &p[1], m_units); 
				pad->size_l = GetDimensionFromString( &p[2], m_units); 
				pad->size_r = GetDimensionFromString( &p[3], m_units);
				if( np >= 6 )
				{
					pad->radius = GetDimensionFromString( &p[4], m_units);
					int rad_min = min( pad->size_h/2, pad->size_l );
					pad->radius = max( pad->radius, rad_min );
				}
				if( np >= 7 )
					pad->connect_flag = my_atoi( &p[5] );
			}
			else if( key_str == "bottom_mask" && np >= 6 )
			{
				CPad *pad = &currPs->bottom_mask;
				pad->shape = my_atoi( &p[0] ); 
				if( pad->shape > (PAD_DEFAULT - 50) )
					pad->shape = PAD_DEFAULT; 
				pad->size_h = GetDimensionFromString( &p[1], m_units); 
				pad->size_l = GetDimensionFromString( &p[2], m_units); 
				pad->size_r = GetDimensionFromString( &p[3], m_units);
				pad->radius = GetDimensionFromString( &p[4], m_units);
				int rad_min = min( pad->size_h/2, pad->size_l );
				pad->radius = max( pad->radius, rad_min );
			}
			else if( key_str == "bottom_paste" && np >= 6 )
			{
				CPad *pad = &currPs->bottom_paste;
				pad->shape = my_atoi( &p[0] ); 
				if( pad->shape > (PAD_DEFAULT - 50) )
					pad->shape = PAD_DEFAULT; 
				pad->size_h = GetDimensionFromString( &p[1], m_units); 
				pad->size_l = GetDimensionFromString( &p[2], m_units); 
				pad->size_r = GetDimensionFromString( &p[3], m_units);
				pad->radius = GetDimensionFromString( &p[4], m_units);
				int rad_min = min( pad->size_h/2, pad->size_l );
				pad->radius = max( pad->radius, rad_min );
			}
			line_num++;
		}

normal_return:
		// eliminate any polylines with only one corner
		// CPT2
		CIter<COutline> io (&m_outlines);
		for (COutline *o = io.First(); o; o = io.Next())
			if( o->NumCorners() == 1 )
				m_outlines.Remove(o);
		// NM deprecated
		if( m_units == NM )
			m_units = MM;
		// generate centroid if not found
		if( !bCentroidFound )
		{
			CPoint c = GetDefaultCentroid();
			m_centroid->m_type = CENTROID_DEFAULT;
			m_centroid->m_x = c.x;
			m_centroid->m_y = c.y;
			m_centroid->m_angle = 0;
		}
		if( !bValue )
			GenerateValueParams();
		return 0;
	}

	catch( CFileException * e)
	{
		// file error
		if( !in_file )
			delete file;
		return 2;
	}

	catch( CString * err_str )
	{
		// parsing error
		delete err_str;
		if( !in_file )
			delete file;
		return 3;
	}
}

// copy another shape into this shape.
// CPT2 converted after changing CShape members a bit.  Ensures that copies all have correct CFreePcbDoc pointers.
// New feature (used when doing copy-to-clipboard operations in the fp editor):  copy utility bits over from each sub-object.
//
void CShape::Copy( CShape * src )
{
	doc = src->doc;				// CPT2
	if (!doc) 
	{
		CMainFrame * pMainWnd = (CMainFrame*)AfxGetMainWnd();
		doc = (CFreePcbDoc*)pMainWnd->GetActiveDocument();
	}

	// description
	m_name = src->m_name;
	m_author = src->m_author;
	m_source = src->m_source;
	m_desc = src->m_desc;
	m_units = src->m_units;
	// selection box
	m_sel_xi = src->m_sel_xi;
	m_sel_yi = src->m_sel_yi;
	m_sel_xf = src->m_sel_xf;
	m_sel_yf = src->m_sel_yf;
	// reference designator and value text
	m_ref->Copy( src->m_ref );
	m_value->Copy( src->m_value );
	m_ref->m_shape = m_value->m_shape = this;
	// centroid.
	m_centroid->m_type = src->m_centroid->m_type;
	m_centroid->m_x = src->m_centroid->m_x;
	m_centroid->m_y = src->m_centroid->m_y;
	m_centroid->m_angle = src->m_centroid->m_angle;
	m_centroid->m_shape = this;
	m_centroid->utility = src->m_centroid->utility;
	// padstacks
	m_padstacks.RemoveAll();
	CIter<CPadstack> ips (&src->m_padstacks);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
		new CPadstack(ps, this);										// Takes care of adding new padstack to this->m_padstacks
	// outline polys
	m_outlines.RemoveAll();
	CIter<COutline> io (&src->m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
		new COutline(o, this);											// Ditto
	// texts
	m_tl->texts.RemoveAll();
	CIter<CText> it (&src->m_tl->texts);
	for (CText *t = it.First(); t; t = it.Next())
	{
		CText *t2 = m_tl->AddText( t->m_x, t->m_y, t->m_angle, t->m_bMirror, t->m_bNegative, 
			t->m_layer, t->m_font_size, t->m_stroke_width, &t->m_str, NULL, this );
		t2->utility = t->utility;
	}
	// glue spots
	m_glues.RemoveAll();
	CIter<CGlue> ig (&src->m_glues);
	for (CGlue *g = ig.First(); g; g = ig.Next())
		new CGlue(g, this);
}

bool CShape::SameAs( CShape * shape )
{
	// parameters
	if( m_name != shape->m_name 
		|| m_author != shape->m_author 
		|| m_source != shape->m_source 
		|| m_desc != shape->m_desc 
		|| m_sel_xi != shape->m_sel_xi 
		|| m_sel_yi != shape->m_sel_yi 
		|| m_sel_xf != shape->m_sel_xf 
		|| m_sel_yf != shape->m_sel_yf 
		|| m_ref->m_font_size != shape->m_ref->m_font_size 
		|| m_ref->m_stroke_width != shape->m_ref->m_stroke_width 
		|| m_ref->m_x != shape->m_ref->m_x 
		|| m_ref->m_y != shape->m_ref->m_y 
		|| m_ref->m_angle != shape->m_ref->m_angle 
		|| m_ref->m_layer != shape->m_ref->m_layer 
		|| m_value->m_font_size != shape->m_value->m_font_size 
		|| m_value->m_stroke_width != shape->m_value->m_stroke_width 
		|| m_value->m_x != shape->m_value->m_x 
		|| m_value->m_y != shape->m_value->m_y 
		|| m_value->m_angle != shape->m_value->m_angle 
		|| m_value->m_layer != shape->m_value->m_layer 
		)
			return FALSE;

	// padstacks.  If other shape has the same padstacks, but in a different order, tough luck...
	int np = m_padstacks.GetSize();
	if( np != shape->m_padstacks.GetSize() )
		return FALSE;
	CIter<CPadstack> ips1 (&m_padstacks), ips2 (&shape->m_padstacks);
	for (CPadstack *ps1 = ips1.First(), *ps2 = ips2.First(); ps1; ps1 = ips1.Next(), ps2 = ips2.Next())
		if( !ps1->SameAs(ps2) )
			return FALSE;

	// outline polys
	np = m_outlines.GetSize();
	if( np != shape->m_outlines.GetSize() )
		return FALSE;
	CIter<COutline> io1 (&m_outlines), io2 (&shape->m_outlines);
	for (COutline *o1 = io1.First(), *o2 = io2.First(); o1; o1 = io1.Next(), o2 = io2.Next())
	{
		if (o1->m_layer != o2->m_layer) return false;
		if (o1->m_hatch != o2->m_hatch) return false;
		if (o1->m_w != o2->m_w) return false;
		if (o1->contours.GetSize() != o2->contours.GetSize()) return false;
		CIter<CContour> ictr1 (&o1->contours), ictr2 (&o2->contours);
		for (CContour *ctr1 = ictr1.First(), *ctr2 = ictr2.First(); ctr1; ctr1 = ictr1.Next(), ctr2 = ictr2.Next())
		{
			if (ctr1->NumCorners() != ctr2->NumCorners()) return false;
			if (ctr1->sides.GetSize() != ctr2->sides.GetSize()) return false;		// Will distinguish a closed ctr from and open one
			if (ctr1->sides.GetSize()==0) continue;									// :(
			// Traverse in geometrical order:
			CSide *s1 = ctr1->head->postSide, *s2 = ctr2->head->postSide;
			for ( ; s1->postCorner!=ctr1->tail; s1 = s1->postCorner->postSide, s2 = s2->postCorner->postSide)
				if (s1->preCorner->x != s2->preCorner->x) return false;
				else if (s1->preCorner->y != s2->preCorner->y) return false;
			if (ctr1->tail->x != ctr2->tail->x) return false;
			if (ctr1->tail->y != ctr2->tail->y) return false;
		}
	}
	// text
	if (m_tl->texts.GetSize() != shape->m_tl->texts.GetSize()) return false;
	CIter<CText> it1 (&m_tl->texts), it2 (&shape->m_tl->texts);
	for (CText *t1 = it1.First(), *t2 = it2.First(); t1; t1 = it1.Next(), t2 = it2.Next())
		if (t1->m_x != t2->m_x
			|| t1->m_y != t2->m_y
			|| t1->m_layer != t2->m_layer
			|| t1->m_angle != t2->m_angle
			|| t1->m_bMirror != t2->m_bMirror
			|| t1->m_bNegative != t2->m_bNegative
			|| t1->m_font_size != t2->m_font_size
			|| t1->m_stroke_width != t2->m_stroke_width
			|| t1->m_str != t2->m_str )
			return false;

	return TRUE;
}

int CShape::GetNumPins()
{
	return m_padstacks.GetSize();
}

CPadstack *CShape::GetPadstackByName( CString *name )
{
	CIter<CPadstack> ips (&m_padstacks);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
		if (ps->name == *name)
			return ps;
	return NULL;
}


// write one footprint
//
int CShape::WriteFootprint( CStdioFile * file )
{
	CString line;
	CString key;
	try
	{
		line.Format( "name: \"%s\"\n", m_name.Left(MAX_NAME_SIZE) ); 
		file->WriteString( line );
		if( m_author != "" )
		{
			line.Format( "author: \"%s\"\n", m_author );
			file->WriteString( line );
		}
		if( m_source != "" )
		{
			line.Format( "source: \"%s\"\n", m_source );
			file->WriteString( line );
		}
		if( m_desc != "" )
		{
			line.Format( "description: \"%s\"\n", m_desc );
			file->WriteString( line );
		}
#ifdef NO_MM
		if( m_units == MM )
			m_units = NM;
#endif
		if( m_units == NM )
			file->WriteString( "  units: NM\n" );
		else if( m_units == MIL )
			file->WriteString( "  units: MIL\n" );
		else if( m_units == MM )
			file->WriteString( "  units: MM\n" );
		else
			ASSERT(0);
		line.Format( "  sel_rect: %s %s %s %s\n", 
			ws(m_sel_xi,m_units), ws(m_sel_yi,m_units), ws(m_sel_xf,m_units), ws(m_sel_yf,m_units) );
		file->WriteString( line );
		int layer_index = 0;
		line.Format( "  ref_text: %s %s %s %d %s %d\n", 
			ws(m_ref->m_font_size,m_units), ws(m_ref->m_x,m_units), ws(m_ref->m_y,m_units), m_ref->m_angle, 
			ws(m_ref->m_stroke_width,m_units), m_ref->m_layer );
		file->WriteString( line );
		line.Format( "  value_text: %s %s %s %d %s %d\n", 
			ws(m_value->m_font_size,m_units), ws(m_value->m_x,m_units), ws(m_value->m_y,m_units), m_value->m_angle, 
			ws(m_value->m_stroke_width,m_units), m_value->m_layer );
		file->WriteString( line );
		line.Format( "  centroid: %d %s %s %d\n", 
			m_centroid->m_type, ws(m_centroid->m_x,m_units), ws(m_centroid->m_y,m_units), m_centroid->m_angle );
		file->WriteString( line );
		CIter<CGlue> ig (&m_glues);
		for (CGlue *g = ig.First(); g; g = ig.Next())
		{
			line.Format( "  adhesive: %d %s %s %s\n", 
				g->type, ws(g->w,m_units), ws(g->x,m_units), ws(g->y,m_units) );
			file->WriteString( line );
		}
		CIter<CText> it (&m_tl->texts);
		for (CText *t = it.First(); t; t = it.Next())
			line.Format( "  text: \"%s\" %s %s %s %d %s %d %d\n", t->m_str,
				ws(t->m_font_size,m_units), ws(t->m_x,m_units), ws(t->m_y,m_units), t->m_angle, 
				ws(t->m_stroke_width,m_units), t->m_bMirror, t->m_layer ),
			file->WriteString( line ); 
		CIter<COutline> io (&m_outlines);
		for (COutline *o = io.First(); o; o = io.Next())
		{
			// CPT2 TODO NB assuming no secondary contours
			CCorner *c = o->main->head;
			line.Format( "  outline_polyline: %s %s %s %d\n", ws(o->m_w,m_units),
				ws(c->x,m_units), ws(c->y,m_units),	o->m_layer );
			file->WriteString( line );
			while (c->postSide)
			{
				c = c->postSide->postCorner; 
				if (c==o->main->head)
				{
					line.Format( "    close_polyline: %d\n", c->preSide->m_style );
					file->WriteString( line );
					break;
				}
				line.Format( "    next_corner: %s %s %d\n",	ws(c->x,m_units), ws(c->y,m_units), c->preSide->m_style );
				file->WriteString( line );
			}
		}

		line.Format( "  n_pins: %d\n", m_padstacks.GetSize() );
		file->WriteString( line );
		CIter<CPadstack> ips (&m_padstacks);
		for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
		{
			line.Format( "    pin: \"%s\" %s %s %s %d\n",
				ps->name, ws(ps->hole_size,m_units), ws(ps->x_rel,m_units), ws(ps->y_rel,m_units), ps->angle ); 
			file->WriteString( line );
			//** for debugging, trap bad shapes
			if( (ps->top_mask.shape < PAD_DEFAULT && ps->top_mask.shape > PAD_OCTAGON)
				|| (ps->top_paste.shape < PAD_DEFAULT && ps->top_paste.shape > PAD_OCTAGON)
				|| (ps->bottom_mask.shape < PAD_DEFAULT && ps->bottom_mask.shape > PAD_OCTAGON)
				|| (ps->bottom_paste.shape < PAD_DEFAULT && ps->bottom_paste.shape > PAD_OCTAGON) )
			{
				CString s ((LPCSTR) IDS_ErrorTryingToWriteBadPadShapeInFootprint);
				AfxMessageBox( s );
			}
			//** end
			if( ps->hole_size || ps->top.shape != PAD_NONE )
			{
				if( ps->top.connect_flag )
					line.Format( "      top_pad: %d %s %s %s %s %d\n",
						ps->top.shape, ws(ps->top.size_h,m_units), ws(ps->top.size_l,m_units), 
						ws(ps->top.size_r,m_units), ws(ps->top.radius,m_units), ps->top.connect_flag );
				else
					line.Format( "      top_pad: %d %s %s %s %s\n",
						ps->top.shape, ws(ps->top.size_h,m_units), ws(ps->top.size_l,m_units), 
						ws(ps->top.size_r,m_units), ws(ps->top.radius,m_units) );
				file->WriteString( line );
			}
			if( ps->top_mask.shape != PAD_DEFAULT )
			{
				line.Format( "      top_mask: %d %s %s %s %s\n",
					ps->top_mask.shape, ws(ps->top_mask.size_h,m_units), ws(ps->top_mask.size_l,m_units), 
					ws(ps->top_mask.size_r,m_units), ws(ps->top_mask.radius,m_units) );
				file->WriteString( line );
			}
			if( ps->top_paste.shape != PAD_DEFAULT )
			{
				line.Format( "      top_paste: %d %s %s %s %s\n",
					ps->top_paste.shape, ws(ps->top_paste.size_h,m_units), ws(ps->top_paste.size_l,m_units), 
					ws(ps->top_paste.size_r,m_units), ws(ps->top_paste.radius,m_units) );
				file->WriteString( line );
			}
			if( ps->hole_size )
			{
				line.Format( "      inner_pad: %d %s %s %s %s %d\n",
					ps->inner.shape, ws(ps->inner.size_h,m_units), ws(ps->inner.size_l,m_units), 
					ws(ps->inner.size_r,m_units), ws(ps->inner.radius,m_units), ps->inner.connect_flag );
				file->WriteString( line );
			}
			if( ps->hole_size || ps->bottom.shape != PAD_NONE )
			{
				if( ps->bottom.connect_flag )
					line.Format( "      bottom_pad: %d %s %s %s %s %d\n",
						ps->bottom.shape, ws(ps->bottom.size_h,m_units), ws(ps->bottom.size_l,m_units), 
						ws(ps->bottom.size_r,m_units), ws(ps->bottom.radius,m_units), ps->bottom.connect_flag );
				else
					line.Format( "      bottom_pad: %d %s %s %s %s\n",
						ps->bottom.shape, ws(ps->bottom.size_h,m_units), ws(ps->bottom.size_l,m_units), 
						ws(ps->bottom.size_r,m_units), ws(ps->bottom.radius,m_units) );
				file->WriteString( line );
			}
			if( ps->bottom_mask.shape != PAD_DEFAULT )
			{
				line.Format( "      bottom_mask: %d %s %s %s %s\n",
					ps->bottom_mask.shape, ws(ps->bottom_mask.size_h,m_units), ws(ps->bottom_mask.size_l,m_units), 
					ws(ps->bottom_mask.size_r,m_units), ws(ps->bottom_mask.radius,m_units) );
				file->WriteString( line );
			}
			if( ps->bottom_paste.shape != PAD_DEFAULT )
			{
				line.Format( "      bottom_paste: %d %s %s %s %s\n",
					ps->bottom_paste.shape, ws(ps->bottom_paste.size_h,m_units), ws(ps->bottom_paste.size_l,m_units), 
					ws(ps->bottom_paste.size_r,m_units), ws(ps->bottom_paste.radius,m_units) );
				file->WriteString( line );
			}
		}
		file->WriteString( "\n" );

	}
	catch( CFileException * e )
	{
		CString str, s ((LPCSTR) IDS_FileError1), s2 = ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			str.Format( s, e->m_cause );
		else
			str.Format( s2, e->m_cause, e->m_lOsError,
			_sys_errlist[e->m_lOsError] );
		return 1;
	}
	return 0;
}

// create metafile and draw footprint into it
// CPT2 converted
//
HENHMETAFILE CShape::CreateMetafile( CMetaFileDC * mfDC, CDC * pDC, CRect const &window, CString ref, int bDrawSelectionRect )
{
	int x_size = window.Width();
	int y_size = window.Height();

	// CPT:  substituted in a call to GetBounds() (I get my jollies by culling duplicate code).  It's probably possible to factor out some
	// of the drawing that occurs later in the routine as well...
	CRect r = GetBounds(), rRef;
	// Account for the ref-text bounds also (which GetBounds() didn't do)
	m_ref->GenerateStrokes();
	r.left = min(r.left, m_ref->m_br.left);
	r.right = max(r.right, m_ref->m_br.right);
	r.bottom = min(r.bottom, m_ref->m_br.bottom);
	r.top = max(r.top, m_ref->m_br.top);

	// convert to mils
	int x_min = r.left/NM_PER_MIL;
	int x_max = r.right/NM_PER_MIL;
	int y_min = r.bottom/NM_PER_MIL;
	int y_max = r.top/NM_PER_MIL;

	// set up scale factor for drawing, mils per pixel
	double x_scale = (double)(x_max-x_min)/(double)x_size;
	double y_scale = (double)(y_max-y_min)/(double)y_size;
	double scale = max( x_scale, y_scale) * 1.2;
	if( scale < 1.0 )
		scale = 1.0;
	double x_center = (x_max+x_min)/2;
	double y_center = (y_max+y_min)/2;

	// set up frame rectangle for metafile
	int widthmm = pDC->GetDeviceCaps( HORZSIZE );
	int heightmm = pDC->GetDeviceCaps( VERTSIZE );
	int widthpixel = pDC->GetDeviceCaps( HORZRES );
	int heightpixel = pDC->GetDeviceCaps( VERTRES );

	// set up bounding rectangle, with bottom > top, in mils
	double left = x_center - scale*x_size/2;
	double right = x_center + scale*x_size/2;
	double top = y_center - scale*y_size/2;
	double bottom = y_center + scale*y_size/2;
	CRect rNM;
	rNM.left = left*100.0*widthmm/widthpixel;
	rNM.right = right*100.0*widthmm/widthpixel;
	rNM.top = -bottom*100.0*heightmm/heightpixel;		// notice Y flipped
	rNM.bottom = -top*100.0*heightmm/heightpixel;
	// offsets to ensure that origin is in rNM
	int xoffset = -(rNM.left+rNM.right)/2;
	rNM.left += xoffset;
	rNM.right += xoffset;
	xoffset = xoffset/(100.0*widthmm/widthpixel);
	int yoffset = -(rNM.top+rNM.bottom)/2;
	rNM.top += yoffset;
	rNM.bottom += yoffset;
	yoffset = yoffset/(100.0*heightmm/heightpixel);
	// create enhanced metafile
	mfDC->CreateEnhanced( NULL, NULL, rNM, NULL );
	mfDC->SetAttribDC( *pDC );

	// now we can draw using mils
	// flip Y since metafile likes top < bottom
	CPen * old_pen;
	CBrush * old_brush;
	CPen backgnd_pen( PS_SOLID, 1, C_RGB::black );
	CBrush backgnd_brush( C_RGB::black );
	CPen SMT_pad_pen( PS_SOLID, 1, C_RGB::green );
	CBrush SMT_pad_brush( C_RGB::green );
	CPen SMT_B_pad_pen( PS_SOLID, 1, C_RGB::red );
	CBrush SMT_B_pad_brush( C_RGB::red );
	CPen TH_pad_pen( PS_SOLID, 1, C_RGB::blue );
	CBrush TH_pad_brush( C_RGB::blue );

	// draw background
	old_pen = mfDC->SelectObject( &backgnd_pen );
	old_brush = mfDC->SelectObject( &backgnd_brush );
	mfDC->Rectangle( rNM );

	// draw pads and holes
	// iterate twice to draw top copper over bottom copper
	for( int ipass=0; ipass<2; ipass++ )
	{
		CIter<CPadstack> ips (&m_padstacks);
		for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
		{
			int x = ps->x_rel/NM_PER_MIL;
			int y = ps->y_rel/NM_PER_MIL;
			int angle = ps->angle;
			int hole_size = ps->hole_size/NM_PER_MIL;
			// CPT2 TODO.  Check that the following is right, maybe make it less confusing?
			CPad * p = &ps->top;
			if( ps->top.shape == PAD_NONE && ps->bottom.shape != PAD_NONE )
				p = &ps->bottom;
			int p_x_min, p_x_max, p_y_min, p_y_max;
			if( hole_size && ipass == 0 )
				continue;
			if( p == &ps->top && ipass == 0 )
				continue;
			if( p == &ps->bottom && ipass == 1 )
				continue;
			if( hole_size )
			{
				mfDC->SelectObject( &TH_pad_pen );
				mfDC->SelectObject( &TH_pad_brush );
			}
			else
			{
				if( p == &ps->top )
				{
					mfDC->SelectObject( &SMT_pad_pen );
					mfDC->SelectObject( &SMT_pad_brush );
				}
				else
				{
					mfDC->SelectObject( &SMT_B_pad_pen );
					mfDC->SelectObject( &SMT_B_pad_brush );
				}
			}
			if( p->shape == PAD_NONE )
			{
				p_x_min = x - hole_size/2;
				p_x_max = x + hole_size/2;
				p_y_min = y - hole_size/2;
				p_y_max = y + hole_size/2;
				mfDC->SelectStockObject( NULL_BRUSH);
				mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
			}
			else if( p->shape == PAD_ROUND )
			{
				p_x_min = x - (p->size_h/NM_PER_MIL)/2;
				p_x_max = x + (p->size_h/NM_PER_MIL)/2;
				p_y_min = y - (p->size_h/NM_PER_MIL)/2;
				p_y_max = y + (p->size_h/NM_PER_MIL)/2;
				mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					p_x_min = x - hole_size/2;
					p_x_max = x + hole_size/2;
					p_y_min = y - hole_size/2;
					p_y_max = y + hole_size/2;
					mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				}
			}
			else if( p->shape == PAD_SQUARE )
			{
				p_x_min = x - (p->size_h/NM_PER_MIL)/2;
				p_x_max = x + (p->size_h/NM_PER_MIL)/2;
				p_y_min = y - (p->size_h/NM_PER_MIL)/2;
				p_y_max = y + (p->size_h/NM_PER_MIL)/2;
				mfDC->Rectangle( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					p_x_min = x - hole_size/2;
					p_x_max = x + hole_size/2;
					p_y_min = y - hole_size/2;
					p_y_max = y + hole_size/2;
					mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				}
			}
			else if( p->shape == PAD_RECT )
			{
				CRect pr( -p->size_l, p->size_h/2, p->size_r, -p->size_h/2 );
				if( angle > 0 )
					RotateRect( &pr, angle, zero );
				p_x_min = x + pr.left/NM_PER_MIL;
				p_x_max = x + pr.right/NM_PER_MIL;
				p_y_min = y + pr.bottom/NM_PER_MIL;
				p_y_max = y + pr.top/NM_PER_MIL;
				mfDC->Rectangle( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					p_x_min = x - hole_size/2;
					p_x_max = x + hole_size/2;
					p_y_min = y - hole_size/2;
					p_y_max = y + hole_size/2;
					mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				}
			}
			else if( p->shape == PAD_RRECT )
			{
				CRect pr( -p->size_l, p->size_h/2, p->size_r, -p->size_h/2 );
				if( angle > 0 )
					RotateRect( &pr, angle, zero );
				int xi = x + pr.left/NM_PER_MIL;
				int xf = x + pr.right/NM_PER_MIL;
				int yi = y + pr.bottom/NM_PER_MIL;
				int yf = y + pr.top/NM_PER_MIL;
				int h = xf - xi;
				int v = yf - yi;
				int radius = p->radius/NM_PER_MIL;
				mfDC->RoundRect( xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset, 2*radius, 2*radius );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					xi = x - hole_size/2;
					xf = x + hole_size/2;
					yi = y - hole_size/2;
					yf = y + hole_size/2;
					mfDC->Ellipse( xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset );
				}
			}
			else if( p->shape == PAD_OVAL )
			{
				CRect pr( -p->size_l, p->size_h/2, p->size_r, -p->size_h/2 );
				if( angle > 0 )
					RotateRect( &pr, angle, zero );
				int xi = x + pr.left/NM_PER_MIL;
				int xf = x + pr.right/NM_PER_MIL;
				int yi = y + pr.bottom/NM_PER_MIL;
				int yf = y + pr.top/NM_PER_MIL;
				int h = abs(xf-xi);
				int v = abs(yf-yi);
				int radius = min(h,v);
				mfDC->RoundRect( xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset, radius, radius );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					xi = x - hole_size/2;
					xf = x + hole_size/2;
					yi = y - hole_size/2;
					yf = y + hole_size/2;
					mfDC->Ellipse( xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset );
				}
			}
			else if( p->shape == PAD_OCTAGON )
			{
				const double pi = 3.14159265359;
				POINT pt[8];
				double angle = pi/8.0;
				for( int iv=0; iv<8; iv++ )
				{
					pt[iv].x = x + (0.5*p->size_h/NM_PER_MIL) * cos(angle)+xoffset;
					pt[iv].y = -(y + (0.5*p->size_h/NM_PER_MIL) * sin(angle))+yoffset;
					angle += pi/4.0;
				}
				mfDC->Polygon( pt, 8 );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					p_x_min = x - hole_size/2;
					p_x_max = x + hole_size/2;
					p_y_min = y - hole_size/2;
					p_y_max = y + hole_size/2;
					mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				}
			}
		}
	}

	// draw part outline
	CIter<COutline> io (&m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
	{
		int thickness = o->m_w/NM_PER_MIL;
		CPen silk_pen( PS_SOLID, thickness, C_RGB::yellow );
		mfDC->SelectObject( &silk_pen );
		CIter<CContour> ictr (&o->contours);
		for (CContour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		{
			CIter<CSide> is (&ctr->sides);
			for (CSide *s = is.First(); s; s = is.Next())
			{
				// CPT2.  NB sides may be drawn out of geometrical order, but who cares?
				int dl_shape;
				if( s->m_style == CPolyline::STRAIGHT )
					dl_shape = DL_LINE;
				else if( s->m_style == CPolyline::ARC_CW )
					dl_shape = DL_ARC_CCW;				// notice CW/CCW flipped since Y flipped
				else if( s->m_style == CPolyline::ARC_CCW )
					dl_shape = DL_ARC_CW;
				else
					ASSERT(0);
				int xi = s->preCorner->x/NM_PER_MIL, yi = s->preCorner->y/NM_PER_MIL;
				int xf = s->postCorner->x/NM_PER_MIL, yf = s->postCorner->y/NM_PER_MIL;
				::DrawArc( mfDC, dl_shape, xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset, TRUE );
			}
		}
	}

	// draw ref text placeholder.  CPT2 TODO could also factor out shared code for rendering ref text strokes and rendering footprint-text
	// strokes (just below)
	int thickness = m_ref->m_stroke_width/NM_PER_MIL;
	CPen ref_pen( PS_SOLID, thickness, C_RGB::yellow );
	mfDC->SelectObject( &ref_pen );
	m_ref->GenerateStrokes();
	for (int is = 0; is < m_ref->m_stroke.GetSize(); is++)
	{
		int xi = m_ref->m_stroke[is].xi, yi = m_ref->m_stroke[is].yi;
		int xf = m_ref->m_stroke[is].xf, yf = m_ref->m_stroke[is].yf;
		// draw
		mfDC->MoveTo( xi/NM_PER_MIL+xoffset, -yi/NM_PER_MIL+yoffset );
		mfDC->LineTo( xf/NM_PER_MIL+xoffset, -yf/NM_PER_MIL+yoffset );
	}

	// draw text
	CIter<CText> it (&m_tl->texts);
	for (CText *t = it.First(); t; t = it.Next())
	{
		int thickness = t->m_stroke_width/NM_PER_MIL;
		CPen text_pen( PS_SOLID, thickness, C_RGB::yellow );
		mfDC->SelectObject( &text_pen );
		t->GenerateStrokes();
		for (int is = 0; is < t->m_stroke.GetSize(); is++)
		{
			int xi = t->m_stroke[is].xi, yi = t->m_stroke[is].yi;
			int xf = t->m_stroke[is].xf, yf = t->m_stroke[is].yf;
			// draw
			mfDC->MoveTo( xi/NM_PER_MIL+xoffset, -yi/NM_PER_MIL+yoffset );
			mfDC->LineTo( xf/NM_PER_MIL+xoffset, -yf/NM_PER_MIL+yoffset );
		}
	}

	// draw selection rectangle
	if( bDrawSelectionRect )
	{
		CPen sel_pen( PS_SOLID, 1, C_RGB::white );
		mfDC->SelectObject( &sel_pen );
		mfDC->MoveTo( m_sel_xi/NM_PER_MIL+xoffset, -m_sel_yi/NM_PER_MIL+yoffset );
		mfDC->LineTo( m_sel_xf/NM_PER_MIL+xoffset, -m_sel_yi/NM_PER_MIL+yoffset );
		mfDC->LineTo( m_sel_xf/NM_PER_MIL+xoffset, -m_sel_yf/NM_PER_MIL+yoffset );
		mfDC->LineTo( m_sel_xi/NM_PER_MIL+xoffset, -m_sel_yf/NM_PER_MIL+yoffset );
		mfDC->LineTo( m_sel_xi/NM_PER_MIL+xoffset, -m_sel_yi/NM_PER_MIL+yoffset );
		mfDC->SelectObject( old_pen );
	}

	// restore DC
	mfDC->SelectObject( old_brush );
	mfDC->SelectObject( old_pen );
	HENHMETAFILE hMF = mfDC->CloseEnhanced();
	return hMF;
}


// Get default centroid
// if no pads, returns (0,0)
CPoint CShape::GetDefaultCentroid()
{
	if( m_padstacks.GetSize() == 0 )
		return CPoint(0,0);
	CRect r = GetAllPadBounds();
	CPoint c( (r.left+r.right)/2, (r.top+r.bottom)/2 );
	return c;
}

// Get bounding rectangle of all pads
// if no pads, returns with rect.left = INT_MAX:
CRect CShape::GetAllPadBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	CIter<CPadstack> ips (&m_padstacks);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		CRect pad_r = ps->GetBounds();
		r.left = min( r.left, pad_r.left );
		r.bottom = min( r.bottom, pad_r.bottom );
		r.right = max( r.right, pad_r.right );
		r.top = max( r.top, pad_r.top );
	}
	return r;
}

// Get bounding rectangle of footprint
//
CRect CShape::GetBounds( BOOL bIncludeLineWidths )
{
	CRect br = GetAllPadBounds();

	CIter<COutline> io (&m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
	{
		CRect polyr;
		if( bIncludeLineWidths )
			polyr = o->GetBounds();
		else
			polyr = o->GetCornerBounds();
		br.left = min( br.left, polyr.left ); 
		br.bottom = min( br.bottom, polyr.bottom ); 
		br.right = max( br.right, polyr.right ); 
		br.top = max( br.top, polyr.top ); 
	}

	CRect tr;
	BOOL bText = m_tl->GetTextBoundaries( &tr );
	if( bText )
	{
		br.left = min( br.left, tr.left ); 
		br.bottom = min( br.bottom, tr.bottom );  
		br.right = max( br.right, tr.right ); 
		br.top = max( br.top, tr.top ); 
	}
	if(	br.left == INT_MAX || br.bottom == INT_MAX || br.right == INT_MIN || br.top == INT_MIN )
	{
		// no elements, make it a 100 mil square
		br.left = 0;
		br.right = 100*NM_PER_MIL;
		br.bottom = 0;
		br.top = 100*NM_PER_MIL;
	}
	return br;
}

// Get bounding rectangle of footprint, not including polyline widths
//
CRect CShape::GetCornerBounds()
{
	return GetBounds( FALSE );
}


// draw footprint into display list (while Footprint Editor is active)
// CPT2 updated.
//
int CShape::Draw()
{
	CDisplayList *dlist = doc->m_dlist;
	SMFontUtil *smf = doc->m_smfontutil;
	if( !dlist )
		return NO_DLIST;
	if (bDrawn)
		Undraw();					// NB this is different from other classes' Draw(), because it's only done from the fp editor.

	// draw pins.  CPT2 Note that for each padstack ps we use drawing elements ps->top.dl_el, ps->inner.dl_el, etc.  We also use ps->dl_el to represent the
	// hole (if any), plus of course ps->dl_sel for the selector.
	CIter<CPadstack> ips (&m_padstacks);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		CPoint pin (ps->x_rel, ps->y_rel);
		int sel_w = 0;	// width of selection rect
		int sel_h = 0;	// height of selection rect
		CDLElement * pad_sel;
		for( int il=0; il<7; il++ ) 
		{
			// set layer for pads within padstack
			int pad_lay;
			CPad * p; 
			if( il == 0 )
			{
				pad_lay = LAY_FP_TOP_COPPER;
				p = &ps->top;
			}
			else if( il == 1 )
			{
				pad_lay = LAY_FP_INNER_COPPER;
				p = &ps->inner;	
			}
			else if( il == 2 )
			{
				pad_lay = LAY_FP_BOTTOM_COPPER;
				p = &ps->bottom;
			}
			else if( il == 3 )
			{
				pad_lay = LAY_FP_TOP_MASK;
				p = &ps->top_mask;
			}
			else if( il == 4 )
			{
				pad_lay = LAY_FP_TOP_PASTE;
				p = &ps->top_paste;
			}
			else if( il == 5 )
			{
				pad_lay = LAY_FP_BOTTOM_MASK;
				p = &ps->bottom_mask;
			}
			else if( il == 6 )
			{
				pad_lay = LAY_FP_BOTTOM_PASTE;
				p = &ps->bottom_paste;
			}

			// draw pad
			int gtype;
			if( p->shape == PAD_ROUND )
			{
				if( pad_lay >= LAY_FP_TOP_COPPER && pad_lay <= LAY_FP_BOTTOM_COPPER )
					gtype = DL_CIRC;
				else
					gtype = DL_HOLLOW_CIRC;
				p->dl_el = dlist->Add( ps, CDLElement::DL_OTHER, pad_lay, 
					gtype, 1, p->size_h, 0, 0, 
					pin.x, pin.y, 0, 0, pin.x, pin.y );
				sel_w = max( sel_w, p->size_h );
				sel_h = max( sel_h, p->size_h );
			}
			else if( p->shape == PAD_SQUARE )
			{
				if( pad_lay >= LAY_FP_TOP_COPPER && pad_lay <= LAY_FP_BOTTOM_COPPER )
					p->dl_el = dlist->Add( ps, CDLElement::DL_OTHER, pad_lay, 
						DL_SQUARE, 1, p->size_h, 0, 0, 
						pin.x, pin.y, 0, 0, pin.x, pin.y );
				else
					p->dl_el = dlist->Add( ps, CDLElement::DL_OTHER, pad_lay, 
						DL_HOLLOW_RECT, 1, 1, 0, 0,
						pin.x - p->size_h/2, pin.y - p->size_h/2, 
						pin.x + p->size_h/2, pin.y + p->size_h/2, 
						pin.x, pin.y );
				sel_w = max( sel_w, p->size_h );
				sel_h = max( sel_h, p->size_h );
			}
			else if( p->shape == PAD_RECT 
				|| p->shape == PAD_RRECT 
				|| p->shape == PAD_OVAL )
			{
				CPoint pad_pf, pad_pi;
				pad_pi.x = pin.x - p->size_l;
				pad_pi.y = pin.y - p->size_h/2;
				pad_pf.x = pin.x + p->size_r;
				pad_pf.y = pin.y + p->size_h/2;
				// rotate pad about pin if necessary
				if( ps->angle > 0 )
				{
					RotatePoint( &pad_pi, ps->angle, pin );
					RotatePoint( &pad_pf, ps->angle, pin );
				}
				// add pad to dlist
				gtype = DL_RECT;
				if( pad_lay >= LAY_FP_TOP_COPPER && pad_lay <= LAY_FP_BOTTOM_COPPER )
				{
					if( p->shape == PAD_RRECT )
						gtype = DL_RRECT;
					if( p->shape == PAD_OVAL )
						gtype = DL_OVAL;
				}
				else
				{
					gtype = DL_HOLLOW_RECT;
					if( p->shape == PAD_RRECT )
						gtype = DL_HOLLOW_RRECT;
					if( p->shape == PAD_OVAL )
						gtype = DL_HOLLOW_OVAL;
				}
				p->dl_el = dlist->Add( ps, CDLElement::DL_OTHER, pad_lay, 
					gtype, 1, 0, 0, 0,										// CPT r292 arg list bug fix
					pad_pi.x, pad_pi.y, pad_pf.x, pad_pf.y, 
					pin.x, pin.y, p->radius );
				sel_w = max( sel_w, abs(pad_pf.x-pad_pi.x) );
				sel_h = max( sel_h, abs(pad_pf.y-pad_pi.y) );
			}
			else if( p->shape == PAD_OCTAGON )
			{
				gtype = DL_OCTAGON;
				if( pad_lay < LAY_FP_TOP_COPPER || pad_lay > LAY_FP_BOTTOM_COPPER )
					gtype = DL_HOLLOW_OCTAGON;
				p->dl_el = dlist->Add( ps, CDLElement::DL_OTHER, pad_lay, 
					gtype, 1, p->size_h, 0, 0, 
					pin.x, pin.y, 0, 0, pin.x, pin.y );
				sel_w = max( sel_w, p->size_h );
				sel_h = max( sel_h, p->size_h );
			}
			else
				p->dl_el = NULL;											// nothing drawn
		}

		if( ps->hole_size )
		{
			// add to display list
			ps->dl_el = dlist->AddMain( ps, LAY_FP_PAD_THRU, 
				DL_HOLE, 1, ps->hole_size, 0, 0,
				pin.x, pin.y, 0, 0, pin.x, pin.y );
			sel_w = max( sel_w, ps->hole_size );
			sel_h = max( sel_h, ps->hole_size );
		}

		if( sel_w && sel_h )
			// add selector
			ps->dl_sel = dlist->AddSelector( ps, LAY_FP_PAD_THRU, 
				DL_HOLLOW_RECT, 1, 1, 0,
				pin.x - sel_w/2, pin.y - sel_h/2, 
				pin.x + sel_w/2, pin.y + sel_h/2, 
				pin.x, pin.y );
	}

	// draw ref designator text
	m_ref->DrawRelativeTo( NULL );								// CPT2: DrawRelativeTo( NULL ) ensures that text on the bottom layers gets mirrored
	// draw value text (CPT: only if height is >0)
	if (m_value->m_font_size)
		m_value->DrawRelativeTo( NULL );

	// now draw outline polylines
	CIter<COutline> io (&m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
	{
		int sel_box_size = max( o->m_w, 5*NM_PER_MIL );
		o->SetSelBoxSize( sel_box_size );
		o->Draw();
	}

	// draw text
	CIter<CText> it (&m_tl->texts);
	for (CText *t = it.First(); t; t = it.Next())
		t->DrawRelativeTo( NULL );

	// draw centroid and glue-spots
	m_centroid->Draw();
	CIter<CGlue> ig (&m_glues);
	for (CGlue *g = ig.First(); g; g = ig.Next())
		g->Draw();

	bDrawn = true;
	return NOERR;
}

void CShape::Undraw()
{
	// CPT2 converted
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;

	CIter<CPadstack> ips (&m_padstacks);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		dl->Remove( ps->dl_el );
		dl->Remove( ps->dl_sel );
		dl->Remove( ps->top.dl_el );
		dl->Remove( ps->inner.dl_el );
		dl->Remove( ps->bottom.dl_el );
		dl->Remove( ps->top_mask.dl_el );
		dl->Remove( ps->top_paste.dl_el );
		dl->Remove( ps->bottom_mask.dl_el );
		dl->Remove( ps->bottom_paste.dl_el );
		ps->dl_el = ps->dl_sel = ps->top.dl_el = ps->inner.dl_el =
			ps->bottom.dl_el = ps->top_mask.dl_el = ps->top_paste.dl_el =
			ps->bottom_mask.dl_el = ps->bottom_paste.dl_el = NULL;
	}

	m_ref->Undraw();
	m_value->Undraw();

	CIter<COutline> io (&m_outlines);
	for (COutline *o = io.First(); o; o = io.Next())
		o->Undraw();

	CIter<CText> it (&m_tl->texts);
	for (CText *t = it.First(); t; t = it.Next())
		t->Undraw();

	m_centroid->Undraw();
	CIter<CGlue> ig (&m_glues);
	for (CGlue *g = ig.First(); g; g = ig.Next())
		g->Undraw();

	bDrawn = false;
}


// Start dragging row of pads
//
/* CPT2 obsolete, use CFootprintView::StartDraggingGroup
void CShape::StartDraggingPadRow( CDC * pDC, CHeap<CPadstack> *row )
{
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	// make pads invisible
	CIter<CPadstack> ips (row);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		dl->Set_visible( ps->dl_el, 0 );
		dl->Set_visible( ps->top.dl_el, 0 );			// CPT2 TODO how 'bout the other layers?
		dl->Set_visible( ps->inner.dl_el, 0 );
		dl->Set_visible( ps->bottom.dl_el, 0 );
	}
	// drag.  CPT2 First get pad row bounds (used to be a separate function, but why bother?)
	CRect rr;
	rr.left = rr.bottom = INT_MAX;
	rr.right = rr.top = INT_MIN;
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		CRect r = ps->GetBounds();
		rr.left = min( r.left, rr.left );
		rr.bottom = min( r.bottom, rr.bottom );
		rr.right = max( r.right, rr.right );
		rr.top = max( r.top, rr.top );
	}
	int x = row->First()->x_rel;
	int y = row->First()->y_rel;
	dl->StartDraggingRectangle( pDC, x, y,
						rr.left - x, rr.bottom - y, rr.right - x, rr.top - y,
						0, LAY_SELECTION );
}

// Cancel dragging row of pads
//
void CShape::CancelDraggingPadRow( CHeap<CPadstack> *row )
{
	// make pad visible
	CDisplayList *dl = doc->m_dlist;
	CIter<CPadstack> ips (row);
	for (CPadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		dl->Set_visible( ps->dl_el, 1 );
		dl->Set_visible( ps->top.dl_el, 1 );
		dl->Set_visible( ps->inner.dl_el, 1 );
		dl->Set_visible( ps->bottom.dl_el, 1 );
	}
	dl->StopDragging();
}
*/

void CShape::ShiftToInsertPadName( CString * astr, int n )
{
	// CPT2 converted.  User wants to give a new pin the name "astr+n".  Check if this would result in a conflict;  if so, shift the old pin's name
	// up by one.
	CString test;
	test.Format( "%s%d", *astr, n );
	CPadstack *ps = GetPadstackByName( &test );
	if (!ps)
		// no conflict, shift not needed
		return;
	ShiftToInsertPadName( astr, n+1 );
	ps->name.Format( "%s%d", *astr, n+1 );
}

// Generate selection rectangle from footprint elements
// doesn't draw it
//
bool CShape::GenerateSelectionRectangle( CRect * r )
{
	int num_elements = GetNumPins() + m_outlines.GetSize() + m_tl->texts.GetSize();
	if( num_elements == 0 )
		return false;

	CRect br = GetBounds( TRUE );

	br.left -= 10*NM_PER_MIL;
	br.right += 10*NM_PER_MIL;
	br.bottom -= 10*NM_PER_MIL;
	br.top += 10*NM_PER_MIL;

	m_sel_xi = br.left;
	m_sel_xf = br.right;
	m_sel_yi = br.bottom;
	m_sel_yf = br.top;
	return true;
}


void CShape::GenerateValueParams() {
	// CPT:  factored-out helper function.  If value has been hidden or isn't specified, we call this function & position it relative to the ref-text
	int ref_size = m_ref->m_font_size;
	int ref_xi = m_ref->m_x, ref_yi = m_ref->m_y;
	int ref_angle = m_ref->m_angle;
	int value_xi, value_yi;
	if( ref_angle == 0 )
	{
		value_xi = ref_xi;
		value_yi = ref_yi - ref_size*2;
	}
	else if( ref_angle == 90 )
	{
		value_xi = ref_xi - ref_size*2;
		value_yi = ref_yi;
	}
	else if( ref_angle == 180 )
	{
		value_xi = ref_xi;
		value_yi = ref_yi + ref_size*2;
	}
	else
	{
		value_xi = ref_xi + ref_size*2;
		value_yi = ref_yi;
	}
	m_value->Move(value_xi, value_yi, ref_angle, false, false, LAY_FP_SILK_TOP, ref_size, m_ref->m_stroke_width);
}


CShape *CShapeList::GetShapeByName( CString *name )
{
	CIter<CShape> is (&shapes);
	for (CShape *s = is.First(); s; s = is.Next())
		if (s->m_name == *name)
			return s;
	return NULL;
}

// write footprint info from this shapelist (the local cache) to file
//
void CShapeList::WriteShapes( CStdioFile * file )
{
	file->WriteString( "[footprints]\n\n" );
	CIter<CShape> is (&shapes);
	for (CShape *s = is.First(); s; s = is.Next())
		s->WriteFootprint( file );
}

// read footprint info into this list.  bFindSection is true by default
void CShapeList::ReadShapes( CStdioFile * pcb_file, bool bFindSection )
{
	shapes.RemoveAll();
	CString in_str;
	if( bFindSection )
	{
		// find beginning of shapes section
		do
		{
			if (!pcb_file->ReadString( in_str ))
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
		int pos = pcb_file->GetPosition();
		if (!pcb_file->ReadString( in_str ))
			if( bFindSection )
				throw new CString( "unexpected EOF in project file" );
			else
				break;
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
			CShape * s = new CShape (m_doc);
			pcb_file->Seek( pos, CFile::begin );					// back up
			if (!s->MakeFromFile( pcb_file, "", "", 0 ))
				shapes.Add(s);
		}
	}
}

