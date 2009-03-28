#include "stdafx.h"
#include "SubdlgTraceWidth.h"
#include "SubdlgViaWidth.h"

CSubDlg_ViaWidth::CSubDlg_ViaWidth(CSubDlg_TraceWidth *pSubDlg_TraceWidth)
{
	my_SubDlg_TraceWidth = pSubDlg_TraceWidth;

	if( my_SubDlg_TraceWidth != NULL )
	{
		my_SubDlg_TraceWidth->AddSub_TraceWidth(m_TraceWidthUpdate);
	}
}

void CSubDlg_ViaWidth::OnInitDialog(CInheritableInfo const &width_attrib)
{
	m_attrib.Undef();
	m_attrib = width_attrib;
	m_attrib.Update();

	int n = ( my_SubDlg_TraceWidth == NULL ) ? -1 : my_SubDlg_TraceWidth->m_w->GetSize();

	if( !m_attrib.m_via_width.isDefined() ||
		!m_attrib.m_via_hole.isDefined() )
	{
		m_text_v_group.EnableWindow( 0 );
		m_check_v_modify.SetCheck( 0 );
		m_check_v_modify.EnableWindow( 0 );
	}
	else 
	{
		m_text_v_group.EnableWindow( 1 );

		if( m_attrib.m_via_width.m_status < 0 )
		{
			if( m_attrib.m_via_width.m_status == CII_FreePcb::E_USE_DEF_FROM_WIDTH )
			{
				if( n > 0 )
				{
					m_attrib.m_via_width.m_status = CII_FreePcb::E_USE_DEF_FROM_WIDTH;
					m_attrib.m_via_hole .m_status = CII_FreePcb::E_USE_DEF_FROM_WIDTH;

					m_rb_v_def_for_width.SetCheck( 1 );
				}
				else
				{
					// Default width menu has no entries
					m_attrib.m_via_width.m_status = CII_FreePcb::E_USE_VAL;
					m_attrib.m_via_hole .m_status = CII_FreePcb::E_USE_VAL;

					m_rb_v_set.SetCheck( 1 );
				}
			}
			else
			{
				m_attrib.m_via_width.m_status = CInheritableInfo::E_USE_PARENT;
				m_attrib.m_via_hole .m_status = CInheritableInfo::E_USE_PARENT;

				m_rb_v_default.SetCheck( 1 );
			}
		}
		else
		{
			m_rb_v_set.SetCheck( 1 );
		}

		CString str;

		str.Format("%d", m_attrib.m_via_width.m_val / NM_PER_MIL );
		m_edit_v_pad_w.SetWindowText( str );
		str.Format("%d", m_attrib.m_via_hole.m_val / NM_PER_MIL );
		m_edit_v_hole_w.SetWindowText( str );
	}

	OnChangeWidthType();
	OnBnClicked_v_modify();
}

int CSubDlg_ViaWidth::OnDDXOut()
{
	if( m_check_v_modify.GetCheck() )
	{
		if( m_rb_v_default.GetCheck() )
		{
			m_attrib.m_via_width.m_status = CInheritableInfo::E_USE_PARENT;
			m_attrib.m_via_hole .m_status = CInheritableInfo::E_USE_PARENT;
		}
		else if ( m_rb_v_def_for_width.GetCheck() )
		{
			m_attrib.m_via_width.m_status = CII_FreePcb::E_USE_DEF_FROM_WIDTH;
			m_attrib.m_via_hole .m_status = CII_FreePcb::E_USE_DEF_FROM_WIDTH;
		}
		else
		{
			CString str;
			int i;

			m_edit_v_pad_w.GetWindowText( str );
			if( (sscanf(str, "%d", &i) != 1) || (i < 1) || (i > MAX_VIA_W_MIL) )
			{
				str.Format("Invalid via width (1-%d)", MAX_VIA_W_MIL);
				AfxMessageBox( str );

				return 0;
			}
			m_attrib.m_via_width = i * NM_PER_MIL;

			m_edit_v_hole_w.GetWindowText( str );
			if( (sscanf(str, "%d", &i) != 1) || (i < 1) || (i > MAX_VIA_HOLE_MIL) )
			{
				str.Format("Invalid via hole width (1-%d)", MAX_VIA_HOLE_MIL);
				AfxMessageBox( str );

				return 0;
			}
			m_attrib.m_via_hole = i * NM_PER_MIL;
		}
	}
	else
	{
		m_attrib.m_via_width.Undef();
		m_attrib.m_via_hole.Undef();

		m_check_v_apply.SetCheck( 0 );
	}

	// Undef any non-via items in m_attrib
	m_attrib.m_seg_width.Undef();

	return 1;
}


void CSubDlg_ViaWidth::CTraceWidthUpdate::Update(CConnectionWidthInfo const &width_attrib)
{
	CDataSubscriber_GET_COMPOSITION(CSubDlg_ViaWidth, m_TraceWidthUpdate)->TraceWidthUpdate(width_attrib);
}

void CSubDlg_ViaWidth::TraceWidthUpdate(CConnectionWidthInfo const &width_attrib)
{
	int n = ( my_SubDlg_TraceWidth->m_w == NULL ) ? -1 : my_SubDlg_TraceWidth->m_w->GetSize();
	if( m_rb_v_def_for_width.GetCheck() && (n > 0) )
	{
		CString text;

		int new_w = width_attrib.m_seg_width.m_val;

		int new_v_w = 0;
		int new_v_h_w = 0;

		if( new_w >= 0 )
		{
			if( new_w == 0 )
			{
				new_v_w = 0;
				new_v_h_w = 0;
			}
			else
			{
				int n = my_SubDlg_TraceWidth->m_w->GetSize();
				int i;
				for( i=0; i < n-1; i++ )
				{
					if( new_w <= (*my_SubDlg_TraceWidth->m_w)[i] )
					{
						break;
					}
				}
				new_v_w   = (*my_SubDlg_TraceWidth->m_v_w  )[i];
				new_v_h_w = (*my_SubDlg_TraceWidth->m_v_h_w)[i];
			}
		}

		m_attrib.m_via_width.m_status = CII_FreePcb::E_USE_DEF_FROM_WIDTH;
		m_attrib.m_via_width.m_val    = new_v_w;

		m_attrib.m_via_hole.m_status = CInheritableInfo::E_USE_PARENT;
		m_attrib.m_via_hole.m_val    = new_v_h_w;

		text.Format( "%d", new_v_w / NM_PER_MIL );
		m_edit_v_pad_w.SetWindowText( text );

		text.Format( "%d", new_v_h_w / NM_PER_MIL );
		m_edit_v_hole_w.SetWindowText( text );
	}
}


void CSubDlg_ViaWidth::OnBnClicked_v_Default()
{
	OnChangeWidthType();

	m_attrib.m_via_width = CInheritableInfo::E_USE_PARENT;
	m_attrib.m_via_hole  = CInheritableInfo::E_USE_PARENT;
	m_attrib.Update_via_width();
	m_attrib.Update_via_hole();

	CString str;

	str.Format( "%d", m_attrib.m_via_width.m_val / NM_PER_MIL );
	m_edit_v_pad_w.SetWindowText( str );

	str.Format( "%d", m_attrib.m_via_hole.m_val / NM_PER_MIL );
	m_edit_v_hole_w.SetWindowText( str );
}

void CSubDlg_ViaWidth::OnBnClicked_v_DefForTrace()
{
	OnChangeWidthType();
}

void CSubDlg_ViaWidth::OnBnClicked_v_Set()
{
	OnChangeWidthType();
}


void CSubDlg_ViaWidth::OnBnClicked_v_modify()
{
	if( !m_check_v_modify.GetCheck() )
	{
		m_rb_v_default.EnableWindow( 0 );
		m_rb_v_def_for_width.EnableWindow( 0 );
		m_rb_v_set.EnableWindow( 0 );

		m_check_v_apply.EnableWindow( 0 );

		m_text_v_pad_w.EnableWindow( 0 );
		m_text_v_hole_w.EnableWindow( 0 );

		m_edit_v_pad_w .EnableWindow( 0 );
		m_edit_v_hole_w.EnableWindow( 0 );
	}
	else
	{
		m_rb_v_default.EnableWindow( m_attrib.hasParent() );
		m_rb_v_set.EnableWindow( 1 );

		m_check_v_apply.EnableWindow( 1 );

		OnChangeWidthType();
	}
}


void CSubDlg_ViaWidth::OnChangeWidthType()
{
	int enable = m_rb_v_set.GetCheck() ? 1 : 0;

	m_text_v_pad_w.EnableWindow( enable );
	m_text_v_hole_w.EnableWindow( enable );

	m_edit_v_pad_w .EnableWindow( enable );
	m_edit_v_hole_w.EnableWindow( enable );

	// Only enable & process "default for width" if defaults exist
	int n = ( my_SubDlg_TraceWidth == NULL ) ? -1 : my_SubDlg_TraceWidth->m_w->GetSize();

	if( n > 0 )
	{
		if( m_rb_v_def_for_width.GetCheck() )
		{
			// Force update from publisher
			my_SubDlg_TraceWidth->Update_TraceWidth();
		}

		m_rb_v_def_for_width.EnableWindow( 1 );
	}
	else
	{
		m_rb_v_def_for_width.EnableWindow( 0 );
	}
}
