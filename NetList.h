// NetList.h: interface for the CNetList class.
//
// This is a basically a map of nets
//	- each net is represented by an cnet object
//	- each cnet is mapped by its name
//	- each cnet contains arrays of pins, connections between pins, and copper areas
//	- each pin is represented by a cpin object
//	- each connection is represented by a cconnect object
//  - each cconnect contains arrays of segments and vertices between segments
//	- each segment is represented by a cseg object
//	- each vertex is represented by a cvertex object
//	- each copper area is represented by a carea object
//
// Since most of these objects are responsible for drawing themselves into a CDisplayList,
// a global pointer to the CDisplayList is set when the netlist is constructed.
// In the future, this might be changed to a member variable,
// which would be passed to each object.
//
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "ids.h"
#include "DisplayList.h"
#include "PartList.h"
#include "PolyLine.h"
#include "UndoList.h"
#include "ii_clearance.h"
#include "ii_seg_width.h"

extern int m_layer_by_file_layer[MAX_LAYERS];

class cnet;
class CNetList;

#define MAX_NET_NAME_SIZE 39

// these definitions are for ImportSessionFile()
//
enum NODE_TYPE { NONE, NPIN, NVIA, NJUNCTION };

typedef class {
public:
	NODE_TYPE type;
	int x, y, layer, via_w;
	BOOL bUsed;
	int pin_index;	// if type == NPIN
	CArray<int> path_index;
	CArray<int> path_end;  // 0 or 1
} cnode;

typedef class {
public:
	int x, y, inode;
} cpath_pt;

typedef class {
public:
	// return inode at end of path
	int GetInode( int iend )
	{
		int last_pt = pt.GetSize()-1;
		if(iend)
			return pt[last_pt].inode;
		else
			return pt[0].inode;
	};
	// member variables
	int layer, width;
	CArray<cpath_pt> pt;
	int n_used;		// number of times used
} cpath;
//
// end definitions for ImportSessionFile()

// these structures are used for undoing
struct undo_pin {
	char ref_des[MAX_REF_DES_SIZE+1];
	char pin_name[CShape::MAX_PIN_NAME_SIZE+1];
};

struct undo_corner {
	int x, y, end_contour, style;	// style is for following side
};

struct undo_area {
	int size;
	CNetList * nlist;
	char net_name[MAX_NET_NAME_SIZE+1];
	int iarea;
	int layer;
	int hatch;
	int w;
	int sel_box_w;
	int ncorners;
	// array of undo_corners starts here
};

struct undo_seg {
	int layer;				// copper layer

	CSegWidthInfo width;
	//int width;				// width
	//int via_w, via_hole_w;	// via width and hole width

	CClearanceInfo clearance;

	// Placement new
	static void *operator new(size_t size, void *pMem) { return pMem; }
};

struct undo_vtx {
	int x, y;				// coords
	int pad_layer;			// layer of pad if this is first or last vertex
	int force_via_flag;		// force a via even if no layer change
	int tee_ID;				// identifier for t-connection

	CConnectionWidthInfo width; // via width and hole width  BAF FIX
	CClearanceInfo       clearance;

	// Placement new
	static void *operator new(size_t size, void *pMem) { return pMem; }
};

struct undo_con {
	int size;
	CNetList * nlist;
	char net_name[MAX_NET_NAME_SIZE+1];
	int start_pin, end_pin;		// indexes into net.pin array
	int nsegs;					// # elements in seg array
	int locked;					// 1 if locked (will not be optimized away)
	int set_areas_flag;			// 1 to force setting of copper areas
	int seg_offset;				// offset to array of segments
	int vtx_offset;				// offset to array of vertices
	// array of undo_seg structs goes here
	// followed by array of undo_vtx structs
};

struct undo_net {
	int size;
	CNetList * nlist;
	char name[MAX_NET_NAME_SIZE+1];
	int npins;
	// array of undo_pin structs start here
};


// net_info structure
// used as a temporary copy of net info for editing in dialogs
// or importing/exporting netlists
struct net_info {
	CString name;
	cnet * net;
	BOOL visible;
	CConnectionWidthInfo width;
	CClearanceInfo clearance;
	BOOL apply_trace_width;
	BOOL apply_via_width;
	BOOL apply_clearance;
	BOOL deleted;
	BOOL modified;
	CArray<CString> ref_des;
	CArray<CString> pin_name;
};

// netlist_info is an array of net_info for each net
typedef CArray<net_info> netlist_info;

// carea: describes a copper area
class carea
{
public:
	carea();
	carea( const carea& source );	// dummy copy constructor
	~carea();						// destructor
	carea &operator=( carea &a );	// dummy assignment operator
	void Initialize( CDisplayList * dlist );
	CPolyLine * poly;	// outline
	int npins;			// number of thru-hole pins within area on same net
	CArray<int> pin;	// array of thru-hole pins
	int nvias;			// number of via connections to area
	CArray<int> vcon;	// connections
	CArray<int> vtx;	// vertices
	CDisplayList * m_dlist;
	int utility, utility2;
};

// cpin: describes a pin in a net
class cpin
{
public:
	cpin(){ part = NULL; };
	CString ref_des;	// reference designator such as 'U1'
	CString pin_name;	// pin name such as "1" or "A23"
	cpart * part;		// pointer to part containing the pin
	int utility;
};

// cseg: describes a segment of a connection
class cseg
{
public:
	int layer;				  // copper layer
	CSegWidthInfo  seg_width; // width
	CClearanceInfo seg_clearance; // clearances
	int selected;			  // 1 if selected for editing
	dl_element * dl_el;		  // display element for segment
	dl_element * dl_sel;	  // selection line
	CDisplayList * m_dlist;
	int utility;

public:
	cseg()
	{
		// constructor
		m_dlist = NULL;  // this must be filled in with Initialize()

		layer = 0;
		selected = 0;
		dl_el = NULL;
		dl_sel = NULL;
		utility = 0;
	}

	~cseg()
	{
		// destructor
		RemoveFromDL();
	}


	void RemoveFromDL()
	{
    	if (dl_el)
    	{
    	    dl_el->Remove();
    	    dl_el = NULL;
        }

    	if (dl_sel)
    	{
    	    dl_sel->Remove();
    	    dl_sel = NULL;
        }
	}

	void Initialize( CDisplayList * dlist ) { m_dlist = dlist; }

	int width()     const { return seg_width.m_seg_width.m_val; }
	int clearance() const { return seg_clearance.m_ca_clearance.m_val; }
};

// cvertex: describes a vertex between segments
class cvertex
{
public:
	cvertex() :
		via_width( 0,0,0 ),
		vtx_clearance( CClearanceInfo::E_AUTO_CALC )
	{
		// constructor
		m_dlist = 0;	// this must set with Initialize()
		x = 0; y = 0;
		pad_layer = 0;	// only for first or last
		force_via_flag = 0;		// only used for end of stub trace
		dl_sel = 0;
		dl_hole = 0;
		tee_ID = 0;
		utility = 0;
		utility2 = 0;
	}
	~cvertex()
	{
		// destructor
		if( m_dlist )
		{
			for( int il=0; il<dl_el.GetSize(); il++ )
				dl_el[il]->Remove();

			if( dl_sel )  dl_sel->Remove();
			if( dl_hole ) dl_hole->Remove();
		}
	}
	cvertex &operator=( cvertex &v )	// assignment operator BAF FIX to be const
	{
		// copy all params
		x = v.x;
		y = v.y;
		pad_layer = v.pad_layer;
		force_via_flag = v.force_via_flag;
		via_width = v.via_width;
		vtx_clearance = v.vtx_clearance;
		m_dlist = v.m_dlist;
		tee_ID = v.tee_ID;
		utility = v.utility;
		utility2 = v.utility2;

		// copy dl_elements and remove from source
		// they still need to be renumbered
		if( dl_hole )
			dl_hole->Remove();
		dl_hole = v.dl_hole;
		v.dl_hole = NULL;

		if( dl_sel )
			dl_sel->Remove();
		dl_sel = v.dl_sel;
		v.dl_sel = NULL;

		for( int il=0; il<dl_el.GetSize(); il++ )
		{
			dl_el[il]->Remove();
		}
		dl_el.RemoveAll();

		for( int il=0; il<v.dl_el.GetSize(); il++ )
		{
			dl_el.Add( v.dl_el[il] );
		}
		v.dl_el.RemoveAll();

		return *this;
	};
	void Initialize( CDisplayList * dlist ){m_dlist = dlist;}
	int x, y;					// coords
	int pad_layer;				// layer of pad if this is first or last vertex, otherwise 0
	int force_via_flag;			// force a via even if no layer change
	CConnectionWidthInfo via_width; // via width and hole width (via_w==0 means no via) BAF-use CViaWidthInfo when complete
	CClearanceInfo vtx_clearance; // via clearance
	CArray<dl_element*> dl_el;	// array of display elements for each layer
	dl_element * dl_sel;		// selection box
	dl_element * dl_hole;		// hole in via
	CDisplayList * m_dlist;
	int tee_ID;					// used to flag a t-connection point
	int utility, utility2;		// used for various functions

	int via_w()         const { return via_width.m_via_width.m_val; }
	int via_hole_w()    const { return via_width.m_via_hole.m_val; }
	int via_clearance() const { return vtx_clearance.m_ca_clearance.m_val; }

	int viaExists() const { return via_w(); }
	void SetNoVia() { via_width.m_via_width = via_width.m_via_hole = 0; }
};

// cconnect: describes a connection between two pins or a stub trace with no end pin
class cconnect
{
public:
	enum {
		NO_END = -1		// used for end_pin if stub trace
//		T_END = -2		// used for t-connection to another trace
	};
	cconnect()
	{	// constructor
		locked = 0;
		nsegs = 0;
		seg.SetSize( 0 );
		vtx.SetSize( 0 );
		utility = 0;
	};
	int start_pin, end_pin;		// indexes into net.pin array
	int nsegs;					// # elements in seg array
	int locked;					// 1 if locked (will not be optimized away)
	CArray<cseg> seg;			// array of segments
	CArray<cvertex> vtx;		// array of vertices, size = nsegs + 1
	int utility;				// used for various temporary ops
	// these params used only by DRC
	int min_x, max_x;			// bounding rect
	int min_y, max_y;
	BOOL vias_present;
	int seg_layers;
};

// cnet: describes a net
class cnet
{
public:
	cnet( CDisplayList * dlist )
	{
		m_dlist = dlist;
	}

	id id;				// net id
	CString name;		// net name
	int nconnects;		// number of connections
	CArray<cconnect> connect; // array of connections (size = max_pins-1)
	int npins;			// number of pins
	CArray<cpin> pin;	// array of pins
	int nareas;			// number of copper areas
	CArray<carea,carea> area;	// array of copper areas
	CConnectionWidthInfo  def_width;      // default widths
	CClearanceInfo def_clearance;  // default clearances
	BOOL visible;		// FALSE to hide ratlines and make unselectable
	int utility;		// used to keep track of which nets have been optimized
	CDisplayList * m_dlist;
};

// CNetlist
class CNetList
{
public:
	enum{ MAX_ITERATORS=10 };
	enum {
		VIA_NO_CONNECT = 0,
		VIA_TRACE = 1,
		VIA_AREA = 2
	};
	enum {						// used for UNDO records
		UNDO_CONNECT_MODIFY=1,	// undo modify connection
		UNDO_AREA_CLEAR_ALL,	// flag to remove all areas
		UNDO_AREA_ADD,			// undo add area (i.e. delete area)
		UNDO_AREA_MODIFY,		// undo modify area
		UNDO_AREA_DELETE,		// undo delete area (i.e. add area)
		UNDO_NET_ADD,			// undo add net (i.e delete net)
		UNDO_NET_MODIFY,		// undo modify net
		UNDO_NET_OPTIMIZE		// flag to optimize net on undo
	};
	CMapStringToPtr m_map;	// map net names to pointers
	CNetList( CDisplayList * dlist, CPartList * plist );
	~CNetList();
	void SetNumCopperLayers( int layers ){ m_layers = layers;};
	void SetWidths( CNetWidthInfo const &width );
	void SetViaAnnularRing( int ring ){ m_annular_ring = ring; };
	void SetSMTconnect( BOOL bSMTconnect ){ m_bSMT_connect = bSMTconnect; };

	// functions for nets and pins
	void MarkAllNets( int utility );
	void MoveOrigin( int x_off, int y_off );
	cnet * GetNetPtrByName( CString * name );
	cnet * AddNet( CString name, int max_pins, CConnectionWidthInfo const &def_width, CClearanceInfo const &def_clearance );
	void RemoveNet( cnet * net );
	void RemoveAllNets();
	void AddNetPin( cnet * net, CString * ref_des, CString * pin_name, CClearanceInfo const &clearance, BOOL set_areas=TRUE );
	void AddNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL set_areas=TRUE ) { AddNetPin(net,ref_des,pin_name, CClearanceInfo(CClearanceInfo::E_AUTO_CALC), set_areas); }
	void RemoveNetPin( cpart * part, CString * pin_name );
	void RemoveNetPin( cnet * net, CString * ref_des, CString * pin_name );
	void RemoveNetPin( cnet * net, int pin_index );
	void DisconnectNetPin( cpart * part, CString * pin_name );
	void DisconnectNetPin( cnet * net, CString * ref_des, CString * pin_name );
	int GetNetPinIndex( cnet * net, CString * ref_des, CString * pin_name );
	int SetNetWidth( cnet * net, CConnectionWidthInfo const &width );
	int SetNetClearance( cnet * net, CClearanceInfo const &clearance );
	int UpdateNetAttributes( cnet * net );
	void SetNetVisibility( cnet * net, BOOL visible );
	BOOL GetNetVisibility( cnet * net );
	int CheckNetlist( CString * logstr );
	int CheckConnectivity( CString * logstr );
	void HighlightNetConnections( cnet * net );
	void HighlightNet( cnet * net );
	cnet * GetFirstNet();
	cnet * GetNextNet();
	void CancelNextNet();
	BOOL GetNetBoundaries( CRect * r );

	// functions for connections
	int AddNetConnect( cnet * net, int p1, int p2 );
	int AddNetStub( cnet * net, int p1 );
	int RemoveNetConnect( cnet * net, int ic, BOOL set_areas=TRUE );
	int UnrouteNetConnect( cnet * net, int ic );
	int SetConnectionWidth( cnet * net, int ic, CConnectionWidthInfo const &width );
	int SetConnectionClearance( cnet * net, int ic, CClearanceInfo const &clearance );
	void OptimizeConnections();
	int OptimizeConnections( cnet * net, int ic=-1 );
	void OptimizeConnections( cpart * part );
	void RenumberConnection( cnet * net, int ic );
	void RenumberConnections( cnet * net );
	BOOL TestHitOnConnectionEndPad( int x, int y, cnet * net, int ic, int layer, int dir );
	int TestHitOnAnyPadInNet( int x, int y, int layer, cnet * net );
	void ChangeConnectionPin( cnet * net, int ic, int end_flag,
		cpart * part, CString * pin_name );
	void HighlightConnection( cnet * net, int ic );
	void UndrawConnection( cnet * net, int ic );
	void DrawConnection( cnet * net, int ic );
	void CleanUpConnections( cnet * net, CString * logstr=NULL );
	void CleanUpAllConnections( CString * logstr=NULL );

	// functions for segments
	int AppendSegment( cnet * net, int ic, int x, int y, int layer, CSegWidthInfo const &width, CClearanceInfo const &clearance );
	int AppendSegment( cnet * net, int ic, int x, int y, int layer, CSegWidthInfo const &width )
	{
		return AppendSegment( net, ic, x, y, layer, width, CClearanceInfo() );
	}

	int InsertSegment( cnet * net, int ic, int iseg, int x, int y, int layer,
						CConnectionWidthInfo const &width,
						CClearanceInfo const &clearance,
						int dir );
	int InsertSegment( cnet * net, int ic, int iseg, int x, int y, int layer,
						CConnectionWidthInfo const &width,
						int dir )
	{
		return InsertSegment( net, ic, iseg, x, y, layer, width, CClearanceInfo(), dir );
	}

	int RouteSegment( cnet * net, int ic, int iseg, int layer, CSegWidthInfo const &width, CClearanceInfo const &clearance );
	int RouteSegment( cnet * net, int ic, int iseg, int layer, CSegWidthInfo const &width )
	{
		return RouteSegment( net, ic, iseg, layer, width, CClearanceInfo() );
	}

	id  UnrouteSegment( cnet * net, int ic, int iseg );
	void UnrouteSegmentWithoutMerge( cnet * net, int ic, int iseg );
	id MergeUnroutedSegments( cnet * net, int ic );
	void RemoveSegment( cnet * net, int ic, int iseg, BOOL bHandleTees=FALSE );
	int ChangeSegmentLayer( cnet * net, int ic, int iseg, int layer );
	int SetSegmentWidth( cnet * net, int ic, int is, CConnectionWidthInfo const &width );
	int SetSegmentClearance( cnet * net, int ic, int is, CClearanceInfo const &clearance );
	void HighlightSegment( cnet * net, int ic, int iseg );
	int StartMovingSegment( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair, int use_third_segment );
	int StartDraggingSegment(
		CDC * pDC,
		cnet * net,
		int ic, int iseg,
		int x, int y,
		int layer1, int layer2,
		int layer_no_via,
		CConnectionWidthInfo const &width,
		int dir,
		int crosshair = 1 );

	int CancelDraggingSegment( cnet * net, int ic, int iseg );
	int StartDraggingSegmentNewVertex( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer, int w, int crosshair );
	int CancelDraggingSegmentNewVertex( cnet * net, int ic, int iseg );
	void StartDraggingStub( CDC * pDC, cnet * net, int ic, int iseg,
						int x, int y, int layer1, int w,
						int layer_no_via, int via_w, int via_hole_w,
						int crosshair, int inflection_mode );
	void CancelDraggingStub( cnet * net, int ic, int iseg );
	int CancelMovingSegment( cnet * net, int ic, int ivtx );

	// functions for vias
	int ReconcileVia( cnet * net, int ic, int ivtx );
	int ForceVia( cnet * net, int ic, int ivtx, BOOL set_areas=TRUE );
	int UnforceVia( cnet * net, int ic, int ivtx, BOOL set_areas=TRUE );
	int DrawVia( cnet * net, int ic, int iv );
	void UndrawVia( cnet * net, int ic, int iv );
	void SetViaVisible( cnet * net, int ic, int iv, BOOL visible );

	// functions for vertices
	void HighlightVertex( cnet * net, int ic, int ivtx );
	int StartDraggingVertex( CDC * pDC, cnet * net, int ic, int iseg,
						int x, int y, int cosshair = 1 );
	int CancelDraggingVertex( cnet * net, int ic, int ivtx );
	void StartDraggingEndVertex( CDC * pDC, cnet * net, int ic,
		int ivtx, int crosshair = 1 );
	void CancelDraggingEndVertex( cnet * net, int ic, int ivtx );
	void MoveEndVertex( cnet * net, int ic, int ivtx, int x, int y );
	void MoveVertex( cnet * net, int ic, int ivtx, int x, int y );
	int GetViaConnectionStatus( cnet * net, int ic, int iv, int layer );
	void GetViaPadInfo( cnet * net, int ic, int iv, int layer,
		int * pad_w, int * hole_w, int * connect_status );
	BOOL TestForHitOnVertex( cnet * net, int layer, int x, int y,
		cnet ** hit_net, int * hit_ic, int * hit_iv );

	// functions related to parts
	int RehookPartsToNet( cnet * net );
	void PartAdded( cpart * part );
	int PartMoved( cpart * part );
	int PartFootprintChanged( cpart * part );
	int PartDeleted( cpart * part );
	int PartDisconnected( cpart * part );
	void SwapPins( cpart * part1, CString * pin_name1,
						cpart * part2, CString * pin_name2 );
	void PartRefChanged( CString * old_ref_des, CString * new_ref_des );

	// functions for copper areas
	int AddArea( cnet * net, int layer, int x, int y, int hatch );
	void InsertArea( cnet * net, int iarea, int layer, int x, int y, int hatch );
	int AppendAreaCorner( cnet * net, int iarea, int x, int y, int style, BOOL bDraw=TRUE );
	int InsertAreaCorner( cnet * net, int iarea, int icorner,
		int x, int y, int style );
	void MoveAreaCorner( cnet * net, int iarea, int icorner, int x, int y );
	void HighlightAreaCorner( cnet * net, int iarea, int icorner );
	void HighlightAreaSides( cnet * net, int ia );
	CPoint GetAreaCorner( cnet * net, int iarea, int icorner );
	int CompleteArea( cnet * net, int iarea, int style );
	void SetAreaConnections();
	void SetAreaConnections( cnet * net, int iarea );
	void SetAreaConnections( cnet * net );
	void SetAreaConnections( cpart * part );
	BOOL TestPointInArea( cnet * net, int x, int y, int layer, int * iarea );
	int RemoveArea( cnet * net, int iarea );
	void SelectAreaSide( cnet * net, int iarea, int iside );
	void SelectAreaCorner( cnet * net, int iarea, int icorner );
	void SetAreaSideStyle( cnet * net, int iarea, int iside, int style );
	int StartDraggingAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair = 1 );
	int CancelDraggingAreaCorner( cnet * net, int iarea, int icorner );
	int StartDraggingInsertedAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair = 1 );
	int CancelDraggingInsertedAreaCorner( cnet * net, int iarea, int icorner );
	void RenumberAreas( cnet * net );
	int TestAreaPolygon( cnet * net, int iarea );
	int ClipAreaPolygon( cnet * net, int iarea,
		BOOL bMessageBoxArc, BOOL bMessageBoxInt, BOOL bRetainArcs=TRUE );
	int AreaPolygonModified( cnet * net, int iarea, BOOL bMessageBoxArc, BOOL bMessageBoxInt );
	int CombineAllAreasInNet( cnet * net, BOOL bMessageBox, BOOL bUseUtility );
	int TestAreaIntersections( cnet * net, int ia );
	int TestAreaIntersection( cnet * net, int ia1, int ia2 );
	int CombineAreas( cnet * net, int ia1, int ia2 );
	void ApplyClearancesToArea( cnet * net, int ia, int flags,
			int fill_clearance, int min_silkscreen_stroke_wid,
			int thermal_wid, int hole_clearance );

	// I/O  functions
	int WriteNets( CStdioFile * file );
	void ReadNets( CStdioFile * pcb_file, double read_version, int * layers=NULL );
	void ExportNetListInfo( netlist_info * nl );
	void ImportNetListInfo( netlist_info * nl, int flags, CDlgLog * log );
	void Copy( CNetList * nl );
	void RestoreConnectionsAndAreas( CNetList * old_nl, int flags, CDlgLog * log=NULL );
	void ReassignCopperLayers( int n_new_layers, int * layer );
	void ImportNetRouting( CString * name, CArray<cnode> * nodes,
		CArray<cpath> * paths, int tolerance, CDlgLog * log=NULL, BOOL bVerbose=TRUE );

	// undo functions
	undo_con * CreateConnectUndoRecord( cnet * net, int icon, BOOL set_areas=TRUE );
	undo_area * CreateAreaUndoRecord( cnet * net, int iarea, int type );
	undo_net * CreateNetUndoRecord( cnet * net );
	static void ConnectUndoCallback( int type, void * ptr, BOOL undo );
	static void AreaUndoCallback( int type, void * ptr, BOOL undo );
	static void NetUndoCallback( int type, void * ptr, BOOL undo );

	// functions for tee_IDs
	void ClearTeeIDs();
	int GetNewTeeID();
	int FindTeeID( int id );
	void RemoveTeeID( int id );
	void AddTeeID( int id );
	// functions for tees and branches
	BOOL FindTeeVertexInNet( cnet * net, int id, int * ic=NULL, int * iv=NULL );
	BOOL FindTeeVertex( int id, cnet ** net, int * ic=NULL, int * iv=NULL );
	int RemoveTee( cnet * net, int id );
	BOOL DisconnectBranch( cnet * net, int ic );
	int RemoveTeeIfNoBranches( cnet * net, int id );
	BOOL TeeViaNeeded( cnet * net, int id );
	BOOL RemoveOrphanBranches( cnet * net, int id, BOOL bRemoveSegs=FALSE );

private:
	CDisplayList * m_dlist;
	CPartList * m_plist;
	int m_layers;	// number of copper layers
	CConnectionWidthInfo  m_def_width;
	CClearanceInfo m_def_clearance;
	int m_pos_i;	// index for iterators
	POSITION m_pos[MAX_ITERATORS];	// iterators for nets
	CArray<int> m_tee;
	BOOL m_bSMT_connect;

public:
	int m_annular_ring;

	CConnectionWidthInfo const &Get_def_width() const { return m_def_width; }
	CClearanceInfo const &Get_def_clearance() const { return m_def_clearance; }
};
