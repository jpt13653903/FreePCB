#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "PcbItem.h"
#include "NetListNew.h"
#include "FreePcbDoc.h"
#include "PartListNew.h"
#include "TextListNew.h"

extern BOOL bDontShowSelfIntersectionWarning;		// CPT2 TODO make these "sticky" by putting settings into default.cfg.
extern BOOL bDontShowSelfIntersectionArcsWarning;
extern BOOL bDontShowIntersectionWarning;
extern BOOL bDontShowIntersectionArcsWarning;

class CFreePcbDoc;
class cpcb_item;

int cpcb_item::next_uid = 1;
int cpcb_item::uid_hash_sz = 0x2000;
cpcb_item **cpcb_item::uid_hash = NULL;

cpcb_item::cpcb_item(CFreePcbDoc *_doc)
{
	carray_list = NULL; 
	if (!uid_hash)
	{
		// First ever item creation.  Initialize uid_hash table.
		int sz = uid_hash_sz * sizeof(cpcb_item*);
		uid_hash = (cpcb_item**) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz);
	}
	m_uid = next_uid++;
	doc = _doc;
	doc->items.Add(this);
	dl_el = dl_sel = NULL;
	utility = 0;
	bDrawn = false;
	// Enter item into uid_hash table.  It's an incredibly simple hash function:  just extract the lower few bits, depending on uid_hash_sz, which
	// must be a power of 2.  Given how objects are allocated and eventually die, this function should work as well as any.
	int hashVal = m_uid & (uid_hash_sz-1);
	uid_hash_next = uid_hash[hashVal];
	uid_hash[hashVal] = this;
}

cpcb_item::cpcb_item(CFreePcbDoc *_doc, int _uid)
{
	// Constructor used only during undo operations, for recreating items that user removed and that were subsequently garbage-collected.
	// Creates an empty object with the specified uid.  See the constructors cvertex2::cvertex2(_doc, _uid), etc.
	carray_list = NULL;
	m_uid = _uid;
	doc = _doc;
	doc->items.Add(this);
	dl_el = dl_sel = NULL;
	utility = 0;
	bDrawn = false;
	// Enter item into uid_hash table.
	int hashVal = m_uid & (uid_hash_sz-1);
	uid_hash_next = uid_hash[hashVal];
	uid_hash[hashVal] = this;
}

cpcb_item::~cpcb_item()
{
	// When an item is destroyed, references to it are automatically removed from any carrays to which it belongs:
	for (carray_link *link = carray_list, *next; link; link = next)
	{
		carray<cpcb_item> *arr = (carray<cpcb_item>*) link->arr;
		int off = link->off;
		ASSERT(arr->heap[off]==this);				// For now...
		arr->heap[off] = (cpcb_item*) arr->free;	// Add "off" to arr's free-offset list
		arr->free = off;
		arr->flags[off>>3] &= ~(1 << (off&7));
		arr->nItems--;
		next = link->next;
		delete link;
	}
	// Item is also removed from the uid-hash table.
	cpcb_item *i, *prev = NULL;
	int hashVal = m_uid & (uid_hash_sz-1);
	for (i = uid_hash[hashVal]; i; i = i->uid_hash_next)
	{
		if (i==this)
		{
			if (prev)
				prev->uid_hash_next = i->uid_hash_next;
			else
				uid_hash[hashVal] = i->uid_hash_next;
			break;
		}
		prev = i;
	}
}

cpcb_item *cpcb_item::FindByUid(int uid)
{
	// CPT2 new.  Static function.  Looks in static uid_hash[], using an extremely simple hash function (extract lower few bits).
	for (cpcb_item *i = uid_hash[uid & (uid_hash_sz-1)]; i; i = i->uid_hash_next)
		if (i->m_uid == uid)
			return i;
	return NULL;
}

void cpcb_item::MustRedraw()
{
	Undraw();
	if (doc)
		doc->redraw.Add(this);
}

void cpcb_item::Undraw()
{
	// Default behavior, often overridden
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	dl->Remove( dl_el );
	dl->Remove( dl_sel );
	dl_el = dl_sel = NULL;
	bDrawn = false;
}

void cpcb_item::SaveUndoInfo()
{
	// Default behavior, overridden in complex classes like cconnect2 and cnet2
	doc->m_undo_items.Add( this->MakeUndoItem() );
}

void cpcb_item::RemoveForUndo()
{
	// On a prior editing operation, user created this object, but now undo needs to get rid of it.  It should suffice to detach the object
	// from any carrays in which it exists (for instance, we remove a net from its netlist).  We can assume that any other pcb-items that 
	// reference this now-defunct object will be getting revised by this same undo operation.
	for (carray_link *link = carray_list, *next; link; link = next)
	{
		carray<cpcb_item> *arr = (carray<cpcb_item>*) link->arr;
		int off = link->off;
		ASSERT(arr->heap[off]==this);				// For now...
		arr->heap[off] = (cpcb_item*) arr->free;	// Add "off" to arr's free-offset list
		arr->free = off;
		arr->flags[off>>3] &= ~(1 << (off&7));
		arr->nItems--;
		next = link->next;
		delete link;
	}
	carray_list = NULL;
	// However, we may as well leave this item in doc->items.   There's a chance it might get recycled later (if user hits redo).  If not,
	// the garbage collector will clean it out eventually.
	doc->items.Add(this);
}

bool cpcb_item::IsHit(int x, int y)
{
	// CPT2 new.  Return true if this item's selector contains (x,y), which is in pcb units
	if (!dl_sel || !dl_sel->visible) 
		return false;
	int pcbu_per_wu = doc->m_dlist->m_pcbu_per_wu;
	double xx = double(x)/pcbu_per_wu;				// CPT (was int, which caused range issues)
	double yy = double(y)/pcbu_per_wu;				// CPT (was int)
	double d;
	return dl_sel->IsHit(xx, yy, d);
}

carea2 *cpcb_item::GetArea() 
{
	if (cpolyline *p = GetPolyline())
		return p->ToArea();
	return NULL;
}


/*
void cpcb_item::GarbageCollect() {
	// Part of my proposed new architecture is to take some of the hassle out of memory management for pcb-items by using garbage collection.
	// Each time an item is constructed, it is added to the "items" array.  If an item goes out of use, one does not have to "delete" it;
	// just unhook it from its parent entity or array.  When garbage collection time comes, we'll first clear the utility bits on all members of
	// "items".  Then we'll scan through the doc's netlist (recursing through connects, segs, vtxs, tees, areas), partlist (recursing through pins), 
	// textlist, and otherlist, marking utility bits as we go.  At the end, items with clear utility bits can be deleted.
	citer<cpcb_item> ii (&items);
	for (cpcb_item *i = ii.First(); i; i = ii.Next())
		i->utility = 0;

	citer<cnet2> in (&m_nlist->nets);
	for (cnet2 *n = in.First(); n; n = in.Next()) {
		n->utility = 1;
		citer<cconnect2> ic (&n->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next()) {
			c->utility = 1;
			citer<cvertex2> iv (&c->vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next()) {
				v->utility = 1;
				if (v->tee) v->tee->utility = 1;
				}
			citer<cseg2> is (&c->segs);
			for (cseg2 *s = is.First(); s; s = is.Next())
				s->utility = 1;
			}
		citer<carea2> ia (&n->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			a->MarkConstituents();
		}
	citer<cpart2> ip (&m_plist->parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next()) {
		p->utility = 1;
		citer<cpin2> ipin (&p->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			pin->utility = 1;
		}
	citer<ctext> it (&m_tlist->texts);
	for (ctext *t = it.First(); t; t = it.Next()) 
		t->utility = 1;
	citer<cpcb_item> ii2 (&others);
	for (cpcb_item *i = ii2.First(); i; i = ii2.Next()) {
		if (cpolyline *p = i->ToPolyline())
			p->MarkConstituents();
		else
			p->utility = 1;

	// Do the deletions of unused items!
	for (cpcb_item *i = ii.First(); i; i = ii.Next())
		if (!i.utility)
			delete i;
	}

*/

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

bool cvertex2::IsValid()
{
	if (!m_net->IsValid()) return false;
	if (!m_con->IsValid()) return false;
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
	int u = doc->m_units;
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
		//	m_con->m_net->m_dlist->Remove( dl_el[i] );				// CPT2.  Was this way.  Not sure why...
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

	int u = doc->m_units;
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

bool cseg2::IsValid()
{
	if (!m_net->IsValid()) return false;
	if (!m_con->IsValid()) return false;
	return m_con->segs.Contains(this);
}

// CPT:  added width param.  If this is 0 (the default) replace it with this->m_width
void cseg2::GetStatusStr( CString * str, int width )
{
	int u = doc->m_units;
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

bool cconnect2::IsValid()
	{ return m_net->IsValid() && m_net->connects.Contains(this); }

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
	// CPT2 new.  After this routine, connect will consist of a single vertex (v) and no segs.
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
/*  RELATED TO cpin2/cpart2                                                                     */
/**********************************************************************************************/

cpin2::cpin2(cpart2 *_part, cpadstack *_ps, cnet2 *_net)					// CPT2. Added args
	: cpcb_item (_part->doc)
{
	part = _part;
	part->pins.Add(this);
	ps = _ps;
	if( ps->hole_size ) 
		pad_layer = LAY_PAD_THRU;
	else if( part->side == 0 && ps->top.shape != PAD_NONE || part->side == 1 && ps->bottom.shape != PAD_NONE )
		pad_layer = LAY_TOP_COPPER;
	else
		pad_layer = LAY_BOTTOM_COPPER;
	pin_name = ps->name;
	net = _net;
	if (net)
		net->pins.Add(this);
	dl_hole = dl_thermal = NULL;
	bNeedsThermal = false;
}

cpin2::cpin2(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	part = NULL;
	ps = NULL;
	net = NULL;
	dl_hole = dl_thermal = NULL;
}

bool cpin2::IsValid()
{
	if (!part || !part->IsValid()) return false;
	return part->pins.Contains(this);
}

void cpin2::Disconnect() 
{
	// Detach pin from its part, and whichever net it's attached to.  Rip out any attached connections completely.
	part->pins.Remove(this);
	part = NULL;
	if (!net) return;
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
		if (c->head->pin == this || c->tail->pin == this)
			c->Remove();
	net->pins.Remove(this);
	net = NULL;
}

int cpin2::GetWidth()													// CPT2. Derived from CPartList::GetPinWidth()
{
	ASSERT(part->shape!=NULL);
	int w = ps->top.size_h;
	w = max( w, ps->bottom.size_h );
	w = max( w, ps->hole_size );
	return w;
}

void cpin2::GetVtxs(carray<cvertex2> *vtxs)
{
	// CPT2 new.  Fill "vtxs" with the vertices that are associated with this pin
	vtxs->RemoveAll();
	if (!net) return;
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		if (c->head->pin == this)
			vtxs->Add(c->head);
		if (c->tail->pin == this)
			vtxs->Add(c->tail);
	}
}

bool cpin2::TestHit( int _x, int _y, int _layer )
{
	// CPT2 Derived from CPartList::TestHitOnPad
	if( !part )
		return FALSE;
	if( !part->shape )
		return FALSE;

	cpad * p;
	if( ps->hole_size == 0 )
	{
		// SMT pad
		if( _layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else
			return FALSE;
	}
	else
	{
		// TH pad
		if( _layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( _layer == LAY_TOP_COPPER && part->side == 1 )
			p = &ps->bottom;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else if( _layer == LAY_BOTTOM_COPPER && part->side == 0 )
			p = &ps->bottom;
		else
			p = &ps->inner;
	}
	double dx = abs( x - _x );
	double dy = abs( y - _y );
	double dist = sqrt( dx*dx + dy*dy );
	if( dist < ps->hole_size/2 )
		return true;

	switch( p->shape )
	{
	case PAD_NONE: 
		return false;
	case PAD_ROUND: 
		return dist < (p->size_h/2);
	case PAD_SQUARE:
		return dx < (p->size_h/2) && dy < (p->size_h/2);
	case PAD_RECT:
	case PAD_RRECT:
	case PAD_OVAL:
		int pad_angle = part->angle + ps->angle;
		if( pad_angle > 270 )
			pad_angle -= 360;
		if( pad_angle == 0 || pad_angle == 180 )
			return dx < (p->size_l) && dy < (p->size_h/2);
		else
			return dx < (p->size_h/2) && dy < (p->size_l);
	}
	return false;
}


void cpin2::SetPosition()												
{
	// CPT2 New, but derived from CPartList::GetPinPoint().
	// Derives correct values for this->(x,y) based on the part's position and on the padstack position.
	// NB Doesn't call MustRedraw().
	ASSERT( part->shape!=NULL );

	// get pin coords relative to part origin
	CPoint pp;
	pp.x = ps->x_rel;
	pp.y = ps->y_rel;
	// flip if part on bottom
	if( part->side )
		pp.x = -pp.x;
	// rotate if necess.
	int angle = part->angle;
	if( angle > 0 )
	{
		CPoint org (0,0);
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	x = part->x + pp.x;
	y = part->y + pp.y;

	SetNeedsThermal();
}

bool cpin2::SetNeedsThermal()
{
	// CPT2 new, but related to the old CNetList::SetAreaConnections.  Sets bNeedsThermal to true if some net area overlaps this pin.
	// CPT2 TODO needs an SMT check?
	bool bOldVal = bNeedsThermal;
	bNeedsThermal = false;
	if (net) 
	{
		citer<carea2> ia (&net->areas);
		for (carea2 *a = ia.First(); a; a = ia.Next())
			if (a->TestPointInside(x, y))
				{ bNeedsThermal = true; break; }
	}
	if (bNeedsThermal!=bOldVal)
		MustRedraw();
	return bNeedsThermal;
}

void cpin2::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;							// CPT2 Necessary check?  NB selector was created drawn by cpart2::Draw
	dl->Highlight( DL_RECT_X, 
				dl->Get_x(dl_sel), dl->Get_y(dl_sel),
				dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 
				1, pad_layer );
}

cseg2 *cpin2::AddRatlineToPin( cpin2 *p2 )
{
	// Add new connection to net, consisting of one unrouted segment
	// from this to p2.
	// CPT2 Adapted from old cnet::AddConnectFromPinToPin.  NOW RETURNS THE RATLINE SEG, AND DOES NO DRAWING.
	cpin2 *p1 = this;
	if (!p1->part || !p2->part)
		return NULL;								// Seems mighty unlikely.
	if (!p1->part->shape || !p2->part->shape)
		return NULL;								// Ditto

	// create connection with a single vertex
	cconnect2 *c = new cconnect2 (net);
	cvertex2 *v1 = new cvertex2 (c, p1->x, p1->y);
	c->Start(v1);
	c->AppendSegment(p2->x, p2->y, LAY_RAT_LINE, 0);
	v1->pin = p1;
	c->tail->pin = p2;
	return c->head->postSeg;
}

bool cpin2::GetDrawInfo(int layer,	bool bUse_TH_thermals, bool bUse_SMT_thermals,  int mask_clearance, int paste_mask_shrink,
					  int * type, int * w, int * l, int * r, int * hole,
					  int * connection_status, int * pad_connect_flag, int * clearance_type )
{
	// CPT2 derived from CPartList::GetPadDrawInfo().  Used during DRC to determine the pin's pad info on a particular layer.
	// Got rid of some args that are no longer useful.  In particular got rid of "angle" and am just returning directly the
	// correct width and length, depending on the angle.
	CShape * s = part->shape;
	if( !s )
		return false;

	// get pin and padstack info
	BOOL bUseDefault = FALSE; // if TRUE, use copper pad for mask
	int connect_status = GetConnectionStatus( layer );
	// set default return values for no pad and no hole
	bool ret_code = false;
	int ttype = PAD_NONE;
	int ww = 0;
	int ll = 0;
	int rr = 0;
	int angle = (ps->angle + part->angle) % 180;
	int hole_size = ps->hole_size;
	int clear_type = CLEAR_NORMAL;	
	int connect_flag = PAD_CONNECT_DEFAULT;

	// get pad info
	cpad * p = NULL;
	if( (layer == LAY_TOP_COPPER && part->side == 0 )
		|| (layer == LAY_BOTTOM_COPPER && part->side == 1 ) ) 
		// top copper pad is on this layer 
		p = &ps->top;
	else if( (layer == LAY_MASK_TOP && part->side == 0 )
		|| (layer == LAY_MASK_BOTTOM && part->side == 1 ) ) 
	{
		// top mask pad is on this layer 
		if( ps->top_mask.shape != PAD_DEFAULT )
			p = &ps->top_mask;
		else
		{
			bUseDefault = TRUE;		// get mask pad from copper pad
			p = &ps->top;
		}
	}
	else if( (layer == LAY_PASTE_TOP && part->side == 0 )
		|| (layer == LAY_PASTE_BOTTOM && part->side == 1 ) ) 
	{
		// top paste pad is on this layer 
		if( ps->top_paste.shape != PAD_DEFAULT )
			p = &ps->top_paste;
		else
		{
			bUseDefault = TRUE;
			p = &ps->top;
		}
	}
	else if( (layer == LAY_TOP_COPPER && part->side == 1 )
			|| (layer == LAY_BOTTOM_COPPER && part->side == 0 ) ) 
		// bottom copper pad is on this layer
		p = &ps->bottom;
	else if( (layer == LAY_MASK_TOP && part->side == 1 )
		|| (layer == LAY_MASK_BOTTOM && part->side == 0 ) ) 
	{
		// bottom mask pad is on this layer 
		if( ps->bottom_mask.shape != PAD_DEFAULT )
			p = &ps->bottom_mask;
		else
		{
			bUseDefault = TRUE;
			p = &ps->bottom;
		}
	}
	else if( (layer == LAY_PASTE_TOP && part->side == 1 )
		|| (layer == LAY_PASTE_BOTTOM && part->side == 0 ) ) 
	{
		// bottom paste pad is on this layer 
		if( ps->bottom_paste.shape != PAD_DEFAULT )
			p = &ps->bottom_paste;
		else
		{
			bUseDefault = TRUE;
			p = &ps->bottom;
		}
	}
	else if( layer > LAY_BOTTOM_COPPER && ps->hole_size > 0 )
		// inner pad is on this layer
		p = &ps->inner;

	// now set parameters for return
	if( p )
		connect_flag = p->connect_flag;
	if( p == NULL )
		// no pad definition, return defaults
		;
	else if( p->shape == PAD_NONE && ps->hole_size == 0 )
		// no hole, no pad, return defaults
		;
	else if( p->shape == PAD_NONE )
	{
		// hole, no pad
		ret_code = true;
		if( connect_status > ON_NET )
		{
			// connected to copper area or trace
			// make annular ring
			ret_code = true;
			ttype = PAD_ROUND;
			ww = 2*doc->m_annular_ring_pins + hole_size;
		}
		else if( ( layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM ) && bUseDefault )
		{
			// if solder mask layer and no mask pad defined, treat hole as pad to get clearance
			ret_code = true;
			ttype = PAD_ROUND;
			ww = hole_size;
		}
	}
	else if( p->shape != PAD_NONE )
	{
		// normal pad
		ret_code = true;
		ttype = p->shape;
		ww = p->size_h;
		ll = 2*p->size_l;
		rr = p->radius;
	}
	else
		ASSERT(0);	// error

	// adjust mask and paste pads if necessary
	if( (layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM) && bUseDefault )
	{
		ww += 2*mask_clearance;
		ll += 2*mask_clearance;
		rr += mask_clearance;
	}
	else if( (layer == LAY_PASTE_TOP || layer == LAY_PASTE_BOTTOM) && bUseDefault )
	{
		if( ps->hole_size == 0 )
		{
			ww -= 2*paste_mask_shrink;
			ll -= 2*paste_mask_shrink;
			rr -= paste_mask_shrink;
			if( rr < 0 )
				rr = 0;
			if( ww <= 0 || ll <= 0 )
			{
				ww = 0;
				ll = 0;
				ret_code = 0;
			}
		}
		else
		{
			ww = ll = 0;	// no paste for through-hole pins
			ret_code = 0;
		}
	}

	// if copper layer connection, decide on thermal
	if( layer >= LAY_TOP_COPPER && (connect_status & AREA_CONNECT) )
	{
		// copper area connection, thermal or not?
		if( p->connect_flag == PAD_CONNECT_NEVER )
			ASSERT(0);										// shouldn't happen, this is an error by GetConnectionStatus(...)
		else if( p->connect_flag == PAD_CONNECT_NOTHERMAL )
			clear_type = CLEAR_NONE;
		else if( p->connect_flag == PAD_CONNECT_THERMAL )
			clear_type = CLEAR_THERMAL;
		else if( p->connect_flag == PAD_CONNECT_DEFAULT )
		{
			if( bUse_TH_thermals && ps->hole_size )
				clear_type = CLEAR_THERMAL;
			else if( bUse_SMT_thermals && !ps->hole_size )
				clear_type = CLEAR_THERMAL;
			else
				clear_type = CLEAR_NONE;
		}
		else
			ASSERT(0);
	}

	// CPT2 Get length and width right, depending on angle (used to be done within DRC(), but I think it makes more sense here)
	if( ttype != PAD_RECT && ttype != PAD_RRECT && ttype != PAD_OVAL )
		ll = ww;
	int tmp;
	if( angle == 90 )
		tmp = ll, ll = ww, ww = tmp;

	if( type )
		*type = ttype;
	if( w )
		*w = ww;
	if( l )
		*l = ll;
	if( r )
		*r = rr;
	if( hole )
		*hole = hole_size;
	if( connection_status )
		*connection_status = connect_status;
	if( pad_connect_flag )
		*pad_connect_flag = connect_flag;
	if( clearance_type )
		*clearance_type = clear_type;
	return ret_code;

}

int cpin2::GetConnectionStatus( int layer )
{
	// CPT2 derived from CPartList::GetPinConnectionStatus
	// checks to see if a pin is connected to a trace or a copper area on a
	// particular layer
	//
	// returns: ON_NET | TRACE_CONNECT | AREA_CONNECT
	// where:
	//		ON_NET = 1 if pin is on a net
	//		TRACE_CONNECT = 2 if pin connects to a trace
	//		AREA_CONNECT = 4 if pin connects to copper area
	if( !net )
		return NOT_CONNECTED;
	int status = ON_NET;

	// now check for traces
	citer<cconnect2> ic (&net->connects);
	for (cconnect2 *c = ic.First(); c; c = ic.Next())
	{
		if (c->head->pin == this && c->head->postSeg->m_layer == layer)
			{ status |= TRACE_CONNECT; break; }
		if (c->tail->pin == this && c->tail->preSeg->m_layer == layer)
			{ status |= TRACE_CONNECT; break; }
	}
	// now check for connection to copper area
	citer<carea2> ia (&net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		if (a->m_layer == layer && a->TestPointInside(x, y))
			{ status |= AREA_CONNECT; break; }

	return status;
}


cpart2::cpart2( cpartlist * pl )			// CPT2 TODO.  Will probably add more args...
	: cpcb_item (pl->m_doc)
{ 
	m_pl = pl;
	if (pl)
		pl->parts.Add(this);
	x = y = side = angle = 0;
	glued = false;
	m_ref = new creftext(this, 0, 0, 0, false, false, 0, 0, 0, doc->m_smfontutil, &CString(""), true);
	m_value = new cvaluetext(this, 0, 0, 0, false, false, 0, 0, 0, doc->m_smfontutil, &CString(""), false);
	m_tl = new ctextlist(doc);
	shape = NULL;
}

cpart2::cpart2(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	m_pl = NULL;
	m_ref = NULL;
	m_value = NULL;
	m_tl = new ctextlist(doc);
	shape = NULL;
}

bool cpart2::IsValid()
	{ return doc->m_plist->parts.Contains(this); }

void cpart2::SaveUndoInfo(bool bSaveAttachedNets)
{
	doc->m_undo_items.Add( new cupart(this) );
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		doc->m_undo_items.Add( new cupin(pin) );
		if (bSaveAttachedNets && pin->net)
			pin->net->SaveUndoInfo( cnet2::SAVE_CONNECTS );
	}
	doc->m_undo_items.Add( new cureftext(m_ref) );
	doc->m_undo_items.Add( new cuvaluetext(m_value) );
	citer<ctext> it (&m_tl->texts);
	for (ctext *t = it.First(); t; t = it.Next())
		doc->m_undo_items.Add( new cutext(t) );
}

void cpart2::Move( int _x, int _y, int _angle, int _side )
{
	// Move part (includes resetting pin positions and undrawing).  If _angle and/or _side are -1 (the default values), don't change 'em.
	MustRedraw();
	// move part
	x = _x;
	y = _y;
	if (_angle != -1)
		angle = _angle % 360;
	if (_side != -1)
		side = _side;
	// calculate new pin positions
	if( shape )
	{
		citer<cpin2> ip (&pins);
		for (cpin2* p = ip.First(); p; p = ip.Next())
			p->SetPosition();
	}
}

// Part moved, so unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// CPT:  added dx and dy params whereby caller may indicate how much the part moved (both are 1 by default).  If, say, dx==0
// and an attached seg is vertical, then we don't have to unroute it.
// CPT2:  Derived from CNetList::PartMoved.

void cpart2::PartMoved( int dx, int dy )
{
	// We need to ensure that the part is redrawn first, so that the pins have correct x/y values (critical in what follows):
	doc->Redraw();

	// first, mark all nets unmodified
	doc->m_nlist->nets.SetUtility(0);

	// find nets that connect to this part, and unroute segments adjacent to pins.  
	// CPT2 I'll let the subroutines take care of calling MustUndraw() for the appropriate objects.
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
	{
		cnet2 *net = pin->net;
		if (!net || net->utility) continue;
		citer<cconnect2> ic (&net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			if (c->segs.GetSize()==0) continue;				// Unlikely but hey...
			if (c->head->pin && c->head->pin->part == this)
			{
				cvertex2 *head = c->head;
				head->postSeg->Unroute( dx, dy, 0 );
				head->postSeg->MustRedraw();				// Even if it wasn't unrouted...
				head->x = head->pin->x;
				head->y = head->pin->y;
				net->utility = 1;	// mark net modified
				// CPT2 TODO decide if it's really safe to dump the following:
				/*
				if( part->shape->m_padstack[pin_index1].hole_size )
					// through-hole pad
					v0->pad_layer = LAY_PAD_THRU;
				else if( part->side == 0 && part->shape->m_padstack[pin_index1].top.shape != PAD_NONE
					|| part->side == 1 && part->shape->m_padstack[pin_index1].bottom.shape != PAD_NONE )
					// SMT pad on top
					v0->pad_layer = LAY_TOP_COPPER;
				else
					// SMT pad on bottom
					v0->pad_layer = LAY_BOTTOM_COPPER;
				if( part->pin[pin_index1].net != net )
					part->pin[pin_index1].net = net;
				*/
			}
			if (c->tail->pin && c->tail->pin->part == this)
			{
				// end pin is on part, unroute last segment
				cvertex2 *tail = c->tail;
				tail->preSeg->Unroute( dx, dy, 1 );
				tail->preSeg->MustRedraw();
				tail->x = tail->pin->x;
				tail->y = tail->pin->y;
				net->utility = 1;	// mark net modified
				// CPT2 TODO similar to issue in previous if-statement
			}
		}
	}
}

void cpart2::Remove(bool bEraseTraces, bool bErasePart)
{
	// CPT2 mostly new.  Implements my new system for deleting parts, where one can either erase all traces emanating from it, or not.
	// Also added bErasePart param (true by default);  calling Remove(true, false) allows one to delete attached traces only.
	if (bErasePart)
		Undraw(),
		m_pl->parts.Remove(this);

	// Now go through the pins.  If bEraseTraces, rip out connects attached to each one.  Otherwise, gather a list of vertices associated
	// with each one (such vertices will later get united into a tee).  TODO for each pin, consider maintaining a list of vtxs associated with it
	// at all times?
	citer<cpin2> ipin (&pins);
	carray<cvertex2> vtxs;
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		if (!pin->net) continue;
		pin->net->pins.Remove(pin);							// CPT2 Prevent phantom pins in nets!
		pin->net->MustRedraw();
		if (bEraseTraces)
		{
			citer<cconnect2> ic (&pin->net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next())
				if (c->head->pin==pin || c->tail->pin==pin)
					c->Remove();
		}
		else
		{
			// Make a list of vertices that are attached to the current pin.
			pin->GetVtxs(&vtxs);
			if (vtxs.IsEmpty()) continue;
			// Loop thru each vtx, and if the incoming seg is a ratline, erase it (and the vertex).  Otherwise, null out the vtx's "pin" member, and
			// assign it instead to a new tee structure.
			ctee *tee = new ctee(pin->net);
			citer<cvertex2> iv (&vtxs);
			for (cvertex2 *v = iv.First(); v; v = iv.Next())
				if (v->preSeg && v->preSeg->IsValid() && v->preSeg->m_layer == LAY_RAT_LINE)
					v->preSeg->RemoveBreak();
				else if (v->postSeg && v->postSeg->IsValid() && v->postSeg->m_layer == LAY_RAT_LINE)
					v->postSeg->RemoveBreak();
				else
					v->pin = NULL,
					tee->Add(v);
			tee->Adjust();
		}
	}
}

void cpart2::SetData(CShape * _shape, CString * _ref_des, CString * _value_text, CString * _package, 
					 int _x, int _y, int _side, int _angle, int _visible, int _glued  )
{
	// Derived from old CPartList::SetPartData.  Initializes data members, including "pins".  Ref and value-text related
	// members are initialized according to _shape if possible.
	// CPT2 Delete the following?
	// CDisplayList * old_dlist = m_dlist;
	// m_dlist = NULL;		// cancel further drawing

	// now copy data into part
	MustRedraw();
	visible = _visible;
	ref_des = *_ref_des;
	m_ref->m_str = *_ref_des;
	if (_value_text)
		value_text = m_value->m_str = *_value_text;
	else
		value_text = m_value->m_str = "";
	if( _package )
		package = *_package;
	else
		package = "";
	x = _x;
	y = _y;
	side = _side;
	angle = _angle;
	glued = _glued;

	if( !_shape )
	{
		shape = NULL;
		m_ref->Move(0, 0, 0, 0, 0);
		m_value->Move(0, 0, 0, 0, 0);
	}
	else
	{
		shape = _shape;
		InitPins();
		Move( x, y, angle, side );	// force setting pin positions

		ctext *txt = shape->m_ref;
		int layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_ref->Move( txt->m_x, txt->m_y, txt->m_angle, 
			false, false, layer, txt->m_font_size, txt->m_stroke_width );
		txt = shape->m_value;
		layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_value->Move( txt->m_x, txt->m_y, txt->m_angle, 
			false, false, layer, txt->m_font_size, txt->m_stroke_width );
		citer<ctext> it (&shape->m_tl->texts);
		for (txt = it.First(); txt; txt = it.Next())
		{
			layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
			m_tl->AddText(txt->m_x, txt->m_y, txt->m_angle, txt->m_bMirror, txt->m_bNegative, 
							layer, txt->m_font_size, txt->m_stroke_width, &txt->m_str, this);
		}
	}

	m_outline_stroke.SetSize(0);
	m_ref->m_stroke.SetSize(0);
	m_value->m_stroke.SetSize(0);
}

void cpart2::SetValue(CString *_value, int x, int y, int angle, int size, int w, 
						 BOOL vis, int layer )
{
	value_text = *_value;
	m_value->m_layer = layer;
	m_value->Move(x, y, angle, size, w);
	m_value->m_bShown = vis;
}

void cpart2::ResizeRefText(int size, int width, BOOL vis )
{
	m_ref->Move(m_ref->m_x, m_ref->m_y, m_ref->m_angle, size, width);
	m_ref->m_bShown = vis;
}

CRect cpart2::CalcSelectionRect()
{
	CRect sel;
	if( !side )
	{
		// part on top
		sel.left = shape->m_sel_xi; 
		sel.right = shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
	}
	else
	{
		// part on bottom
		sel.right = - shape->m_sel_xi;
		sel.left = - shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
	}
	if( angle > 0 )
		RotateRect( &sel, angle, zero );
	sel.left += x;
	sel.right += x;
	sel.bottom += y;
	sel.top += y;
	return sel;
}

void cpart2::InitPins()
{
	// CPT2 New.  Initialize pins carray based on the padstacks in the shape.
	ASSERT(shape);
	citer<cpadstack> ips (&shape->m_padstack);
	for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
		pins.Add( new cpin2(this, ps, NULL) );
}

cpin2 *cpart2::GetPinByName(CString *name)
{
	// CPT2 new.
	citer<cpin2> ip (&pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->pin_name == *name)
			return p;
	return NULL;
}


int cpart2::GetBoundingRect( CRect * part_r )
{
	// CPT2 TODO this function is kind of a loser, since it only works for drawn parts.  Phase it out in favor of CalcSelectionRect()...
	CRect r;
	if( !shape || !dl_sel ) 
		return 0;
	CDisplayList *dl = doc->m_dlist;
	r.left = min( dl->Get_x( dl_sel ), dl->Get_xf( dl_sel ) );
	r.right = max( dl->Get_x( dl_sel ), dl->Get_xf( dl_sel ) );
	r.bottom = min( dl->Get_y( dl_sel ), dl->Get_yf( dl_sel ) );
	r.top = max( dl->Get_y( dl_sel ), dl->Get_yf( dl_sel ) );
	*part_r = r;
	return 1;
}

void cpart2::ChangeFootprint(CShape *_shape)
{
	// CPT2.  Loosely derived from CPartList::PartFootprintChanged && CNetList::PartFootprintChanged.
	// The passed-in shape is the one that will replace this->shape.  Setup pins corresponding to the new shape, reusing old pins where 
	// possible.  Mark used pins with
	// a utility value of 1.  Pins that are unused at the end get axed.
	MustRedraw();
	carray<cnet2> nets;									// Maintain a list of attached nets so that we can redraw them afterwards
	citer<cpin2> ip (&pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->net)
			nets.Add(p->net),
			p->net->MustRedraw();

	shape = _shape;
	pins.SetUtility(0);
	int cPins = shape->m_padstack.GetSize();
	carray<cvertex2> vtxs;
	citer<cpadstack> ips (&shape->m_padstack);
	for (cpadstack *ps = ips.First(); ps; ps = ips.Next())
	{
		cpin2 *old = GetPinByName(&ps->name);
		cpin2 *p;
		int oldX, oldY, oldLayer;
		if (old)
		{
			oldX = old->x, oldY = old->y;
			oldLayer = old->pad_layer;
			old->ps = ps;
			p = old;
			if( ps->hole_size ) 
				p->pad_layer = LAY_PAD_THRU;
			else if( side == 0 && ps->top.shape != PAD_NONE || side == 1 && ps->bottom.shape != PAD_NONE )
				p->pad_layer = LAY_TOP_COPPER;
			else
				p->pad_layer = LAY_BOTTOM_COPPER;
			p->SetPosition();
			bool bLayerChange = p->pad_layer!=oldLayer && p->pad_layer!=LAY_PAD_THRU;
			if (oldX != p->x || oldY != p->y || bLayerChange)
			{
				// Pin's position has changed.  Find vertices associated with it, and unroute their adjacent segs as needed
				p->GetVtxs(&vtxs);
				citer<cvertex2> iv (&vtxs);
				for (cvertex2 *v = iv.First(); v; v = iv.Next())
					if (v->preSeg && v->preSeg->m_layer!=LAY_RAT_LINE)
						v->preSeg->Unroute();
					else if (v->postSeg && v->postSeg->m_layer!=LAY_RAT_LINE)
						v->postSeg->Unroute();
			}
		}
		else
			p = new cpin2(this, ps, NULL),
			p->SetPosition();
		p->utility = 1;
	}

	// Look for disused pins (utility==0) and dump 'em.  NB Disconnect() rips out all attached traces completely.  TODO change or allow user an option to 
	// keep traces?
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (!p->utility) 
			p->Disconnect();

	// Dump texts from the old footprint and add texts from the new one:
	m_tl->texts.RemoveAll();
	citer<ctext> it (&shape->m_tl->texts);
	for (ctext *txt = it.First(); txt; txt = it.Next()) 
	{
		int layer = cpartlist::FootprintLayer2Layer( txt->m_layer );
		m_tl->AddText(txt->m_x, txt->m_y, txt->m_angle, txt->m_bMirror, txt->m_bNegative, 
							layer, txt->m_font_size, txt->m_stroke_width, &txt->m_str, this);
	}
}

void cpart2::OptimizeConnections(BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly )
{
	if (!shape) return;

	// find nets which connect to this part, and mark them all unoptimized
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
		if (pin->net)
			pin->net->utility = 0;

	// optimize each net and mark it optimized so it won't be repeated
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
		if (pin->net && !pin->net->utility)
			pin->net->OptimizeConnections(bBelowPinCount, pin_count, bVisibleNetsOnly),
			pin->net->utility = 1;
}

int cpart2::Draw()
{
	// CPT2 TODO:  Think about the CDisplayList arg business
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if( !shape )
		return NO_FOOTPRINT;
	if (bDrawn)
		return ALREADY_DRAWN;

	// draw selection rectangle (layer = top or bottom copper, depending on side)
	CRect sel = CalcSelectionRect();
	dl_sel = dl->AddSelector( this, LAY_SELECTION, DL_HOLLOW_RECT, 1,
		0, 0, sel.left, sel.bottom, sel.right, sel.top, x, y );
	/* dl->Set_sel_vert( dl_sel, 0 );											// CPT2 TODO this function appears axeable
	if( angle == 90 || angle ==  270 )
		dl->Set_sel_vert( dl_sel, 1 ); */

	// draw ref designator text
	m_ref->dl_el = m_ref->dl_sel = NULL;
	if( m_ref->m_bShown && m_ref->m_font_size )
		m_ref->DrawRelativeTo(this);
	else
		m_ref->Undraw();
	
	// draw value text
	m_value->dl_el = m_value->dl_sel = NULL;
	if( m_value->m_bShown && m_value->m_font_size )
		m_value->DrawRelativeTo(this);
	else
		m_value->Undraw();

	// draw part outline (code similar to but sadly not identical to cpolyline::Draw())
	CPoint si, sf;
	m_outline_stroke.SetSize(0);
	citer<coutline> io (&shape->m_outline_poly);
	for (cpolyline *poly = io.First(); poly; poly = io.Next())
	{
		int shape_layer = poly->GetLayer();
		int poly_layer = cpartlist::FootprintLayer2Layer( shape_layer );
		poly_layer = FlipLayer( side, poly_layer );
		int w = poly->W();
		citer<ccontour> ictr (&poly->contours);
		for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		{
			// Draw polyline sides
			citer<cside> is (&ctr->sides);
			for (cside *s = is.First(); s; s = is.Next())
			{
				si.x = s->preCorner->x, si.y = s->preCorner->y;
				sf.x = s->postCorner->x, sf.y = s->postCorner->y;
				if( si.x == sf.x || si.y == sf.y )
					// if endpoints not angled, make side STRAIGHT
					s->m_style = STRAIGHT;
				int g_type = DL_LINE;
				if( s->m_style == STRAIGHT )
					g_type = DL_LINE;
				else if( s->m_style == ARC_CW )
					g_type = DL_ARC_CW;
				else if( s->m_style == ARC_CCW )
					g_type = DL_ARC_CCW;
				// flip if part on bottom
				if( side )
				{
					si.x = -si.x;
					sf.x = -sf.x;
					if( g_type == DL_ARC_CW )
						g_type = DL_ARC_CCW;
					else if( g_type == DL_ARC_CCW )
						g_type = DL_ARC_CW;
				}
				// rotate with part and draw
				RotatePoint( &si, angle, zero );
				RotatePoint( &sf, angle, zero );
				stroke str;
				str.layer = poly_layer;
				str.xi = x+si.x;
				str.xf = x+sf.x;
				str.yi = y+si.y;
				str.yf = y+sf.y;
				str.type = g_type;
				str.w = w;
				str.dl_el = dl->AddMain( this, poly_layer,					// CPT2 TODO Is AddMain() appropriate?
					g_type, 1, w, 0, 0, x+si.x, y+si.y, x+sf.x, y+sf.y, 0, 0 );
				m_outline_stroke.Add(str);
			}
		}
	}

	// draw footprint texts
	citer<ctext> it (&m_tl->texts);
	for (ctext *t = it.First(); t; t = it.Next())
		t->DrawRelativeTo(this, false);

	// draw padstacks and save absolute position of pins
	CPoint pin_pt;
	CPoint pad_pi;
	CPoint pad_pf;
	int nLayers = doc->m_plist->m_layers;
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next()) 
	{
		// CPT2 TODO check:  is pin->x/y always getting set correctly?
		// set layer for pads
		cpadstack * ps = pin->ps;
		pin->dl_els.SetSize(nLayers);
		cpad * p;
		int pad_layer;
		// iterate through all copper layers 
		for( int il=0; il<nLayers; il++ )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			pad_layer = il + LAY_TOP_COPPER;
			pin->dl_els[il] = NULL;
			// get appropriate pad
			cpad * p = NULL;
			if( pad_layer == LAY_TOP_COPPER && side == 0 )
				p = &ps->top;
			else if( pad_layer == LAY_TOP_COPPER && side == 1 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && side == 0 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && side == 1 )
				p = &ps->top;
			else if( ps->hole_size )
				p = &ps->inner;
			int sel_layer = pad_layer;
			if( ps->hole_size )
				sel_layer = LAY_SELECTION;
			if( p )
			{
				// draw pad
				dl_element * pad_el = NULL;
				if( p->shape == PAD_NONE )
					;
				else if( p->shape == PAD_ROUND )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					// add to display list
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_CIRC, 1, 
						p->size_h, 0, 0,
						x + pin_pt.x, y + pin_pt.y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,	pin->y-p->size_h/2, 
							pin->x+p->size_h/2,	pin->y+p->size_h/2, 0, 0 );
				}

				else if( p->shape == PAD_SQUARE )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_SQUARE, 1, 
						p->size_h,
						0, 0,
						pin->x, pin->y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,	pin->y-p->size_h/2, 
							pin->x+p->size_h/2,	pin->y+p->size_h/2, 0, 0 );
				}

				else if( p->shape == PAD_RECT 
					|| p->shape == PAD_RRECT 
					|| p->shape == PAD_OVAL )
				{
					int gtype;
					if( p->shape == PAD_RECT )
						gtype = DL_RECT;
					else if( p->shape == PAD_RRECT )
						gtype = DL_RRECT;
					else
						gtype = DL_OVAL;
					pad_pi.x = pin_pt.x - p->size_l;
					pad_pi.y = pin_pt.y - p->size_h/2;
					pad_pf.x = pin_pt.x + p->size_r;
					pad_pf.y = pin_pt.y + p->size_h/2;
					// rotate pad about pin if necessary
					if( ps->angle > 0 )
					{
						RotatePoint( &pad_pi, ps->angle, pin_pt );
						RotatePoint( &pad_pf, ps->angle, pin_pt );
					}
					// flip if part on bottom
					if( side )
					{
						pin_pt.x = -pin_pt.x;
						pad_pi.x = -pad_pi.x;
						pad_pf.x = -pad_pf.x;
					}
					// rotate part about 
					if( angle > 0 )
					{
						RotatePoint( &pin_pt, angle, zero );
						RotatePoint( &pad_pi, angle, zero );
						RotatePoint( &pad_pf, angle, zero );
					}
					int radius = p->radius;
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						gtype, 1, 
						0, 0, 0, 
						x + pad_pi.x, y + pad_pi.y, 
						x + pad_pf.x, y + pad_pf.y, 
						x + pin_pt.x, y + pin_pt.y, 
						p->radius );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							x + pad_pi.x, y + pad_pi.y, 
							x + pad_pf.x, y + pad_pf.y,
							0, 0 );
				}

				else if( p->shape == PAD_OCTAGON )
				{
					// flip if part on bottom
					if( side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = dl->AddMain( pin, pad_layer, 
						DL_OCTAGON, 1, 
						p->size_h, 0, 0,
						pin->x, pin->y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
						pin->dl_sel = dl->AddSelector( pin, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2, pin->y-p->size_h/2, 
							pin->x+p->size_h/2, pin->y+p->size_h/2, 0, 0 );
				}

				pin->dl_els[il] = pad_el;
				pin->dl_hole = pad_el;
			}
		}
		
		// if through-hole pad, just draw hole and set pin->dl_hole
		if( ps->hole_size )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			// flip if part on bottom
			if( side )
				pin_pt.x = -pin_pt.x;
			// rotate
			if( angle > 0 )
				RotatePoint( &pin_pt, angle, zero );
			// add to display list
			pin->x = x + pin_pt.x;
			pin->y = y + pin_pt.y;
			pin->dl_hole = dl->AddMain( pin, LAY_PAD_THRU, 
								DL_HOLE, 1,	
								ps->hole_size, 0, 0,
								pin->x, pin->y, 0, 0, 0, 0 );  
			if( !pin->dl_sel )
				// make selector for pin with hole only
				pin->dl_sel = dl->AddSelector( pin, LAY_SELECTION, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin->x-ps->hole_size/2, pin->y-ps->hole_size/2,  
					pin->x+ps->hole_size/2, pin->y+ps->hole_size/2,  
					0, 0 );
		}
		else
			pin->dl_hole = NULL;

		if (pin->bNeedsThermal)
			pin->dl_thermal = dl->Add( this, dl_element::DL_OTHER, LAY_RAT_LINE, DL_X, 1,
				2*ps->hole_size/3, 0, 0, pin->x, pin->y, 0, 0, 0, 0 );
		else
			pin->dl_thermal = NULL;
	}

	bDrawn = true;
	return NOERR;
}

void cpart2::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || !IsDrawn() ) return;
	if (!shape)
		{ bDrawn = false; return; }

	// undraw selection rectangle
	dl->Remove( dl_sel );
	dl_sel = 0;

	// undraw ref text and value text
	m_ref->Undraw();
	m_value->Undraw();

	// undraw part outline (this also includes footprint free text)
	for( int i=0; i<m_outline_stroke.GetSize(); i++ )
	{
		dl->Remove( m_outline_stroke[i].dl_el );
		m_outline_stroke[i].dl_el = 0;
	}

	// undraw footprint texts
	citer<ctext> it (&m_tl->texts);
	for (ctext *t = it.First(); t; t = it.Next())
		t->Undraw();

	// undraw padstacks
	citer<cpin2> ip (&pins);
	for (cpin2 *pin = ip.First(); pin; pin = ip.Next())
	{
		if( pin->dl_els.GetSize()>0 )
		{
			for( int il=0; il<pin->dl_els.GetSize(); il++ )
				if( pin->dl_els[il] != pin->dl_hole )
					dl->Remove( pin->dl_els[il] );
			pin->dl_els.RemoveAll();
		}
		dl->Remove( pin->dl_hole );
		dl->Remove( pin->dl_sel );
		dl->Remove( pin->dl_thermal );
		pin->dl_hole = pin->dl_sel = pin->dl_thermal = NULL;
	}

	bDrawn = false;
}

void cpart2::Highlight( )
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if (!shape) return;
	dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
				dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ), 1 );

	// Also highlight ref and value texts if possible
	if (dl_element *ref_sel = m_ref->dl_sel)
		dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( ref_sel ), dl->Get_y( ref_sel ),
				dl->Get_xf( ref_sel ), dl->Get_yf( ref_sel ), 1 );
	if (dl_element *val_sel = m_value->dl_sel)
		dl->Highlight( DL_HOLLOW_RECT, 
				dl->Get_x( val_sel ), dl->Get_y( val_sel ),
				dl->Get_xf( val_sel ), dl->Get_yf( val_sel ), 1 );
	
}

void cpart2::SetVisible(bool bVisible)
{
	// CPT2, derived from CPartList::MakePartVisible()
	// Make part visible or invisible, including thermal reliefs
	// outline strokes
	for( int i=0; i<m_outline_stroke.GetSize(); i++ )
	{
		dl_element * el = m_outline_stroke[i].dl_el;
		el->visible = bVisible;
	}
	// pins
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		// pin pads
		if (pin->dl_hole)
			pin->dl_hole->visible = bVisible;
		for( int i=0; i < pin->dl_els.GetSize(); i++ )
			if( pin->dl_els[i] )
				pin->dl_els[i]->visible = bVisible;
		if (pin->dl_thermal)
			pin->dl_thermal->visible = bVisible;
	}
	m_ref->SetVisible( bVisible );
	m_value->SetVisible( bVisible );
	// CPT2 TODO also set visibility for texts within footprint
}


void cpart2::StartDragging( CDC * pDC, BOOL bRatlines, BOOL bBelowPinCount, int pin_count ) 
{
	// CPT2, derived from CPartList::StartDraggingPart
	// make part invisible
	CDisplayList *dl = doc->m_dlist;
	SetVisible( FALSE );
	dl->CancelHighlight();

	// create drag lines
	CPoint zero(0,0);
	dl->MakeDragLineArray( 2*pins.GetSize() + 4 );
	int xi = shape->m_sel_xi;
	int xf = shape->m_sel_xf;
	if( side )
		xi = -xi,
		xf = -xf;
	int yi = shape->m_sel_yi;
	int yf = shape->m_sel_yf;
	CPoint p1( xi, yi );
	CPoint p2( xf, yi );
	CPoint p3( xf, yf );
	CPoint p4( xi, yf );
	RotatePoint( &p1, angle, zero );
	RotatePoint( &p2, angle, zero );
	RotatePoint( &p3, angle, zero );
	RotatePoint( &p4, angle, zero );
	dl->AddDragLine( p1, p2 ); 
	dl->AddDragLine( p2, p3 ); 
	dl->AddDragLine( p3, p4 ); 
	dl->AddDragLine( p4, p1 );
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		// CPT2 old code used to recalculate pin's x/y-position.  I'm going to try relying on the pin's preexisting x/y member values.  Note
		// that these values must be made relative to the part origin, however.
		// make X for each pin
		int d = pin->ps->top.size_h/2;
		int xRel = pin->x - x, yRel = pin->y - y;
		CPoint p1 (xRel - d, yRel - d);
		CPoint p2 (xRel + d, yRel - d);
		CPoint p3 (xRel + d, yRel + d);
		CPoint p4 (xRel - d, yRel + d);
		// draw X
		dl->AddDragLine( p1, p3 ); 
		dl->AddDragLine( p2, p4 );
	}

	// create drag lines for ratlines (OR OTHER SEGS --- CPT2) connected to pins
	if( bRatlines ) 
	{
		dl->MakeDragRatlineArray( 2*pins.GetSize(), 1 );
		// CPT2 Make a list of segs that have an endpoint (or 2) attached to one of the part's pins.  For efficiency, don't scan any net more than once
		m_pl->m_nlist->nets.SetUtility(0);
		carray<cseg2> segs;
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
		{
			if (!pin->net) continue;
			if (pin->net->utility) continue;
			pin->net->utility = 1;
			citer<cconnect2> ic (&pin->net->connects);
			for (cconnect2 *c = ic.First(); c; c = ic.Next())
			{
				if (c->head->pin && c->head->pin->part == this)
					segs.Add(c->head->postSeg);
				if (c->tail->pin && c->tail->pin->part == this)
					segs.Add(c->tail->preSeg);
			}
		}
		// Loop through the new list of segs that are attached to part's pins:
		citer<cseg2> is (&segs);
		for (cseg2 *s = is.First(); s; s = is.Next())
		{
			s->SetVisible(false);
			// CPT2 old code spent effort worrying about hiding vias on the end of the seg opposite from the pin.  
			// I'm going to try ignoring that issue...
			// As in the old code, I'll suppress drag-ratlines if the seg has both ends attached to the part's pins.  The old code
			// suppressed segs that belonged to single-seg connects with a loose end.  I'm going to try ignoring that issue too:
			CPoint vtxPt, pinPt;
			if (s->preVtx->pin && s->preVtx->pin->part==this)
				if (s->postVtx->pin && s->postVtx->pin->part==this)
					continue;												// Both ends of s are pins on "this"...
				else
					vtxPt = CPoint (s->postVtx->x, s->postVtx->y),
					pinPt = CPoint (s->preVtx->x - x, s->preVtx->y - y);	// NB coords relative to part
			else if (s->postVtx->pin && s->postVtx->pin->part==this)
				vtxPt = CPoint (s->preVtx->x, s->preVtx->y),
				pinPt = CPoint (s->postVtx->x - x, s->postVtx->y - y);		// NB coords relative to part
			dl->AddDragRatline( vtxPt, pinPt );
		}
	}
	
	int vert = 0;
	if( angle == 90 || angle == 270 )
		vert = 1;
	dl->StartDraggingArray( pDC, x, y, vert, LAY_SELECTION );
}

void cpart2::CancelDragging()
{
	// CPT2 derived from CPartList::CancelDraggingPart()
	// make part visible again
	SetVisible(true);

	// get any connecting segments and make visible
	CDisplayList *dl = doc->m_dlist;
	m_pl->m_nlist->nets.SetUtility(0);
	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		if (!pin->net || !pin->net->bVisible || pin->net->utility)
			continue;
		pin->net->utility = 1;
		citer<cconnect2> ic (&pin->net->connects);
		for (cconnect2 *c = ic.First(); c; c = ic.Next())
		{
			if (c->head->pin && c->head->pin->part==this)
				c->head->postSeg->SetVisible(true);
			if (c->tail->pin && c->tail->pin->part==this)
				c->tail->preSeg->SetVisible(true);
		}
	}
	dl->StopDragging();
}

void cpart2::MakeString( CString * str )
{
	// CPT2 derived from old CPartList::SetPartString().  Used when saving to file, etc.
	CString line;
	line.Format( "part: %s\n", ref_des );  
	*str = line;
	if( shape )
		line.Format( "  ref_text: %d %d %d %d %d %d %d\n", 
					m_ref->m_font_size, m_ref->m_stroke_width, m_ref->m_angle%360,
					m_ref->m_x, m_ref->m_y, m_ref->m_bShown,m_ref->m_layer );
	else
		line.Format( "  ref_text: \n" );
	str->Append( line );
	line.Format( "  package: \"%s\"\n", package );
	str->Append( line );
	if( value_text != "" ) 
	{
		if( shape )
			line.Format( "  value: \"%s\" %d %d %d %d %d %d %d\n", 
				value_text, m_value->m_font_size, m_value->m_stroke_width, m_value->m_angle%360,
				m_value->m_x, m_value->m_y, m_value->m_bShown, m_value->m_layer );
		else
			line.Format( "  value: \"%s\"\n", value_text );
		str->Append( line );
	}
	if( shape )
		line.Format( "  shape: \"%s\"\n", shape->m_name );
	else
		line.Format( "  shape: \n" );
	str->Append( line );
	line.Format( "  pos: %d %d %d %d %d\n", x, y, side, angle%360, glued );
	str->Append( line );

	line.Format( "\n" );
	str->Append( line );
}

void cpart2::GetDRCInfo()
{
	// CPT2 new, extracted from parts of the old CPartList::DRC().  Fill in the drc-related fields for this part and for all its pins.
	if (!shape)
		return;
	hole_flag = FALSE;
	min_x = INT_MAX;
	max_x = INT_MIN;
	min_y = INT_MAX;
	max_y = INT_MIN;
	// CPT2 NB the "layers" bitmask used to be the "or" of 1<<(layer-LAY_TOP_COPPER) for each layer touched by the part.  I've gotten rid of the
	// "-LAY_TOP_COPPER" business.  Since there are a max of 32 layers anyway, shouldn't be a problem with bit overflow.
	layers = 0;

	// iterate through copper graphics elements
	for( int igr = 0; igr < m_outline_stroke.GetSize(); igr++ )
	{
		stroke * stk = &m_outline_stroke[igr];
		if( stk->layer >= LAY_TOP_COPPER )
		{
			layers |= 1<<(stk->layer);
			min_x = min( min_x, stk->xi - stk->w/2 );
			max_x = max( max_x, stk->xi + stk->w/2 );
			min_x = min( min_x, stk->xf - stk->w/2 );
			max_x = max( max_x, stk->xf + stk->w/2 );
			min_y = min( min_y, stk->yi - stk->w/2 );
			max_y = max( max_y, stk->yi + stk->w/2 );
			min_y = min( min_y, stk->yf - stk->w/2 );
			max_y = max( max_y, stk->yf + stk->w/2 );
		}
	}

	citer<cpin2> ipin (&pins);
	for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
	{
		drc_pin * drp = &pin->drc;
		int x = pin->x, y = pin->y;

		drp->hole_size = 0;
		drp->min_x = INT_MAX;
		drp->max_x = INT_MIN;
		drp->min_y = INT_MAX;
		drp->max_y = INT_MIN;
		drp->max_r = INT_MIN;
		drp->layers = 0;
		int hole = pin->ps->hole_size;
		if (hole)
		{
			drp->hole_size = hole;
			drp->min_x = min( drp->min_x, x - hole/2 );
			drp->max_x = max( drp->max_x, x + hole/2 );
			drp->min_y = min( drp->min_y, y - hole/2 );
			drp->max_y = max( drp->max_y, y + hole/2 );
			drp->max_r = max( drp->max_r, hole/2 );
			min_x = min( min_x, x - hole/2 );
			max_x = max( max_x, x + hole/2 );
			min_y = min( min_y, y - hole/2 );
			max_y = max( max_y, y + hole/2 );
			hole_flag = TRUE;
		}
		int num_layers = LAY_TOP_COPPER + doc->m_num_copper_layers;
		for( int layer = LAY_TOP_COPPER; layer < num_layers; layer++ )
		{
			// Check pads on each layer.
			int wid, len, r, type, hole, connect;
			BOOL bPad = pin->GetDrawInfo( layer, 0, 0, 0, 0,
				&type, &wid, &len, &r, &hole, &connect );
			if (!bPad || type == PAD_NONE )
				continue;
			drp->min_x = min( drp->min_x, x - len/2 );
			drp->max_x = max( drp->max_x, x + len/2 );
			drp->min_y = min( drp->min_y, y - wid/2 );
			drp->max_y = max( drp->max_y, y + wid/2 );
			drp->max_r = max( drp->max_r, Distance( 0, 0, len/2, wid/2 ) );
			min_x = min( min_x, x - len/2 );
			max_x = max( max_x, x + len/2 );
			min_y = min( min_y, y - wid/2 );
			max_y = max( max_y, y + wid/2 );
			drp->layers |= 1<<layer;
			layers |= 1<<layer;
		}
	}
}


/**********************************************************************************************/
/*  RELATED TO cpolyline/carea2/csmcutout                                                      */
/**********************************************************************************************/

ccorner::ccorner(ccontour *_contour, int _x, int _y)
	: cpcb_item (_contour->doc)
{
	contour = _contour;
	if (contour->corners.IsEmpty())
		contour->head = contour->tail = this;
	contour->corners.Add(this);
	x = _x; y = _y; 
	preSide = postSide = NULL;
}

ccorner::ccorner(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	contour = NULL;
	preSide = postSide = NULL;
}

bool ccorner::IsValid() 
{
	if (!contour->poly->IsValid()) return false;
	if (!contour->IsValid()) return false;
	return contour->corners.Contains(this);
}

bool ccorner::IsAreaCorner() { return contour->poly->IsArea(); }
bool ccorner::IsBoardCorner() { return contour->poly->IsBoard(); }
bool ccorner::IsSmCorner() { return contour->poly->IsSmCutout(); }
bool ccorner::IsOutlineCorner() { return contour->poly->IsOutline(); }
cnet2 *ccorner::GetNet() { return contour->GetNet(); }
int ccorner::GetLayer() { return contour->poly->m_layer; }
cpolyline *ccorner::GetPolyline() { return contour->poly; }


int ccorner::GetTypeBit() 
{														// Later:  put in .cpp file.  There wouldn't be this nonsense in Java...
	if (contour->poly->IsArea()) return bitAreaCorner;
	if (contour->poly->IsSmCutout()) return bitSmCorner;
	if (contour->poly->IsBoard()) return bitBoardCorner;
	if (contour->poly->IsOutline()) return bitOutlineCorner;
	return bitOther;
}

bool ccorner::IsOnCutout()
	// Return true if this side lies on a secondary contour within the polyline
	{ return contour->poly->main!=contour; }

void ccorner::Remove() 
{
	// CPT2 derived loosely from old CPolyLine::DeleteCorner
	cpolyline *poly = GetPolyline();
	poly->MustRedraw();
	contour->corners.Remove(this);
	if (contour->NumCorners() < 2)
		contour->Remove();
	else if (contour->NumCorners() == 2 && contour->head==contour->tail)
		contour->Remove();
	else if (!preSide)
	{
		// First corner of open contour
		contour->sides.Remove(postSide);
		contour->head = postSide->postCorner;
		contour->head->preSide = NULL;
	}
	else if (!postSide)
	{
		// Final corner of open contour
		contour->sides.Remove(preSide);
		contour->tail = preSide->preCorner;
		contour->tail->postSide = NULL;
	}
	else 
	{
		// Normal middle corner
		ccorner *next = postSide->postCorner;
		if (this == contour->head)
			contour->head = contour->tail = next;
		contour->sides.Remove(postSide);
		preSide->postCorner = next;
		next->preSide = preSide;
	}
}

void ccorner::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}

bool ccorner::Move( int _x, int _y, BOOL bEnforceCircularArcs )
{
	// move corner of polyline
	// if bEnforceCircularArcs == TRUE, convert adjacent sides to STRAIGHT if angle not
	// a multiple of 45 degrees and return TRUE, otherwise return FALSE
	// CPT2 derived from CPolyLine::MoveCorner().
	cpolyline *poly = contour->poly;
	poly->MustRedraw();
	x = _x;
	y = _y;
	BOOL bReturn = FALSE;
	if( bEnforceCircularArcs )
	{
		ccorner *prev = preSide? preSide->preCorner: NULL;
		ccorner *next = postSide? postSide->postCorner: NULL;
		if (prev && abs(prev->x - x) != abs(prev->y - y))
			preSide->m_style = STRAIGHT,
			bReturn = TRUE;
		if (next && abs(next->x - x) != abs(next->y - y))
			postSide->m_style = STRAIGHT,
			bReturn = TRUE;
	}
	return bReturn;
}

void ccorner::StartDragging( CDC * pDC, int x, int y, int crosshair )
{
	CDisplayList *dl = doc->m_dlist;
	ASSERT(dl);
	dl->CancelHighlight();
	// Hide adjacent sides (if present) and area hatching
	if (preSide)
		dl->Set_visible(preSide->dl_el, 0);
	if (postSide)
		dl->Set_visible(postSide->dl_el, 0);
	cpolyline *poly = GetPolyline();
	for( int ih=0; ih < poly->m_nhatch; ih++ )
		dl->Set_visible( poly->dl_hatch[ih], 0 );

	// see if corner is the first or last corner of an open contour
	if (!preSide || !postSide)
	{
		int style, xi, yi;
		cside *side;
		if (!preSide)
		{
			style = postSide->m_style;
			if (style==ARC_CW) 
				style = ARC_CCW;
			else if (style==ARC_CCW) 
				style = ARC_CW;								// Reverse arc since we are drawing from corner 1 to 0
			xi = postSide->postCorner->x;
			yi = postSide->postCorner->y;
			side = postSide;
		}
		else
			style = preSide->m_style,
			xi = preSide->preCorner->x,
			yi = preSide->preCorner->y,
			side = preSide;
		dl->StartDraggingArc( pDC, style, this->x, this->y, xi, yi, LAY_SELECTION, 1, crosshair );
	}
	else
	{
		int style1, style2;
		if( preSide->m_style == STRAIGHT )
			style1 = DSS_STRAIGHT;
		else if( preSide->m_style == ARC_CW )
			style1 = DSS_ARC_CW;
		else
			style1 = DSS_ARC_CCW;
		if( postSide->m_style == STRAIGHT )
			style2 = DSS_STRAIGHT;
		else if( postSide->m_style == ARC_CW )
			style2 = DSS_ARC_CW;
		else
			style2 = DSS_ARC_CCW;
		int xi = preSide->preCorner->x;
		int yi = preSide->preCorner->y;
		int xf = postSide->postCorner->x;
		int yf = postSide->postCorner->y;
		dl->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
			LAY_SELECTION, LAY_SELECTION, 1, 1, style1, style2, 0, 0, 0, 0, crosshair );
	}
}

void ccorner::CancelDragging()
{
	// CPT2:  Derived from CPolyLine::CancelDraggingToMoveCorner.  Stop dragging, make sides and hatching visible again
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );
	dl->StopDragging();
	if (preSide)
		dl->Set_visible(preSide->dl_el, 1);
	if (postSide)
		dl->Set_visible(postSide->dl_el, 1);
	cpolyline *poly = GetPolyline();
	for( int ih=0; ih < poly->m_nhatch; ih++ )
		dl->Set_visible( poly->dl_hatch[ih], 1 );
}


cside::cside(ccontour *_contour, int _style)
	: cpcb_item(_contour->doc)
{ 
	contour = _contour;
	contour->sides.Add(this);
	m_style = _style;
	preCorner = postCorner = NULL;
}

cside::cside(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	contour = NULL;
	preCorner = postCorner = NULL;
}
bool cside::IsValid() 
{
	if (!contour->poly->IsValid()) return false;
	if (!contour->IsValid()) return false;
	return contour->sides.Contains(this);
}

bool cside::IsAreaSide() { return contour->poly->IsArea(); }
bool cside::IsBoardSide() { return contour->poly->IsBoard(); }
bool cside::IsSmSide() { return contour->poly->IsSmCutout(); }
bool cside::IsOutlineSide() { return contour->poly->IsOutline(); }
int cside::GetLayer() { return contour->poly->m_layer; }

int cside::GetTypeBit() 
{
	if (contour->poly->IsArea()) return bitAreaSide;
	if (contour->poly->IsSmCutout()) return bitSmSide;
	if (contour->poly->IsBoard()) return bitBoardSide;
	if (contour->poly->IsOutline()) return bitOutlineSide;
	return bitOther;
}

cnet2 *cside::GetNet() { return contour->GetNet(); }

cpolyline *cside::GetPolyline() { return contour->poly; }

bool cside::IsOnCutout()
	// Return true if this side lies on a secondary contour within the polyline
	{ return contour->poly->main!=contour; }

void cside::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	int s;
	if( m_style == CPolyLine::STRAIGHT )
		s = DL_LINE;
	else if( m_style == CPolyLine::ARC_CW )
		s = DL_ARC_CW;
	else if( m_style == CPolyLine::ARC_CCW )
		s = DL_ARC_CCW;
	dl->Highlight( s, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}

void cside::InsertCorner(int x, int y)
{
	// CPT2 new.  Add an intermediate corner into this side.  "this" gets reused as the second of the 2 half-sides, and the styles of both are made straight.
	ccorner *c = new ccorner(contour, x, y);
	cside *s = new cside(contour, STRAIGHT);
	m_style = STRAIGHT;
	contour->AppendSideAndCorner(s, c, preCorner);
}

bool cside::Remove( carray<cpolyline> *arr ) 
{
	// CPT2 new.  Remove this side from its parent contour (which must be the main contour in the polyline).  This can easily result in a whole 
	// new polyline getting created (by means of virtual function cpolyline::CreateCompatible()).  Said new polyline gets added to "arr".  It can
	// also result in the whole original polyline disappearing, in which case it is removed from "arr".
	cpolyline *poly = GetPolyline();
	if (poly->contours.GetSize() > 1) return false;
	contour->sides.Remove(this);
	preCorner->postSide = postCorner->preSide = NULL;
	if (contour->sides.IsEmpty())
		arr->Remove(poly);
	else if (contour->head == contour->tail) 
		// Break a closed contour.
		contour->head = postCorner,
		contour->tail = preCorner;
	else if (!preCorner->preSide)
		// Eliminate 1st seg of open contour.
		contour->head = postCorner,
		contour->corners.Remove(preCorner);
	else if (!postCorner->postSide)
		// Eliminate last seg of open contour.
		contour->tail = preCorner,
		contour->corners.Remove(postCorner);
	else
	{
		// Break!
		cpolyline *poly2 = poly->CreateCompatible();
		arr->Add(poly2);
		ccontour *ctr2 = new ccontour(poly2, true);
		ctr2->head = postCorner;
		ctr2->tail = contour->tail;
		contour->tail = preCorner;
		for (ccorner *c = postCorner; 1; c = c->postSide->postCorner)
		{
			contour->corners.Remove(c);
			ctr2->corners.Add(c);
			c->contour = ctr2;
			if (!c->postSide) break;
			contour->sides.Remove(c->postSide);
			ctr2->sides.Add(c->postSide);
			c->postSide->contour = ctr2;
		}
	}
	return true;
}

void cside::StartDraggingNewCorner( CDC * pDC, int x, int y, int crosshair )
{
	// CPT2, derived from CPolyLine::StartDraggingToInsertCorner
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );

	int xi = preCorner->x, yi = preCorner->y;
	int xf = postCorner->x, yf = postCorner->y;
	dl->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
		LAY_SELECTION, LAY_SELECTION, 1, 1, DSS_STRAIGHT, DSS_STRAIGHT,
		0, 0, 0, 0, crosshair );
	dl->CancelHighlight();
	dl->Set_visible( dl_el, 0 );
	cpolyline *p = GetPolyline();
	for( int ih=0; ih < p->m_nhatch; ih++ )
		dl->Set_visible( p->dl_hatch[ih], 0 );
}

//
void cside::CancelDraggingNewCorner()
{
	// CPT2 derived from CPolyLine::CancelDraggingToInsertCorner
	CDisplayList *dl = doc->m_dlist;
	ASSERT( dl );

	dl->StopDragging();
	dl->Set_visible( dl_el, 1 );
	cpolyline *p = GetPolyline();
	for( int ih=0; ih < p->m_nhatch; ih++ )
		dl->Set_visible( p->dl_hatch[ih], 1 );
}





ccontour::ccontour(cpolyline *_poly, bool bMain)
	: cpcb_item (_poly->doc)
{
	poly = _poly;
	if (bMain) 
		poly->main = this;
	poly->contours.Add(this);
	head = tail = NULL;
}

ccontour::ccontour(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	poly = NULL;
	head = tail = NULL;
}

ccontour::ccontour(cpolyline *_poly, ccontour *src)
	: cpcb_item (_poly->doc)
{
	// Create a new contour with the same points/sides as "src", but belonging to poly "_poly"
	poly = _poly;
	poly->contours.Add(this);
	corners.RemoveAll();
	sides.RemoveAll();
	head = tail = NULL;
	if (src->corners.GetSize()==0) 
		return;

	// Loop thru src's corners and sides in geometrical order.  Must take care since src's head and tail may be the same.
	cside *preSide = NULL;
	for (ccorner *c = src->head; 1; c = c->postSide->postCorner)
	{
		ccorner *c2 = new ccorner(this, c->x, c->y);
		corners.Add(c2);
		c2->preSide = preSide;
		if (preSide)
			preSide->postCorner = c2;
		if (c==src->head) 
			head = c2;
		cside *s = c->postSide;
		if (!s) 
			{ tail = c2; break; }
		cside *s2 = new cside(this, s->m_style);
		sides.Add(s2);
		c2->postSide = s2;
		s2->preCorner = c2;
		if (s->postCorner == src->head)
		{ 
			tail = head; 
			tail->preSide = s2; s2->postCorner = tail; 
			break; 
		}
		preSide = s2;
	}
}

bool ccontour::IsValid() 
	{ return poly->contours.Contains(this); }

cnet2 *ccontour::GetNet() { return poly->GetNet(); }

int ccontour::GetLayer() { return poly->m_layer; }

void ccontour::SaveUndoInfo()
{
	doc->m_undo_items.Add( new cucontour(this) );
	citer<ccorner> ic (&corners);
	for (ccorner *c = ic.First(); c; c = ic.Next())
		doc->m_undo_items.Add( new cucorner(c) );
	citer<cside> is (&sides);
	for (cside *s = is.First(); s; s = is.Next())
		doc->m_undo_items.Add( new cuside(s) );
}

void ccontour::AppendSideAndCorner( cside *s, ccorner *c, ccorner *after )
{
	// Append s+c into this connection after corner "after".  Assumes that s+c were constructed so that they point to "this"
	// Very similar to cconnect2::AppendSegAndVertex.
	if (poly)
		poly->MustRedraw();
	corners.Add(c);
	sides.Add(s);
	cside *nextSide = after->postSide;
	after->postSide = s;
	s->preCorner = after;
	s->postCorner = c;
	c->preSide = s;
	c->postSide = nextSide;
	if (nextSide)
		nextSide->preCorner = c;
	else 
		tail = c;
}

void ccontour::AppendCorner( int x, int y, int style )
{
	// CPT2 convenience method, basically a wrapper around AppendSideAndCorner()
	if (poly)
		poly->MustRedraw();
	bool bWasEmpty = corners.IsEmpty();
	ccorner *c = new ccorner(this, x, y);
	if (bWasEmpty)
		// First corner only. ccorner::ccorner has now setup this->head/tail, so we're done
		return;
	cside *s = new cside(this, style);
	AppendSideAndCorner(s, c, tail);
}

void ccontour::Close(int style)
{
	if (head==tail) return;
	if (poly)
		poly->MustRedraw();
	cside *s = new cside(this, style);
	s->preCorner = tail;
	s->postCorner = head;
	tail->postSide = s;
	head->preSide = s;
	tail = head;
}

void ccontour::Unclose()
{
	if (head!=tail || sides.GetSize()<2) return;
	if (poly)
		poly->MustRedraw();
	tail = tail->preSide->preCorner;
	sides.Remove(tail->postSide);
	tail->postSide = NULL;
	head->preSide = NULL;
}

CRect ccontour::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	citer<ccorner> ic (&corners);
	for (ccorner *c = ic.First(); c; c = ic.Next())
	{
		r.left = min( r.left, c->x );
		r.right = max( r.right, c->x );
		r.bottom = min( r.bottom, c->y );
		r.top = max( r.top, c->y );
	}
	return r;
}

void ccontour::SetPoly( cpolyline *_poly )
{
	// CPT2 new.  Reassign this to a different polyline as a secondary contour (used when creating new cutouts).    
	if (poly)
		poly->MustRedraw(),
		poly->contours.Remove(this);
	poly = _poly;
	poly->contours.Add(this);
	poly->MustRedraw();
}

void ccontour::Remove()
{
	poly->MustRedraw();
	if (this ==  poly->main)
		poly->Remove();
	else
		poly->contours.Remove(this);
}


cpolyline::cpolyline(CFreePcbDoc *_doc)
	: cpcb_item (_doc)
{ 
	main = NULL;
	m_layer = m_w = m_sel_box = m_hatch = m_nhatch = 0;
	m_gpc_poly = new gpc_polygon;
	m_gpc_poly->num_contours = 0;
	m_php_poly = NULL;			// CPT2 TODO for now. 
}

cpolyline::cpolyline(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{
	main = NULL;
	m_gpc_poly = new gpc_polygon;
	m_gpc_poly->num_contours = 0;
}

cpolyline::cpolyline(cpolyline *src, bool bCopyContours)
	: cpcb_item (src->doc)
{
	main = NULL;
	m_layer = src->m_layer;
	m_w = src->m_w;
	m_sel_box = src->m_sel_box;
	m_hatch = src->m_hatch;
	m_nhatch = src->m_nhatch;					// CPT2.  I guess...
	m_gpc_poly = NULL, m_php_poly = NULL;
	if (!src->main || !bCopyContours)
		return;
	citer<ccontour> ictr (&src->contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		ccontour *ctr2 = new ccontour(this, ctr);
		contours.Add(ctr2);
		if (ctr==src->main) 
			main = ctr2;
	}
}

void cpolyline::MarkConstituents(int util)
{
	// Mark the utility flags on this polyline and on its constituent contours, sides, and corners.
	utility = util;
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		ctr->utility = util;
		ctr->sides.SetUtility(util);
		ctr->corners.SetUtility(util);
	}
}

CRect cpolyline::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CRect r2 = ctr->GetCornerBounds();
		r.left = min(r.left, r2.left);
		r.right = max(r.right, r2.right);
		r.bottom = min(r.bottom, r2.bottom);
		r.top = max(r.top, r2.top);
	}
	return r;
}

CRect cpolyline::GetBounds()
{
	CRect r = GetCornerBounds();
	r.left -= m_w/2;
	r.right += m_w/2;
	r.bottom -= m_w/2;
	r.top += m_w/2;
	return r;
}

void cpolyline::Offset(int dx, int dy) 
{
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		citer<ccorner> ic (&ctr->corners);
		for (ccorner *c = ic.First(); c; c = ic.Next())
			c->x += dx,
			c->y += dy;
	}
}

int cpolyline::NumCorners() 
{
	citer<ccontour> ictr (&contours);
	int ret = 0;
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ret += ctr->corners.GetSize();
	return ret;
}

int cpolyline::NumSides() 
{
	citer<ccontour> ictr (&contours);
	int ret = 0;
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ret += ctr->sides.GetSize();
	return ret;
}

bool cpolyline::SetClosed(bool bClose)
{
	if (contours.GetSize()>1) 
		return false;
	if (bClose)
		if (IsClosed()) 
			return false;
		else
			main->Close(STRAIGHT);
	else
		if (!IsClosed())
			return false;
		else
			main->Unclose();
	return true;
}

void cpolyline::Copy(cpolyline *src)
{
	// CPT2 new.  Give this polyline copies of the contours found in "src".  Used when copying to the clipboard
	contours.RemoveAll();
	citer<ccontour> ictr (&src->contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		if (ctr->NumCorners()<2) continue;
		ccontour *ctr2 = new ccontour(this, ctr==src->main);
		ccorner *c = ctr->head;
		ctr2->AppendCorner( c->x, c->y );
		for (c = c->postSide->postCorner; c!=ctr->tail; c = c->postSide->postCorner)
			ctr2->AppendCorner( c->x, c->y, c->preSide->m_style );
		if (ctr->head == ctr->tail)
			ctr2->Close( ctr->head->preSide->m_style );
	}
}

void cpolyline::AddSidesTo(carray<cpcb_item> *arr)
{
	// CPT2 new.  Append the sides within this polyline to "arr".
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		// Typecast is surely safe.
		arr->Add((carray<cpcb_item>*) &ctr->sides);
}

bool cpolyline::TestPointInside(int x, int y) 
{
	enum { MAXPTS = 100 };
	ASSERT( IsClosed() );

	// define line passing through (x,y), with slope = 2/3;
	// get intersection points
	double xx[MAXPTS], yy[MAXPTS];
	double slope = (double)2.0/3.0;
	double a = y - slope*x;
	int nloops = 0;
	int npts;
	// make this a loop so if my homebrew algorithm screws up, we try it again
	do
	{
		// now find all intersection points of line with polyline sides
		npts = 0;
		citer<ccontour> ic (&contours);
		for (ccontour *c = ic.First(); c; c = ic.Next())
		{
			cside *s0 = c->head->postSide, *s = s0;
			do {
				double x, y, x2, y2;
				int ok = FindLineSegmentIntersection( a, slope, 
					s->preCorner->x, s->preCorner->y,
					s->postCorner->x, s->postCorner->y,
					s->m_style,
					&x, &y, &x2, &y2 );
				if( ok )
				{
					xx[npts] = (int)x;
					yy[npts] = (int)y;
					npts++;
					ASSERT( npts<MAXPTS );	// overflow
				}
				if( ok == 2 )
				{
					xx[npts] = (int)x2;
					yy[npts] = (int)y2;
					npts++;
					ASSERT( npts<MAXPTS );	// overflow
				}
				s = s->postCorner->postSide;
			}
			while (s!=s0);
		}
		nloops++;
		a += PCBU_PER_MIL/100;
	} while( npts%2 != 0 && nloops < 3 );
	ASSERT( npts%2==0 );	// odd number of intersection points, error

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
	{
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	}
	if( ncount%2 )
		return TRUE;
	else
		return FALSE;
}

void cpolyline::GetSidesInRect( CRect *r, carray<cpcb_item> *arr)
{
	// CPT2 new, helper for CFreePcbView::SelectItemsInRect().
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		citer<cside> is (&ctr->sides);
		for (cside *s = is.First(); s; s = is.Next())
		{
			CPoint pre (s->preCorner->x, s->preCorner->y);
			CPoint post (s->postCorner->x, s->postCorner->y);
			if (r->PtInRect(pre) && r->PtInRect(post))
				arr->Add(s);
		}
	}
}

bool cpolyline::IsClosed()
{
	// CPT2.  Trying a new definition of closedness.  Every contour within the polyline must have head equal to tail, and further no contour
	// can consist of one corner.  This will work decently when user is dragging a new cutout contour and it is still incomplete --- Hatch() will
	// then consider the polyline open and bail out.
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		if (ctr->head != ctr->tail || ctr->NumCorners()<2)
			return false;
	return true;
}

// Test a polyline for self-intersection.
// Returns:
//	-1 if arcs intersect other sides
//	 0 if no intersecting sides
//	 1 if intersecting sides, but no intersecting arcs
// Also sets utility flag of area with return value
//
int cpolyline::TestPolygon()
{	
	ASSERT(IsClosed());

	// first, check for sides intersecting other sides, especially arcs 
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	// make bounding rect for each contour.  CPT2 for each contour, store an index into "cr" temporarily in the utility field.
	CArray<CRect> cr;
	int i=0;
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		CRect r = ctr->GetCornerBounds();
		cr.Add(r);
		ctr->utility = i++;
	}
	for (ccontour *ctr1 = ictr.First(); ctr1; ctr1 = ictr.Next())
	{
		citer<cside> is1 (&ctr1->sides);
		for (cside *s1 = is1.First(); s1; s1 = is1.Next())
		{
			CRect r1 = cr[ctr1->utility];
			cside *s1prev = s1->preCorner->preSide;
			cside *s1next = s1->postCorner->postSide;
			int style1 = s1->m_style;
			int x1i = s1->preCorner->x, y1i = s1->preCorner->y;
			int x1f = s1->postCorner->x, y1f = s1->postCorner->y;
			citer<ccontour> ictr2 (&contours, ctr1);
			for (ccontour *ctr2 = ictr2.Next(); ctr2; ctr2 = ictr2.Next())
			{
				CRect r2 = cr[ctr2->utility];
				if( r1.left > r2.right
					|| r1.bottom > r2.top
					|| r2.left > r1.right
					|| r2.bottom > r1.top )
						// rectangles don't overlap, do nothing
						continue;
				citer<cside> is2 (&ctr2->sides);
				for (cside *s2 = is2.First(); s2; s2 = is2.Next())
				{
					if (s1==s2 || s1prev==s2 || s1next==s2) continue;
					int style2 = s2->m_style;
					int x2i = s2->preCorner->x, y2i = s2->preCorner->y;
					int x2f = s2->postCorner->x, y2f = s2->postCorner->y;
					int ret = FindSegmentIntersections( x1i, y1i, x1f, y1f, style1, x2i, y2i, x2f, y2f, style2 );
					if( !ret ) continue;
					// intersection between non-adjacent sides
					bInt = TRUE;
					if( style1 != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
					{
						bArcInt = TRUE;
						goto finish;
					}
				}
			}
		}
	}

	finish:
	if( bArcInt )
		utility = -1;
	else if( bInt )
		utility = 1;
	else 
		utility = 0;
	return utility;
}

void cpolyline::Hatch()
{
	if( m_hatch == NO_HATCH )
	{
		m_nhatch = 0;
		return;
	}

	CDisplayList *dl = doc->m_dlist;
	if (!dl || !IsClosed()) return;
	enum {
		MAXPTS = 100,
		MAXLINES = 1000
	};
	dl_hatch.SetSize( MAXLINES, MAXLINES );

	int xx[MAXPTS], yy[MAXPTS];
	CRect r = GetCornerBounds();
	int slope_flag = 1 - 2*(m_layer%2);	// 1 or -1
	double slope = 0.707106*slope_flag;
	int spacing;
	if( m_hatch == DIAGONAL_EDGE )
		spacing = 10*PCBU_PER_MIL;
	else
		spacing = 50*PCBU_PER_MIL;
	int max_a, min_a;
	if( slope_flag == 1 )
	{
		max_a = (int)(r.top - slope*r.left);
		min_a = (int)(r.bottom - slope*r.right);
	}
	else
	{
		max_a = (int)(r.top - slope*r.right);
		min_a = (int)(r.bottom - slope*r.left);
	}
	min_a = (min_a/spacing)*spacing;
	int offset;
	if( m_layer < (LAY_TOP_COPPER+2) )
		offset = 0;
	else if( m_layer < (LAY_TOP_COPPER+4) )
		offset = spacing/2;
	else if( m_layer < (LAY_TOP_COPPER+6) )
		offset = spacing/4;
	else if( m_layer < (LAY_TOP_COPPER+8) )
		offset = 3*spacing/4;
	else if( m_layer < (LAY_TOP_COPPER+10) )
		offset = 1*spacing/8;
	else if( m_layer < (LAY_TOP_COPPER+12) )
		offset = 3*spacing/8;
	else if( m_layer < (LAY_TOP_COPPER+14) )
		offset = 5*spacing/8;
	else if( m_layer < (LAY_TOP_COPPER+16) )
		offset = 7*spacing/8;
	else
		ASSERT(0);
	min_a += offset;

	// now calculate and draw hatch lines
	int nhatch = 0;
	// loop through hatch lines
	for( int a=min_a; a<max_a; a+=spacing )
	{
		// get intersection points for this hatch line
		int nloops = 0;
		int npts;
		// make this a loop in case my homebrew hatching algorithm screws up
		do
		{
			npts = 0;
			citer<ccontour> ictr (&contours);
			for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
			{
				citer<cside> is (&ctr->sides);
				for (cside *s = is.First(); s; s = is.Next())
				{
					double x, y, x2, y2;
					int ok = FindLineSegmentIntersection( a, slope, 
								s->preCorner->x, s->preCorner->y,
								s->postCorner->x, s->postCorner->y,
								s->m_style,
								&x, &y, &x2, &y2 );
					if( ok )
					{
						xx[npts] = (int)x;
						yy[npts] = (int)y;
						npts++;
						ASSERT( npts<MAXPTS );	// overflow
					}
					if( ok == 2 )
					{
						xx[npts] = (int)x2;
						yy[npts] = (int)y2;
						npts++;
						ASSERT( npts<MAXPTS );	// overflow
					}
				}
			}
			nloops++;
			a += PCBU_PER_MIL/100;
		} while( npts%2 != 0 && nloops < 3 );
		ASSERT( npts%2==0 );	// odd number of intersection points, error

		// sort points in order of descending x (if more than 2)
		if( npts>2 )
		{
			for( int istart=0; istart<(npts-1); istart++ )
			{
				int max_x = INT_MIN;
				int imax;
				for( int i=istart; i<npts; i++ )
				{
					if( xx[i] > max_x )
					{
						max_x = xx[i];
						imax = i;
					}
				}
				int temp = xx[istart];
				xx[istart] = xx[imax];
				xx[imax] = temp;
				temp = yy[istart];
				yy[istart] = yy[imax];
				yy[imax] = temp;
			}
		}

		// draw lines
		for( int ip=0; ip<npts; ip+=2 )
		{
			double dx = xx[ip+1] - xx[ip];
			if( m_hatch == DIAGONAL_FULL || fabs(dx) < 40*NM_PER_MIL )
			{
				dl_element * el = dl->Add( this, dl_element::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0,
					xx[ip], yy[ip], xx[ip+1], yy[ip+1], 0, 0 );
				dl_hatch.SetAtGrow(nhatch, el);
				nhatch++;
			}
			else
			{
				double dy = yy[ip+1] - yy[ip];	
				double slope = dy/dx;
				if( dx > 0 )
					dx = 20*NM_PER_MIL;
				else
					dx = -20*NM_PER_MIL;
				double x1 = xx[ip] + dx;
				double x2 = xx[ip+1] - dx;
				double y1 = yy[ip] + dx*slope;
				double y2 = yy[ip+1] - dx*slope;
				dl_element * el = dl->Add( this, dl_element::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0,
					xx[ip], yy[ip], x1, y1, 0, 0 );
				dl_hatch.SetAtGrow(nhatch, el);
				el = dl->Add( this, dl_element::DL_HATCH, m_layer, DL_LINE, 1, 0, 0, 0, 
					xx[ip+1], yy[ip+1], x2, y2, 0, 0 );
				dl_hatch.SetAtGrow(nhatch+1, el);
				nhatch += 2;
			}
		}
	}
	m_nhatch = nhatch;
	dl_hatch.SetSize( m_nhatch );
}


int cpolyline::Draw()
{
	int i_start_contour = 0;
	CDisplayList *dl = doc->m_dlist;
	if (!dl) 
		return NO_DLIST;
	if (bDrawn) 
		return ALREADY_DRAWN;

	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		// Draw corner selectors:
		if( m_sel_box )
		{
			citer<ccorner> ic (&ctr->corners);
			for (ccorner *c = ic.First(); c; c = ic.Next())
			{
				int x = c->x, y = c->y;
				c->dl_sel = dl->AddSelector( c, m_layer, DL_HOLLOW_RECT, 
					1, 0, 0, x-m_sel_box, y-m_sel_box, 
					x+m_sel_box, y+m_sel_box, 0, 0 );
			}
		}

		// Draw sides and side selectors
		citer<cside> is (&ctr->sides);
		int side_sel_w = IsOutline()? m_w: m_w*4;						// CPT2 For areas, smcutouts, and board-outlines, we want the side selectors to be 
																		// wider for improved visibility.  But not with fp-editor outlines...
		for (cside *s = is.First(); s; s = is.Next())
		{
			int xi = s->preCorner->x, yi = s->preCorner->y;
			int xf = s->postCorner->x, yf = s->postCorner->y;
			if( xi == xf || yi == yf )
				// if endpoints not angled, make side STRAIGHT
				s->m_style = STRAIGHT;
			int g_type = DL_LINE;
			if( s->m_style == STRAIGHT )
				g_type = DL_LINE;
			else if( s->m_style == ARC_CW )
				g_type = DL_ARC_CW;
			else if( s->m_style == ARC_CCW )
				g_type = DL_ARC_CCW;
			s->dl_el = dl->AddMain( s, m_layer, g_type, 
				1, m_w, 0, 0, xi, yi, xf, yf, 0, 0 );	
			if( m_sel_box )
				s->dl_sel = dl->AddSelector( s, m_layer, g_type, 
					1, side_sel_w, 0, xi, yi, xf, yf, 0, 0 );
		}
	}

	if( m_hatch )
		Hatch();
	bDrawn = true;
	return NOERR;
}

// undraw polyline by removing all graphic elements from display list
//
void cpolyline::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl || !IsDrawn()) return;

	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		citer<cside> is (&ctr->sides);
		for (cside *s = is.First(); s; s = is.Next())
		{
			dl->Remove( s->dl_el );
			dl->Remove( s->dl_sel );
			s->dl_el = s->dl_sel = NULL;
		}
		citer<ccorner> ic (&ctr->corners);
		for (ccorner *c = ic.First(); c; c = ic.Next())
		{
			dl->Remove( c->dl_sel );
			c->dl_sel = NULL;
		}
		for( int i=0; i<dl_hatch.GetSize(); i++ )
			dl->Remove( dl_hatch[i] );

		// remove pointers
		dl_hatch.RemoveAll();
		m_nhatch = 0;
	}

	bDrawn = false;
}

void cpolyline::Highlight()
{
	citer<ccontour> ic (&contours);
	for (ccontour *c = ic.First(); c; c = ic.Next())
		c->Highlight();
}

void cpolyline::SetVisible( BOOL visible ) 
{	
	CDisplayList *dl = doc->m_dlist;
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		citer<cside> is (&ctr->sides);
		for (cside *s = is.First(); s; s = is.Next())
			dl->Set_visible( s->dl_el, visible ); 
	}
	for( int ih=0; ih<m_nhatch; ih++ )
		dl->Set_visible( dl_hatch[ih], visible ); 
} 

#define pi  3.14159265359

void TestGpc()
{
	// CPT2 TODO.  For testing only.
	gpc_polygon * gpc = new gpc_polygon;
	gpc->num_contours = 0;
	gpc->hole = NULL;
	gpc->contour = NULL;
	gpc_polygon * gpc2 = new gpc_polygon;
	gpc2->num_contours = 0;
	gpc2->hole = NULL;
	gpc2->contour = NULL;
	gpc_vertex_list * g_v_list = new gpc_vertex_list;
	g_v_list->vertex = (gpc_vertex*)calloc( sizeof(gpc_vertex), 6 );
	g_v_list->num_vertices = 6;
	static int x[] = { 100, 200, 100, 100, 0,   100 };
	static int y[] = { 100, 0,   0,   100, 200, 200 };
	for (int i=0; i<6; i++)
		g_v_list->vertex[i].x = x[i],
		g_v_list->vertex[i].y = y[i];
	// add vertex_list to gpc
	gpc_add_contour( gpc, g_v_list, 0 );
	// now clip m_gpc_poly with gpc, put new poly into result
	gpc_polygon * result = new gpc_polygon;
	gpc_polygon_clip( GPC_UNION, gpc2, gpc, result );
}

void cpolyline::MakeGpcPoly( ccontour *ctr0, CArray<CArc> * arc_array )
{
	// make a gpc_polygon for a closed polyline contour
	// approximates arcs with multiple straight-line segments
	// if ctr0 == NULL, make polygon with all contours,
	// combining intersecting contours if possible.
	// Otherwise just do the one contour.  (Kind of an ungainly system, but hey...)
	// returns data on arcs in arc_array
	if( m_gpc_poly->num_contours )
		FreeGpcPoly();
	if (ctr0 && ctr0->head!=ctr0->tail)
		return;														// Open contour, error (indicated by the absence of an allocated gpc-poly)
	if (!ctr0 && !IsClosed())
		return;														// Main contour is open, error

	// initialize m_gpc_poly
	m_gpc_poly->num_contours = 0;
	m_gpc_poly->hole = NULL;
	m_gpc_poly->contour = NULL;
	int n_arcs = 0;
	if( arc_array )
		arc_array->SetSize(0);
	int iarc = 0;

	// Create a temporary carray of contours in which the external (main) contour is guaranteed to come first:
	carray<ccontour> tmp;
	tmp.Add(main);
	citer<ccontour> ictr0 (&contours);
	for (ccontour *ctr = ictr0.First(); ctr; ctr = ictr0.Next())
		if (ctr!=main)
			tmp.Add(ctr);
	// Now go thru the contours in the preferred order.
	citer<ccontour> ictr (&tmp);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
	{
		if (ctr0 && ctr!=ctr0) 
			continue;
		// make gpc_polygon for this contour
		gpc_polygon * gpc = new gpc_polygon;
		gpc->num_contours = 0;
		gpc->hole = NULL;
		gpc->contour = NULL;

		ASSERT( ctr->sides.GetSize()>0 );
		// first, calculate number of vertices in contour (including extra vertices inserted into arcs)
		int n_vertices = 0;
		citer<cside> is (&ctr->sides);
		for (cside *s = is.First(); s; s = is.Next())
		{
			if (s->m_style == STRAIGHT)
				n_vertices++;
			else
			{
				// Arc!
				int x1 = s->preCorner->x, y1 = s->preCorner->y;
				int x2 = s->postCorner->x, y2 = s->postCorner->y;
				int n;	// number of steps for arcs
				n = (abs(x2-x1)+abs(y2-y1))/(CArc::MAX_STEP);
				n = max( n, CArc::MIN_STEPS );	// or at most 5 degrees of arc
				n_vertices += n;
				n_arcs++;
			}
		}

		// now create gpc_vertex_list for this contour
		gpc_vertex_list * g_v_list = new gpc_vertex_list;
		g_v_list->vertex = (gpc_vertex*)calloc( sizeof(gpc_vertex), n_vertices );
		g_v_list->num_vertices = n_vertices;
		int ivtx = 0;
		ccorner *c = ctr->head;
		do
		{
			int style = c->postSide->m_style;
			int x1 = c->x, y1 = c->y;
			int x2 = c->postSide->postCorner->x, y2 = c->postSide->postCorner->y;
			if( style == STRAIGHT )
			{
				g_v_list->vertex[ivtx].x = x1;
				g_v_list->vertex[ivtx].y = y1;
				ivtx++;
			}
			else
			{
				// style is arc_cw or arc_ccw
				int n = (abs(x2-x1)+abs(y2-y1))/(CArc::MAX_STEP);	// number of steps for arcs
				n = max( n, CArc::MIN_STEPS );						// or at most 5 degrees of arc
				double xo, yo, theta1, theta2, a, b;
				a = fabs( (double)(x1 - x2) );
				b = fabs( (double)(y1 - y2) );
				if( style == CPolyLine::ARC_CW )
				{
					// clockwise arc (ie.quadrant of ellipse)
					int i=0, j=0;
					if( x2 > x1 && y2 > y1 )
					{
						// first quadrant, draw second quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = pi;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						// second quadrant, draw third quadrant of ellipse
						xo = x1;	
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = pi;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						// third quadrant, draw fourth quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = 2.0*pi;
						theta2 = 3.0*pi/2.0;
					}
					else
					{
						xo = x1;	// fourth quadrant, draw first quadrant of ellipse
						yo = y2;
						theta1 = pi/2.0;
						theta2 = 0.0;
					}
				}
				else
				{
					// counter-clockwise arc
					int i=0, j=0;
					if( x2 > x1 && y2 > y1 )
					{
						xo = x1;	// first quadrant, draw fourth quadrant of ellipse
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = 2.0*pi;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						xo = x2;	// second quadrant
						yo = y1;
						theta1 = 0.0;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						xo = x1;	// third quadrant
						yo = y2;
						theta1 = pi/2.0;
						theta2 = pi;
					}
					else
					{
						xo = x2;	// fourth quadrant
						yo = y1;
						theta1 = pi;
						theta2 = 3.0*pi/2.0;
					}
				}
				// now write steps for arc
				if( arc_array )
				{
					arc_array->SetSize(iarc+1);
					(*arc_array)[iarc].style = style;
					(*arc_array)[iarc].n_steps = n;
					(*arc_array)[iarc].xi = x1;
					(*arc_array)[iarc].yi = y1;
					(*arc_array)[iarc].xf = x2;
					(*arc_array)[iarc].yf = y2;
					iarc++;
				}
				for( int is=0; is<n; is++ )
				{
					double theta = theta1 + ((theta2-theta1)*(double)is)/n;
					double x = xo + a*cos(theta);
					double y = yo + b*sin(theta);
					if( is == 0 )
					{
						x = x1;
						y = y1;
					}
					g_v_list->vertex[ivtx].x = x;
					g_v_list->vertex[ivtx].y = y;
					ivtx++;
				}
			}
			c = c->postSide->postCorner;
		} while (c != ctr->head);
		ASSERT( n_vertices == ivtx );

		// add vertex_list to gpc
		gpc_add_contour( gpc, g_v_list, 0 );
		// now clip m_gpc_poly with gpc, put new poly into result
		gpc_polygon * result = new gpc_polygon;
		if( !ctr0 && ctr!=main )
			gpc_polygon_clip( GPC_DIFF, m_gpc_poly, gpc, result );	// hole
		else
			gpc_polygon_clip( GPC_UNION, m_gpc_poly, gpc, result );	// outside
		// now copy result to m_gpc_poly
		gpc_free_polygon( m_gpc_poly );
		delete m_gpc_poly;
		m_gpc_poly = result;
		gpc_free_polygon( gpc );
		delete gpc;
		free( g_v_list->vertex );
		free( g_v_list );
	}
}

void cpolyline::FreeGpcPoly()
{
	if( m_gpc_poly->num_contours )
	{
		delete m_gpc_poly->contour->vertex;
		delete m_gpc_poly->contour;
		delete m_gpc_poly->hole;
	}
	m_gpc_poly->num_contours = 0;
}

void cpolyline::NormalizeWithGpc( bool bRetainArcs )
{
	// Use the General Polygon Clipping Library to clip "this"
	// If this results in new polygons, create them (invoke cpolyline::CreateCompatible(), which
	// will attach the same net/membership-list as "this").
	// If bRetainArcs == TRUE, try to retain arcs in polys
	// CPT2 converted.  NB does not do any drawing/undrawing.  TODO perhaps one day it would be nice to
	// figure out a way to generalize this to any type of cpolyline.
	CArray<CArc> arc_array;
	if( bRetainArcs )
		MakeGpcPoly( NULL, &arc_array );
	else
		MakeGpcPoly( NULL, NULL );

	// now, recreate poly
	// first, find outside contours and create new carea2's if necessary
	int n_ext_cont = 0;
	carray<cpolyline> arr;
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ )
	{
		if( m_gpc_poly->hole[ic] ) continue;
		cpolyline *poly;
		n_ext_cont++;
		if( n_ext_cont == 1 )
			poly = this,
			poly->MustRedraw(),
			contours.RemoveAll();
		else
			poly = CreateCompatible(),
			poly->MustRedraw(),
			arr.Add(poly);
		ccontour *ctr = new ccontour(poly, true);						// NB the new contour will be poly->main
		for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
		{
			int x = m_gpc_poly->contour[ic].vertex[i].x;
			int y = m_gpc_poly->contour[ic].vertex[i].y;
			ccorner *c = new ccorner(ctr, x, y);						// Constructor adds corner to ctr->corners and will also set ctr->head/tail
			if (i>0)
			{
				cside *s = new cside(ctr, STRAIGHT);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
		}
		ctr->Close(STRAIGHT);
		}

	// now add cutouts to the cpolylines
	citer<cpolyline> ip (&arr);
	for( int ic=0; ic<m_gpc_poly->num_contours; ic++ ) 
	{
		if( !m_gpc_poly->hole[ic] ) continue;
		// Find external poly containing this cutout.
		cpolyline *ext_poly = NULL;
		if( n_ext_cont == 1 )
			ext_poly = this;
		else
			// find the polygon that contains this hole
			for( int i=0; i<m_gpc_poly->contour[ic].num_vertices && !ext_poly; i++ )
			{
				int x = m_gpc_poly->contour[ic].vertex[i].x;
				int y = m_gpc_poly->contour[ic].vertex[i].y;
				if( TestPointInside( x, y ) )
					ext_poly = this;
				else
					for( cpolyline *poly = ip.First(); poly; poly = ip.Next() )
						if( poly->TestPointInside( x, y ) )
							{ ext_poly = poly; break; }
			}
		ASSERT( ext_poly );

		ccontour *ctr = new ccontour(ext_poly, false);						// NB the new contour will not be the main one
		for( int i=0; i<m_gpc_poly->contour[ic].num_vertices; i++ )
		{
			int x = m_gpc_poly->contour[ic].vertex[i].x;
			int y = m_gpc_poly->contour[ic].vertex[i].y;
			ccorner *c = new ccorner(ctr, x, y);							// Constructor adds corner to ctr->corners; on iteration 0 it sets ctr->head/tail
			if (i>0)
			{
				cside *s = new cside(ctr, STRAIGHT);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
		}
		ctr->Close(STRAIGHT);
	}
	if( bRetainArcs )
		RestoreArcs( &arc_array, &arr );
	FreeGpcPoly();
}

int cpolyline::TestIntersection( cpolyline *poly2, bool bCheckArcIntersections )
{
	// Test for intersection of 2 copper areas
	// returns: 0 if no intersection
	//			1 if intersection
	//			2 if arcs intersect.  But if bCheckArcIntersections is false, return 1 as soon as any intersection whatever is found
	// CPT2 adapted from CNetList::TestAreaIntersection.  By adding the bCheckArcIntersections param, this covers CNetList::TestAreaIntersections too
	// TODO I don't think this will work if poly2 is totally within this.  We might consider using 
	//   the CMyBitmap class, or gpc?
	// First see if polygons are on same layer
	cpolyline *poly1 = this;
	if( poly1->GetLayer() != poly2->GetLayer() )
		return 0;

	// test bounding rects
	CRect b1 = poly1->GetCornerBounds();
	CRect b2 = poly2->GetCornerBounds();
	if(    b1.bottom > b2.top
		|| b1.top < b2.bottom
		|| b1.left > b2.right
		|| b1.right < b2.left )
		return 0;

	// now test for intersecting segments
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	citer<ccontour> ictr1 (&poly1->contours);
	for (ccontour *ctr1 = ictr1.First(); ctr1; ctr1 = ictr1.Next())
	{
		citer<cside> is1 (&ctr1->sides);
		for (cside *s1 = is1.First(); s1; s1 = is1.Next())
		{
			int xi1 = s1->preCorner->x, yi1 = s1->preCorner->y;
			int xf1 = s1->postCorner->x, yf1 = s1->postCorner->y;
			int style1 = s1->m_style;
			citer<ccontour> ictr2 (&poly2->contours);
			for (ccontour *ctr2 = ictr2.First(); ctr2; ctr2 = ictr2.Next())
			{
				citer<cside> is2 (&ctr2->sides);
				for (cside *s2 = is2.First(); s2; s2 = is2.Next())
				{
					int xi2 = s2->preCorner->x, yi2 = s2->preCorner->y;
					int xf2 = s2->postCorner->x, yf2 = s2->postCorner->y;
					int style2 = s2->m_style;
					int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
									xi2, yi2, xf2, yf2, style2 );
					if( n_int )
					{
						bInt = TRUE;
						if (!bCheckArcIntersections)
							return 1;
						if( style1 != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
							return 2;
						break;
					}
				}
			}
		}
	}

	if( !bInt )
		return 0;
	// An intersection found, but no arc-intersections:
	return 1;
}

bool cpolyline::TestIntersections()
{
	// CPT2.  Generalizes old CNetList::TestAreaIntersections().  Returns true if "this" intersects any other polyline of the same type + layer.
	// Invokes cpolyline::TestIntersection() with bTestArcIntersections set to false for efficiency.
	carray<cpolyline> ia;
	GetCompatiblePolylines(&ia);
	citer<cpolyline> ip (&ia);
	for (cpolyline *p = ip.First(); p; p = ip.Next())
	{
		if (p==this) continue;
		if (TestIntersection(p, false)) return true;
	}
	return false;
}

int cpolyline::CombinePolyline( cpolyline *poly2 )
{
	// CPT2. Adapted from CNetList::CombineAreas.  Unions "poly2" onto "this" polyline.  Does not deal with the removal of poly2 from the net in
	// the case that this and poly2 are copper areas:
	// that's the caller's job.  Assumes that the intersection check has already taken place.
	// Returns 1 if we actually combine 'em, or 0 in the unusual cases where they're actually disjoint.
	cpolyline * poly1 = this;
	CArray<CArc> arc_array1;
	CArray<CArc> arc_array2;
	poly1->MakeGpcPoly( NULL, &arc_array1 );
	poly2->MakeGpcPoly( NULL, &arc_array2 );
	int n_ext_cont1 = 0;
	for( int ic=0; ic<poly1->GetGpcPoly()->num_contours; ic++ )
		if( !((poly1->GetGpcPoly()->hole)[ic]) )
			n_ext_cont1++;
	int n_ext_cont2 = 0;
	for( int ic=0; ic<poly2->GetGpcPoly()->num_contours; ic++ )
		if( !((poly2->GetGpcPoly()->hole)[ic]) )
			n_ext_cont2++;

	gpc_polygon * union_gpc = new gpc_polygon;
	gpc_polygon_clip( GPC_UNION, poly1->GetGpcPoly(), poly2->GetGpcPoly(), union_gpc );

	// get number of outside contours
	int n_union_ext_cont = 0;
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
		if( !((union_gpc->hole)[ic]) )
			n_union_ext_cont++;

	// if no intersection, free new gpc and return.  Somewhat unlikely since an intersection check has usually already occurred, but if areas
	// meet at a single point it could happen.
	if( n_union_ext_cont == n_ext_cont1 + n_ext_cont2 )
	{
		gpc_free_polygon( union_gpc );
		delete union_gpc;
		return 0;
	}

	// intersection, as expected.  Replace poly1/this with combined area
	poly1->MustRedraw();
	poly2->Undraw();
	contours.RemoveAll();
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
	{
		ccontour *ctr = new ccontour(this, !union_gpc->hole[ic]);		// NB the new contour will be this->main if the current gpc contour is a non-hole
		for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
		{
			int x = union_gpc->contour[ic].vertex[i].x;
			int y = union_gpc->contour[ic].vertex[i].y;
			ccorner *c = new ccorner(ctr, x, y);			// Constructor adds corner to ctr->corners (and may also set ctr->head/tail if
															// it was previously empty)
			if (i>0)
			{
				cside *s = new cside(ctr, STRAIGHT);
				ctr->AppendSideAndCorner(s, c, ctr->tail);
			}
		}
		ctr->Close(STRAIGHT);
	}

	utility = 1;
	RestoreArcs( &arc_array1 ); 
	RestoreArcs( &arc_array2 );
	gpc_free_polygon( union_gpc );
	delete union_gpc;
	return 1;
}

int cpolyline::ClipPolygon( bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs )
{	
	// Process a polyline that has been modified, by clipping its polygon against itself.
	// This may change the number of polylines in the parent list.
	// If bMessageBoxInt == TRUE, shows message when clipping occurs.
	// If bMessageBoxArc == TRUE, shows message when clipping can't be done due to arcs.
	// Returns:
	//	-1 if arcs intersect other sides, so polygon can't be clipped
	//	 0 if no intersecting sides
	//	 1 if intersecting sides
	// Also sets poly->utility flags if polylines are modified
	// CPT2 converted and generalized from old area-based function.  Subroutines take care of calling MustRedraw(), as appropriate.
	int test = TestPolygon();				// this sets utility flag
	if( test == -1 && !bRetainArcs )
		test = 1;
	if( test == -1 )
	{
		// arc intersections, don't clip unless bRetainArcs == FALSE
		if( bMessageBoxArc && !bDontShowSelfIntersectionArcsWarning )
		{
			CString str, s ((LPCSTR) IDS_AreaOfNetHasArcsIntersectingOtherSides);		// CPT2 changed text to read "Polygon has arcs intersecting..."
			str.Format( s, UID() );
			CDlgMyMessageBox dlg;
			dlg.Initialize( str );
			dlg.DoModal();
			bDontShowSelfIntersectionArcsWarning = dlg.bDontShowBoxState;
		}
		return -1;	// arcs intersect with other sides, error
	}

	if( test == 1 )
	{
		// non-arc intersections, clip the polygon
		if( bMessageBoxInt && bDontShowSelfIntersectionWarning == FALSE)
		{
			CString str, s ((LPCSTR) IDS_AreaOfNetIsSelfIntersectingAndWillBeClipped);		// CPT2 changed text to read "Polygon is self-intersecting..."
			str.Format( s, UID() );
			CDlgMyMessageBox dlg;
			dlg.Initialize( str );
			dlg.DoModal();
			bDontShowSelfIntersectionWarning = dlg.bDontShowBoxState;
		}
		NormalizeWithGpc( bRetainArcs );						// NB Routine will change "this", and will attach any new polylines in the appropriate places
	}
	return test;
}

void cpolyline::CombinePolylines( carray<cpolyline> *pa, BOOL bMessageBox )
{
	// Static function; checks all polylines in "pa" for intersections, combining them if found
	// CPT2 NB trying it out without the old bUseUtility param
	// Sets utility flag = 1 for any areas modified, and calls virtual function Remove() as necessary.
	// If an area has self-intersecting arcs, doesn't try to combine it
	// CPT2 converted, generalized from old CNetList::CombinAllAreasInNet().  The subroutines are in charge of calling MustRedraw() for affected areas.
	if (pa->GetSize()<2) return;

	// start by testing all polylines to set utility flags.
	citer<cpolyline> ip (pa);
	for (cpolyline *p = ip.First(); p; p = ip.Next())
		p->TestPolygon();
	// now loop through all combinations
	BOOL message_shown = FALSE, mod_p1;											
	citer<cpolyline> ip1 (pa);
	for (cpolyline *p1 = ip1.First(); p1; p1 = ip1.Next())
	{
		do {
			CRect b1 = p1->GetCornerBounds();
			mod_p1 = FALSE;
			citer<cpolyline> ip2 (pa, p1);
			ip2.Next();														// Advance to polyline AFTER p1.
			for (cpolyline *p2 = ip2.Next(); p2; p2 = ip2.Next())
			{
				// if (p1->GetLayer() != p2->GetLayer()) continue;			// Should be safe to omit this check (pa was set up to contain polys all on one layer)
				if (p1->utility == -1 || p2->utility == -1) continue;
				CRect b2 = p2->GetCornerBounds();
				if ( b1.left > b2.right || b1.right < b2.left
						|| b1.bottom > b2.top || b1.top < b2.bottom ) continue;
				// CPT2 obsolete I think:  (plus, having dumped the old utility2 field), I have other uses for "utility" now)
				// if( bUseUtility && !p1->utility && !p2->utility ) continue;
				// check p2 against p1 
				int ret = p1->TestIntersection( p2, true );
				if( ret == 1 )
				{
					ret = p1->CombinePolyline( p2 );
					if (ret == 0) continue;
					pa->Remove( p2 );
					p2->Remove();												// Virtual func. undraws and detaches polyline from the appropriate arrays.
					if( ret == 1 && bMessageBox && !bDontShowIntersectionWarning )
					{
						CString str, s ((LPCSTR) IDS_AreasOfNetIntersectAndWillBeCombined);	// Text now reads "Polylines %d and %d intersect and will be..."
						str.Format( s, p1->UID(), p2->UID() );								// Just to provide a number, give 'em the uid's
						CDlgMyMessageBox dlg;
						dlg.Initialize( str );
						dlg.DoModal();
						bDontShowIntersectionWarning = dlg.bDontShowBoxState;
					}
					mod_p1 = TRUE;
				}
				else if( ret == 2 )
				{
					if( bMessageBox && !bDontShowIntersectionArcsWarning )
					{
						CString str, s ((LPCSTR) IDS_AreasOfNetIntersectButSomeOfTheIntersectingSidesAreArcs); // Now reads "Polylines %d and %d intersect but..."
						str.Format( s, p1->UID(), p2->UID() );
						CDlgMyMessageBox dlg;
						dlg.Initialize( str );
						dlg.DoModal();
						bDontShowIntersectionArcsWarning = dlg.bDontShowBoxState;
					}
				}
			}
		}
		while (mod_p1);		// if p1 is modified, we need to check it against all the other areas again.
	}
}


bool cpolyline::PolygonModified( bool bMessageBoxArc, bool bMessageBoxInt )
{	
	// Process a polyline that has been modified, by clipping its polygon against
	// itself and the polygons for any other areas on the same net.
	// This may wind up adding or removing polylines from the parent array that contains "this".
	// If bMessageBoxXXX == TRUE, shows message boxes when clipping occurs.
	// Returns false if an error occured, i.e. if arcs intersect other sides, so polygon can't be clipped.
	// CPT2 converted.  Subroutines take care of calling MustRedraw() on affected area(s), as needed.
	if (IsValid())
	{
		int test = ClipPolygon( bMessageBoxArc, bMessageBoxInt );
		if( test == -1 )
		{
			// CPT2 always gives an error msg in case of intersecting arcs:
			CString s ((LPCSTR) IDS_ErrorUnableToClipPolygon);
			AfxMessageBox( s );
			return false;
		}
		// now see if we need to clip against other areas
		BOOL bCheckAllAreas = test==1 || TestIntersections();
		if( bCheckAllAreas )
		{
			carray<cpolyline> arr;
			GetCompatiblePolylines( &arr );
			citer<cpolyline> ip (&arr);
			for (cpolyline *poly = ip.First(); poly; poly = ip.Next())
				poly->SaveUndoInfo();
			CombinePolylines( &arr, bMessageBoxInt );
		}
	}
	if (this->IsArea())
		GetNet()->SetThermals();
	return true;
}

void cpolyline::RestoreArcs( CArray<CArc> *arc_array, carray<cpolyline> *pa )
{
	// Restore arcs to a polygon where they were replaced with steps
	// If pa != NULL, also use polygons in pa array
	// CPT2 converted.
	carray<cpolyline> pa2;
	pa2.Add(this);
	if (pa)
		pa2.Add(pa);

	// find arcs and replace them
	BOOL bFound;
	int arc_start;
	int arc_end;
	for( int iarc=0; iarc<arc_array->GetSize(); iarc++ )
	{
		int arc_xi = (*arc_array)[iarc].xi;
		int arc_yi = (*arc_array)[iarc].yi;
		int arc_xf = (*arc_array)[iarc].xf;
		int arc_yf = (*arc_array)[iarc].yf;
		int n_steps = (*arc_array)[iarc].n_steps;
		int style = (*arc_array)[iarc].style;
		ccorner *arc_start = NULL, *arc_end = NULL;
		// loop through polys
		citer<cpolyline> ip (&pa2);
		for (cpolyline *p = ip.First(); p; p = ip.Next())
		{
			citer<ccontour> ictr (&p->contours);
			for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
			{
				if (ctr->corners.GetSize() <= n_steps) continue;
				citer<ccorner> ic (&ctr->corners);
				for (ccorner *c = ic.First(); c; c = ic.Next())
				{
					if (c->x != arc_xi || c->y != arc_yi) continue;
					// Check if we find a corner at location (arc_xf,arc_yf), either n_steps positions forward from c or n_steps positions back
					ccorner *c2 = c;
					for (int i=0; i<n_steps; i++)
						c2 = c2->postSide->postCorner;
					if (c2->x == arc_xf && c2->y == arc_yf) 
					{
						arc_start = c; arc_end = c2;
						goto arc_found;
					}
					c2 = c;
					for (int i=0; i<n_steps; i++)
						c2 = c2->preSide->preCorner;
					if (c2->x == arc_xf && c2->y == arc_yf)
					{
						arc_start = c2; arc_end = c;
						style = style==ARC_CW? ARC_CCW: ARC_CW;						// Arc has been reversed, so flip cw & ccw
						goto arc_found;
					}
				}
			}
		}
		continue;

		arc_found:
		(*arc_array)[iarc].bFound = TRUE;											// Necessary?
		ccontour *ctr = arc_start->contour;
		ctr->poly->MustRedraw();
		for (ccorner *c = arc_start; c != arc_end; c = c->postSide->postCorner)
		{
			if (c != arc_start) 
				ctr->corners.Remove(c);
			ctr->sides.Remove(c->postSide);
		}
		cside *new_side = new cside(ctr, style);
		arc_start->postSide = new_side;
		new_side->preCorner = arc_start;
		arc_end->preSide = new_side;
		new_side->postCorner = arc_end;
		// As a safety precaution, set contour's head and tail equal to arc_start.  (It's conceivable that the old head/tail was in the midst
		// of the segments that are getting eliminated.)
		ctr->head = ctr->tail = arc_start;
	}
}




carea2::carea2(cnet2 *_net, int _layer, int _hatch, int _w, int _sel_box)
	: cpolyline(_net->doc)
{ 
	m_layer = _layer;
	m_hatch = _hatch;
	m_net = _net;
	m_net->areas.Add(this);
	m_w = _w;
	m_sel_box = _sel_box;
}

carea2::carea2(CFreePcbDoc *_doc, int _uid):
	cpolyline(_doc, _uid)
{
	m_net = NULL;
}

bool carea2::IsValid()
{
	if (!m_net->IsValid()) return false;
	return m_net->areas.Contains(this);
}

void carea2::SaveUndoInfo()
{
	doc->m_undo_items.Add( new cuarea(this) );
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ctr->SaveUndoInfo();
}

void carea2::Remove()
{
	// CPT2. User wants to delete area, so detach it from the network (garbage collector will later delete this object and its
	// constituents for good).
	Undraw();
	m_net->areas.Remove(this);
	m_net->SetThermals();
}

void carea2::SetNet( cnet2 *net )
{
	// CPT2 new.  Assign this area to the given net.  Does not deal with potential changes to thermals.
	if (m_net) 
		m_net->areas.Remove(this);
	m_net = net;
	m_net->areas.Add(this);
}

void carea2::GetCompatiblePolylines( carray<cpolyline> *arr )
{
	// CPT2 new.  Virtual function in class cpolyline.  The idea is to put into "arr" all other polylines that might potentially be merged with
	// "this", in the event that they intersect.  For areas, this will be all the areas in the same net that are in the same layer.
	citer<carea2> ia (&m_net->areas);
	for (carea2 *a = ia.First(); a; a = ia.Next())
		if (a->m_layer == m_layer)
			arr->Add(a);
}

cpolyline *carea2::CreateCompatible() 
	// CPT2 new.  Virtual function in class cpolyline.  Returns a new polyline of the same specs as this (but with an empty contour array)
	{ return new carea2(m_net, m_layer, m_hatch, m_w, m_sel_box); }



csmcutout::csmcutout(CFreePcbDoc *_doc, int layer, int hatch)
	: cpolyline(_doc)
{ 
	doc->smcutouts.Add(this); 
	m_layer = layer; 
	m_w = 2*NM_PER_MIL;
	m_sel_box = 10*NM_PER_MIL;
	m_hatch = hatch;
}

csmcutout::csmcutout(CFreePcbDoc *_doc, int _uid):
	cpolyline(_doc, _uid)
	{ }

bool csmcutout::IsValid()
	{ return doc->smcutouts.Contains(this); }

void csmcutout::SaveUndoInfo()
{
	doc->m_undo_items.Add( new cusmcutout(this) );
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ctr->SaveUndoInfo();
}

void csmcutout::GetCompatiblePolylines( carray<cpolyline> *arr )
{
	// CPT2 new.  Virtual function in class cpolyline.  The idea is to put into "arr" all other polylines that might potentially be merged with
	// "this", in the event that they intersect.  For smcutouts, this will be all the smc's in the doc in the same layer.
	citer<csmcutout> ism (&doc->smcutouts);
	for (csmcutout *sm = ism.First(); sm; sm = ism.Next())
		if (sm->m_layer == m_layer)
			arr->Add(sm);
}

cpolyline *csmcutout::CreateCompatible() 
	// CPT2 new.  Virtual function in class cpolyline.  Returns a new polyline of the same specs as this (but with an empty contour array)
	{ return new csmcutout(doc, m_layer, m_hatch); }


void csmcutout::Remove()
{
	Undraw();
	doc->smcutouts.Remove(this);
}


cboard::cboard(CFreePcbDoc *_doc) 
	: cpolyline(_doc)
{ 
	doc->boards.Add(this);
	m_layer = LAY_BOARD_OUTLINE; 
	m_w = 2*NM_PER_MIL;
	m_sel_box = 10*NM_PER_MIL;
	m_hatch = cpolyline::NO_HATCH;
}

cboard::cboard(CFreePcbDoc *_doc, int _uid):
	cpolyline(_doc, _uid)
	{ }

bool cboard::IsValid()
	{ return doc->boards.Contains(this); }

void cboard::SaveUndoInfo()
{
	doc->m_undo_items.Add( new cuboard(this) );
	citer<ccontour> ictr (&contours);
	for (ccontour *ctr = ictr.First(); ctr; ctr = ictr.Next())
		ctr->SaveUndoInfo();
}

void cboard::Remove()
{
	Undraw();
	doc->boards.Remove(this);
}

void cboard::GetCompatiblePolylines( carray<cpolyline> *arr )
	// CPT2 new.  Virtual function in class cpolyline.  For class cboard, this function is only useful (as far as I am aware)
	// as a subroutine within CFreePcbView::TryToReselectCorner().  Therefore it should suffice to do the following, though 
	// theoretically we could scan through doc->others and retrieve all other board outlines as well.
	{ arr->Add(this); }


coutline::coutline(CFreePcbDoc *_doc, int layer, int w)
	: cpolyline(_doc)
{
	if (doc)
		doc->outlines.Add(this);
	m_layer = layer;
	m_w = w;
}

bool coutline::IsValid() 
{
	if (doc && doc->m_edit_footprint)
		return doc->m_edit_footprint->m_outline_poly.Contains(this);
	return false;
}

cpolyline *coutline::CreateCompatible() 
	// CPT2 new.  Virtual function in class cpolyline.  Returns a new polyline of the same specs as this (but with an empty contour array)
	{ return new coutline(doc, m_layer, m_w); }


/**********************************************************************************************/
/*  RELATED TO cnet2                                                                           */
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

bool cnet2::IsValid()
	{ return doc->m_nlist->nets.Contains(this); }

void cnet2::GetStatusStr( CString * str )
{
	CString s ((LPCSTR) IDS_Net2);
	str->Format( s, name ); 
}

void cnet2::SaveUndoInfo(int mode)
{
	// "mode", which is SAVE_ALL by default, can also have the value SAVE_CONNECTS, SAVE_AREAS, or SAVE_NET_ONLY
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
	// CPT2 TODO consider if we want an option to save pin info too.
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
		if (v->m_con->IsValid())
			v->m_con->Remove();
	pins.Remove(pin);
	pin->net = NULL;
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
	// TODO maybe obsolete.  Frequently cvertex2::TestForHit() does the trick.
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

void cnet2::OptimizeConnections( BOOL bBelowPinCount, int pin_count, BOOL bVisibleNetsOnly )
{
	// CPT2 TODO.  Derive from CNetList::OptimizeConnections.  I think we can ditch the ic_track argument of the old routine. 
#ifdef PROFILE
	StartTimer();	//****
#endif
#ifndef CPT2
	// see if we need to do this
	if( bVisibleNetsOnly && net->visible == 0 )
		return ic_track;
	if( bBelowPinCount && net->NumPins() >= pin_count )
		return ic_track;

	// record id of connection to track
	id id_track;
	if( ic_track != -1 )
		id_track = net->ConByIndex( ic_track )->Id();

	// get number of pins N and make grid[NxN] array and pair[N*2] array
	int npins = net->NumPins();
	char * grid = (char*)calloc( npins*npins, sizeof(char) );
	for( int ip=0; ip<npins; ip++ )
		grid[ip*npins+ip] = 1;
	CArray<int> pair;			// use CArray because size is unknown,
	pair.SetSize( 2*npins );	// although this should be plenty

	// go through net, deleting unrouted and unlocked connections 
	// unless they end on a tee or end-vertex
	CIterator_cconnect iter_con( net );
	for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
	{
		if( c->NumSegs() == 0 )
		{
			// this is an error, fix it
			ASSERT(0);
			net->RemoveConnectAdjustTees( c );
		}
		else
		{
			// flag routed or partially routed connections
			int routed = 0;
			if( c->NumSegs() > 1						// at least partially routed
				|| c->seg[0].m_layer != LAY_RAT_LINE	// first segment routed
				|| c->end_pin == cconnect::NO_END		// ends on stub or tee-vertex
				|| c->start_pin == cconnect::NO_END		// starts on stub or tee-vertex
				)
				routed = 1;
			if( !routed && !c->locked )
			{
				// unrouted and unlocked, so delete connection, adjust tees
				net->RemoveConnectAdjustTees( c );
			}
		}
	}


	// AMW r289: Programming note:
	// The following code figures out which pins are connected to each other through
	// existing traces, tees and copper areas, and therefore don't need new ratlines.
	// Previously, when every connection had to start on a pin,
	// this was pretty easy to do since the levels of branching were limited.
	// Now that traces can be routed between other traces and copper areas pretty much ad lib,
	// it is more complicated. Probably the best approach would be some sort of recursive
	// tree-following or grid-following algorithm, but since I am lazy and modifying existing
	// code, I am using a different method of starting with a pin,
	// iterating through all the connections in the net (multiple times if necessary),
	// and building maps of the connected pins, tees and copper areas, so I can eventually
	// identify every connected pin. Hopefully, this won't be too inefficient.
	int dummy;
	CMap<int, int, int, int> pins_analyzed;  // list of all pins analyzed
	CIterator_cpin iter_cpin(net);
	for( cpin * pin=iter_cpin.GetFirst(); pin; pin=iter_cpin.GetNext() )
	{
		// skip pins that have already been analyzed through connections to earlier pins
		int ipin = pin->Index();
		if( pins_analyzed.Lookup( ipin, dummy ) )
			continue;
		pins_analyzed.SetAt( ipin, ipin );

		// look for all connections to this pin, or to any pin, tee or area connected to this pin
		CMap<int, int, int, int> cons_eliminated;  // list of connections that don't connect to this pin
		CMap<int, int, int, int> cons_connected;   // list of connections that do connect to this pin
		CMap<int, int, int, int> tee_ids_connected;	// list of tee_ids connected to this pin
		CMap<int, int, int, int> pins_connected;	// list of pins connected to this pin
		CMap<int, int, int, int> areas_connected;	// list of areas connected to this pin
		int num_new_connections = 1;
		while( num_new_connections )	// iterate as long as we are still finding new connections
		{
			num_new_connections = 0;
			for( cconnect * c=iter_con.GetFirst(); c; c=iter_con.GetNext() )
			{
				int ic = c->Index();
				if( cons_eliminated.Lookup( ic, dummy ) )	// already eliminated
					continue;
				if( cons_connected.Lookup( ic, dummy ) )	// already analyzed
					continue;
				// see if this connection can be eliminated, 
				// or is connected to a pin, tee or area that connects to the pin being analyzed
				bool bConEliminated = TRUE;
				bool bConConnected = FALSE;
				CIterator_cvertex iter_vtx(c);
				for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
				{
					if( v->GetType() == cvertex::V_PIN )
					{
						// vertex is a pin
						cpin * v_pin = v->GetNetPin();
						if( v_pin == pin )
						{
							// pin being analyzed
							bConEliminated = FALSE;
							bConConnected = TRUE;
							break;
						}
						else if( pins_connected.Lookup( v_pin->Index(), dummy ) )
						{
							// pin connected to pin being analyzed
							bConEliminated = FALSE;
							bConConnected = TRUE;
							break;
						}
						else
						{
							// other pin, might connect in later iterations
							bConEliminated = FALSE;
						}
					}
					else if( v->GetType() == cvertex::V_TEE || v->GetType() == cvertex::V_SLAVE )
					{
						// vertex is a tee, might connect
						bConEliminated = FALSE;
						if( tee_ids_connected.Lookup( abs(v->tee_ID), dummy ) )
							bConConnected = TRUE;	// does connect
					}
					CArray<int> ca;		// array of indices to copper areas
					if( v->GetConnectedAreas( &ca ) > 0 )
					{
						// connected to copper area(s), might connect
						bConEliminated = FALSE;
						for( int iarray=0; iarray<ca.GetSize(); iarray++ )
						{
							// get copper area
							int ia = ca[iarray];
							if( areas_connected.Lookup( ia, dummy ) )
								bConConnected = TRUE;	// does connect
						}
					}
				}
				if( bConEliminated )
				{
					// don't need to look at this connection any more
					cons_eliminated.SetAt( ic, ic );
				}
				else if( bConConnected )
				{
					cons_connected.SetAt( ic, ic );
					num_new_connections++;
					// add pins. tees and areas to maps of connected items
					for( cvertex * v=iter_vtx.GetFirst(); v; v=iter_vtx.GetNext() )
					{
						if( v->GetType() == cvertex::V_PIN )
						{
							// vertex is a pin
							cpin * v_pin = v->GetNetPin();
							if( v_pin != pin )
							{
								// connected
								pins_connected.SetAt( v_pin->Index(), v_pin->Index() );
								pins_analyzed.SetAt( v_pin->Index(), v_pin->Index() );
							}
						}
						else if( v->GetType() == cvertex::V_TEE || v->GetType() == cvertex::V_SLAVE )
						{
							// vertex is a tee
							tee_ids_connected.SetAt( abs(v->tee_ID), abs(v->tee_ID) );
						}
						CArray<int> ca;		// array of indices to copper areas
						if( v->GetConnectedAreas( &ca ) > 0 )
						{
							// vertex connected to copper area(s), add to map
							for( int iarray=0; iarray<ca.GetSize(); iarray++ )
							{
								// get copper area(s)
								int ia = ca[iarray];
								areas_connected.SetAt( ia, ia );
								// get pins attached to copper area
								carea* a = net->AreaByIndex(ia);
								for( int ip=0; ip<a->NumPins(); ip++ )
								{
									cpin * pin = a->PinByIndex(ip);
									ipin = pin->Index();
									pins_connected.SetAt( ipin, ipin );
								}
							}
						}
					}
				}
			}	// end connection loop
		}	// end while loop

		// now loop through all connected pins and mark them as connected
		int m_pins = pins_connected.GetCount();
		if( m_pins > 0 )
		{
			int p1 = pin->Index();
			POSITION pos = pins_connected.GetStartPosition();
			int    nKey, nValue;
			while (pos != NULL)
			{
				pins_connected.GetNextAssoc( pos, nKey, nValue );
				int p2 = nValue;
				AddPinsToGrid( grid, p1, p2, npins );
			}
		}

	}	// end loop through net pins


#ifdef PROFILE
	double time1 = GetElapsedTime();
	StartTimer();
#endif

	// now optimize the unrouted and unlocked connections
	long num_loops = 0;
	int n_optimized = 0;
	int min_p1, min_p2, flag;
	double min_dist;

	// create arrays of pin params for efficiency
	CArray<BOOL>legal;
	CArray<double>x, y;
	CArray<double>d;
	x.SetSize(npins);
	y.SetSize(npins);
	d.SetSize(npins*npins);
	legal.SetSize(npins);
	CPoint p;
	for( int ip=0; ip<npins; ip++ )
	{
		legal[ip] = FALSE;
		cpart * part = net->pin[ip].part;
		if( part )
			if( part->shape )
			{
				{
					CString pin_name = net->pin[ip].pin_name;
					int pin_index = part->shape->GetPinIndexByName( pin_name );
					if( pin_index != -1 )
					{
						p = m_plist->GetPinPoint( net->pin[ip].part, pin_name );
						x[ip] = p.x;
						y[ip] = p.y;
						legal[ip] = TRUE;
					}
				}
			}
	}
	for( int p1=0; p1<npins; p1++ )
	{
		for( int p2=0; p2<p1; p2++ )
		{
			if( legal[p1] && legal[p2] )
			{
				double dist = sqrt((x[p1]-x[p2])*(x[p1]-x[p2])+(y[p1]-y[p2])*(y[p1]-y[p2]));
				d[p1*npins+p2] = dist;
				d[p2*npins+p1] = dist;
			}
		}
	}

	// make array of distances for all pin pairs p1 and p2
	// where p2<p1 and index = (p1)*(p1-1)/2
	// first, get number of legal pins
	int n_legal = 0;
	for( int p1=0; p1<npins; p1++ )
		if( legal[p1] )
			n_legal++;

	int n_elements = (n_legal*(n_legal-1))/2;
	int * numbers = (int*)calloc( sizeof(int), n_elements );
	int * index = (int*)calloc( sizeof(int), n_elements );
	int i = 0;
	for( int p1=1; p1<npins; p1++ )
	{
		for( int p2=0; p2<p1; p2++ )
		{
			if( legal[p1] && legal[p2] )
			{
				index[i] = p1*npins + p2;
				double number = d[p1*npins+p2];
				if( number > INT_MAX )
					ASSERT(0);
				numbers[i] = number;
				i++;
				if( i > n_elements )
					ASSERT(0);
			}
		}
	}
	// sort
	::q_sort(numbers, index, 0, n_elements - 1);
	for( int i=0; i<n_elements; i++ )
	{
		int dd = numbers[i];
		int p1 = index[i]/npins;
		int p2 = index[i]%npins;
		if( i>0 )
		{
			if( dd < numbers[i-1] )
				ASSERT(0);
		}
	}

	// now make connections, shortest first
	for( int i=0; i<n_elements; i++ )
	{
		int p1 = index[i]/npins;
		int p2 = index[i]%npins;
		// find shortest connection between unconnected pins
		if( legal[p1] && legal[p2] && !grid[p1*npins+p2] )
		{
			// connect p1 to p2
			AddPinsToGrid( grid, p1, p2, npins );
			pair.SetAtGrow(n_optimized*2, p1);	
			pair.SetAtGrow(n_optimized*2+1, p2);		
			n_optimized++;
		}
	}
	free( numbers );
	free( index );

//#if 0
	// add new optimized connections
	for( int ic=0; ic<n_optimized; ic++ )
	{
		// make new connection with a single unrouted segment
		int p1 = pair[ic*2];
		int p2 = pair[ic*2+1];
		net->AddConnectFromPinToPin( p1, p2 );
	}
//#endif

	free( grid );

	// find ic_track if still present, and return index
	int ic_new = -1;
	if( ic_track >= 0 )
	{
		if( id_track.Resolve() )
		{
			ic_new = id_track.I2();
		}
	}

#ifdef PROFILE
	double time2 = GetElapsedTime();
	if( net->name == "GND" )
	{
		CString mess;
		mess.Format( "net \"%s\", %d pins\nloops = %ld\ntime1 = %f\ntime2 = %f", 
			net->name, net->npins, num_loops, time1, time2 );
		AfxMessageBox( mess );
	}
#endif

	return ic_new;
#endif
}


int cnet2::Draw() 
{
	// CPT2 TODO decide what to do if anything about bDrawn flag.
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

/**********************************************************************************************/
/*  OTHERS: ctext, cglue, ccentroid                                                       */
/**********************************************************************************************/

ctext::ctext( CFreePcbDoc *_doc, int _x, int _y, int _angle, 
	BOOL _bMirror, BOOL _bNegative, int _layer, int _font_size, 
	int _stroke_width, SMFontUtil *_smfontutil, CString * _str )			// CPT2 Removed selType/selSubtype args.  Will use derived creftext and cvaluetext
	: cpcb_item (_doc)														// classes in place of this business.
{
	m_x = _x, m_y = _y;
	m_angle = _angle;
	m_bMirror = _bMirror; m_bNegative = _bNegative;
	m_layer = _layer;
	m_font_size = _font_size;
	m_stroke_width = _stroke_width;
	m_smfontutil = _smfontutil;
	m_str = *_str;
	m_bShown = true;
	m_part = NULL;
}

ctext::ctext(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{ 
	m_smfontutil = NULL;
	m_part = NULL;
}


bool ctext::IsValid() 
{
	CShape *fp = doc? doc->m_edit_footprint: NULL;
	if (fp)
		return fp->m_tl->texts.Contains(this);
	else if (m_part)
		return m_part->IsValid() && m_part->m_tl->texts.Contains(this);
	else
		return doc->m_tlist->texts.Contains(this); 
}

void ctext::Copy( ctext *other )
{
	// CPT2 new.  Copy the contents of "other" into "this"
	MustRedraw();
	m_x = other->m_x;
	m_y = other->m_y;
	m_angle = other->m_angle;
	m_bMirror = other->m_bMirror; 
	m_bNegative = other->m_bNegative;
	m_layer = other->m_layer;
	m_font_size = other->m_font_size;
	m_stroke_width = other->m_stroke_width;
	m_smfontutil = other->m_smfontutil;
	m_str = other->m_str;
	m_bShown = other->m_bShown;
}

void ctext::Move( int x, int y, int angle, BOOL mirror, BOOL negative, int layer, int size, int w )
{
	MustRedraw();
	m_x = x;
	m_y = y;
	m_angle = angle;
	m_layer = layer;
	m_bMirror = mirror;
	m_bNegative = negative;
	if (size>=0) m_font_size = size;
	if (w>=0) m_stroke_width = w;
}

void ctext::Move(int x, int y, int angle, int size, int w) 
	// CPT:  extra version of Move(); appropriate for ref and value-texts, where the layer, mirroring, and negation don't change.
	// NB DrawRelativeTo() will take care of automatic mirroring for ref and value-texts that wind up on the bottom layers.
	{ Move(x, y, angle, false, false, m_layer, size, w); }

void ctext::Resize(int size, int w)
	{ Move (m_x, m_y, m_angle, m_bMirror, m_bNegative, m_layer, size, w); }

void ctext::MoveRelative( int _x, int _y, int _angle, int _size, int _w )
{
	// CPT2 used for moving ref and value texts, whose coordinate values are relative to the parent part.
	// _x and _y are in absolute world coords.
	// angle is relative to part angle.  (Change? It's a bit confusing.)  "_angle", "_size", and "_w" are now -1 by default (=> no change)
	// get position of new text box origin relative to part
	cpart2 * part = GetPart();
	CPoint part_org, tb_org;
	tb_org.x = _x - part->x;
	tb_org.y = _y - part->y;
	
	// correct for rotation of part
	RotatePoint( &tb_org, 360-part->angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( part->side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset text parameters
	MustRedraw();
	m_x = tb_org.x;
	m_y = tb_org.y;
	if (_angle!=-1) 
		m_angle = _angle % 360;
	if (_size!=-1)
		m_font_size = _size;
	if (_w!=-1)
		m_stroke_width = _w;
}

CPoint ctext::GetAbsolutePoint()
{
	// CPT2.  Used for reftexts and valuetexts, whose absolute position is computed relative to the parent part.
	cpart2 *part = GetPart();
	CPoint pt;

	// move origin of text box to position relative to part
	pt.x = m_x;
	pt.y = m_y;
	// flip if part on bottom
	if( part->side )
		pt.x = -pt.x;
	// rotate with part about part origin
	RotatePoint( &pt, part->angle, zero );
	pt.x += part->x;
	pt.y += part->y;
	return pt;
}

CRect ctext::GetRect()
{
	// CPT2 utility.  Get bounding rectangle.  If this is a reftext or valuetext, the result is relative to the parent part.
	GenerateStrokes();
	return m_br;
}

void ctext::GenerateStrokes() {
	// CPT2 new.  Helper for Draw(), though it might also be called independently of drawing.
	// Generate strokes and put them in m_stroke.  Also setup the bounding rectangle member m_br.
	// TODO consider caching
	m_stroke.SetSize( 1000 );
	CPoint si, sf;
	double x_scale = (double)m_font_size/22.0;
	double y_scale = (double)m_font_size/22.0;
	double y_offset = 9.0*y_scale;
	int i = 0;
	double xc = 0.0;
	int xmin = INT_MAX;
	int xmax = INT_MIN;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	int nChars = m_str.GetLength();
	// CPT2.  Setup font (in case m_smfontutil is NULL).  This font business is a bit of a pain...
	SMFontUtil *smf = m_smfontutil;
	if (!smf)
		smf = ((CFreePcbApp*)AfxGetApp())->m_doc->m_smfontutil;

	for( int ic=0; ic<nChars; ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes;
		if( !m_bMirror )
			nstrokes = smf->GetCharStrokes( m_str[ic], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		else
			nstrokes = smf->GetCharStrokes( m_str[nChars-ic-1], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		// loop through strokes and create stroke structures
		for( int is=0; is<nstrokes; is++ )
		{
			if( m_bMirror )
			{
				xi = (max_x - coord[is][0])*x_scale;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (max_x - coord[is][2])*x_scale;
				yf = coord[is][3]*y_scale + y_offset;
			}
			else
			{
				xi = (coord[is][0] - min_x)*x_scale;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (coord[is][2] - min_x)*x_scale;
				yf = coord[is][3]*y_scale + y_offset;
			}

			// get stroke relative to x,y
			si.x = xi + xc;
			sf.x = xf + xc;
			si.y = yi;
			sf.y = yf;
			// rotate
			RotatePoint( &si, m_angle, zero );
			RotatePoint( &sf, m_angle, zero );
			// add m_x, m_y, and fill in stroke structure
			stroke * s = &m_stroke[i];
			s->w = m_stroke_width;
			s->xi = m_x + si.x;
			s->yi = m_y + si.y;
			s->xf = m_x + sf.x;
			s->yf = m_y + sf.y;
			s->layer = m_layer;
			s->type = DL_LINE;
			// update bounding rectangle
			ymin = min( ymin, s->yi - s->w );
			ymin = min( ymin, s->yf - s->w );
			ymax = max( ymax, s->yi + s->w );
			ymax = max( ymax, s->yf + s->w );
			xmin = min( xmin, s->xi - s->w );
			xmin = min( xmin, s->xf - s->w );
			xmax = max( xmax, s->xi + s->w );
			xmax = max( xmax, s->xf + s->w );
			// Next stroke...
			i++;
			if( i >= m_stroke.GetSize() )
				m_stroke.SetSize( i + 100 );
		}
		if( nstrokes > 0 )
			xc += (max_x - min_x + 8.0)*x_scale;
		else
			xc += 16.0*x_scale;
	}

	// Wrap up
	m_stroke.SetSize( i );
	m_br.left = xmin - m_stroke_width/2;
	m_br.right = xmax + m_stroke_width/2;
	m_br.bottom = ymin - m_stroke_width/2;
	m_br.top = ymax + m_stroke_width/2;
}

void ctext::GenerateStrokesRelativeTo(cpart2 *part) {
	// CPT2 new.  Helper for DrawRelativeTo(), though it might also be called independently of drawing.
	// Somewhat descended from the old GenerateStrokesFromPartString() in PartList.cpp.
	// Used for texts (including reftexts and valuetexts) whose position is relative to "part".
	// Generate strokes and put them in m_stroke.  Also setup the bounding rectangle member m_br.
	// If "part" is null, then we're in the footprint editor.  The only way calling GenerateStrokes() differs 
	// from GenerateStrokesRelativeTo(NULL) is that in the latter we make sure that text on the bottom silk or bottom copper
	// gets mirrored.
	// TODO consider caching
	SMFontUtil *smf = doc->m_smfontutil;
	m_stroke.SetSize( 1000 );
	CPoint si, sf;
	double x_scale = (double)m_font_size/22.0;
	double y_scale = (double)m_font_size/22.0;
	double y_offset = 9.0*y_scale;
	int partX = part? part->x: 0;
	int partY = part? part->y: 0;
	int partAngle = part? part->angle: 0;
	// Adjust layer value if part is on bottom
	int layer = m_layer;
	int bMirror = m_bMirror, bOnBottom;
	if (part) 
		bOnBottom = layer==LAY_SILK_BOTTOM || layer==LAY_BOTTOM_COPPER;
	else
		bOnBottom = layer==LAY_FP_SILK_BOTTOM || layer==LAY_FP_BOTTOM_COPPER;
	if (bOnBottom)
		bMirror = !bMirror;
	if (part && part->side)
		if (layer==LAY_SILK_TOP) layer = LAY_SILK_BOTTOM;
		else if (layer==LAY_TOP_COPPER) layer = LAY_BOTTOM_COPPER;
		else if (layer==LAY_SILK_BOTTOM) layer = LAY_SILK_TOP;
		else if (layer==LAY_BOTTOM_COPPER) layer = LAY_TOP_COPPER;
	int i = 0;
	double xc = 0.0;
	int xmin = INT_MAX;
	int xmax = INT_MIN;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	int nChars = m_str.GetLength();

	for( int ic=0; ic<nChars; ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes;
		if( !bMirror )
			nstrokes = smf->GetCharStrokes( m_str[ic], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		else
			nstrokes = smf->GetCharStrokes( m_str[nChars-ic-1], SIMPLEX, &min_x, &min_y, &max_x, &max_y,
				coord, 64 );
		// loop through strokes and create stroke structures
		for( int is=0; is<nstrokes; is++ )
		{
			if( bMirror )
			{
				xi = (max_x - coord[is][0])*x_scale;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (max_x - coord[is][2])*x_scale;
				yf = coord[is][3]*y_scale + y_offset;
			}
			else
			{
				xi = (coord[is][0] - min_x)*x_scale;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (coord[is][2] - min_x)*x_scale;
				yf = coord[is][3]*y_scale + y_offset;
			}
			// Get stroke points relative to text box origin
			si.x = xi + xc;
			sf.x = xf + xc;
			si.y = yi;
			sf.y = yf;
			// rotate about text box origin
			RotatePoint( &si, m_angle, zero );
			RotatePoint( &sf, m_angle, zero );
			// move origin of text box to position relative to part
			si.x += m_x;
			sf.x += m_x;
			si.y += m_y;
			sf.y += m_y;
			if (part && part->side)
				si.x = -si.x,
				sf.x = -sf.x;
			// rotate with part about part origin
			RotatePoint( &si, partAngle, zero );
			RotatePoint( &sf, partAngle, zero );
			// add part's (x,y), then add stroke to array
			stroke * s = &m_stroke[i];
			s->w = m_stroke_width;
			s->xi = partX + si.x;
			s->yi = partY + si.y;
			s->xf = partX + sf.x;
			s->yf = partY + sf.y;
			s->layer = layer;
			s->type = DL_LINE;
			// update bounding rectangle
			ymin = min( ymin, s->yi - s->w );
			ymin = min( ymin, s->yf - s->w );
			ymax = max( ymax, s->yi + s->w );
			ymax = max( ymax, s->yf + s->w );
			xmin = min( xmin, s->xi - s->w );
			xmin = min( xmin, s->xf - s->w );
			xmax = max( xmax, s->xi + s->w );
			xmax = max( xmax, s->xf + s->w );
			// Next stroke...
			i++;
			if( i >= m_stroke.GetSize() )
				m_stroke.SetSize( i + 100 );
		}
		if( nstrokes > 0 )
			xc += (max_x - min_x + 8.0)*x_scale;
		else
			xc += 16.0*x_scale;
	}

	// Wrap up
	m_stroke.SetSize( i );
	m_br.left = xmin - m_stroke_width/2;
	m_br.right = xmax + m_stroke_width/2;
	m_br.bottom = ymin - m_stroke_width/2;
	m_br.top = ymax + m_stroke_width/2;
}


int ctext::Draw() 
{
	// CPT2 TODO.  Deal with drawing negative text.
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	if (!m_bShown)
		return NOERR;

	GenerateStrokes();													// Fills m_stroke and m_br
	// Now draw each stroke
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el = dl->AddMain( this, m_layer, 
			DL_LINE, 1, m_stroke[is].w, 0, 0,
			m_stroke[is].xi, m_stroke[is].yi, 
			m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
	// draw selection rectangle for text
	dl_sel = dl->AddSelector( this, m_layer, DL_HOLLOW_RECT, 1,	
		0, 0, m_br.left, m_br.bottom, m_br.right, m_br.top, m_br.left, m_br.bottom );
	bDrawn = true;
	return NOERR;
}

int ctext::DrawRelativeTo(cpart2 *part, bool bSelector) 
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	if (m_str.GetLength()==0)
		return EMPTY_TEXT;
	if (!m_bShown)
		return NOERR;

	GenerateStrokesRelativeTo( part );													// Fills m_stroke and m_br
	// Now draw each stroke
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el = dl->AddMain( this, m_stroke[is].layer, 
			DL_LINE, 1, m_stroke[is].w, 0, 0,
			m_stroke[is].xi, m_stroke[is].yi, 
			m_stroke[is].xf, m_stroke[is].yf, 0, 0 );
	// draw selection rectangle for text
	if (bSelector)
		dl_sel = dl->AddSelector( this, m_layer, DL_HOLLOW_RECT, 1,	
									0, 0, m_br.left, m_br.bottom, m_br.right, m_br.top, m_br.left, m_br.bottom );
	bDrawn = true;
	return NOERR;
}

void ctext::Undraw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl || !IsDrawn() ) return;

	dl->Remove( dl_sel );
	dl_sel = NULL;
	for( int i=0; i<m_stroke.GetSize(); i++ )
		dl->Remove( m_stroke[i].dl_el );
	m_stroke.RemoveAll();
	// m_smfontutil = NULL;							// indicate that strokes have been removed.  CPT2 Removed, caused a crash
	bDrawn = false;
}

void ctext::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 1 );
}

void ctext::SetVisible(bool bVis)
{
	for( int is=0; is<m_stroke.GetSize(); is++ )
		m_stroke[is].dl_el->visible = bVis;
}

void ctext::StartDragging( CDC * pDC )
{
	// CPT2 derived from CTextList::StartDraggingText
	SetVisible(false);
	CDisplayList *dl = doc->m_dlist;
	dl->CancelHighlight();
	dl->StartDraggingRectangle( pDC, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_x(dl_sel) - dl->Get_x_org(dl_sel), dl->Get_y(dl_sel) - dl->Get_y_org(dl_sel),
		dl->Get_xf(dl_sel) - dl->Get_x_org(dl_sel), dl->Get_yf(dl_sel) - dl->Get_y_org(dl_sel), 
		0, LAY_SELECTION );
}

void ctext::CancelDragging()
{
	// CPT2 derived from CTextList::CancelDraggingText
	doc->m_dlist->StopDragging();
	SetVisible(true);
}


creftext::creftext( cpart2 *part, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown ) :
		ctext(part->doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ m_part = part; m_bShown = bShown; }

creftext::creftext( CFreePcbDoc *doc, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown ) :
		ctext(doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ m_part = NULL; m_bShown = bShown; }
creftext::creftext(CFreePcbDoc *_doc, int _uid):
	ctext(_doc, _uid)
		{ m_part = NULL; }


bool creftext::IsValid() 
{ 
	if (doc->m_edit_footprint)
		return doc->m_edit_footprint->m_ref == this;
	return m_part && m_part->IsValid(); 
}

cvaluetext::cvaluetext( cpart2 *part, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown ) :
		ctext(part->doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ m_part = part; m_bShown = bShown; }

cvaluetext::cvaluetext( CFreePcbDoc *doc, int x, int y, int angle, 
	BOOL bMirror, BOOL bNegative, int layer, int font_size, 
	int stroke_width, SMFontUtil * smfontutil, CString * str_ptr, bool bShown ) :
		ctext(doc, x, y, angle, bMirror, bNegative, layer, font_size,
			stroke_width, smfontutil, str_ptr) 
		{ m_part = NULL; m_bShown = bShown; }

cvaluetext::cvaluetext(CFreePcbDoc *_doc, int _uid):
	ctext(_doc, _uid)
		{ m_part = NULL; }

bool cvaluetext::IsValid() 
{ 
	if (doc->m_edit_footprint)
		return doc->m_edit_footprint->m_value == this;
	return m_part && m_part->IsValid(); 
}


bool ccentroid::IsValid()
	{ return doc && doc->m_edit_footprint && doc->m_edit_footprint->m_centroid == this; }

int ccentroid::Draw()
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

void ccentroid::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 1 );
}

void ccentroid::StartDragging( CDC * pDC )
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
#if 0
	// CPT2 the following old code produces results that look just like what StartDraggingRectangle() did, so I'd say go with the simpler option...
	dl->MakeDragLineArray( 8 );
	int w = CENTROID_WIDTH;
	int xa = 0, ya = 0;
	if( m_angle == 0 )
		xa += w;
	else if( m_angle == 90 )
		ya -= w;
	else if( m_angle == 180 )
		xa -= w;
	else if( m_angle == 270 )
		ya += w;
	dl->AddDragLine( CPoint(-w/2, -w/2), CPoint(+w/2, -w/2) );
	dl->AddDragLine( CPoint(+w/2, -w/2), CPoint(+w/2, +w/2) );
	dl->AddDragLine( CPoint(+w/2, +w/2), CPoint(-w/2, +w/2) );
	dl->AddDragLine( CPoint(-w/2, +w/2), CPoint(-w/2, -w/2) );
	dl->AddDragLine( CPoint(0, 0), CPoint(xa, ya) );
	// drag
	dl->StartDraggingArray( pDC, m_x, m_y, 0, LAY_FP_SELECTION );
#endif
}

// Cancel dragging centroid
//
void ccentroid::CancelDragging()
{
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible( dl_el, 1 );
	// stop dragging
	dl->StopDragging();
}


bool cglue::IsValid()
	{ return doc && doc->m_edit_footprint && doc->m_edit_footprint->m_glues.Contains(this); }

int cglue::Draw()
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
		ccentroid *c = doc->m_edit_footprint->m_centroid;
		x0 = c->m_x, y0 = c->m_y;
	}
	dl_el = dl->AddMain( this, LAY_FP_DOT, DL_CIRC, TRUE, w0, 0, 0, x0, y0, 0, 0, 0, 0 );
	dl_sel = dl->AddSelector( this, LAY_FP_DOT, DL_HOLLOW_RECT, TRUE, 0, 0, 
		x0 - w0/2, y0 - w0/2, x0 + w0/2, y0 + w0/2, 0, 0, 0 );
	bDrawn = true;
	return NOERR;
}


void cglue::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if (!dl) return;
	dl->Highlight( DL_HOLLOW_RECT, 
		dl->Get_x(dl_sel), dl->Get_y(dl_sel),
		dl->Get_xf(dl_sel), dl->Get_yf(dl_sel), 1 );
}


// Start dragging glue spot
//
void cglue::StartDragging( CDC * pDC )
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
void cglue::CancelDragging()
{
	CDisplayList *dl = doc->m_dlist;
	dl->Set_visible( dl_el, 1 );
	// stop dragging
	dl->StopDragging();
}


cdre::cdre(CFreePcbDoc *_doc, int _index, int _type, CString *_str, cpcb_item *_item1, cpcb_item *_item2, 
		   int _x, int _y, int _w, int _layer) 
	: cpcb_item(_doc)
{ 
	index = _index;
	type = _type;
	str = *_str;
	item1 = _item1; 
	item2 = _item2;
	x = _x, y = _y;
	w = _w;
	layer = _layer;
}

cdre::cdre(CFreePcbDoc *_doc, int _uid):
	cpcb_item(_doc, _uid)
{ 
	item1 = item2 = NULL;
}

bool cdre::IsValid()
{
	return doc->m_drelist->dres.Contains(this);
}

int cdre::Draw()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl )
		return NO_DLIST;
	if (bDrawn)
		return ALREADY_DRAWN;
	dl_el = dl->AddMain( this, LAY_DRC_ERROR, DL_HOLLOW_CIRC, 1, w, 0, 0, x, y, 0, 0, x, y ); 
	dl_sel = dl->AddSelector( this, LAY_DRC_ERROR, DL_HOLLOW_CIRC, 1, w, 0, x, y, 0, 0, x, y ); 
	bDrawn = true;
	return NOERR;
}

void cdre::Highlight()
{
	CDisplayList *dl = doc->m_dlist;
	if( !dl ) return;
	if( !dl_sel ) return;
	dl->Highlight( DL_HOLLOW_CIRC, 
		dl->Get_x( dl_sel ), dl->Get_y( dl_sel ),
		dl->Get_xf( dl_sel ), dl->Get_yf( dl_sel ),
		dl->Get_w( dl_sel ) );
}
