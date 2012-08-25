// Undo.h:  CPT2 new system for undo/redo.  More-or-less as in the old system, we have for each pcb-item class a 
// corresponding undo-item class.   Class cuvertex, for instance, mirrors cvertex2.  The main difference in
// the data structure is that where cvertex2 has pointers to other pcb-item objects (segments, connects),
// cuvertex has in their place uid values.  Using uid's makes it possible for an undo operation to reinsert into
// the overall PCB hierarchy objects that were previously deleted completely, with their out-going and incoming pointers all reset correctly.

// The protocol for using the undo system is inspired by my latest drawing system.  Before making a change to the global pcb hierarchy,
// call SaveUndoInfo for the object(s), low- or high-level, that are going to be changing.  This will create objects of the types 
// defined below, and add them to a list of new undo-items stored within the main CFreePcbDoc (CFreePcbDoc::undo_items).  
// When the user's sequence of changes is finished, 
// call CFreePcbDoc::FinishUndoRecord, which will generate a complete cundo_record on the basis of the stored undo-items, and also on the basis of
// new objects that it detects have been created.  The cundo_record then gets added to the main undo list.

// The main undo list CFreePcbDoc::m_undo_records contains both undo and redo records.  Entries at positions less than CFreePcbDoc::m_undo_pos are
// undo records, those in positions at or above there are redos.  When user hits ctrl-z, the entry at m_undo_pos-1 is read in for processing,
// then switched out for a redo record;  after that, m_undo_pos is decremented.  When user hits redo, the reverse process occurs.  When user does
// a normal editing operation, FinishUndoRecord is invoked and makes sure that all redo records are scrapped before appending the new undo record 
// at m_undo_pos.

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "PcbItem.h"

class cundo_item;
class cuvertex;
class cutee;
class cuseg;
class cuconnect;
class cupin;
class cupart;
class cucorner;
class cuside;
class cucontour;
class cupolyline;
class cuarea;
class cusmcutout;
class cuboard;
class cuoutline;
class cunet;
class cutext;	
class cureftext;
class cuvaluetext;
class cudre;
class cupadstack;
class cucentroid;
class cuglue;
class cushape;

class cpcb_item;
class cvertex2;
class ctee;
class cseg2;
class cconnect2;
class cpin2;
class cpart2;
class ccorner;
class cside;
class ccontour;
class cpolyline;
class carea2;
class csmcutout;
class cboard;
class coutline;
class cnet2;
class ctext;
class creftext;
class cvaluetext; 
class cdre;
class cpad;
class cpadstack;
class ccentroid;
class cglue;
class cshape;

class CFreePcbDoc;
class cpartlist;
class cnetlist;

class cundo_item 
{
	// THE BASE CLASS FOR ALL THE UNDO ("cu...") CLASSES.  
	friend class cundo_record;
	friend class CFreePcbDoc;
protected:
	int m_uid;
	CFreePcbDoc *m_doc;
	bool m_bWasCreated;							// If true, then this undo_item provides us with the uid of an object that got created during the
												// user op.  This will be an instance of cundo_item plain and simple, not one of the derived classes.
	cpcb_item *target;							// Used during the undo itself.  This is the item's corresponding object within the pcb (recently edited
												// by the user but now getting reverted); it may be null if during the edit user deleted the object.
protected:
	cundo_item(CFreePcbDoc *doc, int uid, bool bWasCreated = false)
		{ m_doc = doc; m_uid = uid; m_bWasCreated = bWasCreated; }
public:
	~cundo_item() { }							// Simple destructor since these classes contain essentially no pointers at all.

	int UID() { return m_uid; }

	// Virtual functions, invoked during an actual undo/redo operation.  The first to be called is CreateTarget(), which 
	// is invoked if the operation has to recreate an item that was deleted.  It builds an empty new object of the 
	// right type, with uid equal to m_uid, and puts it in this->target.
	// A little later, FixTarget goes in (for both recreated and existing objects) and sets up all of the target's member fields, including 
	// the pointers (which have to be derived from stored-away uid values).
	// Finally, AddToLists() will ensure that the reconstructed objects belong to the lists they need to (e.g. nets must belong to netlist->nets).
	virtual void CreateTarget() { }
	virtual void FixTarget() { }
	virtual void AddToLists() { }
};

class cuvertex: public cundo_item
{
public:
	int m_con;					// UID of parent connection
	int m_net;					// Similarly
	int preSeg, postSeg;		// Succeeding and following segs.  -1 => none.
	int tee;					// Corresponding tee object's uid if any
	int pin;					// Corresponding pin if any
	int x, y;
	bool bNeedsThermal;
	int force_via_flag;
	int via_w, via_hole_w;

	cuvertex( cvertex2 *v );
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};

class cutee: public cundo_item 
{
	// CPT2 new.  Basically a carray<cvertex2>, indicating the vertices that together join up to form a single tee junction.
public:
	int nVtxs, *vtxs;
	int via_w, via_hole_w;					// Could be recalculated on the basis of the constituent vtxs, but let's keep it simple and store the values.

	cutee( ctee *t );
	~cutee()
		{ free(vtxs); }
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};


class cuseg: public cundo_item
{
public:
	// static int m_array_step;
	int m_layer;
	int m_width;
	int m_curve;
	int m_net;
	int m_con;
	int preVtx, postVtx;

	cuseg( cseg2 *s );	
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};

class cuconnect: public cundo_item
{
public:
	int m_net;
	int nSegs, *segs;
	int nVtxs, *vtxs;
	int head, tail;	
	bool locked;

	cuconnect( cconnect2 *c );
	~cuconnect()
		{ free(segs); free(vtxs); }
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};


class cutext: public cundo_item
{
public:
	int m_x, m_y;
	int m_layer;
	int m_angle;
	bool m_bMirror;
	bool m_bNegative;
	int m_font_size;
	int m_stroke_width;
	CString m_str;
	bool m_bShown;
	int m_part;
	int m_shape;
	SMFontUtil * m_smfontutil;				// Worth the trouble?

	cutext( ctext *t );
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};

class cureftext: public cutext
{
public:
	cureftext( creftext *t )
		: cutext( (ctext*) t )
		{ }
	virtual void CreateTarget();
};

class cuvaluetext: public cutext
{
public:
	cuvaluetext( cvaluetext *t )
		: cutext( (ctext*) t )
		{ }
	virtual void CreateTarget();
};


class cupin : public cundo_item
{
public:
	CString pin_name;		
	int part;
	int x, y;
	int ps;					// CPT2 TODO think more about how padstacks and other footprint types will fit into this.
	int pad_layer;
	int net;
	bool bNeedsThermal;	

	cupin( cpin2 *pin );
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};


class cupart: public cundo_item
{
public:
	cpartlist * m_pl;					// One pointer that I think we can rely on to remain constant 
	int nPins, *pins;
	BOOL visible;
	int x,y;
	int side;
	int angle;
	BOOL glued;
	CString ref_des;
	int m_ref;
	CString value_text;
	int m_value;
	int nTexts, *texts;
	int shape;

	cupart( cpart2 *p );
	~cupart()
		{ free(pins); }
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};


class cucorner: public cundo_item
{
public:
	int x, y;
	int contour;
	int preSide, postSide;

	cucorner(ccorner *c);
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};

class cuside: public cundo_item
{
public:
	int m_style;
	int contour;
	int preCorner, postCorner;

	cuside(cside *s);
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};

class cucontour: public cundo_item
{
public:
	int nCorners, *corners;
	int nSides, *sides;
	int head, tail;
	int poly;

	cucontour(ccontour *ctr);
	~cucontour()
		{ free(corners); free(sides); }
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};

class cupolyline: public cundo_item
{
public:
	int main;
	int nContours, *contours;
	int m_layer;
	int m_w;
	int m_sel_box;
	int m_hatch;

	cupolyline(cpolyline *poly);
	~cupolyline()
		{ free(contours); }
	virtual void FixTarget();
};

class cuarea : public cupolyline
{
public:
	int m_net;

	cuarea(carea2 *a);
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};


class cusmcutout : public cupolyline
{
public:
	cusmcutout(csmcutout *sm);
	virtual void CreateTarget();
	virtual void AddToLists();
};


class cuboard : public cupolyline
{
public:
	cuboard(cboard *b);
	virtual void CreateTarget();
	virtual void AddToLists();
};

class cuoutline : public cupolyline
{
public:
	int shape; 

	cuoutline(coutline *o);
	virtual void CreateTarget();
	virtual void FixTarget();
	// virtual void AddToLists();		// Not needed.  A cuoutline gets added to an undo record only if its parent cushape gets included to.
};

class cunet: public cundo_item
{
public:
	CString name;
	int nConnects, *connects;
	int nPins, *pins;
	int nAreas, *areas;
	int nTees, *tees;
	int def_w;
	int def_via_w;
	int def_via_hole_w;
	bool bVisible;
	cnetlist * m_nlist;			// One pointer that I think we can rely on to remain constant.  Probably don't need it anyway...

	cunet(cnet2 *n);
	~cunet()
		{ free(connects); free(pins); free(areas); free(tees); }
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};

class cudre: public cundo_item
{
public:
	int index;					// It's probably not really necessary to store most of this stuff, other than str, x, y, and w.
	int type;
	CString str;
	int item1, item2;
	int x, y;
	int w;
	int layer;

	cudre(cdre *d);
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};

class cupadstack: public cundo_item
{
public:
	CString name;
	int hole_size;
	int x_rel, y_rel;
	int angle;
	int shape;
	/* Bloody C++ compiler makes it near-impossible to do the following (cpad has to be defined, and trying to get the .h's included in the 
	   right order defeated me).  When is C++ going to adopt a Java-style preprocessor so we can avoid this nonsense? 
			cpad top, top_mask, top_paste;
			cpad bottom, bottom_mask, bottom_paste;
			cpad inner;
	Here's the stupid workaround: */
	int top[6], top_mask[6], top_paste[6];
	int bottom[6], bottom_mask[6], bottom_paste[6];
	int inner[6];

	cupadstack(cpadstack *ps);
	virtual void CreateTarget();
	virtual void FixTarget();
	// virtual void AddToLists();		// Not needed --- a cupadstack is added to an undo record only if its parent cushape is there too
};

class cucentroid: public cundo_item
{
public:
	CENTROID_TYPE m_type;
	int m_x, m_y;
	int m_angle;
	int m_shape;

	cucentroid(ccentroid *c);
	virtual void CreateTarget();
	virtual void FixTarget();
};

class cuglue: public cundo_item
{
public:
	GLUE_POS_TYPE type;
	int w, x, y;
	int shape;

	cuglue(cglue *g);
	virtual void CreateTarget();
	virtual void FixTarget();
	// virtual void AddToLists();		// Not needed -- a cuglue will only be added to an undo record if the parent cushape is present also
};


class cushape: public cundo_item
{
public:
	CString m_name;
	CString m_author;
	CString m_source;
	CString m_desc;
	int m_units;
	int m_sel_xi, m_sel_yi, m_sel_xf, m_sel_yf;
	int m_ref;
	int m_value;
	int m_centroid;
	int m_nPadstacks, *m_padstacks;
	int m_nOutlines, *m_outlines;
	int m_nTexts, *m_texts;
	int m_nGlues, *m_glues;

	cushape(cshape *s);
	virtual void CreateTarget();
	virtual void FixTarget();
	virtual void AddToLists();
};



class cundo_record 
{
	// Corresponding to a single user operation, this is a bundle of undo items, including items with the bWasCreated bit set (such items
	//  indicate a uid-value for objects created during the operation, but contain no other data).  CFreePcbDoc::FinishUndoRecord() is in charge
	//  of creating these cundo_records and adding them to the undo list.
	// Method Execute() actually performs the undo, redo, or undo-without-redo!
	friend class CFreePcbDoc;
protected:
	int nItems;
	cundo_item **items;
	int moveOriginDx, moveOriginDy;					// These are set if user does a move-origin operation, and are zero otherwise.
													// Theoretically I should have a cmoveorigin_undo_record subclass, but screw it...
public:
	enum { OP_UNDO, OP_REDO, OP_UNDO_NO_REDO };

	cundo_record( CArray<cundo_item*> *_items );
	cundo_record( int _moveOriginDx, int _moveOriginDy );
	~cundo_record()
	{
		for (int i=0; i<nItems; i++)
			delete items[i];
		free(items); 
	}

	bool Execute( int op );							// The biggie!  Performs the actual undo/redo/undo-no-redo.
};

