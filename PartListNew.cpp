#include "stdafx.h"
#include "PartListNew.h"


cpartlist::cpartlist( CFreePcbDoc *_doc )
{
	m_doc = _doc;
	m_dlist = m_doc->m_dlist;
	m_layers = m_doc->m_num_copper_layers;
	m_annular_ring = m_doc->m_annular_ring_pins;					// (CPT2 TODO check)
	m_nlist = NULL;
	m_footprint_cache_map = NULL;
}

void cpartlist::RemoveAllParts()
{
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
		p->Remove();
}

cpart2 *cpartlist::GetPartByName( CString *ref_des )
{
	// Find part in partlist by ref-des
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
		if (p->ref_des == *ref_des)
			return p;
	return NULL;
}

cpin2 * cpartlist::GetPinByNames ( CString *ref_des, CString *pin_name)
{
	cpart2 *part = GetPartByName(ref_des);
	if (!part) return NULL;
	citer<cpin2> ip (&part->pins);
	for (cpin2 *p = ip.First(); p; p = ip.Next())
		if (p->pin_name == *pin_name)
			return p;
	return NULL;
}

// get bounding rectangle of parts
// return 0 if no parts found, else return 1
//
int cpartlist::GetPartBoundaries( CRect * part_r )
{
	int min_x = INT_MAX;
	int max_x = INT_MIN;
	int min_y = INT_MAX;
	int max_y = INT_MIN;
	int parts_found = 0;
	
	citer<cpart2> ip (&parts);
	for (cpart2 *part = ip.First(); part; part = ip.Next())
	{
		if( part->dl_sel )
		{
			// TODO the ugly Get_x(), Get_y()... functions might be rethought some day
			int x = m_dlist->Get_x( part->dl_sel );
			int y = m_dlist->Get_y( part->dl_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = m_dlist->Get_xf( part->dl_sel );
			y = m_dlist->Get_yf( part->dl_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
		if( dl_element *ref_sel = part->m_ref->dl_sel )
		{
			int x = m_dlist->Get_x( ref_sel );
			int y = m_dlist->Get_y( ref_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = m_dlist->Get_xf( ref_sel );
			y = m_dlist->Get_yf( ref_sel );
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
	}
	part_r->left = min_x;
	part_r->right = max_x;
	part_r->bottom = min_y;
	part_r->top = max_y;
	return parts_found;
}


void cpartlist::SetThermals()
{
	// CPT2 new.  Updates the bNeedsThermal flags for all pins in all parts.
	citer<cpart2> ip (&parts);
	for (cpart2 *p = ip.First(); p; p = ip.Next())
	{
		citer<cpin2> ipin (&p->pins);
		for (cpin2 *pin = ipin.First(); pin; pin = ipin.Next())
			pin->SetNeedsThermal();
	}
}

cpart2 * cpartlist::AddFromString( CString * str )
{
	// set defaults
	CShape * s = NULL;
	CString in_str, key_str;
	CArray<CString> p;
	int pos = 0;
	int len = str->GetLength();
	int np;
	CString ref_des;
	BOOL ref_vis = TRUE;
	int ref_size = 0;
	int ref_width = 0;
	int ref_angle = 0;
	int ref_xi = 0;
	int ref_yi = 0;
	int ref_layer = LAY_SILK_TOP;
	CString value;
	BOOL value_vis = TRUE;
	int value_size = 0;
	int value_width = 0;
	int value_angle = 0;
	int value_xi = 0;
	int value_yi = 0;
	int value_layer = LAY_SILK_TOP;

	// add part to partlist
	CString package;
	int x;
	int y;
	int side;
	int angle;
	int glued;
	
	in_str = str->Tokenize( "\n", pos );
	while( in_str != "" )
	{
		np = ParseKeyString( &in_str, &key_str, &p );
		if( key_str == "ref" )
		{
			ref_des = in_str.Right( in_str.GetLength()-4 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( key_str == "part" )
		{
			ref_des = in_str.Right( in_str.GetLength()-5 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( np >= 6 && key_str == "ref_text" )
		{
			ref_size = my_atoi( &p[0] );
			ref_width = my_atoi( &p[1] );
			ref_angle = my_atoi( &p[2] );
			ref_xi = my_atoi( &p[3] );
			ref_yi = my_atoi( &p[4] );
			if( np > 6 )
				ref_vis = my_atoi( &p[5] );
			else
			{
				if( ref_size )
					ref_vis = TRUE;
				else
					ref_vis = FALSE;
			}
			if( np > 7 )
				ref_layer = my_atoi( &p[6] );
		}
		else if( np >= 7 && key_str == "value" )
		{
			value = p[0];
			value_size = my_atoi( &p[1] );
			value_width = my_atoi( &p[2] );
			value_angle = my_atoi( &p[3] );
			value_xi = my_atoi( &p[4] );
			value_yi = my_atoi( &p[5] );
			if( np > 7 )
				value_vis = my_atoi( &p[6] );
			else
			{
				if( value_size )
					value_vis = TRUE;
				else
					value_vis = FALSE;
			}
			if( np > 8 )
				value_layer = my_atoi( &p[7] );
		}
		else if( key_str == "package" )
		{
			if( np >= 2 )
				package = p[0];
			else
				package = "";
			package = package.Left(CShape::MAX_NAME_SIZE);
		}
		else if( np >= 2 && key_str == "shape" )
		{
			// lookup shape in cache
			s = NULL;
			void * ptr;
			CString name = p[0];
			name = name.Left(CShape::MAX_NAME_SIZE);
			if ( m_footprint_cache_map->Lookup( name, ptr ) )
				// found in cache
				s = (CShape*)ptr; 
		}
		else if( key_str == "pos" )
		{
			if( np >= 6 )
			{
				x = my_atoi( &p[0] );
				y = my_atoi( &p[1] );
				side = my_atoi( &p[2] );
				angle = my_atoi( &p[3] );
				glued = my_atoi( &p[4] );
			}
			else
			{
				x = 0;
				y = 0;
				side = 0;
				angle = 0;
				glued = 0;
			}
		}
		in_str = str->Tokenize( "\n", pos );
	}

	cpart2 * part = new cpart2(this);
	part->SetData( s, &ref_des, &value, &package, x, y, side, angle, 1, glued );					// CPT2.  Also initializes pins.
	part->m_ref_vis = ref_vis;
	part->m_ref->Move(ref_xi, ref_yi, ref_angle,
			false, false, ref_layer, ref_size, ref_width);
	part->m_value_vis = value_vis;
	part->m_value->Move(value_xi, value_yi, value_angle, 
			false, false, value_layer, value_size, value_width );
	part->Draw();																			// CPT2. TODO is this the place to do this?
	return part;
}


int cpartlist::ReadParts( CStdioFile * pcb_file )
{
	int pos, err;
	CString in_str, key_str;
	CArray<CString> p;

	// find beginning of [parts] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess ((LPCSTR) IDS_UnableToFindPartsSectionInFile);
			AfxMessageBox( mess );
			return 0;
		}
		in_str.Trim();
	}
	while( in_str != "[parts]" );

	// get each part in [parts] section
	while( 1 )
	{
		pos = pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
			throw err_str;
		}
		in_str.Trim();
		if( in_str[0] == '[' && in_str != "[parts]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			break;		// next section, exit
		}
		else if( in_str.Left(4) == "ref:" || in_str.Left(5) == "part:" )
		{
			CString str;
			do
			{
				str.Append( in_str );
				str.Append( "\n" );
				pos = pcb_file->GetPosition();
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString((LPCSTR) IDS_UnexpectedEOFInProjectFile);
					throw err_str;
				}
				in_str.Trim();
			} while( (in_str.Left(4) != "ref:" && in_str.Left(5) != "part:" )
						&& in_str[0] != '[' );
			pcb_file->Seek( pos, CFile::begin );

			// now add part to partlist
			cpart2 * part = AddFromString( &str );
		}
	}
	return 0;
}

int cpartlist::FootprintLayer2Layer( int fp_layer )
{
	int layer;
	switch( fp_layer )
	{
	case LAY_FP_SILK_TOP: layer = LAY_SILK_TOP; break;
	case LAY_FP_SILK_BOTTOM: layer = LAY_SILK_BOTTOM; break;
	case LAY_FP_TOP_COPPER: layer = LAY_TOP_COPPER; break;
	case LAY_FP_BOTTOM_COPPER: layer = LAY_BOTTOM_COPPER; break;
	default: ASSERT(0); layer = -1; break;
	}
	return layer;
}

