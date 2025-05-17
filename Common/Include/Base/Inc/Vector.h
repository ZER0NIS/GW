#ifndef _FILE_VECTOR_H_
#define _FILE_VECTOR_H_

#include "../TDContainer.h"
#include <algorithm>
#include <vector>
#include <list>
#include "./SyncObjs.h"

namespace sbase
{
	namespace stl
	{
		template<class _T>
		class Vector :public TDContainer<_T, sbase::CCriticalSection, std::vector >
		{
		public:
			using typename TDContainer<_T, sbase::CCriticalSection, std::vector>::const_itor;
			using typename TDContainer<_T, sbase::CCriticalSection, std::vector>::itor;
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
		void  Vector<_T>::Add(const _T& item)
		{
			sbase::CSingleLock  Lock(&m_Locker);
			m_Container.push_back(item);
		}

		template<class _T>
		_T Vector<_T>::Next()
		{
			sbase::CSingleLock  Lock(&m_Locker);
			_T Elem = m_Container.front();
			Erase(Begin());
			return Elem;
		}

		template<class _T>
		void Vector<_T>::Clear()
		{
			sbase::CSingleLock  Lock(&m_Locker);
			m_Container.clear();
		}

		template<class _T>
		size_t Vector<_T>::Size()
		{
			sbase::CSingleLock  Lock(&m_Locker);
			return m_Container.size();
		}

		template<class _T>
		void Vector<_T>::Erase(itor& item)
		{
			sbase::CSingleLock  Lock(&m_Locker);
			m_Container.erase(item);
		}
	}
}

#endif