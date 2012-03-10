#ifndef _INHERITABLE_ITEM_H /* [ */
#define _INHERITABLE_ITEM_H

// Inheritable information
class CInheritableInfo
{
	// Parent points to self if there is no parent.  This occurs if:
	//	1) Parent not yet assigned
	//  2) This is the end of the item list
	CInheritableInfo const *m_pParent;

public:
	enum EStatus
	{
		E_USE_VAL    =  0,
		E_UNDEF      = -1,
		E_USE_PARENT = -2,

		// First available status for derived class
		E__STATUS_USER = -3
	};

	//-------------------------------------------------------------------------
	//
	// Data item - value with status
	//
	// Status allows items to have special behavior.  The base behaviors
	// are found in EStatus:
	//
	// 1) E_UNDEF      - Undefined
	//                      Assignment A = UNDEF results in A being
	//                      unchanged, not being set to UNDEF (use Undef()
	//                      for this purpose).  This is done to allow
	//                      piecemeal assignment of a set of Items:
	//                          A contains four Items: a,b,c,d
	//                          A = B; B contains defined Items a,b,c
	//                          A = C; C contains defined Items a,d
	//
	//                          Resulting A contains C.a, B.b, B.c, C.d
	//
	// 2) E_USE_VAL    - Simply use the value in m_val
	//
	// 3) E_USE_PARENT - Use the value contained in the parent CInheritableInfo
	struct Item
	{
	protected:
		// Directly set item from int - does not use virtual assignment
		void _SetItemFromInt(int val_status);

		// Virtual assignment operator.  Allows enhanced behavior
		// on assignment (e.g. protection against illegal modes).
		virtual void assign_from(Item const &from);

	public: // Data
		short int m_status;
		int       m_val;

	public: // Constructor/Destructor
		Item()
		{
			Undef();
		}

		Item(Item const &from)
		{
			// Direct assignment
			// Can't use virtual assignment in ctor
			m_status = from.m_status;
			m_val    = from.m_val;
		}

		explicit Item(int val_status)
		{
			// Must clear before set in case val_status
			// is undef (no assignment is done)
			Undef();

			// Direct assignment
			// Can't use virtual assignment in ctor
			_SetItemFromInt(val_status);
		}

		virtual ~Item()
		{
			Undef();
		}

	public: // operators
		// Assignment
		Item &operator = (Item const &from)
		{
			assign_from(from);

			return *this;
		}

		// To/From int --------------------------
		Item &operator = (int val_status)
		{
			Item temp(val_status);

			// Invoke the virtual assignment operator
			assign_from(temp);

			return *this;
		}

		operator int () const
		{
			return GetItemAsInt();
		}

		// Comparison ---------------------------
		int operator >  (Item const &comp) const;
		int operator == (Item const &comp) const;
		int operator <  (Item const &comp) const { return !operator >(comp) && !operator ==(comp); }

	public:
		// Set/Get as int:
		//   negative values map to EStatus
		//   zero & positive values count as values - i.e. actual values must be >= 0
		void SetItemFromInt(int val_status)
		{
			Item temp(val_status);

			// Use virtual assignment operator
			assign_from(temp);
		}

		int GetItemAsInt(void) const
		{
			return (m_status == E_USE_VAL) ? m_val : m_status;
		}

	public: // Functions
		// is Item defined?
		int isDefined(void) const
		{
			return m_status != E_UNDEF;
		}

		// Force Item to UNDEF
		void Undef(void)
		{
			m_status = E_UNDEF;
			m_val    = 0;
		}

		void OnRemoveParent(void)
		{
			if( m_status == E_USE_PARENT )
			{
				m_status = E_USE_VAL;
			}
		}

	public: // UNDEF item
		static Item const UNDEF;
	};

public:	// construction and copying/assignment
	CInheritableInfo() : m_pParent(this)
	{
	}

	CInheritableInfo(CInheritableInfo const &from)
	{
		m_pParent = from.hasParent() ? from.m_pParent : this;
	}

	CInheritableInfo &operator = (CInheritableInfo const &from)
	{
		// Copy parent only if current parent is uninitialized
		// and from's parent is initialized.
		if( !hasParent() && from.hasParent() ) m_pParent = from.m_pParent;

		//don't use this - m_pParent = from.hasParent() ? from.m_pParent : this;
		//It assigns the parent too freely.

		return *this;
	}

public:
	void SetParent(CInheritableInfo const &pParent) { SetParent(&pParent); }
	void SetParent(CInheritableInfo const *pParent = NULL);

	CInheritableInfo const *GetParent() const { return m_pParent; }

	// Acual item is returned. "item" is updated with the status
	// of the initial item_id and value of the most derived item_id.
	Item const *UpdateItem(int item_id, Item &item) const;

	// Returns the item specified
	virtual Item const &GetItem(int item_id) const;

	// Do I have a parent?
	int hasParent(void) const { return m_pParent != this; }

protected:
	virtual void GetItemExt(Item &item, Item const &src) const;

	// Called when parent is removed.  A derived class can
	// use this function to cleanup Items which may have
	// a E_USE_PARENT status.
	virtual void OnRemoveParent( CInheritableInfo const *pOldParent ) {}
};

#endif /* !_INHERITABLE_ITEM_H ] */
