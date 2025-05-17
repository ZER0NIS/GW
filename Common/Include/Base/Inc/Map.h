#ifndef _FILE_MAP_H_
#define _FILE_MAP_H_

#include "../TDContainer.h"

namespace sbase
{
	namespace stl
	{
		template<class _Key, class _Value, class _Locker = sbase::CCriticalSection >
		class Map
		{
		public:

 typedef _Key   key;
 typedef _Value type;
 typedef  typename std::map<_Key, _Value>::iterator  itor;

 void Clear();
 size_t Size() const;
 void  Erase(const itor& item);

 itor Find(const key& keytype)
 {
 	sbase::CSingleLock  Lock(&m_Locker);
 	itor iter = m_Container.find(keytype);
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

 type& operator[](const key& Key)
 {
 	return m_Container[Key];
 }

		protected:

 std::map<_Key, _Value>    m_Container;
 _Locker  m_Locker;
		private:
		};

		template<class _Key, class _Value, class _Locker>
		void Map<_Key, _Value, _Locker>::Clear()
		{
 sbase::CSingleLock  Lock(&m_Locker);
 m_Container.clear();
		}

		template<class _Key, class _Value, class _Locker>
		size_t Map<_Key, _Value, _Locker>::Size()	const
		{
 sbase::CSingleLock  Lock(&m_Locker);
 return m_Container.size();
		}

		template<class _Key, class _Value, class _Locker>
		void Map<_Key, _Value, _Locker>::Erase(const itor& item)
		{
 sbase::CSingleLock  Lock(&m_Locker);
 m_Container.erase(item);
		}
	}
}
#endif