// DlgAddText.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddText.h"
#include "layers.h"

int gLastHeight = 100*NM_PER_MIL;
int gLastWidth = 10*NM_PER_MIL;
BOOL gUseDefaultWidth = TRUE;
BOOL gMirror = FALSE;
BOOL gNegative = FALSE;
int gLastLayer = LAY_SILK_TOP;

// CDlgAddText dialog

IMPLEMENT_DYNAMIC(CDlgAddText, CDialog)
CDlgAddText::CDlgAddText(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddText::IDD, pParent)
{
	m_x = 0;
	m_y = 0;
	m_fp_flag = FALSE;
}

CDlgAddText::~CDlgAddText()
{
}

void CDlgAddText::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LAYER, m_layer_list);
	DDX_Control(pDX, IDC_EDIT_HEIGHT, m_edit_height);
	DDX_Control(pDX, IDC_SET_WIDTH, m_button_set_width);
	DDX_Control(pDX, IDC_DEF_WIDTH, m_button_def_width);
	DDX_Control(pDX, IDC_EDIT_WIDTH, m_edit_width);
	DDX_Control(pDX, IDC_EDIT_TEXT, m_text);
	DDX_Control(pDX, IDC_Y, m_edit_y);
	DDX_Control(pDX, IDC_X, m_edit_x);
	DDX_Text( pDX, IDC_EDIT_TEXT, m_str );
	DDX_Control(pDX, IDC_LIST_ANGLE, m_list_angle);
	DDX_Control(pDX, IDC_RADIO1, m_button_drag);
	DDX_Control(pDX, IDC_RADIO2, m_button_set_position );
	DDX_Control(pDX, IDC_COMBO_ADD_TEXT_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_CHECK_NEGATIVE, m_check_negative);
	DDX_Control(pDX, IDC_CHECK_MIRROR, m_check_mirror);
	if( pDX->m_bSaveAndValidate )
	{
		// leaving the dialog
		if( m_str == "" )
		{
			CString s ((LPCSTR) IDS_InvalidTextString);
			AfxMessageBox( s );
			pDX->Fail();
		}
		if( m_str.Find( '\"' ) != -1 )
		{
			CString s ((LPCSTR) IDS_TextStringCantContainQuote);
			AfxMessageBox( s );
			pDX->Fail();
		}
		GetFields();
		int ia = m_list_angle.GetCurSel();
		m_angle = ia*90;
		int ilay = m_layer_list.GetCurSel();
		if( ilay <= 0 )
			m_layer = LAY_SILK_TOP;
		else if( ilay == 1 )
			m_layer = LAY_SILK_BOTTOM;
		else if( ilay > 1 )
			m_layer = ilay - 2 + LAY_TOP_COPPER;
	}
}


BEGIN_MESSAGE_MAP(CDlgAddText, CDialog)
	ON_BN_CLICKED(IDC_SET_WIDTH, OnBnClickedSetWidth)
	ON_BN_CLICKED(IDC_DEF_WIDTH, OnBnClickedDefWidth)
	ON_EN_CHANGE(IDC_EDIT_HEIGHT, OnEnChangeEditHeight)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedSetPosition)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedDrag)
	ON_CBN_SELCHANGE(IDC_COMBO_ADD_TEXT_UNITS, OnCbnSelchangeComboAddTextUnits)
	ON_LBN_SELCHANGE(IDC_LIST_LAYER, OnLbnSelchangeListLayer)
END_MESSAGE_MAP()

// Initialize dialog
//
void CDlgAddText::Initialize( BOOL fp_flag, int num_layers, int drag_flag, 
		CString * str, int units, int layer, BOOL bMirror, BOOL bNegative, 
		int angle, int height, int width, int x, int y )

{
	m_fp_flag = fp_flag;
	m_num_layers = num_layers;
	m_bDrag = drag_flag;
	m_units = units;
	bNewText = drag_flag;
	if( bNewText )
	{
		m_str = "";
		m_layer = gLastLayer;
		m_bMirror = gMirror;
		m_bNegative = gNegative;
		m_bUseDefaultWidth = gUseDefaultWidth;
		m_height = gLastHeight;
		m_width = gLastWidth;
	}
	else
	{
		if( str )
			m_str = *str;
		else
			m_str = "";
		m_units = units;
		m_layer = layer;
		m_bMirror = bMirror;
		m_bNegative = bNegative;
		m_bUseDefaultWidth = FALSE;
		m_height = height;
		m_width = width;
	}
	m_angle = angle;
	m_x = x;
	m_y = y;
}

// CDlgAddText message handlers

BOOL CDlgAddText::OnInitDialog()
{
	CDialog::OnInitDialog();

	// units
	m_combo_units.InsertString(	0, "MIL" );
	m_combo_units.InsertString(	1, "MM"	);
	if(	m_units	== MIL )
	{
		m_combo_units.SetCurSel(0);
		m_unit_mult = NM_PER_MIL;
	}
	else
	{
		m_combo_units.SetCurSel(1);
		m_unit_mult = 1000000;
	}

	// layers
	CString s;
	if( m_fp_flag )
	{
		s.LoadStringA(IDS_FpLayerStr+LAY_FP_SILK_TOP);
		m_layer_list.InsertString( -1, s );
		m_layer_list.SetCurSel( 0 );
	}
	else
	{
		s.LoadStringA(IDS_LayerStr+LAY_SILK_TOP);
		m_layer_list.InsertString( -1, s );
		s.LoadStringA(IDS_LayerStr+LAY_SILK_BOTTOM);
		m_layer_list.InsertString( -1, s );
		for( int i=LAY_TOP_COPPER; i<m_num_layers; i++ )
			s.LoadStringA(IDS_LayerStr+i),
			m_layer_list.InsertString( -1, s ); 
	}

	// angles
	m_list_angle.InsertString( -1, "0" );
	m_list_angle.InsertString( -1, "90" );
	m_list_angle.InsertString( -1, "180" );
	m_list_angle.InsertString( -1, "270" );
	if( m_angle == 0 )
		m_list_angle.SetCurSel( 0 );
	else if( m_angle == 90 )
		m_list_angle.SetCurSel( 1 );
	else if( m_angle == 180 )
		m_list_angle.SetCurSel( 2 );
	else
		m_list_angle.SetCurSel( 3 );
	SetFields();
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDlgAddText::OnBnClickedSetWidth()
{
	GetFields();
	SetFields();
}

void CDlgAddText::OnBnClickedDefWidth()
{
	GetFields();
	SetFields();
}

void CDlgAddText::OnEnChangeEditHeight()
{
	if( m_button_def_width.GetCheck() )
	{
		CString str;
		m_edit_height.GetWindowText( str );
		m_height = atof(str)*m_unit_mult;
		m_width = m_height/10;
		m_width = max( m_width, 1*NM_PER_MIL );
		if( m_units == MIL )
		{
			MakeCStringFromDouble( &str, m_width/NM_PER_MIL );
			m_edit_width.SetWindowText( str );
		}
		else
		{
			MakeCStringFromDouble( &str, m_width/1000000.0 );
			m_edit_width.SetWindowText( str );
		}
	}
}

void CDlgAddText::OnBnClickedOk()
{
	GetFields();
	if( bNewText )
	{
		gLastHeight = m_height;
		gLastWidth = m_width;
		gUseDefaultWidth = m_button_def_width.GetCheck();
		gLastLayer = m_layer;
		gMirror = m_bMirror;
		gNegative = m_bNegative;
	}
	OnOK();
}

void CDlgAddText::OnBnClickedSetPosition()
{
	GetFields();
	SetFields();
}

void CDlgAddText::OnBnClickedDrag()
{
	GetFields();
	SetFields();
}

void CDlgAddText::OnCbnSelchangeComboAddTextUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
	{
		m_units = MIL;
		m_unit_mult = NM_PER_MIL;
	}
	else
	{
		m_units = MM;
		m_unit_mult = 1000000;
	}
	SetFields();
}

void CDlgAddText::GetFields()
{
	CString str;
	double mult;
	if( m_units == MIL )
		mult = NM_PER_MIL;
	else
		mult = 1000000.0;
	m_edit_height.GetWindowText( str );
	m_height = atof( str ) * mult;
	m_edit_width.GetWindowText( str );
	m_width = atof( str ) * mult;
	m_edit_x.GetWindowText( str );
	m_x = atof( str ) * mult;
	m_edit_y.GetWindowText( str );
	m_y = atof( str ) * mult;
	m_bMirror = m_check_mirror.GetCheck();
	m_bNegative = m_check_negative.GetCheck();
	m_bUseDefaultWidth = m_button_def_width.GetCheck();
	m_bDrag = m_button_drag.GetCheck();
	int ilay = m_layer_list.GetCurSel();
	if( ilay <= 0 )
		m_layer = LAY_SILK_TOP;
	else if( ilay == 1 )
		m_layer = LAY_SILK_BOTTOM;
	else if( ilay > 1 )
		m_layer = ilay - 2 + LAY_TOP_COPPER;
}

void CDlgAddText::SetFields()
{
	CString str;
	double mult;
	if( m_units == MIL )
		mult = NM_PER_MIL;
	else
		mult = 1000000.0;
	MakeCStringFromDouble( &str, m_height/mult );
	m_edit_height.SetWindowText( str );
	MakeCStringFromDouble( &str, m_width/mult );
	m_edit_width.SetWindowText( str );
	MakeCStringFromDouble( &str, m_x/mult );
	m_edit_x.SetWindowText( str );
	MakeCStringFromDouble( &str, m_y/mult );
	m_edit_y.SetWindowText( str );
	m_check_mirror.SetCheck( m_bMirror );
	if( m_layer >= LAY_TOP_COPPER )
		m_check_negative.EnableWindow( TRUE );
	else
		m_check_negative.EnableWindow( FALSE );
		if( m_layer == LAY_SILK_TOP )
			m_layer_list.SetCurSel( 0 );
		else if( m_layer == LAY_SILK_BOTTOM )
			m_layer_list.SetCurSel( 1 );
		else if( m_layer >= LAY_TOP_COPPER )
			m_layer_list.SetCurSel( m_layer -LAY_TOP_COPPER + 2 );
		else
			m_layer_list.SetCurSel( 0 );
	m_check_negative.SetCheck( m_bNegative );
	m_button_def_width.SetCheck( m_bUseDefaultWidth );
	m_button_set_width.SetCheck( !m_bUseDefaultWidth );
	m_button_def_width.SetCheck( m_bUseDefaultWidth );
	m_edit_width.EnableWindow( !m_bUseDefaultWidth );
	m_button_drag.SetCheck( m_bDrag );
	m_button_set_position.SetCheck( !m_bDrag );
	m_edit_x.EnableWindow( !m_bDrag );
	m_edit_y.EnableWindow( !m_bDrag );
	m_list_angle.EnableWindow( !m_bDrag );
}


void CDlgAddText::OnLbnSelchangeListLayer()
{
	GetFields();
	SetFields();
}
