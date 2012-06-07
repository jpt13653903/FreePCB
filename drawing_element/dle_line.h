#ifndef _DLE_LINE_H /* [ */
#define _DLE_LINE_H

#include "DrawingElement.h"

// line segment with round end-caps
class CDLE_LINE : public dl_element
{
protected:
	virtual void DrawHiliteSegs(CDrawInfo &di) 
		{ if( visible && dlist->m_vis[ orig_layer ] ) _Draw(di, true); }	// CPT.  Overrides default behavior, which is to do nothing.
	virtual void _Draw(CDrawInfo &di, bool bHiliteSegs);
	virtual int _isHit(CPoint &point);
	virtual int _getBoundingRect(CRect &rect);
};

#endif /* !_DLE_LINE_H ] */
