//////////////////////////////////////////////////////////////////////////
//简单智能指针 不支持copy和=。该模板需要Release()函数支持
//该类的指针可以为NULL。
//注意：包含该对象的类必须是个不能copy和=的类

namespace sbase
{
	template<typename T>
	class POINT_NO_RELEASE : public T
	{
		virtual ULONG Release()	= 0;         //prevent call this function
	private:
		POINT_NO_RELEASE(){}
		virtual ~POINT_NO_RELEASE(){}
	};


	template<typename T/*, typename T2*/>
	class CAutoPtr
	{
	public:
		CAutoPtr():m_ptr(NULL)	{}
		CAutoPtr(T* ptr) : m_ptr(ptr)		{}
		~CAutoPtr()	{ Release(); }

		CAutoPtr& operator=(T* ptr)	{ if (m_ptr && m_ptr != ptr) Release(); m_ptr = ptr; return *this; }
		T*	pop()	{ T* p = m_ptr; m_ptr = NULL; return p; }

	private:
		CAutoPtr& operator=(const CAutoPtr& ptr);
		CAutoPtr(const CAutoPtr&);

	public:
	//	T* New(DWORD id)	{ Release(); m_ptr = T2::CreateNew(id); ASSERT(m_ptr); return m_ptr; }
		ULONG Release()	{ if (m_ptr) m_ptr->Release(); m_ptr = NULL; return 0; }

	public:
		operator T*()	{ return m_ptr; }
		POINT_NO_RELEASE<T>*	operator->()	{ ASSERT(m_ptr); return static_cast<POINT_NO_RELEASE<T>* >(m_ptr); }

		operator T*()	const	{ return m_ptr; }
		POINT_NO_RELEASE<T>*	operator->()const	{ ASSERT(m_ptr); return static_cast<POINT_NO_RELEASE<T>* >(m_ptr); }


	protected:
		T*	m_ptr;
	};
}