#include "stdafx.h"
#include "dle_arc.h"

// arcs
int CDLE_ARC::onScreen(void)
{
	return ( (    (f.x >= i.x && i.x < dlist->m_max_x && f.x > dlist->m_org_x)
	         ||   (f.x <  i.x && f.x < dlist->m_max_x && i.x > dlist->m_org_x) )
	         && ( (f.y >= i.y && i.y < dlist->m_max_y && f.y > dlist->m_org_y)
	         ||   (f.y <  i.y && f.y < dlist->m_max_y && i.y > dlist->m_org_y) ) );
}

int CDLE_ARC::_isHit(CPoint &point)
{
    // found selection rectangle, test for hit
    return (   ( (point.x>i.x && point.x<f.x) || (point.x<i.x && point.x>f.x) )
	        && ( (point.y>i.y && point.y<f.y) || (point.y<i.y && point.y>f.y) ) );
}


// arc with clockwise curve
void CDLE_ARC_CW::_Draw(CDrawInfo &di, bool) 
{
	if( onScreen() )
	{
		CPen pen( PS_SOLID, w, di.layer_color[1] );
		di.DC->SelectObject( &pen );
		DrawArc( di.DC, DL_ARC_CW, i.x, i.y, f.x, f.y );
		di.DC->SelectObject( di.line_pen );
	}
}


// arc with counter-clockwise curve
void CDLE_ARC_CCW::_Draw(CDrawInfo &di, bool) 
{
	if( onScreen() )
	{
		CPen pen( PS_SOLID, w, di.layer_color[1] );
		di.DC->SelectObject( &pen );
		DrawArc( di.DC, DL_ARC_CCW, i.x, i.y, f.x, f.y );
		di.DC->SelectObject( di.line_pen );
	}
}
