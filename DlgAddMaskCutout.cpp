// DlgAddMaskCutout.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddMaskCutout.h"

// CDlgAddMaskCutout dialog

IMPLEMENT_DYNAMIC(CDlgAddMaskCutout, CDialog)
CDlgAddMaskCutout::CDlgAddMaskCutout(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddMaskCutout::IDD, pParent)
{
}

CDlgAddMaskCutout::~CDlgAddMaskCutout()
{
}
	
void CDlgAddMaskCutout::Initialize ( CPolyLine *poly )
{
   m_poly = poly;
}

void CDlgAddMaskCutout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_layer);
	DDX_Control(pDX, IDC_RADIO1, m_radio_none);
	DDX_Control(pDX, IDC_RADIO2, m_radio_edge);
	DDX_Control(pDX, IDC_RADIO3, m_radio_full);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		// init
      if (m_poly) {
         m_hatch = m_poly->GetHatch();
         m_layer = m_poly->GetLayer();

         isNewCutout = FALSE;
      }
      else {
         isNewCutout = TRUE;
      }

		m_combo_layer.InsertString( 0, "TOP SOLDER MASK" );
		m_combo_layer.InsertString( 1, "BOTTOM SOLDER MASK" );
		
		if( m_hatch == -1 )
			m_hatch = CPolyLine::NO_HATCH;	

		if( m_hatch == CPolyLine::NO_HATCH )
			m_radio_none.SetCheck( 1 );
		else if( m_hatch == CPolyLine::DIAGONAL_EDGE )
			m_radio_edge.SetCheck( 1 );
		else if( m_hatch == CPolyLine::DIAGONAL_FULL )
			m_radio_full.SetCheck( 1 );

		if (m_layer == LAY_SM_TOP)
         m_combo_layer.SetCurSel(0);
      else if (m_layer == LAY_SM_BOTTOM)
         m_combo_layer.SetCurSel(1);
      else 
         m_combo_layer.SetCurSel(0);
	}
	else
	{
		// outgoing
		if( m_combo_layer.GetCurSel() == 0 )
			m_layer = LAY_SM_TOP;
		else if( m_combo_layer.GetCurSel() == 1 )
			m_layer = LAY_SM_BOTTOM;
		else
			ASSERT(0);
		if( m_radio_none.GetCheck() )
			m_hatch = CPolyLine::NO_HATCH;
		else if( m_radio_edge.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_EDGE;
		else if( m_radio_full.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_FULL;
		else
			ASSERT(0);
	}
}


BEGIN_MESSAGE_MAP(CDlgAddMaskCutout, CDialog)
END_MESSAGE_MAP()


// CDlgAddMaskCutout message handlers
