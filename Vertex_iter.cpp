#include "stdafx.h"
#include "netlist.h"

CVertexIterator::CVertexIterator( cnet * net, int ic, int ivtx )
	: m_net(net)
	, m_ic(ic)
	, m_ivtx(ivtx)
	, m_tee_ID(0)
	, m_idx(-1)
	, m_icc(ic)
	, m_icvtx(ivtx)
{
	ASSERT( m_ic < m_net->nconnects );

	cconnect * c = &m_net->connect[m_ic];

	ASSERT( m_ivtx <= c->nsegs );
}

cconnect       *CVertexIterator::getcur_connect()       { return &m_net->connect[m_icc]; }
cconnect const *CVertexIterator::getcur_connect() const { return &m_net->connect[m_icc]; }

cvertex *CVertexIterator::GetFirst()
{
	m_idx = -1;
	return GetNext();
}

cvertex *CVertexIterator::GetNext()
{
	if ( m_net == NULL ) return NULL;

	cconnect * c;
	cvertex * vtx;

	if( m_idx < 0 )
	{
		// First return vertex defined by is & ivtx
		m_icc   = m_ic;
		m_icvtx = m_ivtx;

		if( m_icc >= m_net->nconnects ) return NULL;

		c = &m_net->connect[m_icc];
		if( m_icvtx > c->nsegs ) return NULL; // > is OK, >= not OK

		vtx = &c->vtx[m_icvtx];

		m_tee_ID = vtx->tee_ID;

		// Next state
		m_idx = 0;
	}
	else
	{
		// Next, return other vertexes with matching tee ID

		// Check for no T's
		if( m_tee_ID == 0) return NULL;

		while( m_idx < m_net->nconnects )
		{
			m_icc = m_idx++;
			c = &m_net->connect[m_icc];

			int i;
			if( c->end_pin == cconnect::NO_END )
			{
				// Stub/branch - test last vertex
				i = c->nsegs;
			}
			else
			{
				// Regular connection - test all vertexes
				i = 1;
			}

			for ( ; i <= c->nsegs ; i++ )
			{
				vtx = &c->vtx[i];
				if( vtx->tee_ID == m_tee_ID )
				{
					// Found matching vertex

					// Update vertex info
					m_icvtx = i;

					return vtx;
				}
			}
		}

		// No vertexes matching m_tee_ID found
		vtx = NULL;
	}

	return vtx;
}
