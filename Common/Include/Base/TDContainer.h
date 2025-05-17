#ifndef _FILE_LIST_H_
#define _FILE_LIST_H_

#include <memory>
#include "../Inc/SyncObjs.h"

namespace sbase
{
	namespace stl
	{
		template<
			class _T,
			class _Locker,
			template<typename _T, typename = std::allocator<_T> >
		class MCONTAINER
		>
		class TDContainer
		{
		public:
			typedef typename MCONTAINER<_T>::const_iterator  const_itor;
			typedef typename MCONTAINER<_T>::iterator  itor;
			virtual void Add(const _T& item) = 0;
			virtual _T Next() = 0;
			virtual void Clear() = 0;
			virtual size_t Size() = 0;
			virtual void  Erase(itor& item) = 0;
		protected:
			MCONTAINER<_T>  m_Container;
			_Locker  m_Locker;
		private:
		};
	}
}
#endif
