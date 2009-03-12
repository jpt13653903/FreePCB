#ifndef _II_CLEARANCE_H /* [ */
#define _II_CLEARANCE_H

#include "inheritable_item.h"

// Information for clearances
class CClearanceInfo : public CInheritableInfo
{
public:
	enum EStatus
	{
        // Remember, these are negative so normal enum
        // counting order won't work here.
		E_AUTO_CALC = CInheritableInfo::E__STATUS_USER
	};

public:
	explicit CClearanceInfo(CInheritableInfo const &from) :
		CInheritableInfo(from)
	{
		*this = from;
	}

	explicit CClearanceInfo(int ca_clearance = E_USE_PARENT)
	{
		m_ca_clearance.SetItemFromInt(ca_clearance);
	}

	CClearanceInfo &operator = (CInheritableInfo const &from);
	CClearanceInfo &operator = (CClearanceInfo const &from) { operator = (static_cast<CInheritableInfo const &>(from)); return *this; }

public:
	// Direct for non-updating version
	Item m_ca_clearance;

    // Update the current value of ...
	void Update_ca_clearance();

	// Update all
	void Update()
	{
		Update_ca_clearance();
	}

	virtual Item const &GetItem(int item_id) const;

protected:
    virtual void GetItemExt(Item &item, Item const &src) const;
};

#endif /* !_II_CLEARANCE_H ] */
