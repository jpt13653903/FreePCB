// DlgAddArea.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddArea.h"
#include "layers.h"
#include "Net_iter.h"

// globals
int gHatch = CPolyLine::NO_HATCH;

// CDlgAddArea dialog

IMPLEMENT_DYNAMIC(CDlgAddArea, CDialog)
CDlgAddArea::CDlgAddArea(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddArea::IDD, pParent)
{
}

CDlgAddArea::~CDlgAddArea()
{
}

void CDlgAddArea::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_NET, m_combo_net);
	DDX_Control(pDX, IDC_LIST_LAYER, m_list_layer);
	DDX_Control(pDX, IDC_RADIO_NONE, m_radio_none);
	DDX_Control(pDX, IDC_RADIO_FULL, m_radio_full);
	DDX_Control(pDX, IDC_RADIO_EDGE, m_radio_edge);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming, initialize net list
		citer<cnet2> in (&m_nlist->nets);
		for (cnet2 *net = in.First(); net; net = in.Next())
			m_combo_net.AddString( net->name );
		if( m_net )
		{
			bNewArea = FALSE;
			int i = m_combo_net.SelectString( -1, m_net->name );
			if( i == CB_ERR )
				ASSERT(0);
		}
		else
			bNewArea = TRUE;

		// initialize layers
		m_num_layers = m_num_layers-LAY_TOP_COPPER;
		for( int il=0; il<m_num_layers; il++ )
		{
			CString s ((LPCSTR) (IDS_LayerStr+il+LAY_TOP_COPPER));
			m_list_layer.InsertString( il, s );
		}
		m_list_layer.SetCurSel( m_layer - LAY_TOP_COPPER );
		if( m_hatch == -1 )
			m_hatch = gHatch;	
		if( m_hatch == CPolyLine::NO_HATCH )
			m_radio_none.SetCheck( 1 );
		else if( m_hatch == CPolyLine::DIAGONAL_EDGE )
			m_radio_edge.SetCheck( 1 );
		else if( m_hatch == CPolyLine::DIAGONAL_FULL )
			m_radio_full.SetCheck( 1 );
	}
	else
	{
		// outgoing
		m_layer = m_list_layer.GetCurSel() + LAY_TOP_COPPER;
		m_combo_net.GetWindowText( m_net_name );

		POSITION pos;
		CString name;
		void * ptr;
		m_net = m_nlist->GetNetPtrByName( &m_net_name );
		if( !m_net )
		{
			CString msg ((LPCSTR) IDS_IllegalNetName);
			AfxMessageBox( msg );
			pDX->Fail();
		}
		if( m_radio_none.GetCheck() )
			m_hatch = CPolyLine::NO_HATCH;
		else if( m_radio_full.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_FULL;
		else if( m_radio_edge.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_EDGE;
		else 
			ASSERT(0);
		if( bNewArea )
			gHatch = m_hatch;
	}
}


BEGIN_MESSAGE_MAP(CDlgAddArea, CDialog)
END_MESSAGE_MAP()


// CDlgAddArea message handlers

void CDlgAddArea::Initialize( cnetlist * nl, int nlayers, 
							 cnet2 * net, int layer, int hatch )
{
	m_nlist = nl;
	m_num_layers = nlayers;
	m_net = net;
	m_layer = layer;
	m_hatch = hatch;
}