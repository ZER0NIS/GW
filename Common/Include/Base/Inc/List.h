#ifndef _FILE_LIST_H_
#define _FILE_LIST_H_

#include "../TDContainer.h"

namespace sbase
{
	namespace stl
	{
		template<class _T>
		class List :public TDContainer<_T, sbase::CCriticalSection, std::list >
		{
		public:
			using typename TDContainer<_T, sbase::CCriticalSection, std::list>::const_itor;
			using typename TDContainer<_T, sbase::CCriticalSection, std::list>::itor;
			virtual void Add(const _T& item);
			virtual _T Next();
			virtual void Clear();
			virtual size_t Size();
			virtual void  Erase(itor& item);
			itor Find(const _T& item)
			{
				itor iter = std::find(m_Container.begin(), m_Container.end(), item);
				return iter;
			}
			itor Begin()
			{
				itor iter = m_Container.begin();
				return iter;
			}
			itor End()
			{
				itor iter = m_Container.end();
				return iter;
			}
		protected:

		private:
		};

		template<class _T>
		void  List<_T>::Add(const _T& item)
		{
			sbase::CSingleLock  Lock(&m_Locker);
			m_Container.push_back(item);
		}

		template<class _T>
		_T List<_T>::Next()
		{
			sbase::CSingleLock  Lock(&m_Locker);
			_T Elem = m_Container.front();
			m_Container.pop_front();
			return Elem;
		}

		template<class _T>
		void List<_T>::Clear()
		{
			sbase::CSingleLock  Lock(&m_Locker);
			m_Container.clear();
		}

		template<class _T>
		size_t List<_T>::Size() const
		{
			sbase::CSingleLock  Lock(&m_Locker);
			return m_Container.size();
		}

		template<class _T>
		void List<_T>::Erase(itor& item)
		{
			sbase::CSingleLock  Lock(&m_Locker);
			m_Container.erase(item);
		}
	}
}

#endif