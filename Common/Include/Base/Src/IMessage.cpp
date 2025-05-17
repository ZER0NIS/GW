#include "..\inc\IMessage.h"

namespace sbase
{
	DEFINE_FIXEDSIZE_ALLOCATOR(IMessage, 33, CMemoryPool::GROW_FAST);

	IMessage::IMessage(void)
	{
		ZeroMemory(bufMsg, sizeof(bufMsg));
		usFrom = 0;
		usTo = NULL;
	}

	IMessage::IMessage(int size, int type, const void* buf, int from, void* to)
		: usFrom((USHORT)from), usTo(to)
	{
		ZeroMemory(bufMsg, sizeof(bufMsg));
		ASSERT(size >= 2 * sizeof(WORD) && size <= MAX_MSGSIZE);

		usMsgSize = (WORD)size;
		usMsgType = (WORD)type;

		if (buf)
			memcpy(bufMsg + 2 * sizeof(WORD), buf, size - 2 * sizeof(WORD));
	}

	IMessage::IMessage(const IMessage& rhs)
	{
		memcpy(bufMsg, rhs.bufMsg, sizeof(bufMsg));
		usFrom = rhs.usFrom;
		usTo = rhs.usTo;
	}

	USHORT IMessage::Release()
	{
		delete this;
		return 0;
	}
}