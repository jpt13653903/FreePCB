#include "stdafx.h"
#include "SubdlgClearance.h"

CSubDlg_Clearance::CSubDlg_Clearance(int AutoMode) :
	m_AutoMode(AutoMode)
{
}

void CSubDlg_Clearance::OnInitDialog(CInheritableInfo const &width_attrib)
{
	CString str;

	m_attrib.Undef();
	m_attrib = width_attrib;
	m_attrib.Update();

	m_rb_c_auto.ShowWindow( m_AutoMode == E_NO_AUTO_MODE ? 0 : 1 );

	if( !m_attrib.m_ca_clearance.isDefined() )
	{
		m_text_c_group.EnableWindow( 0 );
		m_check_c_modify.SetCheck( 0 );
		m_check_c_modify.EnableWindow( 0 );
	}
	else 
	{
		if( m_attrib.m_ca_clearance.m_status < 0 )
		{
			if( m_attrib.m_ca_clearance.m_status == CII_FreePcb::E_AUTO_CALC )
			{
				if( m_AutoMode == E_NO_AUTO_MODE )
				{
					m_attrib.m_ca_clearance.m_status = CInheritableInfo::E_USE_PARENT;
					m_attrib.Update_ca_clearance();

					m_rb_c_default.SetCheck(1);
				}
				else
				{
					m_rb_c_auto.SetCheck(1);
				}
			}
			else
			{
				m_attrib.m_ca_clearance.m_status = CInheritableInfo::E_USE_PARENT;
				m_attrib.Update_ca_clearance();

				m_rb_c_default.SetCheck(1);
			}
		}
		else
		{
			m_rb_c_set.SetCheck(1);
		}

		str.Format("%d", m_attrib.m_ca_clearance.m_val / NM_PER_MIL);
		m_edit_c_clearance.SetWindowText(str);
	}

	OnChangeClearanceType();
	OnBnClicked_c_modify();
}

int CSubDlg_Clearance::OnDDXOut()
{
	if( m_check_c_modify.GetCheck() )
	{
		if( m_rb_c_default.GetCheck() )
		{
			m_attrib.m_ca_clearance.m_status = CInheritableInfo::E_USE_PARENT;
		}
		else if( m_rb_c_auto.GetCheck() )
		{
			m_attrib.m_ca_clearance.m_status = CII_FreePcb::E_AUTO_CALC;
		}
		else
		{
			CString str;
			int i;

			m_edit_c_clearance.GetWindowText(str);
			if( (sscanf(str, "%d", &i) != 1) || (i < 0) || (i > MAX_CLEARANCE_MIL) )
			{
				str.Format("Invalid clearance value (0-%d)", MAX_CLEARANCE_MIL);
				AfxMessageBox( str );

				return 0;
			}

			m_attrib.m_ca_clearance = i * NM_PER_MIL;
		}
	}
	else
	{
		m_attrib.m_ca_clearance.Undef();

		m_check_c_apply.SetCheck( 0 );
	}

	return 1;
}


void CSubDlg_Clearance::OnBnClicked_c_Default()
{
	OnChangeClearanceType();

	m_attrib.m_ca_clearance = CInheritableInfo::E_USE_PARENT;
	m_attrib.Update_ca_clearance();

	CString str;

	str.Format( "%d", m_attrib.m_ca_clearance.m_val / NM_PER_MIL );
	m_edit_c_clearance.SetWindowText( str );
}

void CSubDlg_Clearance::OnBnClicked_c_Auto()
{
	OnChangeClearanceType();
}

void CSubDlg_Clearance::OnBnClicked_c_Set()
{
	OnChangeClearanceType();
}


void CSubDlg_Clearance::OnBnClicked_c_modify()
{
	if( !m_check_c_modify.GetCheck() )
	{
		m_rb_c_default.EnableWindow( 0 );
		m_rb_c_auto.EnableWindow( 0 );
		m_rb_c_set.EnableWindow( 0 );

		m_check_c_apply.EnableWindow( 0 );

		m_edit_c_clearance.EnableWindow( 0 );
	}
	else
	{
		m_rb_c_default.EnableWindow( m_attrib.hasParent() );
		m_rb_c_auto.EnableWindow( 1 );
		m_rb_c_set.EnableWindow( 1 );

		m_check_c_apply.EnableWindow( 1 );

		OnChangeClearanceType();
	}
}


void CSubDlg_Clearance::OnChangeClearanceType()
{
	m_edit_c_clearance.EnableWindow( m_rb_c_set.GetCheck() ? 1 : 0 );
}
