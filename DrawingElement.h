#ifndef _DRAWINGELEMENT_H /* [ */
#define _DRAWINGELEMENT_H

#include "afxwin.h"
#include "ids.h"
#include "layers.h"
// #include "LinkList.h"
#include "PcbItem.h"

struct CDrawInfo;
class CDisplayList;

// this structure contains an element of the display list
// CPT:  no longer derived from CDLinkList.
class dl_element
{
	friend class CDisplayList;
	friend class CDisplayLayer;

public:
	int magic;
	id id;			// identifier (see ids.h)						// CPT2 will phase out in favor of member "item"
	void * ptr;		// pointer to object drawing this element		// CPT2 ditto
	cpcb_item *item;												// CPT2.
	int usage;														// CPT2. A given pcb-item may have various types of drawing-elements associated
																	// with it (e.g. most have a main element and a selector;  parts have ref and value 
																	// elements, etc.)
																	// Currently id's are used to show an element's role, but I'm trying this method
	enum { DL_MAIN, DL_SEL, DL_REF, DL_REF_SEL, 
		DL_VALUE, DL_VALUE_SEL, DL_HATCH, DL_OTHER };				// Values for "usage" 
	int gtype;		// type of primitive
	int visible;	// 0 to hide

	dl_element();

	// CPT: reworked the following 6 declarations a bit:
	void Draw(CDrawInfo &di);
	virtual void DrawHiliteSeg (CDrawInfo &di) { }						// CPT: Does nothing except with CDLE_LINE objects.
	// LATER?  void _DrawClearance(CDrawInfo &di);
    void DrawThermalRelief(CDrawInfo &di);
	int isHit(double x, double y, double &d) ;							// CPT: changed args.
	int getBoundingRect(CRect &rect)  { return _getBoundingRect(rect); }

protected:
	virtual void _Draw(CDrawInfo &di, bool fHiliteSegs) {}
    virtual void _DrawThermalRelief(CDrawInfo &di) {}

	virtual int  _isHit(double x, double y, double &d) { return 0; }
	virtual int  _getBoundingRect(CRect &rect)  { return 0; }

protected:
	CDisplayList * dlist;

	int sel_vert;     // for selection rectangles, 1 if part is vertical
	int w;            // width (for round or square shapes)
	int holew;        // hole width (for round holes)
	int clearancew;   // clearance width (for lines and pads)
	CPoint org;       // x origin (for rotation, reflection, etc.)
	CPoint i;         // starting or center position of element
	CPoint f;         // opposite corner (for rectangle or line)
	int radius;       // radius of corners for DL_RRECT
	int layer;        // layer to draw on.  CPT:  for elements in the selection layer, this is not necessarily LAY_SELECTION!
	int orig_layer;   // for elements on highlight layer,
	                  // the original layer, the highlight will
	                  // only be drawn if this layer is visible
	dl_element *prev, *next;			// CPT.  I'm phasing out references to CDLinkList, and am implementing the linked lists this way.
	CDisplayLayer *displayLayer;		// CPT
public:
	void Unhook();						// CPT.  Used to be called Remove(), but this name is more descriptive.

};

// n-sided with equal length sides
class CDLE_Symmetric : public dl_element
{
protected:
    int onScreen(void) ;
	virtual int  _getBoundingRect(CRect &rect) ;
};

// rectangular
class CDLE_Rectangular : public dl_element
{
protected:
    int onScreen(void) ;
	virtual int  _getBoundingRect(CRect &rect) ;
	virtual void _DrawThermalRelief(CDrawInfo  &di) ;
};


#endif /* !_DRAWINGELEMENT_H ] */
