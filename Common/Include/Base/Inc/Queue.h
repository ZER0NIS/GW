#ifndef _FILE_QUEUE_H_
#define _FILE_QUEUE_H_

#include "../TDContainer.h"

namespace sbase
{
	namespace stl
	{
		template<class _T>
		class Queue :public TDContainer<_T, sbase::CCriticalSection, std::deque>
		{
		public:
 virtual void Add(const _T& item);
 virtual _T Next() { return PopFront(); }
 _T PopFront();
 virtual void Clear();
 virtual size_t Size();
 virtual void  Erase(itor& item);
 itor Find(const _T& item)
 {
 	sbase::CSingleLock  Lock(&m_Locker);
 	itor iter = std::find(m_Container.begin(), m_Container.end(), item);
 	return iter;
 }
 itor Begin()
 {
 	sbase::CSingleLock  Lock(&m_Locker);
 	itor iter = m_Container.begin();
 	return iter;
 }
 itor End()
 {
 	sbase::CSingleLock  Lock(&m_Locker);
 	itor iter = m_Container.end();
 	return iter;
 }
		protected:

		private:
		};

		template<class _T>
		void  Queue<_T>::Add(const _T& item)
		{
 sbase::CSingleLock  Lock(&m_Locker);
 m_Container.push_back(item);
		}

		template<class _T>
		_T Queue<_T>::PopFront()
		{
 sbase::CSingleLock  Lock(&m_Locker);
 _T Temp = m_Container.front();
 m_Container.pop_front();
 return Temp;
		}

		template<class _T>
		void Queue<_T>::Clear()
		{
 sbase::CSingleLock  Lock(&m_Locker);
 m_Container.clear();
		}

		template<class _T>
		size_t Queue<_T>::Size() const
		{
 sbase::CSingleLock  Lock(&m_Locker);
 return m_Container.size();
		}

		template<class _T>
		void Queue<_T>::Erase(itor& item)
		{
 sbase::CSingleLock  Lock(&m_Locker);
 m_Container.erase(item);
		}
	}
}
#endif