#include "stdafx.h"

#include "inheritable_item.h"

CInheritableInfo::Item const CInheritableInfo::Item::UNDEF;

// Comparison operators -------------------------------------------------------
int CInheritableInfo::Item::operator > (Item const &comp) const
{
	if (m_status != E_USE_VAL)
	{
		if (comp.m_status != E_USE_VAL) return m_status > comp.m_status;
		else                            return 0;
	}
	else
	{
		if (comp.m_status != E_USE_VAL) return 1;
		else                            return m_val > comp.m_val;
	}
}

int CInheritableInfo::Item::operator == (Item const &comp) const
{
	if (m_status != E_USE_VAL)
	{
		if (comp.m_status != E_USE_VAL) return m_status == comp.m_status;
		else                            return 0;
	}
	else
	{
		if (comp.m_status != E_USE_VAL) return 0;
		else                            return m_val == comp.m_val;
	}
}


// ----------------------------------------------------------------------------
// Set item from int:
//   negative values map to EStatus
//   zero & positive values count as values
void CInheritableInfo::Item::SetItemFromInt(int val_status)
{
	if (val_status < 0)
	{
		if (val_status != E_UNDEF)
		{
			// Defined data - assign
			m_status = val_status;
			m_val    = 0;
		}
		else
		{
			// Don't assign to <unassigned>
		}
	}
	else
	{
		m_status = E_USE_VAL;
		m_val    = val_status;
	}
}


// ----------------------------------------------------------------------------
// Update val/status of item with the item specified by item_id.
//
CInheritableInfo::Item const *CInheritableInfo::UpdateItem(int item_id, Item &item) const
{
	CInheritableInfo const *ci = this;
	Item const *pSrcItem = &ci->GetItem(item_id);

	item.m_status = pSrcItem->m_status;

	do
	{
		switch( pSrcItem->m_status )
		{
		case E_USE_VAL:
			item.m_val = pSrcItem->m_val;
			goto Item_OK;

		default:
			GetItemExt(item, *pSrcItem);
			goto Item_OK;

		case E_USE_PARENT:
		{
			CInheritableInfo const *next_ci = ci->m_pParent;
			if (next_ci != ci)
			{
				// Got a valid parent - loop to handle parent
				ci = next_ci;

				pSrcItem = &ci->GetItem(item_id);
			}
			else
			{
				// Invalid parent - force break from loop
				ci = NULL;
				break;
			}
		}
		break;

		case E_UNDEF:
			// Invalid data - force break from loop
			ci = NULL;
			break;
		}
	}
	while( ci != NULL );

	// Any bad/illegal info detected - mark item as undefined
	item.Undef();

Item_OK:
	return pSrcItem;
}


// ----------------------------------------------------------------------------
// Default GetItem function
//
// Returns: UNDEFINED item
//
CInheritableInfo::Item const &CInheritableInfo::GetItem(int item_id) const
{
	return Item::UNDEF;
}


// ----------------------------------------------------------------------------
// Default Extended item type function
//
// Sets item to UNDEFINED.
//
void CInheritableInfo::GetItemExt(Item &item, Item const &src) const
{
	item.Undef();
}
