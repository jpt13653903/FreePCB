// Net.cpp --- source file for classes related most closely to nets, many of which are descendants of cpcb_item:
//  cvertex2, ctee, cseg2, cconnect2, cnet2, cnetlist.  Note that carea2 is dealt with in Polyline.h/.cpp.

#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "Net.h"
#include "Part.h"
#include "Polyline.h"

/**********************************************************************************************/
/*  RELATED TO cseg2/cvertex2/cconnect2                                                          */
/**********************************************************************************************/

cvertex2::cvertex2(cconnect2 *c, int _x, int _y):				// Added args
	cpcb_item(c->doc)
{
	m_con = c;
	m_con->vtxs.Add(this);
	m_net = c->m_net;
	x = _x, y = _y;
	tee = NULL;
	pin = NULL;
	preSeg = postSeg = NULL;
	force_via_flag = via_w = via_hole_w = 0;
	dl_hole = dl_thermal = NULL;
	SetNeedsThermal();
}

cvertex2::cvertex2(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	// This constructor (like its analogues in the other cpcb_item classes) is used only during undo operations, where a blank object gets constructed
	// with a particular uid before being filled in
	m_con = NULL;
	m_net = NULL;
	tee = NULL;
	pin = NULL;
	preSeg = postSeg = NULL;
	dl_hole = dl_thermal = NULL;
}

bool cvertex2::IsOnPcb()
{
	if (!m_net->IsOnPcb()) return false;
	if (!m_con->IsOnPcb()) return false;
	return m_con->vtxs.Contains(this);
}

bool cvertex2::IsVia() 
	{ return via_w>0 || tee && tee->via_w>0; }

int cvertex2::GetLayer()
{
	// Return LAY_PAD_THRU for vias.  Otherwise return the layer of whichever emerging seg isn't a ratline.  Failing that, return LAY_RAT_LINE.
	if (tee) return tee->GetLayer();
	if (pin) return pin->GetLayer();
	if (via_w) return LAY_PAD_THRU;
	if (preSeg && preSeg->m_layer != LAY_RAT_LINE) return preSeg->m_layer;
	if (postSeg && postSeg->m_layer != LAY_RAT_LINE) return postSeg->m_layer;
	return LAY_RAT_LINE;
}

void cvertex2::GetStatusStr( CString * str )
{
	int u = doc->m_view->m_units;
	CString type_str, x_str, y_str, via_w_str, via_hole_str, s;
	if( pin )
		type_str.LoadStringA(IDS_PinVertex);	// should never happen
	else if( tee )
		s.LoadStringA(IDS_TVertex),
		type_str.Format( s, tee->UID() );
	else if( !preSeg || !postSeg )
		type_str.LoadStringA(IDS_EndVertex);
	else
		type_str.LoadStringA(IDS_Vertex);
	::MakeCStringFromDimension( &x_str, x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	::MakeCStringFromDimension( &y_str, y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	if( via_w )
	{
		::MakeCStringFromDimension( &via_w_str, via_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &via_hole_str, via_hole_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		CString s ((LPCSTR) IDS_XYVia);
		str->Format( s, type_str, x_str, y_str, via_w_str, via_hole_str );
		// CPT2 added (used to be there, liked it)
		if (force_via_flag)
			str->Append(" (F)");
	}
	else
	{
		CString s ((LPCSTR) IDS_XYNoVia);
		str->Format( s,	type_str, x_str, y_str );
	}
}

void cvertex2::GetTypeStatusStr( CString * str )
{
	if( pin )
		str->Format( "%s.%s", pin->part->ref_des, pin->pin_name );
	else if( tee )
		str->LoadStringA(IDS_Tee);
	/* CPT2. Having connects described as "from end vertex to end vertex" seems overlengthy.  How about "from vtx to vtx"?
	else if( type == V_END )
	{
		str->LoadStringA(IDS_EndVertex);
	}
	*/
	else
		str->LoadStringA(IDS_Vertex);
}


bool cvertex2::Remove()
{
	// Derived from old cnet::RemoveVertex() functions.  Remove vertex from the network.  If it's a tee-vertex, all associated vertices must be
	// axed as well.  If it's an end vertex of another type, remove vertex and the single attached seg.  If it's a middle vertex, merge the two adjacent
	// segs.
	if (tee)
	{
		tee->Remove(true);														// Removes tee and all its constituents.
		return false;
	}

	Undraw();																	// To get rid of selector drawing-el (?)
	int nSegsInConnect = m_con->segs.GetSize();
	if (nSegsInConnect<2) 
		{ m_con->Remove(); return true; }

	if (!preSeg)
		postSeg->RemoveBreak();
	else if (!postSeg)
		preSeg->RemoveBreak();
	else 
	{
		if (preSeg->m_layer != postSeg->m_layer)
			postSeg->SetLayer(LAY_RAT_LINE);
		preSeg->RemoveMerge(1);
		m_con->MergeUnroutedSegments();
	}

	return false;
}

void cvertex2::SetConnect(cconnect2 *c)
{
	// CPT2 new.  Attach this to the given connect.  If it's already attached to a connect, detach it from the old one.  Does not alter preSeg/postSeg
	if (m_con==c) return;
	if (m_con)
		m_con->vtxs.Remove(this);
	c->vtxs.Add(this);
	m_con = c;
}

bool cvertex2::IsLooseEnd() 
	{ return (!preSeg || !postSeg) && !pin && !tee && via_w==0; }

void cvertex2::ForceVia()
{
	force_via_flag = 1;
	ReconcileVia();
}

void cvertex2::UnforceVia()
{
	force_via_flag = 0;
	ReconcileVia();
}

bool cvertex2::IsViaNeeded()
{
	if (tee) return tee->IsViaNeeded();
	if (pin) return false;
	if (force_via_flag) return true;
	if (!preSeg || !postSeg) return false;					// CPT2 TODO: check...
	if (preSeg->m_layer == postSeg->m_layer) return false;
	return preSeg->m_layer!=LAY_RAT_LINE && postSeg->m_layer!=LAY_RAT_LINE;
}

void cvertex2::SetViaWidth()
{
	// CPT2.  We've determined that "this" needs a via, so determine the appropriate width based on incoming segs
	if (force_via_flag && via_w)
		return;
	MustRedraw();
	via_w = via_hole_w = 0;
	if (preSeg)
		m_net->CalcViaWidths(preSeg->m_width, &via_w, &via_hole_w);
	if (postSeg) 
	{
		int vw2, vhw2;
		m_net->CalcViaWidths(postSeg->m_width, &vw2, &vhw2);
		if (vw2 > via_w) 
			via_w = vw2, via_hole_w = vhw2;
	}
	if (via_w<=0 || via_hole_w<=0)
		// Not likely.  Use defaults...
		via_w = m_net->def_via_w,
		via_hole_w = m_net->def_via_hole_w;
}

void cvertex2::ReconcileVia()
{
	// CPT2.  Gets an appropriate width for a via on this vertex, if any.  Calls MustRedraw() as appropriate
	if (tee)
		{ tee->ReconcileVia(); return; }

	if (IsViaNeeded()) 
		SetViaWidth();
	else if (via_w || via_hole_w)
		via_w = via_hole_w = 0,
		MustRedraw();
}

bool cvertex2::TestHit(int _x, int _y, int _layer)
{
	// CPT2 Semi-supplants cnet2::TestHitOnVertex.  This routine also behaves appropriately if the vertex is part of a tee or associated with a pin.
	if (pin)
		return pin->TestHit(_x, _y, _layer);

	// First check that the layer is OK:
	if (_layer>0)
	{
		int layer = GetLayer();
		if (layer != LAY_PAD_THRU && layer != LAY_RAT_LINE && layer != _layer)
			return false; 
	}
	// Determine a test radius (perhaps this is a little obsessive...)
	int test_w = max( via_w, 10*NM_PER_MIL );
	if (preSeg)
		test_w = max( test_w, preSeg->m_width );
	if (postSeg)
		test_w = max( test_w, postSeg->m_width );
	if (tee)
	{
		test_w = max( test_w, tee->via_w );
		citer<cvertex2> iv (&tee->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next()) 
		{
			if (v->preSeg) 
				test_w = max(test_w, v->preSeg->m_width);
			if (v->postSeg)
				test_w = max(test_w, v->postSeg->m_width);
		}
	}

	double dx = x - _x, dy = y - _y;
	double d = sqrt( dx*dx + dy*dy );
	return d < test_w/2;
}

void cvertex2::Move( int _x, int _y )
{
	if (tee)
		{ tee->Move(_x, _y); return; }
	m_con->MustRedraw();						// CPT2 crude but probably adequate.
	x = _x;
	y = _y;
	ReconcileVia();
	SetNeedsThermal();
}

cconnect2 * cvertex2::SplitConnect()
{
	// Split a connection into two connections sharing a tee-vertex
	// return pointer to the new connection
	// both connections will end at the tee-vertex
	// CPT2 adapted from old cnet::SplitConnectAtVertex.  Assumes that this is in the middle of a seg (therefore not part of a tee)
	ASSERT(preSeg && postSeg);
	m_net->MustRedraw();										// CPT2 crude but probably adequate.
	cconnect2 *old_c = m_con;
	cconnect2 * new_c = new cconnect2(m_net);
	cvertex2 *new_v = new cvertex2(new_c, x, y);
	new_c->head = new_v;
	new_c->tail = old_c->tail;
	new_v->postSeg = postSeg;
	postSeg->preVtx = new_v;
	// Now reassign the second half of connect's vtxs and segs to new_c
	for (cseg2 *s = postSeg; s; s = s->postVtx->postSeg)
		s->SetConnect(new_c),
		s->postVtx->SetConnect(new_c);
	// Truncate old_c
	old_c->tail = this;
	this->postSeg = NULL;
	// Bind this and new_v together in a new tee
	ctee *t = new ctee(m_net);
	t->Add(this);
	t->Add(new_v);
	return new_c;
}

cseg2 * cvertex2::AddRatlineToPin( cpin2 *pin )
{
	// Add a ratline from a vertex to a pin
	// CPT2 Adapted from cnet::AddRatlineFromVtxToPin().  NOW RETURNS THE RATLINE SEG.
	cconnect2 * c = m_con;
	if (IsTraceVtx())
	{
		// normal internal vertex in a trace 
		// split trace and make this vertex a tee.  SplitConnect() also calls m_net->MustRedraw()
		SplitConnect();
		// Create new connect and attach its first end to the new tee
		cconnect2 *new_c = new cconnect2 (m_net);
		cvertex2 *new_v1 = new cvertex2 (new_c, x, y);
		new_c->Start(new_v1);
		this->tee->Add(new_v1);
		new_c->AppendSegment(pin->x, pin->y, LAY_RAT_LINE, 0);
		new_c->tail->pin = pin;
		return new_v1->postSeg;
	}
	else if( !tee )
	{
		// loose endpoint vertex, just extend it to the pin
		c->MustRedraw();
		cseg2 *ret;
		if (!postSeg)
			c->AppendSegment(pin->x, pin->y, LAY_RAT_LINE, 0),
			c->tail->pin = pin,
			ret = c->tail->preSeg;
		else
			c->PrependSegment(pin->x, pin->y, LAY_RAT_LINE, 0),
			c->head->pin = pin,
			ret = c->head->postSeg;
		return ret;
	}
	else 
	{
		// new connection from tee-vertex to pin
		cconnect2 *new_c = new cconnect2 (m_net);
		cvertex2 *new_v1 = new cvertex2 (new_c, x, y);
		new_c->Start(new_v1);
		tee->Add(new_v1);
		new_c->AppendSegment(pin->x, pin->y, LAY_RAT_LINE, 0);
		new_c->tail->pin = pin;
		new_c->MustRedraw();
		return new_v1->postSeg;
	}
}

void cvertex2::StartDraggingStub( CDC * pDC, int _x, int _y, int layer1, int w, 
								   int layer_no_via, int crosshair, int inflection_mode )
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	dl->CancelHighlight();
	// CPT2 The following doesn't seem right to me, plus it's a lot of trouble, so I commented it out.
	// For instance if "this" is a forced via, why hide it?
	/*
	// Hide this vertex (if a via) and its thermals.
	SetVisible(false);
	citer<carea2> ia (&m_net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next()) 
		for (int i=0; i < a->dl_via_thermal.GetSize(); i++)
			if (a->dl_via_thermal[i] -> item == this)
				dl->Set_visible(a->dl_via_thermal[i], 0);
	*/
	// start dragging, start point is preceding vertex
	dl->StartDraggingLine( pDC, _x, _y, this->x, this->y, layer1, 
		w, layer_no_via, w*3/2, w,												// CPT r295.  Put in approx args for via_w and via_hole_w.  Fairly bogus... 
		crosshair, DSS_STRAIGHT, inflection_mode );
}

// CPT2 NB CancelDraggingStub() is pretty useless at this point, so it's eliminated.  (Just need to do a dlist->StopDragging().)

// Start dragging vertex to reposition it
//
void cvertex2::StartDragging( CDC * pDC, int x, int y, int crosshair )
{
	// cancel previous selection and make segments and via invisible
	CDisplayList *dl = doc->m_dlist;
	cconnect2 *c = m_con;
	cnet2 *net = m_net;
	dl->CancelHighlight();
	if (preSeg)
		dl->Set_visible(preSeg->dl_el, 0);				// CPT2 TODO change name to SetVisible (et al)
	if (postSeg)
		dl->Set_visible(postSeg->dl_el, 0);
	SetVisible( FALSE );

	// start dragging
	if( preSeg && postSeg )
	{
		// vertex with segments on both sides
		int xi = preSeg->preVtx->x;
		int yi = preSeg->preVtx->y;
		int xf = postSeg->postVtx->x;
		int yf = postSeg->postVtx->y;
		int layer1 = preSeg->m_layer;
		int layer2 = postSeg->m_layer;
		int w1 = preSeg->m_width;
		int w2 = postSeg->m_width;
		dl->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1, 
									layer2, w1, w2, DSS_STRAIGHT, DSS_STRAIGHT, 
									0, 0, 0, 0, crosshair );
	}
	else
	{
		// end-vertex, only drag one segment
		cseg2 *s = preSeg? preSeg: postSeg;
		cvertex2 *other = preSeg? preSeg->preVtx: postSeg->postVtx;
		int xi = other->x;
		int yi = other->y;
		int layer1 = s->m_layer;
		int w1 = s->m_width;
		dl->StartDraggingLine( pDC, x, y, xi, yi, layer1, 
									w1, layer1, 0, 0, crosshair, DSS_STRAIGHT, 0 );
	}
}

void cvertex2::CancelDragging()
{
	// make segments and via visible
	CDisplayList *dl = doc->m_dlist;
	if (preSeg)
		dl->Set_visible(preSeg->dl_el, 1);
	if (postSeg)
		dl->Set_visible(postSeg->dl_el, 1);
	SetVisible( TRUE );
	dl->StopDragging();
}


int cvertex2::Draw()
{
	cnetlist * nl = m_net->m_nlist;
	CDisplayList *dl = doc->m_dlist;
	if (tee)
		return NOERR;					// CPT2.  Rather than draw the individual vertices, just draw the tee itself.
	if (pin)
		return NOERR;					// CPT2.  Similarly, draw the pin and its selector, not the vtx.
	if (bDrawn)
		return ALREADY_DRAWN;
	if (!m_net->bVisible)
		return NOERR;

	// draw via if via_w > 0
	if( via_w )
	{
		int n_layers = nl->GetNumCopperLayers();
		dl_els.SetSize( n_layers );
		for( int il=0; il<n_layers; il++ )
		{
			int layer = LAY_TOP_COPPER + il;
			dl_els[il] = dl->AddMain( this, layer, DL_CIRC, 1, via_w, 0, 0,
									x, y, 0, 0, 0, 0 );
		}
		dl_hole = dl->AddMain( this, LAY_PAD_THRU, DL_HOLE, 1, via_hole_w, 0, 0,
							x, y, 0, 0, 0, 0 );
		// CPT2.  Trying a wider selector for vias (depending on hole size)
		dl_sel = dl->AddSelector( this, LAY_SELECTION, DL_HOLLOW_RECT, 
			1, 0, 0, x-via_w/2, y-via_w/2, x+via_w/2, y+via_w/2, 0, 0 );
		// CPT2.  Now draw a thermal, if the bNeedsThermal flag is set
		if (bNeedsThermal)
			dl_thermal = dl->Add( this, dl_element::DL_OTHER, LAY_RAT_LINE, DL_X, 1,
								2*via_w/3, 0, 0, x, y, 0, 0, 0, 0 );
	}
	else
	{
		// draw selection box for vertex, using layer of adjacent
		// segments.  CPT2 TODO figure out why the layer of the selector matters...
		int sel_layer;
		if( preSeg )
			sel_layer = preSeg->m_layer;
		else
			sel_layer = postSeg->m_layer;
		dl_sel = dl->AddSelector( this, sel_layer, DL_HOLLOW_RECT, 
			1, 0, 0, x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL, x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL, 0, 0 );
	}

	bDrawn = true;
	return NOERR;
}

void cvertex2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !IsDrawn()) return;
	for( int i=0; i<dl_els.GetSize(); i++ )
		dl->Remove( dl_els[i] );
	dl_els.RemoveAll();
	dl->Remove( dl_sel );
	dl->Remove( dl_hole );
	dl->Remove( dl_thermal );
	dl_sel = dl_hole = dl_thermal = NULL;
	bDrawn = false;
}

void cvertex2::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	if (tee)
		// Since vertices within a tee don't have selectors, it's unlikely that Highlight() will be called on them.
		// But just in case...
		{ tee->Highlight(); return; }
	if (pin)
		// Similarly for pin vertices:  highlight the pin, not the vtx
		{ pin->Highlight(); return; }

	// highlite square width is adjacent seg-width*2, via_width or 20 mils
	// whichever is greatest
	int w = 0;
	if (preSeg)
		w = 2 * preSeg->m_width;
	if (postSeg)
		w = max(w, 2 * postSeg->m_width);
	w = max( w, via_w );
	w = max( w, 20*PCBU_PER_MIL );
	dl->Highlight( DL_HOLLOW_RECT, x - w/2, y - w/2, x + w/2, y + w/2, 0 );
}

void cvertex2::SetVisible( bool bVis )
{
	for( int il=0; il<dl_els.GetSize(); il++ )
		if( dl_els[il] )
			dl_els[il]->visible = bVis;
	if( dl_hole )
		dl_hole->visible = bVis;
	if (dl_thermal)
		dl_thermal->visible = bVis;
}


int cvertex2::GetViaConnectionStatus( int layer )
{
	// CPT2 derived from old CNetList function.
	int status = VIA_NO_CONNECT;
	// check for end vertices of traces to pads
	/* CPT2. Old code had:
	if( iv == 0 )
		return status;
	if( c->end_pin != cconnect::NO_END  && iv == (c->NumSegs() + 1) )
		return status;
	I couldn't figure out the logic behind this...
	*/

	// check for normal via pad
	if (via_w == 0 && !tee)
		return status;
	if (tee && tee->via_w == 0)
		return status;

	// check for trace connection to via pad
	if (tee)
	{
		citer<cvertex2> iv (&tee->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			if (v->preSeg && v->preSeg->m_layer == layer)
				{ status |= VIA_TRACE; break; }
			else if (v->postSeg && v->postSeg->m_layer == layer)
				{ status |= VIA_TRACE; break; }
	}
	else if (preSeg && preSeg->m_layer == layer)
		status |= VIA_TRACE;
	else if (postSeg && postSeg->m_layer == layer)
		status |= VIA_TRACE;

	// see if it connects to any area in this net on this layer
	citer<carea2> ia (&m_net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		if( a->m_layer == layer && a->TestPointInside(x, y))
			{ status |= VIA_AREA; break; }
	return status;
}

void cvertex2::GetViaPadInfo( int layer, int * pad_w, int * pad_hole_w, int * connect_status )
{
	// get via parameters for vertex
	// note: if the vertex is the end-vertex of a branch, the via parameters
	// will be taken from the tee-vertex that the branch connects to
	// CPT2 converted from CNetList func.  CPT2 TODO I tweaked the logic in this a bit, because I couldn't see the reasoning behind the old routine.
	//  I'm still not too certain about what's best here.
	int con_stat = GetViaConnectionStatus( layer );
	int w = via_w;
	int hole_w = via_hole_w;
	if (tee)
		w = tee->via_w,
		hole_w = tee->via_hole_w;
	if (con_stat == VIA_NO_CONNECT)
		w = 0;
	if( layer > LAY_BOTTOM_COPPER && w > 0 )
		w = hole_w + 2*doc->m_nlist->m_annular_ring;

	if( pad_w )
		*pad_w = w;
	if( pad_hole_w )
		*pad_hole_w = hole_w;
	if( connect_status )
		*connect_status = con_stat;
}


bool cvertex2::SetNeedsThermal()
{
	// CPT2 new, but related to the old CNetList::SetAreaConnections.  Sets bNeedsThermal to true if some net area overlaps this vertex.
	bool oldVal = bNeedsThermal;
	bNeedsThermal = false;
	citer<carea2> ia (&m_net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		if (a->TestPointInside(x, y))
			{ bNeedsThermal = true; break; }
	if (bNeedsThermal != oldVal)
		MustRedraw();
	return bNeedsThermal;
}



ctee::ctee(cnet2 *n)
	: cpcb_item (n->doc)
{ 
// CPT2 TODO test code, remove
if (UID()==858)
via_w = 1;
	via_w = via_hole_w = 0; 
	n->tees.Add(this);
	dl_hole = dl_thermal = NULL;
}

ctee::ctee(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	dl_hole = dl_thermal = NULL;
}

int ctee::GetLayer()
{
	// If this has a via, return LAY_PAD_THRU.
	// Otherwise search the component vtxs for an emerging segment that has a layer other than LAY_RAT_LINE.  Return the first such layer if found.
	// By default return LAY_RAT_LINE
	if (via_w) 
		return LAY_PAD_THRU;
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		if (v->preSeg && v->preSeg->m_layer != LAY_RAT_LINE)
			return v->preSeg->m_layer;
		else if (v->postSeg && v->postSeg->m_layer != LAY_RAT_LINE)
			return v->postSeg->m_layer;
	return LAY_RAT_LINE;
}

void ctee::GetStatusStr( CString * str )
{
	cvertex2 *v = vtxs.First();
	if (!v) 
		{ *str = "???"; return; }

	int u = doc->m_view->m_units;
	CString type_str, x_str, y_str, via_w_str, via_hole_str, s;
	s.LoadStringA(IDS_TVertex),
	type_str.Format( s, UID() );
	::MakeCStringFromDimension( &x_str, v->x, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	::MakeCStringFromDimension( &y_str, v->y, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	if( via_w )
	{
		::MakeCStringFromDimension( &via_w_str, via_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		::MakeCStringFromDimension( &via_hole_str, via_hole_w, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
		s.LoadStringA( IDS_XYVia );
		str->Format( s, type_str, x_str, y_str, via_w_str, via_hole_str );
	}
	else
		s.LoadStringA( IDS_XYNoVia ),
		str->Format( s,	type_str, x_str, y_str );
	if (v->force_via_flag)
		str->Append(" (F)");

}

void ctee::SaveUndoInfo()
{
	doc->m_undo_items.Add( new cutee(this) );
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		doc->m_undo_items.Add( new cuvertex(v) );
}

void ctee::Remove(bool bRemoveVtxs)
{
	// Disconnect this from everything:  that is, remove references to this from all vertices in vtxs, and then clear out this->vtxs.  The
	// garbage collector will later delete this.
	Undraw();
	cnet2 *net = vtxs.First()? vtxs.First()->m_net: NULL;
	if (net) net->tees.Remove(this);
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		v->tee = NULL;
		if (bRemoveVtxs) v->Remove();
	}
	vtxs.RemoveAll();
}

void ctee::Add(cvertex2 *v)
{
	// New.  Attach v to this->vtxs, and set v->tee.  Check first if v needs detaching from some other tee.
	if (v->tee && v->tee!=this)
		v->tee->Remove(v);
	v->tee = this;
	vtxs.Add(v);
}

bool ctee::Adjust()
{
	// Typically called after a segment/connect attached to a tee-vertex is removed.  At that point we see if there are still 3+ vertices 
	// remaining within the tee;  if so, do nothing and return false.  If there's 0-1 vertex in the tee (not that likely, I think), just 
	// remove the tee from the network and return true. If there are 2 attached connects, combine them before removing the tee & returning true.
	int nItems = vtxs.GetSize();
	if (nItems > 2) 
		{ ReconcileVia(); return false; }
	if (nItems == 0) 
		{ Remove(); return true; }
	citer<cvertex2> iv (&vtxs);
	cvertex2 *v0 = iv.First(), *v1 = iv.Next();
	v0->m_net->SaveUndoInfo( cnet2::SAVE_CONNECTS );
	if (nItems == 1)
	{
		Remove();
		v0->MustRedraw();
		v0->ReconcileVia();
		return true;
	}
	cconnect2 *c0 = v0->m_con, *c1 = v1->m_con;
	if (c0==c1) 
		// Remaining connect is a circle!  A tee is still required in that case. 
		{ ReconcileVia(); return false; }
	c0->CombineWith(c1, v0, v1);						// Calls c0->MustRedraw()
	Remove();
	v0->ReconcileVia();
	return true;
}

void ctee::ForceVia()
{
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->force_via_flag = 1;
	ReconcileVia();
}

void ctee::UnforceVia()
{
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->force_via_flag = 0;
	ReconcileVia();
}

bool ctee::IsViaNeeded()
{
	int layer0 = -1;
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		if (v->force_via_flag) return true;							// If any constituent vtx has a forced via, the tee needs one too
		int layer = v->preSeg? v->preSeg->m_layer: LAY_RAT_LINE;
		if (layer!=LAY_RAT_LINE)
			if (layer0==-1) layer0 = layer;
			else if (layer!=layer0) return true;
		layer = v->postSeg? v->postSeg->m_layer: LAY_RAT_LINE;
		if (layer!=LAY_RAT_LINE)
			if (layer0==-1) layer0 = layer;
			else if (layer!=layer0) return true;
	}
	return false;
}

void ctee::ReconcileVia()
{
	cvertex2 *v = vtxs.First();
	if (v && v->force_via_flag && via_w>0)
		return;														// Don't tinker with preexisting via width for forced tee vias...
	MustRedraw();
	via_w = via_hole_w = 0;
	if (!IsViaNeeded())
		return;
	// Set via_w and via_hole_w, based on the max of the widths for the constituent vtxs (which have to be set first)
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->SetViaWidth();
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		if (v->via_w > via_w)
			via_w = v->via_w, via_hole_w = v->via_hole_w;
}

void ctee::Remove(cvertex2 *v, bool fAdjust) 
{
	Undraw();
	vtxs.Remove(v);
	v->tee = NULL;
	if (fAdjust)
		Adjust(),
		ReconcileVia();
}

void ctee::Move(int _x, int _y) 
{
	// CPT2 new
	cvertex2 *first = vtxs.First();
	cnet2 *net = first->m_net;
	net->MustRedraw();								// Maybe overkill but simple.
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->x = _x, v->y = _y;
	ReconcileVia();
}

int ctee::Draw()
{
	cvertex2 *v = vtxs.First();
	if (!v) 
		return NOERR;				// Weird...
	if (!v->m_net->bVisible)
		return NOERR;
	int x = v->x, y = v->y;
	cnetlist * nl = v->m_net->m_nlist;
	CDisplayList *dl = doc->m_dlist;
	if (!dl) 
		return NO_DLIST;
	if (bDrawn) 
		return ALREADY_DRAWN;
	// Make darn sure that none of the constituent vertices is drawn
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->Undraw();
	// draw via if via_w > 0
	if( via_w )
	{
		int n_layers = nl->GetNumCopperLayers();
		dl_els.SetSize( n_layers );
		for( int il=0; il<n_layers; il++ )
		{
			int layer = LAY_TOP_COPPER + il;
			dl_els[il] = dl->AddMain( this, layer, DL_CIRC, 1, via_w, 0, 0,
								      x, y, 0, 0, 0, 0 );
		}
		dl_hole = dl->AddMain( this, LAY_PAD_THRU, DL_HOLE, 1, via_hole_w, 0, 0,
							   x, y, 0, 0, 0, 0 );
	}

	// draw selection box for vertex, using LAY_THRU_PAD if via, or layer of adjacent
	// segments if no via
	int sel_layer;
	if( via_w )
		sel_layer = LAY_SELECTION;
	else if( v->preSeg )
		sel_layer = v->preSeg->m_layer;
	else
		sel_layer = v->postSeg->m_layer;
	dl_sel = dl->AddSelector( this, sel_layer, DL_HOLLOW_RECT, 
		1, 0, 0, x-10*PCBU_PER_MIL, y-10*PCBU_PER_MIL, x+10*PCBU_PER_MIL, y+10*PCBU_PER_MIL, 0, 0 );
	bDrawn = true;
	return NOERR;
}

void ctee::Undraw()
{
	// CPT2.  Practically identical to cvertex2::Undraw()
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !IsDrawn()) return;
	for( int i=0; i<dl_els.GetSize(); i++ )
		dl->Remove( dl_els[i] );
	dl_els.RemoveAll();
	dl->Remove( dl_sel );
	dl->Remove( dl_hole );
	dl_sel = NULL;
	dl_hole = NULL;
	bDrawn = false;
}

void ctee::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;

	// highlite square width is adjacent seg-width*2, via_width or 20 mils
	// whichever is greatest
	int w = 0;
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		if (v->preSeg)
			w = max(w, 2 * v->preSeg->m_width);
		if (v->postSeg)
			w = max(w, 2 * v->postSeg->m_width);
	}
	w = max( w, via_w );
	w = max( w, 20*PCBU_PER_MIL );
	int x = vtxs.First()->x, y = vtxs.First()->y;
	dl->Highlight( DL_HOLLOW_RECT, x - w/2, y - w/2, x + w/2, y + w/2, 0 );
}

void ctee::SetVisible( bool bVis )
{
	for( int il=0; il<dl_els.GetSize(); il++ )
		if( dl_els[il] )
			dl_els[il]->visible = bVis;
	if( dl_hole )
		dl_hole->visible = bVis;
	if (dl_thermal)
		dl_thermal->visible = bVis;
}

void ctee::StartDragging( CDC * pDC, int x, int y, int crosshair )
{
	// cancel previous selection and make incoming segments and via invisible.  Also create a DragRatlineArray for the drawing list.
	CDisplayList *dl = doc->m_dlist;
	cnet2 *net = GetNet();
	dl->CancelHighlight();
	dl->MakeDragRatlineArray( vtxs.GetSize(), 0 );
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		CPoint pi, pf = CPoint(0,0);
		if (v->preSeg)
			dl->Set_visible(v->preSeg->dl_el, 0),				// CPT2 TODO change name to SetVisible (et al)
			pi.x = v->preSeg->preVtx->x,
			pi.y = v->preSeg->preVtx->y;
		else if (v->postSeg)
			dl->Set_visible(v->postSeg->dl_el, 0),
			pi.x = v->postSeg->postVtx->x,
			pi.y = v->postSeg->postVtx->y;
		dl->AddDragRatline( pi, pf );
	}
	SetVisible( FALSE );
	dl->StartDraggingArray( pDC, 0, 0, 0, LAY_SELECTION );
}

void ctee::CancelDragging()
{
	// make emerging segments and via visible
	CDisplayList *dl = doc->m_dlist;
	dl->StopDragging();
	SetVisible( TRUE );
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		if (v->preSeg)
			dl->Set_visible(v->preSeg->dl_el, 1);
		else if (v->postSeg)
			dl->Set_visible(v->postSeg->dl_el, 1);
}



cseg2::cseg2(cconnect2 *c, int _layer, int _width)							// CPT2 added args.  Replaces Initialize()
	: cpcb_item( c->doc )
{
	m_con = c;
	m_con->segs.Add(this);
	m_net = c->m_net;
	m_layer = _layer;
	m_width = _width;
	m_curve = 0;
	preVtx = postVtx = NULL;
}

cseg2::cseg2(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	m_con = NULL;
	m_net = NULL;
	preVtx = postVtx = NULL;
}

bool cseg2::IsOnPcb()
{
	if (!m_net->IsOnPcb()) return false;
	if (!m_con->IsOnPcb()) return false;
	return m_con->segs.Contains(this);
}

// CPT:  added width param.  If this is 0 (the default) replace it with this->m_width
void cseg2::GetStatusStr( CString * str, int width )
{
	int u = doc->m_view->m_units;
	if (width==0) width = m_width;
	CString w_str;
	::MakeCStringFromDimension( &w_str, width, u, FALSE, FALSE, FALSE, u==MIL?1:3 );
	CString s ((LPCSTR) IDS_SegmentW);
	str->Format( s, w_str );
}

void cseg2::GetBoundingRect( CRect *br )
{
	// CPT2 new, helper for DRC.  Return a bounding rectangle for this seg, including the vias on the head/tail. 
	int preW = preVtx->tee? preVtx->tee->via_w: preVtx->via_w;
	preW = max(preW, m_width);
	int postW = postVtx->tee? postVtx->tee->via_w: postVtx->via_w;
	postW = max(postW, m_width);
	int preXMax = preVtx->x + preW/2;
	int preXMin = preVtx->x - preW/2;
	int preYMax = preVtx->y + preW/2;
	int preYMin = preVtx->y - preW/2;
	int postXMax = postVtx->x + postW/2;
	int postXMin = postVtx->x - postW/2;
	int postYMax = postVtx->y + postW/2;
	int postYMin = postVtx->y - postW/2;
	br->left = min(preXMin, postXMin);
	br->right = max(preXMax, postXMax);
	br->bottom = min(preYMin, postYMin);
	br->top = max(preYMax, postYMax);
}

void cseg2::SetConnect(cconnect2 *c)
{
	// CPT2 new.  Attach this to the given connect.  If it's already attached to a connect, detach it from the old one.  Does not alter preVtx/postVtx
	if (m_con==c) return;
	if (m_con)
		m_con->segs.Remove(this);
	c->segs.Add(this);
	m_con = c;
}

void cseg2::SetWidth( int w, int via_w, int via_hole_w )
{
	MustRedraw();
	if( m_layer != LAY_RAT_LINE && w )
		m_width = w;
	if( preVtx->via_w && via_w )
		preVtx->via_w = via_w,
		preVtx->via_hole_w = via_hole_w;
	if( postVtx->via_w && via_w )
		postVtx->via_w = via_w,
		postVtx->via_hole_w = via_hole_w;
}

int cseg2::SetLayer( int layer )
{
	// Set segment layer (must be a copper layer, not the ratline layer)
	// returns 1 if unable to comply due to SMT pad
	// CPT2 converted from old CNetList::ChangeSegmentLayer
	// check layer settings of adjacent vertices to make sure this is legal

	// check starting pad layer
	int pad_layer = preVtx->pin? preVtx->pin->pad_layer: layer;
	if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
		return 1;
	// check destination pad layer
	pad_layer = postVtx->pin? postVtx->pin->pad_layer: layer;
	if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
		return 1; 
	// change segment layer
	MustRedraw();
	m_layer = layer;
	// now adjust vias
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	return 0;
}

char cseg2::GetDirectionLabel()
{
	// CPT2 new.  Used for making labels when user does a shift-click.  Return char is '-', '/', '|', or '\' depending on the direction of the seg
	double dx = preVtx->x - postVtx->x;
	double dy = preVtx->y - postVtx->y;
	double slope = dx==0? 1000: dy/dx;
	if (fabs(slope) > 3)
		return '|';
	else if (fabs(slope) < .33)
		return '-';
	else if (slope > 0)
		return '/';
	else
		return '\\';
}

void cseg2::Divide( cvertex2 *v, cseg2 *s, int dir )
{
	// Divide "this" segment into two subsegments, with v the vertex in the middle.  If dir==0,
	// new segment "s" becomes the first subseg, stretching from this->preVtx to v, and "this" 
	// becomes the second subseg, stretching from v to this->postVtx.  If dir==1, s becomes the
	// second subseg and "this" becomes the first.
	// Assumes v and s have been created with m_con members equal to this->m_con.
	m_con->MustRedraw();
	m_con->segs.Add(s);
	m_con->vtxs.Add(v);
	if (dir==0)
		s->preVtx = preVtx,	s->postVtx = v,
		preVtx->postSeg = s,
		v->preSeg = s, v->postSeg = this,
		preVtx = v;
	else
		s->preVtx = v, s->postVtx = postVtx,
		postVtx->preSeg = s,
		v->preSeg = this, v->postSeg = s,
		postVtx = v;
}

bool cseg2::InsertSegment(int x, int y, int _layer, int _width, int dir )
{
	// CPT2, derived from CNetList::InsertSegment().  Used when "this" is a ratline that is getting partially routed.  this will be split at
	// a new vertex (position (x,y)), and either the first or second half will be converted to layer/width, depending on dir.  However, if (x,y)
	// is close to the final point of this (tolerance 10 nm), we'll just convert this to a single seg with the given layer/width and return false.
	// NB NO DRAWING in accordance with my proposed new policy.
	const int TOL = 10;													// CPT2 TODO this seems mighty small...
	cvertex2 *dest = dir==0? postVtx: preVtx;
	bool bHitDest = abs(x - dest->x) + abs(y - dest->y) < TOL;
	if (dest->pin)
		bHitDest &= dest->TestHit(x, y, _layer);						// Have to make sure layer is OK in case of SMT pins.
	if (bHitDest)
	{
		MustRedraw();
		m_layer = _layer;
		m_width = _width;
		preVtx->ReconcileVia();
		postVtx->ReconcileVia();
		return false;
	}

	cvertex2 *v = new cvertex2(m_con, x, y);
	cseg2 *s = new cseg2(m_con, _layer, _width);
	Divide(v, s, dir);													// Calls m_con->MustRedraw()
	s->preVtx->ReconcileVia();
	s->postVtx->ReconcileVia();
	return true;
}

int cseg2::Route( int layer, int width )
{
	// Old CNetList::RouteSegment()
	// Convert segment from unrouted to routed
	// returns 1 if segment can't be routed on given layer due to connection to SMT pad.
	// Adds/removes vias as necessary
	// First check layer settings of adjacent vertices to make sure this is legal:
	if (preVtx->pin)
	{
		int pad_layer = preVtx->pin->pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if (postVtx->pin)
	{
		int pad_layer = postVtx->pin->pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}

	// modify segment parameters
	m_layer = layer;
	m_width = width;

	// now adjust vias
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	MustRedraw();
	return 0;
}

void cseg2::Unroute(int dx, int dy, int end)
{
	// Old CNetList::UnrouteSegment().  Ditched the return value, which was confusing and unused.
	bool bUnrouted = UnrouteWithoutMerge( dx, dy, end );
	if (bUnrouted)
		m_con->MergeUnroutedSegments();				// This routine calls MustRedraw() for the whole connection.
}

bool cseg2::UnrouteWithoutMerge(int dx, int dy, int end) 
{
	// Old CNetList::UnrouteSegmentWithoutMerge().
	// Unroute segment, but don't merge with adjacent unrouted segments
	// ?? Assume that there will be an eventual call to MergeUnroutedSegments() to set vias
	// dx, dy and end arguments are 1/1/0 by default.  If, say, dx==0, dy==100000, end==1, then caller is about
	// to move a group/part to which the segment's end #1 is attached (by distance (0,100000)).  If the segment is itself vertical
	// (or nearly so) then we don't really need to unroute, and we return false.
	if (m_layer == LAY_RAT_LINE) return false;
	bool bUnroute = true;
	int xi = preVtx->x, yi = preVtx->y;
	int xf = postVtx->x, yf = postVtx->y;
	if (dx==0 && abs(xi-xf) < abs(dy/10))
	{
		// Segment is almost vertical, and one of its endpoints is going to move vertically
		// If end 0 is going to move, we will unroute only if it's getting moved to the opposite side of end 1.  Analogously if end 1 is moving.
		if (end==0) 
			bUnroute = double(yf-yi)*(yf-(yi+dy)) <= 0;			// "double" to prevent overflow issues
		else        
			bUnroute = double(yf-yi)*((yf+dy)-yi) <= 0;
	}
	if (dy==0 && abs(yi-yf) < abs(dx/10)) 
	{
		if (end==0) 
			bUnroute = double(xf-xi)*(xf-(xi+dx)) <= 0;
		else        
			bUnroute = double(xf-xi)*((xf+dx)-xi) <= 0;
	}
	if (!bUnroute) 
		return false;

	m_layer = LAY_RAT_LINE;
	m_width = 0;

	MustRedraw();
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	return true;
}

bool cseg2::RemoveMerge(int end)
{
	// Replaces old CNetList::RemoveSegment(), cnet::RemoveSegAndVertexByIndex(), RemoveVertexAndSegByIndex().
	// If this is a segment in the middle of a connect, combine it with its neighbor:  "end" indicates which vertex is
	//   going to get eliminated in the process (0 => remove preVtx, 1 => remove postVtx).
	// If this is an end-segment and "end" indicates the terminval vertex, then we just shorten the connect;  in other words, we call this->RemoveBreak().
	// Routine returns true if the entire connection has been destroyed because connect has just 1 segment.
	if (m_con->NumSegs()<2) 
		{ m_con->Remove(); return true; }

	if (!preVtx->preSeg && end==0 || !postVtx->postSeg && end==1)
		// Seg is at end of trace!
		return RemoveBreak();
	m_con->MustRedraw();
	m_con->segs.Remove(this);
	if (end==0) 
		m_con->vtxs.Remove(preVtx),
		preVtx->preSeg->postVtx = postVtx,
		postVtx->preSeg = preVtx->preSeg;
	else
		m_con->vtxs.Remove(postVtx),
		postVtx->postSeg->preVtx = preVtx,
		preVtx->postSeg = postVtx->postSeg;
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();
	return false;
}

bool cseg2::RemoveBreak()
{
	// Replaces old cnet::RemoveSegmentAdjustTees().  Remove segment from its connect, and if it's in the middle somewhere then we'll have
	// to split the old connect into 2.  If it's at an end point, just remove the segment, plus check for tees at the end point and adjust as needed.
	// Return true if the entire connect is destroyed, otherwise return false.
	if (m_con->NumSegs()<2) 
		{ m_con->Remove(); return true; }
	
	m_con->MustRedraw();
	m_con->segs.Remove(this);
	if (!preVtx->preSeg)
	{
		// this is an initial segment
		m_con->vtxs.Remove(preVtx);
		m_con->head = postVtx;
		postVtx->preSeg = NULL;
		if (preVtx->tee)
			preVtx->tee->Remove(preVtx);
		postVtx->ReconcileVia();
		// If new initial seg is a ratline, remove it too.
		if (postVtx->postSeg->m_layer == LAY_RAT_LINE)
			return postVtx->postSeg->RemoveBreak();
		return false;
	}
	if (!postVtx->postSeg)
	{
		// this is a final segment
		m_con->vtxs.Remove(postVtx);
		m_con->tail = preVtx;
		preVtx->postSeg = NULL;
		if (postVtx->tee)
			postVtx->tee->Remove(postVtx);
		preVtx->ReconcileVia();
		// If new final seg is a ratline, remove it too
		if (preVtx->preSeg->m_layer == LAY_RAT_LINE)
			return preVtx->preSeg->RemoveBreak();
		return false;
	}
	
	// Remove middle segment.  Create new connect for the latter half of the existing connect.
	cconnect2 *newCon = new cconnect2(m_net);
	newCon->MustRedraw();
	for (cvertex2 *v = postVtx; 1; v = v->postSeg->postVtx)
	{
		v->SetConnect(newCon);
		if (!v->postSeg) break;
		v->postSeg->SetConnect(newCon);
	}
	newCon->head = postVtx;
	newCon->tail = m_con->tail;
	m_con->tail = preVtx;
	postVtx->preSeg = preVtx->postSeg = NULL;
	preVtx->ReconcileVia();
	postVtx->ReconcileVia();

	ctee *headT = m_con->head->tee;
	if (headT && newCon->tail->tee == headT)
		// It's just possible that the original connect was a circle.  Having removed "this", it's now two connects with a single tee uniting their 
		// endpoints.  By calling Adjust() on that tee we'll get the two connects united into a single line.
		headT->Adjust();

	// Deal with dangling ratlines that the removal of this seg may have left behind:
	if (preVtx->preSeg->m_layer == LAY_RAT_LINE)
		preVtx->preSeg->RemoveBreak();
	if (postVtx->postSeg->m_layer == LAY_RAT_LINE)
		postVtx->postSeg->RemoveBreak();

	return false;
}

void cseg2::StartDragging( CDC * pDC, int x, int y, int layer1, int layer2, int w, 
								   int layer_no_via, int dir, int crosshair )
{
	// cancel previous selection and make segment invisible
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	dl->Set_visible(dl_el, 0);
	// CPT2 TODO:  Rationalize the whole 3-layer business in the arg list:
	dl->StartDraggingLineVertex( pDC, x, y, preVtx->x, preVtx->y, postVtx->x, postVtx->y, 
								layer1,	layer2, w, 1, DSS_STRAIGHT, DSS_STRAIGHT, 
								layer_no_via, w*3/2, w, dir, crosshair );		// CPT r295 put in approximate args for via_w and via_hole_w.  This
																				// whole business seems a bit bogus to me...
}

void cseg2::CancelDragging()
{
	// make segment visible
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible(dl_el, 1);
	dl->StopDragging();
}

void cseg2::StartDraggingNewVertex( CDC * pDC, int x, int y, int layer, int w, int crosshair )
{
	// Start dragging new vertex in existing trace segment.  CPT2 converted from CNetList func.
	// cancel previous selection and make segment invisible
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	dl->Set_visible(dl_el, 0);
	// start dragging
	dl->StartDraggingLineVertex( pDC, x, y, preVtx->x, preVtx->y, postVtx->x, postVtx->y, 
								layer, layer, w, w, DSS_STRAIGHT, DSS_STRAIGHT, 
								layer, 0, 0, 0, crosshair );
}

void cseg2::CancelDraggingNewVertex()
{
	// CPT2 converted from CNetList func.
	// make segment visible
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible(dl_el, 1);
	dl->StopDragging();
}

void cseg2::StartMoving( CDC * pDC, int x, int y, int crosshair )
{
	// cancel previous selection and make segments and via invisible
	cseg2 *preSeg = preVtx->preSeg, *postSeg = postVtx->postSeg;
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	dl->Set_visible(dl_el, 0);
	preVtx->SetVisible( FALSE );
	postVtx->SetVisible( FALSE );
	if (preSeg)
		dl->Set_visible(preSeg->dl_el, 0);
	if (postSeg)
		dl->Set_visible(postSeg->dl_el, 0);

	int xb = 0, yb = 0;
	if (preSeg)
		xb = preSeg->preVtx->x,
		yb = preSeg->preVtx->y;
	int xi = preVtx->x;
	int yi = preVtx->y;
	int xf = postVtx->x;
	int yf = postVtx->y;
	int xe = 0, ye = 0;
	if (postSeg)
		xe = postSeg->postVtx->x,
		ye = postSeg->postVtx->y;
	int layer0 = 0, layer1, layer2 = 0;
	int w0 = 0, w1, w2 = 0;
	if (preSeg)
		layer0 = preSeg->m_layer,
		w0 = preSeg->m_width;
	layer1 = m_layer;
	w1 = m_width;
	if (postSeg)
		layer2 = postSeg->m_layer,
		w2 = postSeg->m_width;
	dl->StartDraggingLineSegment( pDC, x, y, xb, yb, xi, yi, xf, yf, xe, ye,
								layer0, layer1, layer2,
								w0,		w1,		w2,
								preSeg? DSS_STRAIGHT: DSS_NONE, DSS_STRAIGHT, postSeg? DSS_STRAIGHT: DSS_NONE,
								0, 0, 0, 
								crosshair );
}

void cseg2::CancelMoving()
{
	// CPT2 derived from CNetList::CancelMovingSegment()
	// make segments and via visible
	CDisplayList *dl = doc->m_dlist;
	cseg2 *preSeg = preVtx->preSeg, *postSeg = postVtx->postSeg;
	if (preSeg)
		dl->Set_visible(preSeg->dl_el, 1);
	dl->Set_visible(dl_el, 1);
	if (postSeg)
		dl->Set_visible(postSeg->dl_el, 1);
	preVtx->SetVisible(true);
	postVtx->SetVisible(true);
	dl->StopDragging();
}


int cseg2::Draw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) 
		return NO_DLIST;
	if (bDrawn) 
		return ALREADY_DRAWN;
	if (!m_net->bVisible)
		return NOERR;
	int vis = m_layer == LAY_RAT_LINE? m_net->bVisible: 1;
	int shape = DL_LINE;
	if( m_curve == CW )
		shape = DL_CURVE_CW;
	else if( m_curve == CCW )
		shape = DL_CURVE_CCW;
	dl_el = dl->AddMain( this, m_layer, shape, vis, 
		m_width, 0, 0, preVtx->x, preVtx->y, postVtx->x, postVtx->y, 0, 0 );
	dl_sel = dl->AddSelector( this, m_layer, DL_LINE, vis, 
		m_width, 0, preVtx->x, preVtx->y, postVtx->x, postVtx->y, 0, 0 );
	bDrawn = true;
	return NOERR;
}

void cseg2::Highlight( bool bThin )
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	int w = bThin? 1: dl->Get_w(dl_el);
	dl->Highlight( DL_LINE, dl->Get_x(dl_el), dl->Get_y(dl_el),
							dl->Get_xf(dl_el), dl->Get_yf(dl_el),
							w, dl_el->layer );
}

void cseg2::SetVisible( bool bVis )
{
	doc->m_dlist->Set_visible( dl_el, bVis );
}


cconnect2::cconnect2( cnet2 * _net ) 
	: cpcb_item (_net->doc)
{
	m_net = _net;
	m_net->connects.Add(this);
	locked = 0;
}

cconnect2::cconnect2(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	m_net = NULL;
}

bool cconnect2::IsOnPcb()
	{ return m_net->IsOnPcb() && m_net->connects.Contains(this); }

void cconnect2::GetStatusStr( CString * str )
{
	CString net_str, type_str, locked_str, from_str, to_str;
	m_net->GetStatusStr( &net_str );
	if( NumSegs() == 1 && head->postSeg->m_layer == LAY_RAT_LINE )
		type_str.LoadStringA(IDS_Ratline);
	else
		type_str.LoadStringA(IDS_Connection);
	locked_str = "";
	if( head->pin && tail->pin && locked)
		locked_str = " (L)";
	head->GetTypeStatusStr( &from_str );
	tail->GetTypeStatusStr( &to_str );
	CString s ((LPCSTR) IDS_FromTo);
	str->Format( s, net_str, type_str, from_str, to_str, locked_str );
}

void cconnect2::SaveUndoInfo()
{
	// CPT2 In the case of higher-level entities like cconnect2 and cnet2, save all the constituents as well (it might be possible to
	// do something more economical, but this is simplest at least for now).
	doc->m_undo_items.Add( new cuconnect(this) );
	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		doc->m_undo_items.Add( new cuseg(s) );			// NB CFreePcbDoc::FinishUndoRecord() will have to check for dupes.
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		doc->m_undo_items.Add( new cuvertex(v) );
	if (head->tee)
		doc->m_undo_items.Add( new cutee(head->tee) );
	if (tail->tee)
		doc->m_undo_items.Add( new cutee(tail->tee) );
}

void cconnect2::SetWidth( int w, int via_w, int via_hole_w )
{
	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->SetWidth(w, via_w, via_hole_w);
}

void cconnect2::Remove()
{
	// CPT2. User wants to delete connection, so detach it from the network (garbage collector will later delete this object and its
	// constituents for good).
	// Any tee structures attached at either end get cleaned up.
	m_net->SaveUndoInfo( cnet2::SAVE_CONNECTS );						// Playing it safe...
	Undraw();
	m_net->connects.Remove(this);
	if (head->tee && head->tee==tail->tee)
		// Connect is a loop, tricky special case.  The "false" argument below prevents 
		// the calling of ctee::Adjust(), which might cause a crash
		head->tee->Remove(head, false);					
	else if (head->tee)
		head->tee->Remove(head);
	if (tail->tee)
		tail->tee->Remove(tail);
}


void cconnect2::Start( cvertex2 *v )
{
	// CPT2 new.  After this routine, connect will consist of a single vertex (v) and no segs.  Assumes v was constructed so that it points to "this"
	MustRedraw();
	vtxs.RemoveAll();
	segs.RemoveAll();
	vtxs.Add(v);
	head = tail = v;
	v->preSeg = v->postSeg = NULL;
}

void cconnect2::AppendSegAndVertex( cseg2 *s, cvertex2 *v, cvertex2 *after )
{
	// Append s+v into this connection after vertex "after".  Assumes that s+v were constructed so that they point to "this"
	MustRedraw();
	vtxs.Add(v);
	segs.Add(s);
	cseg2 *nextSeg = after->postSeg;
	after->postSeg = s;
	s->preVtx = after;
	s->postVtx = v;
	v->preSeg = s;
	v->postSeg = nextSeg;
	if (nextSeg)
		nextSeg->preVtx = v;
	else 
		tail = v;
}

void cconnect2::PrependVertexAndSeg( cvertex2 *v, cseg2 *s )
{
	// Similar to AppendSegAndVertex, but used at the start of the connection.
	MustRedraw();
	vtxs.Add(v);
	segs.Add(s);
	v->preSeg = NULL;
	v->postSeg = s;
	s->preVtx = v;
	s->postVtx = head;
	head->preSeg = s;
	head = v;
}

// Append new segment to connection 
void cconnect2::AppendSegment( int x, int y, int layer, int width )
{
	// add new vertex and segment at end of connect.  NB does not do any drawing or undrawing
	cseg2 *s = new cseg2 (this, layer, width);
	cvertex2 *v = new cvertex2 (this, x, y);
	AppendSegAndVertex( s, v, tail );
}

void cconnect2::PrependSegment( int x, int y, int layer, int width )
{
	// Similar to AppendSegment, but inserts at the connect's beginning
	cseg2 *s = new cseg2 (this, layer, width);
	cvertex2 *v = new cvertex2 (this, x, y);
	PrependVertexAndSeg( v, s );
}

void cconnect2::ReverseDirection()
{
	// CPT2.  I'm going to see if it works not to call MustRedraw().
	for (cseg2 *s = head->postSeg, *prev = NULL, *next; s; prev = s, s = next)
	{
		next = s->postVtx->postSeg;
		cvertex2 *tmp = s->preVtx;
		s->preVtx = s->postVtx;
		s->postVtx = tmp;
		s->postVtx->preSeg = s;
		s->postVtx->postSeg = prev;
		s->preVtx->preSeg = next;
		s->preVtx->postSeg = s;
	}
	cvertex2 *tmp = head;
	head = tail;
	tail = tmp;
}

void cconnect2::CombineWith(cconnect2 *c2, cvertex2 *v1, cvertex2 *v2) 
{
	// Combine "this" with connect c2.  To do this, identify vertex v1 (from one end of "this") with v2 (from one end of c2), reversing
	// connects as necessary.  After removing v2 and reassigning c2's remaining segs and vertices to this, removes c2.
	MustRedraw();
	c2->Undraw();
	if (head==v1)
		ReverseDirection();
	else 
		ASSERT(tail==v1);
	if (c2->tail==v2)
		c2->ReverseDirection();
	else 
		ASSERT(c2->head==v2);
	v1->postSeg = v2->postSeg;
	v1->postSeg->preVtx = v1;
	tail = c2->tail;
	c2->vtxs.Remove(v2);				// v2 is now garbage, ripe for the collector...
	citer<cseg2> is (&c2->segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->m_con = this;
	citer<cvertex2> iv (&c2->vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->m_con = this;
	vtxs.Add(&c2->vtxs);
	segs.Add(&c2->segs);
	c2->m_net->connects.Remove(c2);		// c2 is garbage...
	v1->force_via_flag |= v2->force_via_flag;
	v1->via_w = max(v1->via_w, v2->via_w);
	v1->via_hole_w = max(v1->via_hole_w, v2->via_hole_w);
}

void cconnect2::MergeUnroutedSegments() 
{
	MustRedraw();
	if (segs.GetSize() == 0) 
		{ this->Remove(); return; }
	cvertex2 *v = head->postSeg->postVtx, *next;
	for ( ; v->postSeg; v = next)
	{
		next = v->postSeg->postVtx;
		if (v->preSeg->m_layer == LAY_RAT_LINE && v->postSeg->m_layer == LAY_RAT_LINE
			&& !v->tee && !v->force_via_flag)
		{
			// v is between 2 unrouted segs, so remove v and v->postSeg.
			v->preSeg->postVtx = next;
			next->preSeg = v->preSeg;
			segs.Remove(v->postSeg);
			vtxs.Remove(v);
		}
	}
	// CPT2 experimental: let this routine tend to eliminating end ratline segs that don't connect to a pin/tee/via.
	if (head->postSeg->m_layer == LAY_RAT_LINE && head->IsLooseEnd())
		head->postSeg->RemoveBreak();
	if (!m_net->connects.Contains(this))
		// RemoveBreak() deleted the whole connection!
		return;
	if (tail->preSeg && tail->preSeg->m_layer == LAY_RAT_LINE && tail->IsLooseEnd())
		tail->preSeg->RemoveBreak();
	if (!m_net->connects.Contains(this))
		return;
	if (segs.GetSize()==0)
		this->Remove();
	if (segs.GetSize()==1 && segs.First()->m_layer == LAY_RAT_LINE) 
		// Subtlety:  can't have a single-ratline connect that connects identical pins/tees
		if (head->pin && head->pin==tail->pin)
			this->Remove();
		else if (head->tee && head->tee==tail->tee)
			this->Remove();
}

int cconnect2::Draw()
{
	// Draw individual constituents.  The bDrawn flag for the object itself is probably irrelevant.
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return NO_DLIST;
	if (!m_net->bVisible) return NOERR;

	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->Draw();
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
	{
		v->Draw();												// CPT2. NB does NOT reconcile vias (assumes it's been done)
		if (v->tee) 
			v->tee->Draw();
	}
	bDrawn = true;
	return NOERR;
}

void cconnect2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->Undraw();
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->Undraw();
	bDrawn = false;
}

void cconnect2::Highlight()
{
	// Highlight connection, using thin lines so that segment layer colors can be seen.
	citer<cseg2> is (&segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->Highlight(true);
	// Highlight all vtxs (this will take care of tees and pins too)
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->Highlight();

}



/**********************************************************************************************/
/*  cnet2                                                                                     */
/**********************************************************************************************/

cnet2::cnet2( cnetlist *_nlist, CString _name, int _def_w, int _def_via_w, int _def_via_hole_w )
	: cpcb_item (_nlist->m_doc)
{
	m_nlist = _nlist;
	m_nlist->nets.Add(this);
	name = _name;
	def_w = _def_w;
	def_via_w = _def_via_w; def_via_hole_w = _def_via_hole_w;
	bVisible = true;	
}

cnet2::cnet2(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	m_nlist = NULL;
}

bool cnet2::IsOnPcb()
	// Note that items on the clipboard will return false
	{ return doc->m_nlist->nets.Contains(this); }

void cnet2::GetStatusStr( CString * str )
{
	CString s ((LPCSTR) IDS_Net2);
	str->Format( s, name ); 
}

void cnet2::SaveUndoInfo(int mode)
{
	// "mode", which is SAVE_ALL by default, can also have the value SAVE_CONNECTS, SAVE_AREAS, SAVE_NET_ONLY, 
	doc->m_undo_items.Add( new cunet(this) );
	if (mode == SAVE_ALL || mode == SAVE_CONNECTS)
	{
		citer<cconnect2> ic (&connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
			c->SaveUndoInfo();
		citer<ctee> it (&tees);
		for (ctee *t = it.First(); t; t = it.Next())
			t->SaveUndoInfo();
	}
	if (mode == SAVE_ALL || mode == SAVE_AREAS)
	{
		citer<carea2> ia (&areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			a->SaveUndoInfo();
	}
	if (mode == SAVE_ALL)
	{
		// CPT2 now we can save undo info for pins too.
		citer<cpin2> ipin (&pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			pin->SaveUndoInfo();
	}
}

void cnet2::SetVisible( bool _bVisible )
{
	if( bVisible == _bVisible )	return;
	bVisible = _bVisible;
	if( bVisible )
	{
		// make segments visible
		citer<cconnect2> ic (&connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next()) 
		{
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				s->dl_el->visible = s->dl_sel->visible = true;
		}
		// CPT2 TODO what about vias (+thermals)?
	}
	else
	{
		// make ratlines invisible and disable selection items
		citer<cconnect2> ic (&connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next()) 
		{
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				if (s->m_layer == LAY_RAT_LINE)
					s->dl_el->visible = s->dl_sel->visible = true;
		}
		// CPT2 TODO what about vias (+thermals)?
	}
}

void cnet2::Highlight(cpcb_item *exclude)
{
	// CPT2 Derived from old CNetList::HighlightConnection(), plus areas get highlighted too.  (Didn't see a reason to separate out the connection highlighting
	// routine at this point...)
	// AMW r272: thin segment highlights and vertex highlights
	// AMW r274: exclude item(s) indicated by exclude_id
	// if except is a segment, just exclude it
	// if exclude is a vertex, exclude it and adjacent segments
	cseg2 *x_seg1 = NULL, *x_seg2 = NULL;
	if (!exclude) 
		;
	else if (exclude->IsSeg())
		x_seg1 = exclude->ToSeg();
	else if (cvertex2 *v = exclude->ToVertex())
		x_seg1 = v->preSeg,
		x_seg2 = v->postSeg;

	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		// Highlight all segs in net, minus exclusions, with _thin_ lines
		citer<cseg2> is (&c->segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
			if (s != x_seg1 && s != x_seg2)
				s->Highlight(true);
		// Highlight all vtxs, minus exclusions (this will take care of tees and pins too)
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			if (v != exclude)
				v->Highlight();
	}

	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->Highlight();
}

void cnet2::MarkConstituents(int util)
{
	// CPT2 new. Set utility on all constituents (connects, vtxs, segs, tees, areas).
	utility = util;
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next()) {
		c->utility = util;
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next()) {
			v->utility = util;
			if (v->tee) v->tee->utility = 1;
			}
		citer<cseg2> is (&c->segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
			s->utility = util;
		}
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->MarkConstituents(util);
	tees.SetUtility(util);
}

void cnet2::Remove()
{
	Undraw();
	m_nlist->nets.Remove(this);
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		pin->net = NULL;
	// Member connects, areas, and tees are no longer valid and will be garbage-collected...
}

void cnet2::MergeWith(cnet2 *n2)
{
	// CPT2 new.  Merge this net into n2, moving over all connects, areas, pins, and tees.
	m_nlist->nets.Remove(this);
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		c->m_net = n2;
		citer<cseg2> is (&c->segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
			s->m_net = n2;
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			v->m_net = n2;
	}
	n2->connects.Add(&connects);
	connects.RemoveAll();
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->m_net = n2;
	n2->areas.Add(&areas);
	areas.RemoveAll();
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		pin->net = n2;
	n2->pins.Add(&pins);
	pins.RemoveAll();
	n2->tees.Add(&tees);
	tees.RemoveAll();
}

cpin2 *cnet2::AddPin( CString * ref_des, CString * pin_name )
{
	cpin2 *pin = doc->m_plist->GetPinByNames(ref_des, pin_name);
	if (!pin) return NULL;
	AddPin(pin);
	return pin;
}

void cnet2::AddPin( cpin2 *pin )
{
	if (pin->net == this)
		return;
	if (pin->net)
	{
		// Must rip out connects attached to this pin, since they're within the wrong net
		citer<cconnect2> ic (&pin->net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
			if (c->head->pin == pin || c->tail->pin == pin)
				c->Remove();
		pin->net->pins.Remove(pin);
	}
	pins.Add(pin);
	pin->net = this;
}

void cnet2::RemovePin( cpin2 *pin )
{
	// CPT2.  Descended from old cnet::RemovePin.  Detach pin from net, and delete any connections that lead into the pin.
	carray<cvertex2> vtxs;
	pin->GetVtxs(&vtxs);
	citer<cvertex2> iv (&vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		if (v->m_con->IsOnPcb())
			v->m_con->Remove();
	pins.Remove(pin);
	pin->net = NULL;
}

void cnet2::SetWidth( int w, int via_w, int via_hole_w )
{
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		c->SetWidth(w, via_w, via_hole_w);
}

void cnet2::GetWidth( int * w, int * via_w, int * via_hole_w )
{
	// Get net's trace and via widths.  If no net-specific default value set, use the board defaults.
	// CPT2.  Derived from old CFreePcbView::GetWidthsForSegment().  Args may now be null (via_w and via_hole_w are by default)
	if (w)
	{
		*w = doc->m_trace_w;
		if( def_w )
			*w = def_w;
	}
	if (via_w)
	{
		*via_w = doc->m_via_w;
		if( def_via_w )
			*via_w = def_via_w;
	}
	if (via_hole_w)
	{
		*via_hole_w = doc->m_via_hole_w;
		if( def_via_hole_w )
			*via_hole_w = def_via_hole_w;
	}
}

void cnet2::CalcViaWidths(int w, int *via_w, int *via_hole_w) 
{
	// CPT r295.  Helper for cvertex2::ReconcileVia().  
	// Given a segment width value "w", determine a matching via and via hole width.  Do this by first checking if w==net->def_w,
	//  and return the net's default via sizes if so;
	//  otherwise, scan thru m_doc->m_w and find the last entry in there <=w.  The corresponding members of m_doc->m_v_w
	// and m_doc->m_v_h_w are then the return values.
	CFreePcbDoc *doc = m_nlist->m_doc;
	int cWidths = doc->m_w.GetSize(), i;
	if (w == def_w || cWidths == 0)
		{ *via_w = def_via_w; *via_hole_w = def_via_hole_w; return; }
	for (i=1; i<cWidths; i++)
		if (doc->m_w.GetAt(i) > w) break;
	// i-1 = last entry in width table that's <= w:
	*via_w = doc->m_v_w.GetAt(i-1);
	*via_hole_w = doc->m_v_h_w.GetAt(i-1);
}

void cnet2::SetThermals() 
{
	// CPT2.  New system for thermals.  Run SetNeedsThermal() for all vertices and pins in the net.
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			v->SetNeedsThermal();
	}
	citer<cpin2> ip (&pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		p->SetNeedsThermal();
}


cvertex2 *cnet2::TestHitOnVertex(int layer, int x, int y) 
{
	// CPT2 TODO maybe obsolete.  Frequently cvertex2::TestForHit() does the trick.
	// Test for a hit on a vertex in a trace within this net/layer.
	// If layer == 0, ignore layer.  New return value: returns the hit vertex, or 0 if no vtx is near (x,y) on layer
	citer<cconnect2> ic (&connects);
	for( cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		citer<cvertex2> iv (&c->vtxs);
		for( cvertex2 *v = iv.First(); v; v = iv.Next())
		{
			cseg2 *pre = v->preSeg? v->preSeg: v->postSeg;
			cseg2 *post = v->postSeg? v->postSeg: v->preSeg;
			if( v->via_w > 0 || layer == 0 || layer == pre->m_layer || layer == post->m_layer
				|| (pre->m_layer == LAY_RAT_LINE && post->m_layer == LAY_RAT_LINE) )
			{
				int test_w = max( v->via_w, pre->m_width );
				test_w = max( test_w, post->m_width );
				test_w = max( test_w, 10*NM_PER_MIL );		// in case widths are zero
				double dx = x - v->x, dy = y - v->y;
				double d = sqrt( dx*dx + dy*dy );
				if( d < test_w/2 )
					return v;
			}
		}
	}
	return NULL;
}

carea2 *cnet2::NetAreaFromPoint( int x, int y, int layer )
{
	// CPT2 replaces CNetList::TestPointInArea.  CPT2 TODO.  This routine is kind of a dud, inasmuch as if layer==LAY_PAD_THRU there may
	// be more than one appropriate area to return.  Probably I can phase this turkey out...
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		if ((a->GetLayer()==layer || layer==LAY_PAD_THRU) && a->TestPointInside(x, y))
			return a;
	return NULL;
}

inline void ReassignComponents(cpcb_item *i1, cpcb_item *i2, int *reassign)
{
	// CPT2 helper for OptimizeConnections().  See the comments there for the details of my new algorithm.  Basically i1->utility
	//   and i2->utility represent numbers for connected components within their net.  When we determine that i1 and i2 touch, we call this
	//   routine to ensure that both get assigned the same component number.  Use the smallest number available, and use table "reassign" to 
	//   make a note of how these component numbers are getting merged together.
	// First: ensure, by consulting reassign[], that the component numbers in pin->utility 
	//   and a->utility are as small as they can be, given our present state of knowledge
	while (reassign[i1->utility])
		i1->utility = reassign[i1->utility];
	while (reassign[i2->utility])
		i2->utility = reassign[i2->utility];
	if (i1->utility < i2->utility)
		// Note that we not only set reassign[i2->utility], but we also change i2->utility to the lesser value:
		reassign[i2->utility] = i1->utility,
		i2->utility = i1->utility;
	else if (i2->utility < i1->utility)
		reassign[i1->utility] = i2->utility,
		i1->utility = i2->utility;
}


class TwoPinsAndDistance 
{
	// CPT2 helper class used in OptimizeConnections().  In truth "d2" is the distance squared, just to save a little time on a sqrt
	// op that's not necessary
public:
	cpin2 *pin1, *pin2;
	double d2;
	TwoPinsAndDistance()
		{ pin1 = pin2 = NULL; }
	TwoPinsAndDistance( cpin2 *_pin1, cpin2 *_pin2 )
	{ 
		pin1 = _pin1; pin2 = _pin2; 
		double dx = pin1->x - pin2->x, dy = pin1->y - pin2->y;
		d2 = dx*dx + dy*dy;
	}
};

int ComparePinDistances(const TwoPinsAndDistance *pd1, const TwoPinsAndDistance *pd2)
	// Used by qsort.
	{ return pd1->d2 < pd2->d2? -1: 1; }


void cnet2::OptimizeConnections( BOOL bLimitPinCount, BOOL bVisibleNetsOnly )
{
	// CPT2. Derived from CNetList::OptimizeConnections.  I think we can ditch the ic_track argument of the old routine. 
	if( bVisibleNetsOnly && !bVisible )
		return;
	if( bLimitPinCount && doc->m_auto_ratline_disable && NumPins() >= doc->m_auto_ratline_min_pins )
		return;

	// go through net, deleting unlocked connections that consist of a single ratline linking 2 pins.
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		if( c->NumSegs() == 0 )
			// CPT2:  guess I'll tolerate it, and will comment out the old code's ASSERT(0);
			c->SaveUndoInfo(),
			c->Remove();
		else if (!c->locked && c->NumSegs() == 1 && c->head->postSeg->m_layer == LAY_RAT_LINE && c->head->pin && c->tail->pin)
			c->SaveUndoInfo(),
			c->Remove();

	// CPT2 trying a slightly different algorithm.  The goal is to divide up the pins, connects, and areas into connected components.  
	// First assign distinct component numbers 1, 2, 3, ..., to each of these items, storing the numbers in the utility fields;  also 
	// set up an array reassign[] of size equal to the number of items, initially zeroed out.  Then examine all the possible ways pairs
	// of these items can touch:  pins & areas;  connects & pins; connects & other connects; connects & areas.  If x touches y, where 
	// x has component number cx and y has component number cy, then we'll want to ensure that components cx and cy are united under a single
	// number that is as small as possible.  Roughly speaking, we'll set reassign[cx] = cy if cy<cx, or reassign[cy] = cx otherwise.  Once 
	// we're done we loop through items again and update component numbers by looking at reassign[] and tracing a chain downwards.  The end result 
	// will be that all items that are connected, directly or indirectly, will have a common minimal component number.
	int comp = 1;
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		pin->utility = comp++;
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		c->utility = comp++;
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->utility = comp++;
	
	int *reassign = (int*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, comp*sizeof(int));
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		for (carea2 *a = ia.First(); a; a = ia.Next())
			if (pin->pad_layer==LAY_PAD_THRU || pin->pad_layer==a->m_layer)
				if (a->TestPointInside(pin->x, pin->y))
					// "pin" and "a" touch!  Reassign component numbers so that both get a shared (minimal) value
					ReassignComponents(a, pin, reassign);
	
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		for (int end=0; end<2; end++) 
		{
			cvertex2 *v = end==0? c->head: c->tail;
			if (cpin2 *pin = v->pin)
				// c and pin touch (obviously):
				ReassignComponents(c, pin, reassign);
			else if (v->tee)
			{
				citer<cvertex2> iv2 (&v->tee->vtxs);
				for (cvertex2 *v2 = iv2.First(); v2; v2 = iv2.Next())
					if (v2->m_con != c)
						// c and v2->m_con touch!
						ReassignComponents(c, v2->m_con, reassign);
			}
		}
		// NB to see that an area and a connect touch, we only look at the connect's vertices.  Now it's possible to have a 
		// segment slice through near a corner of an area, with no vertices lying in the area, but I'm going to ignore
		// that possibility ;)
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
			for (carea2 *a = ia.First(); a; a = ia.Next())
				if (a->utility == c->utility)
					// Already determined that area and connect are in the same component
					continue;
				else if (v->via_w || v->preSeg && v->preSeg->m_layer == a->m_layer
						          || v->postSeg && v->postSeg->m_layer == a->m_layer)
					if (a->TestPointInside(v->x, v->y))
						// c and a touch!
						ReassignComponents(c, a, reassign);
	}

	// Now get the correct, minimal component numbers for every pin (we don't actually care about the other items any more)
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		while (reassign[pin->utility])
			pin->utility = reassign[pin->utility];
	// Count the distinct components (note that this messes up reassign[], but we're basically done with that anyway)
	int numComponents = 0;
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		if (!reassign[pin->utility])
			numComponents++,
			reassign[pin->utility] = 1;
	HeapFree(GetProcessHeap(), 0, reassign);
	if (numComponents<2)
		return;

	// Generate a table of distances between pins and sort it.  Don't bother to compute in the case where pins belong to the same component.
	CArray<TwoPinsAndDistance> ptbl;
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		citer<cpin2> ipin2 (&pins, pin);
		ipin2.Next();															// Advance ipin2 to the pin AFTER "pin"
		for (cpin2 *pin2 = ipin2.Next(); pin2; pin2 = ipin2.Next())
			if (pin->utility != pin2->utility)
				ptbl.Add( TwoPinsAndDistance(pin, pin2) );
	}
	qsort(ptbl.GetData(), ptbl.GetSize(), sizeof(TwoPinsAndDistance), (int (*)(const void*,const void*)) ComparePinDistances);

	// Time to create the new ratlines!  Scan thru ptbl in distance order, and make a connection each time 
	// we get a pair of pins in different components.
	int pos = 0;
	while (numComponents>1)
	{
		TwoPinsAndDistance *pd;
		for (pd = &ptbl[pos++]; pd->pin1->utility==pd->pin2->utility; pd = &ptbl[pos++])
			;
		pd->pin1->AddRatlineToPin( pd->pin2 );
		// Merge component of pd->pin2 into the component of pd->pin1
		int pin1component = pd->pin1->utility, pin2component = pd->pin2->utility;
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			if (pin->utility == pin2component)
				pin->utility = pin1component;
		numComponents--;
	}
}


int cnet2::Draw() 
{
	// CPT2 TODO decide what to do if anything about bDrawn flag.
	if (!bVisible)
		return NOERR;
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		c->Draw();
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->Draw();
	// Also deal with tees that are in the net.  (The subsidiary vtxs are never drawn.)
	citer<ctee> it (&tees);
	for (ctee *t = it.First(); t; t = it.Next())
		t->Draw();
	
	return NOERR;
}

void cnet2::Undraw() 
{
	// CPT2 Undraw constituents one by one.  The bDrawn flag for the net itself is probably irrelevant.
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		c->Undraw();
	citer<carea2> ia (&areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		a->Undraw();
	// Also deal with tees that are in the net.  (The subsidiary vtxs are never drawn.)
	citer<ctee> it (&tees);
	for (ctee *t = it.First(); t; t = it.Next())
		t->Undraw();
}


void cnet2::AddConnect( cconnect2 *c )
{
	// CPT2 new.  Used when importing from netlist files, when connects may get transferred from one net to another.
	// It's caller's responsibility to make sure that tees and pins associated with c are properly attached to this net.
	if (c->m_net == this && connects.Contains(c))
		return;											// Nothing to do!
	if (c->m_net && c->m_net->connects.Contains(c))
		c->m_net->connects.Remove(c);
	connects.Add(c);
	c->m_net = this;
	citer<cvertex2> iv (&c->vtxs);
	for (cvertex2 *v = iv.First(); v; v = iv.Next())
		v->m_net = this;
	citer<cseg2> is (&c->segs);
	for (cseg2 *s = is.First(); s; s = is.Next())
		s->m_net = this;
}

void cnet2::CleanUpConnections( CString * logstr )
{
	citer<cconnect2> ic (&connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		BOOL bConnectionRemoved = FALSE;
		c->MustRedraw();
		citer<cseg2> is (&c->segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
		{
			// check for zero-length segment
			if (s->preVtx->x != s->postVtx->x || s->preVtx->y != s->postVtx->y)
				continue;
			// yes, analyze segment
			enum { UNDEF=0, THRU_PIN, SMT_PIN, VIA, TEE, SEGMENT, END_STUB };
			int pre_type = UNDEF;	// type of preceding item
			int pre_layer = UNDEF;	// layer if SEGMENT or SMT_PIN
			int post_type = UNDEF;	// type of following item
			int post_layer = UNDEF;	// layer if SEGMENT or SMT_PIN
			int layer = s->m_layer;
			// analyze start of segment
			if( !s->preVtx->preSeg )
			{
				// first segment
				if (s->preVtx->pin)
					pre_layer = s->preVtx->pin->pad_layer,
					pre_type = pre_layer==LAY_PAD_THRU? THRU_PIN: SMT_PIN;
				else if (s->preVtx->tee)
					pre_type = TEE;
				else if (s->preVtx->via_w)
					pre_type = VIA;
				else
					pre_type = END_STUB;
			}
			else
			{
				// not first segment
				pre_layer = s->preVtx->preSeg->m_layer;	// preceding layer
				if( s->preVtx->via_w )
					pre_type = VIA;				// starts on a via
				else
					pre_type = SEGMENT;			// starts on a segment
			}
			// analyze end of segment
			if( !s->postVtx->postSeg )
			{
				// last segment
				if (s->postVtx->pin)
					post_layer = s->postVtx->pin->pad_layer,
					post_type = post_layer==LAY_PAD_THRU? THRU_PIN: SMT_PIN;
				else if (s->postVtx->tee)
					post_type = TEE;
				else if (s->postVtx->via_w)
					post_type = VIA;
			}
			else
			{
				// not last segment
				post_layer = s->postVtx->postSeg->m_layer;
				if( s->postVtx->via_w )
					post_type = VIA;
				else
					post_type = SEGMENT;
			}
			// Obtain descriptor strings for pre- and post-vertices
			CString strPre, strPost;
			s->preVtx->GetStatusStr( &strPre );
			s->postVtx->GetStatusStr( &strPost );

			// OK, now see if we can remove the zero-length segment by removing
			// the starting vertex
			BOOL bRemove = FALSE;
			if( pre_type == SEGMENT && pre_layer == layer
				|| pre_type == SEGMENT && layer == LAY_RAT_LINE
				|| pre_type == VIA && post_type == VIA 
				|| pre_type == VIA && post_type == THRU_PIN
				|| post_type == END_STUB ) 
				s->preVtx->Remove(),
				bRemove = true;
			else if( post_type == SEGMENT && post_layer == layer
				|| post_type == SEGMENT && layer == LAY_RAT_LINE
				|| post_type == VIA && pre_type == THRU_PIN )
				s->postVtx->Remove(),
				bRemove = true;
			if( bRemove  && logstr )
			{
				CString str, s ((LPCSTR) IDS_NetTraceRemovingZeroLength);
				str.Format( s, this->name, strPre, strPost ); 
				*logstr += str;
			}
		}

		// look for segments on same layer, with same width, not separated by a via.  (Actually loop thru verts to find these situations.)
		citer<cvertex2> iv (&c->vtxs);
		for (cvertex2 *v = iv.First(); v; v = iv.Next())
		{
			if (!v->preSeg || !v->postSeg)
				continue;
			if (v->preSeg->m_layer != v->postSeg->m_layer)
				continue;
			if (v->preSeg->m_width != v->postSeg->m_width)
				continue;
			if (v->via_w)
				continue;
			// see if colinear
			double dx1 = v->x - v->preSeg->preVtx->x;
			double dy1 = v->y - v->preSeg->preVtx->y;
			double dx2 = v->postSeg->postVtx->x - v->x;
			double dy2 = v->postSeg->postVtx->y - v->y;
			if( dy1*dx2 == dy2*dx1 && (dx1*dx2>0.0 || dy1*dy2>0.0) )
			{
				// yes, combine these segments
				if( logstr )
				{
					CString str, s ((LPCSTR) IDS_NetTraceCombiningColinear);
					CString strHead, strTail;
					c->head->GetStatusStr( &strHead );
					c->tail->GetStatusStr( &strTail );
					str.Format( s, this->name, strHead, strTail ); 
					*logstr += str;
				}
				v->Remove();
			}
			// CPT2 old code checked for non-branch stubs with a single unrouted segment and no end-via.  I'm hoping to let
			// MergeUnroutedSegments to take care of these situations instead.
		}
	}
}



void cnet2::ImportRouting( CArray<cnode> * nodes, CArray<cpath> * paths,
							int tolerance, CDlgLog * log, bool bVerbose )
{
	// CPT2 TODO.  TEST ME!
	// unroute all traces
	MustRedraw();
	connects.RemoveAll();

	// add all pins in net to list of nodes
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		int inode = nodes->GetSize();
		nodes->SetSize( inode+1 );
		cnode * node = &(*nodes)[inode];
		node->layer = pin->pad_layer;
		node->type = NPIN;
		node->x = pin->x;
		node->y = pin->y;
		node->pin = pin;
	}
	// now hook up paths and nodes
	for( int ipath=0; ipath<paths->GetSize(); ipath++ )
	{
		cpath * path = &(*paths)[ipath];
		for( int iend=0; iend<2; iend++ )
		{
			cpath_pt * pt = &path->pt[0];	// first point in path
			if( iend == 1 )
				pt = &path->pt[path->pt.GetSize()-1];	// last point
			// search all nodes for match
			BOOL bMatch = FALSE;
			for( int inode=0; inode<nodes->GetSize(); inode++ )
			{
				cnode * node = &(*nodes)[inode];
				if( abs(pt->x-node->x)<tolerance && abs(pt->y-node->y)<tolerance 
					&& ( path->layer == node->layer || node->layer == LAY_PAD_THRU ) )
				{
					// match, hook it up
					int ip = node->path_index.GetSize();
					node->path_index.SetSize(ip+1);
					node->path_end.SetSize(ip+1);
					node->path_index[ip] = ipath;
					node->path_end[ip] = iend;
					pt->inode = inode;
					pt->x = node->x;
					pt->y = node->y;
					bMatch = TRUE;
					break;
				}
			}
			if( !bMatch )
			{
				// node not found, make new junction node
				int inode = nodes->GetSize();
				nodes->SetSize(inode+1);
				cnode * node = &(*nodes)[inode];
				node->layer = path->layer;
				node->type = NJUNCTION;
				node->x = pt->x;
				node->y = pt->y;
				int ip = node->path_index.GetSize();
				node->path_index.SetSize(ip+1);
				node->path_end.SetSize(ip+1);
				node->path_index[ip] = ipath;
				node->path_end[ip] = iend;
				pt->inode = inode;
			}
		}
	}

	// CPT2.  Go through the nodes again.  For those that have >2 emerging paths and that are not pins, create a tee structure.
	for( int inode=0; inode<nodes->GetSize(); inode++ )
	{
		cnode * node = &(*nodes)[inode];
		if (!node->pin && node->path_index.GetSize() > 2)
			node->tee = new ctee(this);
	}

	// dump data
	CString mess, str;
	if( log )
	{
		CString s ((LPCSTR) IDS_RoutingNet);
		mess.Format( s, *name );
		log->AddLine( mess );
		if( bVerbose )
		{
			s.LoadStringA(IDS_NumNodes);
			mess.Format( s, nodes->GetSize() );
			log->AddLine( mess );
			for( int inode=0; inode<nodes->GetSize(); inode++ )
			{
				cnode * node = &(*nodes)[inode];
				CString type_str ((LPCSTR) IDS_None);
				if( node->type == NPIN )
					type_str.LoadStringA(IDS_Pin2);
				else if( node->type == NVIA )
					type_str.LoadStringA(IDS_Via);
				else if( node->type == NJUNCTION )
					type_str.LoadStringA(IDS_Junction);
				s.LoadStringA(IDS_NodeXYLayerNpaths);
				mess.Format( s, inode, type_str, node->x/NM_PER_MIL, node->y/NM_PER_MIL, node->layer, node->path_index.GetSize() );
				log->AddLine( mess );
			}
		}
	}

	// start routing.  CPT2 new system.  Loop thru all paths, looking for ones that haven't been incorporated into the net (bUsed==false), and
	// which have on at least one end a pin or tee.  Starting from the pin/tee, trace forward thru vias until one hits another pin or tee, or a loose end.  
	// That's the new connect.  Along the way, mark the paths as bUsed.
	for (int ip = 0; ip < paths->GetSize(); ip++)
	{
		cpath *path = &(*paths)[ip];
		if (path->bUsed) continue;
		int inode0 = path->GetInode(0), inode1 = path->GetInode(1);
		cnode *node0 = &(*nodes)[inode0], *node1 = &(*nodes)[inode1];
		if (!node0->pin && !node0->tee && !node1->pin && !node1->tee)
			continue;
		cpath *p = path;
		int end = node0->pin || node0->tee? 0: 1;
		cnode *node = end==0? node0: node1;
		cconnect2 *c = new cconnect2(this);
		cvertex2 *v = new cvertex2(c, node->x, node->y);
		v->pin = node->pin;
		v->tee = node->tee;
		c->Start(v);
		while (1)
		{
			p->bUsed = true;
			int n_pts = path->pt.GetSize();
			for( int j = 1; j < n_pts-1; j++ )
			{
				int next_pt = end==0? j: n_pts-1-j;
				int x = p->pt[next_pt].x;
				int y = p->pt[next_pt].y;
				c->AppendSegment(x, y, p->layer, p->width);
			}
			int inode = p->GetInode(1-end);
			node = &(*nodes)[inode];
			c->AppendSegment(node->x, node->y, p->layer, p->width);
			c->tail->pin = node->pin;
			c->tail->tee = node->tee;
			if (node->via_w)
				c->tail->via_w = node->via_w,
				c->tail->via_hole_w = def_via_hole_w,
				c->tail->force_via_flag = true;				// Force all vias
			if (node->pin || node->tee) 
				break;
			if (node->path_index.GetSize() == 1)
				break;										// Stub trace...
			ASSERT(node->path_index.GetSize() == 2);
			// Determine the path to go to next, and which end coincides with "node"
			cpath *p0 = &(*paths)[node->path_index[0]];
			cpath *p1 = &(*paths)[node->path_index[1]];
			p = p0==p? p1: p0;
			end = p->GetInode(0)==inode? 0: 1;
		}
	}

	if( log ) 
	{
		CString s ((LPCSTR) IDS_SuccessAllPathsRouted);
		log->AddLine( s );
	}
}



/**********************************************************************************************/
/*  cnetlist                                                                                  */
/**********************************************************************************************/

cnetlist::cnetlist( CFreePcbDoc * _doc )
	{ 
		m_doc = _doc; 
		m_dlist = m_doc->m_dlist;
		m_plist = m_doc->m_plist;
	}													// CPT2 TODO: finish


void cnetlist::SetThermals()
{
	// CPT2 new.  Call SetThermals() for all nets in the list.
	citer<cnet2> in (&nets);
	for (cnet2 *n = in.First(); n; n = in.Next())
		n->SetThermals();
}

BOOL cnetlist::GetNetBoundaries( CRect * r )
{
	// get bounding rectangle for all net elements
	BOOL bValid = FALSE;
	CRect br (INT_MAX, INT_MIN, INT_MIN, INT_MAX);
	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
			{
				br.bottom = min( br.bottom, v->y - v->via_w );
				br.top = max( br.top, v->y + v->via_w );
				br.left = min( br.left, v->x - v->via_w );
				br.right = max( br.right, v->x + v->via_w );
				bValid = TRUE;
			}
		}
		citer<carea2> ia (&net->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
		{
			CRect r = a->GetBounds();
			br.bottom = min( br.bottom, r.bottom );
			br.top = max( br.top, r.top );
			br.left = min( br.left, r.left );
			br.right = max( br.right, r.right );
			bValid = TRUE;
		}
	}

	*r = br;
	return bValid;
}

void cnetlist::OptimizeConnections( BOOL bLimitPinCount, BOOL bVisibleNetsOnly )
{
	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
		net->OptimizeConnections(bLimitPinCount, bVisibleNetsOnly);
}

void cnetlist::ReadNets( CStdioFile * pcb_file, double read_version, int * layer )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;
	int * in_layer = layer;
	if( in_layer == NULL )
		in_layer = m_layer_by_file_layer;	// this is a translation table

	// find beginning of [nets] section
	do
	{
		if( !pcb_file->ReadString( in_str ))
		{
			// error reading pcb file
			CString mess ((LPCSTR) IDS_UnableToFindNetsSectionInFile);
			AfxMessageBox( mess );
			return;
		}
		in_str.Trim();
	}
	while( in_str != "[nets]" );

	// get each net in [nets] section
	while( 1 )
	{
		pos = (long)pcb_file->GetPosition();
		if (!pcb_file->ReadString( in_str ))
		{
			CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
			throw err_str;
		}
		in_str.Trim();
		if( in_str[0] == '[' && in_str != "[nets]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			return;		// start of next section, reset position and exit
		}
		else if( in_str.Left(4) == "net:" )
		{
			np = ParseKeyString( &in_str, &key_str, &p );
			CString net_name = p[0].Left(MAX_NET_NAME_SIZE);
			net_name.Trim();
			int npins = my_atoi( &p[1] ); 
			int nconnects = my_atoi( &p[2] );
			int nareas = my_atoi( &p[3] );
			int def_width = my_atoi( &p[4] );
			int def_via_w = my_atoi( &p[5] );
			int def_via_hole_w = my_atoi( &p[6] );
			int visible = 1;
			if( np > 8 )
				visible = my_atoi( &p[7] );
			cnet2 *net = new cnet2( this, net_name, def_width, def_via_w, def_via_hole_w );
			net->bVisible = visible;
			for( int ip=0; ip<npins; ip++ )
			{
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "pin" || np < 3 )
					throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
				CString pin_str = p[1].Left(cshape::MAX_PIN_NAME_SIZE);
				int dot_pos = pin_str.FindOneOf( "." );
				CString ref_str = pin_str.Left( dot_pos );
				CString pin_num_str = pin_str.Right( pin_str.GetLength()-dot_pos-1 );
				cpin2 * pin = net->AddPin( &ref_str, &pin_num_str );
				// CPT2.  If pin isn't found, just silently ignore it.  I used to error out when this happened, but old files can have nets
				//  that contain "phantom pins,"  and we'd better tolerate them for backwards compatibility.
				if (!pin || !pin->part)
					;
				else if( !pin->part->shape )
				{
					// Mighty mighty unlikely, but I guess I'll keep this clause...
					CString s ((LPCSTR) IDS_FatalErrorInNetPartDoesntHaveAFootprint), *err_str = new CString();
					err_str->Format(s, net_name, ref_str);
					throw err_str;
				}
				else
					pin->utility = ip;												// CPT2: Index value temporarily stored in pin, so that we can use it
																					// during the reading of connects
			}

			for( int ic=0; ic<nconnects; ic++ )
			{
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "connect" || np < 6 )
					throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);

				// Create the connect with one vertex in it.  More precise data gets loaded later.
				cconnect2 *c = new cconnect2(net);
				cvertex2 *v0 = new cvertex2(c);
				c->Start(v0);
				int start_pin = my_atoi( &p[1] );
				int end_pin = my_atoi( &p[2] );
				cpin2 *pin0 = NULL, *pin1 = NULL;
				if (start_pin != cconnect2::NO_END)
					pin0 = net->pins.FindByUtility(start_pin);
					// CPT2 We tolerate it if FindByUtility() fails and pin0 is NULL...
				v0->pin = pin0;
				if (end_pin != cconnect2::NO_END)
					pin1 = net->pins.FindByUtility(end_pin);
				int nsegs = my_atoi( &p[3] );
				c->locked = my_atoi( &p[4] );
				
				// Get more info on first vertex
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "vtx" || np < 8 )
				{
					CString * err_str = new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
					throw err_str;
				}
				v0->x = my_atoi( &p[1] );
				v0->y = my_atoi( &p[2] );
				// int file_layer = my_atoi( &p[3] ); 
				// first_vtx->pad_layer = in_layer[file_layer];				// CPT2 TODO figure this out... looks bogus
				v0->force_via_flag = my_atoi( &p[4] ); 
				v0->via_w = my_atoi( &p[5] ); 
				v0->via_hole_w = my_atoi( &p[6] );
				v0->tee = NULL;
				int tee_ID = np==9? abs(my_atoi(&p[7])): 0;
				if( tee_ID )
				{
					ctee *tee = net->tees.FindByUtility(tee_ID);
					if (!tee) 
						tee = new ctee(net),				// Constructor adds tee to net->tees
						tee->utility = tee_ID;
					tee->vtxs.Add(v0);
					v0->tee = tee;
				}

				// now add all segments
				for( int is=0; is<nsegs; is++ )
				{
					// read segment data
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "seg" || np < 6 )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
						throw err_str;
					}
					int file_layer = my_atoi( &p[1] ); 
					int layer = in_layer[file_layer]; 
					int seg_width = my_atoi( &p[2] );
					cseg2 *seg = new cseg2(c, layer, seg_width);

					// read following vertex data
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "vtx" || np < 8 )
					{
						CString * err_str = new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
						throw err_str;
					}

					cvertex2 *v = new cvertex2(c);
					v->x = my_atoi( &p[1] ); 
					v->y = my_atoi( &p[2] ); 
					// int file_layer = my_atoi( &p[3] ); 
					// int pad_layer = in_layer[file_layer];						// TODO: Figure this out (bogus?)
					v->force_via_flag = my_atoi( &p[4] ); 
					v->via_w = my_atoi( &p[5] ); 
					v->via_hole_w = my_atoi( &p[6] );
					int tee_ID = np==9? abs(my_atoi(&p[7])): 0;
					if( tee_ID )
					{
						ctee *tee = net->tees.FindByUtility(tee_ID);
						if (!tee) 
							tee = new ctee(net),									// Constructor adds tee to net->tees
							tee->utility = tee_ID;
						tee->vtxs.Add(v);
						v->tee = tee;
					}
					if (is==nsegs-1)
						v->pin = pin1;												// Assign end-pin (possibly NULL) to v if we're at the last vertex.

					c->AppendSegAndVertex(seg, v, c->tail);

					/** this code is for bug in versions before 1.313. CPT2 TODO:  Figure it out
					if( force_via_flag )
					{
						if( end_pin == cconnect::NO_END && is == nsegs-1 )
							ForceVia( net, ic, is+1 );
						else if( read_version > 1.312001 )	// i.e. 1.313 or greater
							ForceVia( net, ic, is+1 );
					}
					*/

					/* CPT2.  Don't get.  I'm just assigning the width values for v during this very iteration (above)
					if( is != 0 )
					{
						// set widths of preceding vertex
						net->connect[ic]->vtx[is].via_w = pre_via_w;
						net->connect[ic]->vtx[is].via_hole_w = pre_via_hole_w;
					}
					pre_via_w = via_w;
					pre_via_hole_w = via_hole_w;
					*/
				}

				/* CPT2 TODO
				// connection created
				// if older version of fpc file, split at tees if needed
				if( read_version < 1.360 )
				{
					cconnect * c = net->ConByIndex( ic );
					// iterate through vertices in reverse
					for( int iv=c->NumVtxs()-2; iv>0; iv-- )
					{
						cvertex * v = &c->VtxByIndex( iv );
						if( v->tee_ID )
						{
							// split into 2 connections
							net->SplitConnectAtVtx( v->Id() );
							nconnects++;
							ic++;
						}
					}
				}
				*/
			} // end for(ic)

			for( int ia=0; ia<nareas; ia++ )
			{
				if (!pcb_file->ReadString( in_str ))
					throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "area" || np < 4 )
					throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
				int na = my_atoi( &p[0] );
				if( (na-1) != ia )
					throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
				int ncorners = my_atoi( &p[1] );
				int file_layer = my_atoi( &p[2] );
				int layer = in_layer[file_layer]; 
				int hatch = 1;
				if( np == 5 )
					hatch = my_atoi( &p[3] );
				int last_side_style = cpolyline::STRAIGHT;
				carea2 *a = new carea2(net, layer, hatch, 2*NM_PER_MIL, 10*NM_PER_MIL);
				ccontour *ctr = new ccontour(a, true);				// Adds ctr as a's main contour

				for( int icor=0; icor<ncorners; icor++ )
				{
					if (!pcb_file->ReadString( in_str ))
						throw new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || np < 4 )
						throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
						throw new CString((LPCSTR) IDS_ErrorParsingNetsSectionOfProjectFile);

					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					last_side_style = np >= 5? my_atoi( &p[3] ): cpolyline::STRAIGHT;
					int end_cont = np >= 6? my_atoi( &p[4] ): 0;
					bool bContourWasEmpty = ctr->corners.IsEmpty();
					ccorner *c = new ccorner(ctr, x, y);			// Constructor adds corner to ctr->corners (and may also set ctr->head/tail if
																	// it was previously empty)
					if (!bContourWasEmpty)
					{
						cside *s = new cside(ctr, last_side_style);
						ctr->AppendSideAndCorner(s, c, ctr->tail);
					}
					if( icor == (ncorners-1) || end_cont )
					{
						ctr->Close(last_side_style);
						if (icor<ncorners-1)
							ctr = new ccontour(a, false);		// Make a new secondary contour
					}
				}
			}

			// Delete bogus areas whose main contour has <3 corners
			citer<carea2> ia (&net->areas);
			for (carea2 *a = ia.First(); a; a = ia.Next())
				if (a->main->corners.GetSize() < 3)
					a->Remove();
			
			net->SetThermals();
			net->MustRedraw();
		}
	}
}

void cnetlist::WriteNets( CStdioFile * file )
{
	CString line;
	try
	{
		line.Format( "[nets]\n\n" );
		file->WriteString( line );
		citer<cnet2> in (&nets);
		for (cnet2 *net = in.First(); net; net = in.Next())
		{
			line.Format( "net: \"%s\" %d %d %d %d %d %d %d\n", 
							net->name, net->NumPins(), net->NumCons(), net->NumAreas(),
							net->def_w, net->def_via_w, net->def_via_hole_w,
							net->bVisible );
			file->WriteString( line );
			// CPT2.  First assign ID numbers to each tee (put 'em in the utility field)
			citer<ctee> it (&net->tees);
			int i = 1;
			for (ctee *t = it.First(); t; t = it.Next())
				t->utility = i++;

			// CPT2.  Now output net's pins.  Note that we put the pin's index number (0-based) into the utility field for use when outputting connects
			i = 0;
			citer<cpin2> ip (&net->pins);
			for (cpin2 *p = ip.First(); p; p = ip.Next(), i++)
			{
				p->utility = i;
				line.Format( "  pin: %d %s.%s\n", i+1, p->part->ref_des, p->pin_name );
				file->WriteString( line );
			}

			i = 0;
			citer<cconnect2> ic (&net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next(), i++)
			{
				int start_pin = c->head->pin? c->head->pin->utility: -1;
				int end_pin = c->tail->pin? c->tail->pin->utility: -1;
				line.Format( "  connect: %d %d %d %d %d\n", i+1, start_pin, end_pin, c->NumSegs(), c->locked );
				file->WriteString( line );
				int is = 0;
				for (cvertex2 *v = c->head; v!=c->tail; v = v->postSeg->postVtx, is++)
				{
					line.Format( "    vtx: %d %d %d %d %d %d %d %d\n", 
						is+1, v->x, v->y, v->pin? v->pin->pad_layer: 0, v->force_via_flag, 
						v->via_w, v->via_hole_w, v->tee? v->tee->utility: 0 );
					file->WriteString( line );
					cseg2 *s = v->postSeg;
					line.Format( "    seg: %d %d %d 0 0\n", is+1, s->m_layer, s->m_width );
					file->WriteString( line );
				}
				// last vertex
				cvertex2 *v = c->tail;
				line.Format( "    vtx: %d %d %d %d %d %d %d %d\n", 
					is+1, v->x, v->y, v->pin? v->pin->pad_layer: 0, v->force_via_flag, 
					v->via_w, v->via_hole_w, v->tee? v->tee->utility: 0 );
				file->WriteString( line );
			}

			i = 0;
			citer<carea2> ia (&net->areas);
			for (carea2 *a = ia.First(); a; a = ia.Next(), i++)
			{
				line.Format( "  area: %d %d %d %d\n", i+1, a->NumCorners(), 
					a->m_layer, a->m_hatch );
				file->WriteString( line );
				int icor = 1;
				citer<ccontour> ictr (&a->contours);
				for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
				{
					ccorner *c = ctr->head;
					line.Format( "  corner: %d %d %d %d\n", icor++, c->x, c->y, c->postSide->m_style);
					file->WriteString(line);
					for (c = c->postSide->postCorner; c!=ctr->head; c = c->postSide->postCorner)
					{
						line.Format( "  corner: %d %d %d %d %d\n", icor++, c->x, c->y, 
							c->postSide->m_style, c->postSide->postCorner==ctr->head);
						file->WriteString(line);
					}
				} 
			}
			file->WriteString( "\n" );
		}
	}

	catch( CFileException * e )
	{
		CString str, s ((LPCSTR) IDS_FileError1), s2 ((LPCSTR) IDS_FileError2);
		if( e->m_lOsError == -1 )
			str.Format( s, e->m_cause );
		else
			str.Format( s2, e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
	}
}

/*
CPT2 TODO I think this is no longer needed
void cnetlist::Copy(cnetlist *src)
{
	// CPT2 loosely based on old CNetList func.  Copies all pcb-items from src netlist into this one.
	nets.RemoveAll();
	citer<cnet2> in (&src->nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		cnet2 *net2 = new cnet2(net->doc, net->name, net->def_w, net->def_via_w, net->def_via_hole_w);
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			cconnect2 *c2 = new cconnect2(net2);
			cvertex2 *head = c->head;
		cvertex2 *head2 = new cvertex2(c2, head->x, head->y);
		if (head->pin)
			head2->pin = pl2->GetPinByNames( &head->pin->part->ref_des, &head->pin->pin_name );		// Might be null, which is now legal.
		if (head->tee)
		{
			int util = head->tee->utility;
			if (!util)
				util = head->tee->utility = nextTeeNum++;
			ctee *tee2 = net2->tees.FindByUtility( util );
			if (!tee2)
				tee2 = new ctee(net2), 
				tee2->utility = util;
			head2->tee = tee2;
		}
		if (head->utility)
			// Head vertex of source connect must be a selected via.  Copy the via params over to head2.
			head2->force_via_flag = true,				// ??
			head2->via_w = head->via_w,
			head2->via_hole_w = head->via_hole_w;

		c2->Start(head2);
		cvertex2 *v = head;
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


	}
}
*/

void cnetlist::ExportNetListInfo( netlist_info * nli )
{
	// make copy of netlist data so that it can be edited
	int i = 0;
	nli->SetSize( nets.GetSize() );
	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next(), i++)
	{
		net_info *ni = &(*nli)[i];
		ni->name = net->name;
		ni->net = net;
		ni->visible = net->bVisible;
		ni->w = net->def_w;
		ni->v_w = net->def_via_w;
		ni->v_h_w = net->def_via_hole_w;
		ni->apply_trace_width = FALSE;
		ni->apply_via_width = FALSE;
		ni->modified = FALSE;
		ni->deleted = FALSE;
		ni->merge_into = -1;								// CPT2 new member.  Default value = -1.
		ni->ref_des.SetSize(0);
		ni->pin_name.SetSize(0);
		// now make copy of pin arrays
		ni->ref_des.SetSize( net->NumPins() );
		ni->pin_name.SetSize( net->NumPins() );
		citer<cpin2> ip (&net->pins);
		int j = 0;
		for (cpin2 *pin = ip.First(); pin; pin = ip.Next(), j++)
		{
			ni->ref_des[j] = pin->part->ref_des;
			ni->pin_name[j] = pin->pin_name;
		}
	}
}

// import netlist_info data back into netlist
//
void cnetlist::ImportNetListInfo( netlist_info * nli, int flags, CDlgLog * log,
								 int def_w, int def_w_v, int def_w_v_h )
{
	CString mess;
	// Mark all existing nets for redrawing.  CPT2 TODO think about undo issues
	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
		net->MustRedraw();

	// CPT2 when importing from a netlist file,  go through and try to find pre-existing nets with the names indicated 
	// in nli's entries, and enter them into the net_info::net field.
	if (flags & IMPORT_FROM_NETLIST_FILE)
		for (int i=0; i<nli->GetSize(); i++)
		{
			net_info *ni = &(*nli)[i];
			if (ni->net) continue;
			ni->net = GetNetPtrByName( &ni->name );					// CPT2 TODO rename GetNetByName
		}

	// loop through netlist_info and remove any nets that are flagged for deletion
	int n_info_nets = nli->GetSize();
	for( int i=0; i<n_info_nets; i++ )
	{
		net_info *ni = &(*nli)[i];
		cnet2 *net = ni->net;
		if( ni->deleted && net )
		{
			// net was deleted, remove it
			if( log )
			{
				CString s ((LPCSTR) IDS_RemovingNet);
				mess.Format( s, net->name );
				log->AddLine( mess );
			}
			if (ni->merge_into >= 0)
			{
				// CPT2 new.  This option comes up when we're emerging from the DlgNetlist and user requested the combining of existing nets.
				//  Rather than simply delete the current net, combine it into the one indicated at position 
				// "ni->merge_into" of the netlist-info table.
				int mi = ni->merge_into;
				while ((*nli)[mi].merge_into >= 0)
					mi = (*nli)[mi].merge_into;				// It's possible user merged repeatedly during the life of DlgNetlist
				net_info *ni2 = &(*nli)[mi];
				if (ni2->deleted || !ni2->net)
					// It appears user deleted the combined net after requesting the merge...
					net->Remove();
				else
					net->MergeWith(ni2->net);
				ni->net = NULL;
			}
			else
			{
				net->Remove();
				ni->net = NULL;
			}
		}
	}

	// now handle any nets that were renamed 
	// assumes that the new name is not a duplicate
	for( int i=0; i<n_info_nets; i++ )
	{
		net_info *ni = &(*nli)[i];
		cnet2 *net = ni->net;
		if( net )
			net->name = ni->name;
	}

	// now check for existing nets that are not in netlist_info
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		// check if in netlist_info
		BOOL bFound = FALSE;
		for( int i=0; i<nli->GetSize(); i++ )
			if( net->name == (*nli)[i].name )
			{
				bFound = TRUE;
				break;
			}
		if( bFound ) continue;
		// net is not in netlist_info
		if( (flags & KEEP_NETS) && log )
		{
			CString s ((LPCSTR) IDS_KeepingNetNotInImportedNetlist);
			mess.Format( s, net->name );
			log->AddLine( mess );
		}
		else if (!(flags & KEEP_NETS))
		{
			if( log )
			{
				CString s ((LPCSTR) IDS_RemovingNet);
				mess.Format( s, net->name );
				log->AddLine( mess );
			}
			net->Remove();
		}
	}

	// now reloop, adding and modifying nets and deleting pins as needed
	for( int i=0; i<n_info_nets; i++ )
	{
		net_info *ni = &(*nli)[i];
		// ignore info nets marked for deletion
		if( ni->deleted )
			continue;
	
		// try to find existing net with this name
		cnet2 *net = ni->net;												// net from netlist_info (may be NULL)
		cnet2 *old_net = GetNetPtrByName( &ni->name );
		if( !net && !old_net )
		{
			// no existing net, add to netlist
			if( ni->w == -1 )
				ni->w = 0;
			if( ni->v_w == -1 )
				ni->v_w = 0;
			if( ni->v_h_w == -1 )
				ni->v_h_w = 0;
			net = new cnet2(this, ni->name, ni->w, ni->v_w, ni->v_h_w );
			ni->net = net;
		}
		else
		{
			if (!net)
				// no net from netlist_info but there's an existing net (old_net) with the same name
				// use existing net and modify it
				ni->modified = TRUE,
				ni->net = net = old_net;
			else
				// old_net is non-null, and it SHOULD be identical to the net from netlist_info
				ASSERT( net == old_net );
			// Change existing width values if requested
			if( ni->w != -1 )
				net->def_w = ni->w;
			if( ni->v_w != -1 )
				net->def_via_w = ni->v_w;
			if( ni->v_h_w != -1 )
				net->def_via_hole_w = ni->v_h_w;
		}

		net->name = ni->name;
		// now loop through net pins, detaching any which were removed from the net_info structure
		citer<cpin2> ip (&net->pins);
		for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
		{
			CString ref_des = pin->part->ref_des;
			CString pin_name = pin->pin_name;
			BOOL pin_present = FALSE;
			for( int ip = 0; ip < ni->ref_des.GetSize(); ip++ )
				if( ref_des == ni->ref_des[ip] && pin_name == ni->pin_name[ip] )
				{
					// pin in net found in netlist_info
					pin_present = TRUE;
					break;
				}
			if( pin_present ) 
				continue;

			// pin in net but not in netlist_info 
			if( !(flags & KEEP_PARTS_AND_CON) || !pin->part->bPreserve )
			{
				// delete it from net
				if( log )
				{
					CString s ((LPCSTR) IDS_RemovingPinFromNet);
					mess.Format( s, ref_des, pin_name, net->name  );
					log->AddLine( mess );
				}
				net->RemovePin(pin);
			}
		}
	}

	// now reloop and add any pins that were added to netlist_info
	for( int i=0; i<n_info_nets; i++ )
	{
		net_info *ni = &(*nli)[i];
		cnet2 * net = ni->net;
		if( net && !ni->deleted && ni->modified )
		{
			// loop through local pins, adding any new ones to net
			int n_local_pins = ni->ref_des.GetSize();
			for( int ipl=0; ipl<n_local_pins; ipl++ )
			{
				cpin2 *pin = m_plist->GetPinByNames( &ni->ref_des[ipl], &ni->pin_name[ipl] );
				if (!pin)
				{
					// This could happen if we're importing from a netlist file that refers to a bogus pin.  Ignore...
					CString s ((LPCSTR) IDS_WhileImportingNetIgnoredInvalidPin);
					mess.Format(s, net->name, ni->ref_des[ipl], ni->pin_name[ipl]);
					log->AddLine(mess);
					continue;
				}
				if (pin->net == net) continue;
				net->AddPin( pin );						// Takes care of detaching pin from its old net, if any
				if( log )
				{
					CString s ((LPCSTR) IDS_AddingPinToNet);
					mess.Format( s,	ni->ref_des[ipl], ni->pin_name[ipl], net->name );
					log->AddLine( mess );
				}
			}
		}
	}

	// now set visibility and apply new widths, if requested
	for( int i=0; i<n_info_nets; i++ )
	{
		net_info *ni = &(*nli)[i];
		cnet2 *net = ni->net;
		if (!net) 
			continue;
		net->bVisible = ni->visible;
		if( ni->apply_trace_width )
			net->SetWidth(ni->w, 0, 0);
		if( ni->apply_via_width )
			net->SetWidth(0, ni->v_w, ni->v_h_w);
	}
}



void cnetlist::MarkAllNets(int utility)
{
	// CPT2 like the CNetList func, set the utility flag on the net and its constituents.  I mark the pins in the net also, 
	// though I'm not sure if that's so necessary/desirable.
	citer<cnet2> in (&nets);
	for (cnet2 *n = in.First(); n; n = in.Next()) 
	{
		n->utility = utility;
		citer<cconnect2> ic (&n->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next()) 
		{
			c->utility = utility;
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next()) 
				v->utility = utility;
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				s->utility = utility;
		}
		citer<carea2> ia (&n->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			a->MarkConstituents(utility);
		n->pins.SetUtility(utility);
		n->tees.SetUtility(utility);
	}
}

void cnetlist::MoveOrigin(int dx, int dy)
{
	// CPT2 derived from CNetList func. of the same name
	citer<cnet2> in (&nets);
	for (cnet2 *n = in.First(); n; n = in.Next())
	{
		n->MustRedraw();
		citer<cconnect2> ic (&n->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
				v->x += dx,
				v->y += dy;
		}
		citer<carea2> ia (&n->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			a->Offset(dx, dy);
	}
}

int cnetlist::CheckNetlist( CString * logstr )
{
	// CPT2 converted.  A lot of these errors seem extremely implausible under the new system.  And if a user gets one of these messages,
	// they're not likely to have much clue what to do about it (except save a backup copy, bail out, and hope for the best I guess).
	CString str;
	int nwarnings = 0;
	int nerrors = 0;
	int nfixed = 0;
	CMapStringToPtr net_map;
	CMapStringToPtr pin_map;
	void *ptr;

	str.LoadStringA(IDS_CheckingNets);
	*logstr += str;

	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		CString net_name = net->name;
		if( net_map.Lookup( net_name, ptr ) )
		{
			CString s ((LPCSTR) IDS_ErrorNetIsDuplicate);
			str.Format( s, net_name );
			*logstr += str;
			nerrors++;
		}
		else
			net_map.SetAt( net_name, NULL );
		int npins = net->pins.GetSize();
		if( npins == 0 )
		{
			CString s ((LPCSTR) IDS_WarningNetHasNoPins);
			str.Format( s, net->name );
			*logstr += str;
			nwarnings++;
		}
		else if( npins == 1 )
		{
			CString s ((LPCSTR) IDS_WarningNetHasSinglePin);
			str.Format( s, net->name );
			*logstr += str;
			nwarnings++;
		}
		citer<cpin2> ipin (&net->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		{
			if( !pin->part )
			{
				// CPT2 colossally unlikely if not impossible.  We've got some sort of phantom pin, which I've outlawed.
				CString s ((LPCSTR) IDS_WarningNetPinNotConnectedPartDoesntExist);
				str.Format( s, net->name, "???", pin->pin_name, net->name );
				*logstr += str;
				nerrors++;
				continue;
			}
			CString * ref_des = &pin->part->ref_des;
			CString * pin_name = &pin->pin_name;
			CString pin_id = *ref_des + "." + *pin_name;
			if (pin->net != net)
			{
				CString s ((LPCSTR) IDS_ErrorNetPinAlreadyAssignedToNet);
				str.Format( s, net->name, pin_id, pin->net? pin->net->name: "(0)" );
				*logstr += str;
				nerrors++;
			}
			BOOL test = pin_map.Lookup( pin_id, ptr );
			if( test )
			{
				// CPT2 this error is _really_ weird under the new system (I should practically ASSERT(0)).  The only way I can imagine it
				// happening is that the partlist is somehow corrupt.  The only fix I'll attempt is to do net->pins.Remove(pin).
				CString s ((LPCSTR) IDS_ErrorNetPinIsDuplicate);
				str.Format( s, net->name, pin_id );
				*logstr += str;
				net->pins.Remove(pin);
				*logstr += str;
				nerrors++;
				continue;		// no further testing on this pin
			}
			else
				pin_map.SetAt( pin_id, net );

			if (!m_plist->parts.Contains(pin->part))
			{
				// net->pin->ref_des not found in partlist
				CString s ((LPCSTR) IDS_ErrorNetPinConnectedButPartNotInPartlist);
				str.Format( s, net->name, *ref_des, *pin_name );
				*logstr += str;
				nerrors++;
			}
			/*
			Like some of the other checks that used to be here, the following is something that I can let cpartlist::CheckPartlist() take care of.
			if( !pin->part->shape )
			{
				// part matches, but no footprint
				CString s ((LPCSTR) IDS_WarningNetPinConnectedButPartDoesntHaveFootprint);
				str.Format( s, net->name, *ref_des, *pin_name );
				*logstr += str;
				nwarnings++;
			}
			*/
			else if( !pin->part->pins.Contains(pin) )
			{
				// net->pin->pin_name doesn't exist in part
				CString s ((LPCSTR) IDS_WarningNetPinConnectedButPartDoesntHavePin);
				str.Format( s, net->name, *ref_des, *pin_name );
				*logstr += str;
				nwarnings++;
			}
			else
				// OK, all is well, peace on earth
				;
		}

		// now check connections
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				if( s->m_con != c )
				{
					CString str0 ((LPCSTR) IDS_ErrorNetConnectionSegmentWithInvalidPtrToConnect);
					str.Format( str0, net->name, c->UID(), s->UID() );
					*logstr += str;
					nerrors++;
				}

			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
				if( v->m_con != c )
				{
					CString str0 ((LPCSTR) IDS_ErrorNetConnectionVertexWithInvalidPtrToConnect);
					str.Format( str0, net->name, c->UID(), v->UID() );
					*logstr += str;
					nerrors++;
				}

			if( c->NumSegs() == 0 )
			{
				CString s ((LPCSTR) IDS_ErrorNetConnectionWithNoSegments);
				str.Format( s, net->name );
				*logstr += str;
				c->Remove();
				str.LoadStringA(IDS_FixedConnectionRemoved);
				*logstr += str;
				nerrors++;
				nfixed++;
			}
		}
	}
	CString s ((LPCSTR) IDS_ErrorsFixedWarnings);
	str.Format( s, nerrors, nfixed, nwarnings );
	*logstr += str;
	return nerrors;
}

int cnetlist::CheckConnectivity( CString * logstr )
{
	CString str;
	int nwarnings = 0;
	int nerrors = 0;
	int nfixed = 0;

	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		CString net_name = net->name;
		// now check connections
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			if( c->NumSegs() == 0 )
			{
				CString s ((LPCSTR) IDS_ErrorNetConnectionWithNoSegments);
				str.Format( s, net->name );
				*logstr += str;
				c->Remove();
				str.LoadStringA(IDS_FixedConnectionRemoved);
				*logstr += str;
				nerrors++;
				nfixed++;
			} 
			else if( c->head->pin && c->head->pin == c->tail->pin )
			{
				CString s ((LPCSTR) IDS_ErrorNetConnectionFromPinToItself);
				str.Format( s, net->name );
				*logstr += str;
				c->Remove();
				str.LoadStringA(IDS_FixedConnectionRemoved);
				*logstr += str;
				nerrors++;
				nfixed++;
			}
			else
			{
				bool bUnrouted = false;
				citer<cseg2> is (&c->segs);
				for (cseg2 *s = is.First(); s; s = is.Next())
					if( s->m_layer == LAY_RAT_LINE )
						{ bUnrouted = TRUE;	break; }
				if (!bUnrouted)
					continue;
				CString from_str, to_str, str, str0;
				c->head->GetTypeStatusStr( &from_str );
				c->tail->GetTypeStatusStr( &to_str );
				if( c->NumSegs() > 1 )
					str0.LoadStringA(IDS_PartiallyRoutedConnection);
				else
					str0.LoadStringA(IDS_UnroutedConnection);
				str.Format( str0, 1, net->name, from_str, to_str );
				*logstr += str;
				nerrors++;
			}
		}
	}
	return nerrors;
}

void cnetlist::CleanUpAllConnections( CString * logstr )
{
	CString str, s;
	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
		net->CleanUpConnections( logstr );

	// CPT2 Old code checked tee-ids, which is now an obsolete issue.  However I might as well check for tees with 0 vertices, 
	// and remove them.
	if( logstr )
		s.LoadStringA(IDS_CheckingTeesAndBranches),
		*logstr += s;
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		citer<ctee> it (&net->tees);
		for (ctee *t = it.First(); t; t = it.Next())
			if (t->vtxs.GetSize()==0)
			{
				s.LoadStringA(IDS_RemovingEmptyTeeStructure);
				str.Format(s, t->UID());
				net->tees.Remove(t);
			}
	}
}

void cnetlist::ReassignCopperLayers( int n_new_layers, int * layer )
{
	// reassign copper elements to new layers
	// enter with layer[] = table of new copper layers for each old copper layer
	if( m_layers < 1 || m_layers > 16 )
		ASSERT(0);
	citer<cnet2> in (&nets);
	for (cnet2 *net = in.First(); net; net = in.Next())
	{
		net->MustRedraw();
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
			{
				int old_layer = s->m_layer;
				if( old_layer >= LAY_TOP_COPPER )
				{
					int index = old_layer - LAY_TOP_COPPER;
					int new_layer = layer[index];
					if( new_layer == -1 )
						// delete this layer
						s->UnrouteWithoutMerge();
					else
						s->m_layer = new_layer + LAY_TOP_COPPER;
				}
			}
			// check for first or last segments connected to SMT pins
			int pad_layer1 = c->head->pin? c->head->pin->pad_layer: LAY_PAD_THRU;
			cseg2 *seg1 = c->head->postSeg;
			if( pad_layer1 != LAY_PAD_THRU && seg1->m_layer != pad_layer1 )
				seg1->UnrouteWithoutMerge();
			int pad_layer2 = c->tail->pin? c->tail->pin->pad_layer: LAY_PAD_THRU;
			cseg2 *seg2 = c->tail->preSeg;
			if (pad_layer2 != LAY_PAD_THRU && seg2->m_layer != pad_layer2 )
				seg2->UnrouteWithoutMerge();
			c->MergeUnroutedSegments();
		}
		net->CleanUpConnections();
		
		citer<carea2> ia (&net->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
		{
			int old_layer = a->m_layer;
			int index = old_layer - LAY_TOP_COPPER;
			int new_layer = layer[index];
			if( new_layer == -1 )
				a->Remove();
			else
				a->m_layer = new_layer + LAY_TOP_COPPER,
				a->PolygonModified(false, false);
		}
	}
	m_layers = n_new_layers;
}

