#include <assert.h>

namespace msg
{
	inline void ByteBuffer::Clear(void)
	{
		m_vStorage.clear();
		m_nReadPos = m_nWritePos = 0;
	}

	template <typename T>
	inline	void ByteBuffer::Append(T value)
	{
		Append((char*)&value, sizeof(value));
	}

	inline void ByteBuffer::Append(const uint8* src, size_t cnt)
	{
		if (!cnt) return;

		assert(Size() < 10000000);

		if (m_vStorage.size() < m_nWritePos + cnt)
			m_vStorage.resize(m_nWritePos + cnt);
		memcpy(&m_vStorage[m_nWritePos], src, cnt);
		m_nWritePos += cnt;
	}

	template <typename T>
	inline void ByteBuffer::Put(size_t pos, T value)
	{
		Put(pos, (uint8*)&value, sizeof(value));
	}

	inline void ByteBuffer::Put(size_t pos, const uint8* src, size_t cnt)
	{
		assert(pos + cnt <= Size() || PrintPosError(true, pos, cnt));
		memcpy(&m_vStorage[pos], src, cnt);
	}

	inline ByteBuffer::ByteBuffer() : m_nReadPos(0), m_nWritePos(0)
	{
		m_vStorage.reserve(DEFAULT_SIZE);
	}

	inline ByteBuffer::ByteBuffer(size_t res) : m_nReadPos(0), m_nWritePos(0)
	{
		m_vStorage.reserve(res);
	}

	inline ByteBuffer::ByteBuffer(const ByteBuffer& buf) : m_nReadPos(buf.m_nReadPos),
		m_nWritePos(buf.m_nWritePos),
		m_vStorage(buf.m_vStorage)
	{
	}

	inline ByteBuffer& ByteBuffer::operator<<(bool value)
	{
		Append<int8>((int8)value);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator<<(uint8 value)
	{
		Append<uint8>(value);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator<<(uint16 value)
	{
		Append<uint16>(value);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator<<(uint32 value)
	{
		Append<uint32>(value);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator<<(uint64 value)
	{
		Append<uint64>(value);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator<<(float value)
	{
		Append<float>(value);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator<<(double value)
	{
		Append<double>(value);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator<<(const std::string& value)
	{
		Append((char*)value.c_str(), value.length());
		Append((char)0);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator<<(const char* str)
	{
		Append((char*)str, str ? strlen(str) : 0);
		Append((char)0);
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator>>(bool& value)
	{
		value = Read<char>() > 0 ? true : false;
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator>>(uint8& value)
	{
		value = Read<uint8>();
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator>>(uint16& value)
	{
		value = Read<uint16>();
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator>>(uint32& value)
	{
		value = Read<uint32>();
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator>>(uint64& value)
	{
		value = Read<uint64>();
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator>>(float& value)
	{
		value = Read<float>();
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator>>(double& value)
	{
		value = Read<double>();
		return *this;
	}

	inline ByteBuffer& ByteBuffer::operator>>(std::string& value)
	{
		value.clear();
		while (true)
		{
			char c = Read<char>();
			if (c == 0)
				break;
			value += c;
		}
		return *this;
	}

	inline uint8 ByteBuffer::operator[](size_t pos)
	{
		return Read<char>(pos);
	}

	inline size_t ByteBuffer::ReadPos()
	{
		return m_nReadPos;
	};

	inline size_t ByteBuffer::SetReadPos(size_t rpos)
	{
		m_nReadPos = rpos;
		return m_nReadPos;
	};

	inline size_t ByteBuffer::WritePos()
	{
		return m_nWritePos;
	}

	inline size_t ByteBuffer::SetWritePos(size_t wpos)
	{
		m_nWritePos = wpos;
		return m_nWritePos;
	}

	template <typename T>
	inline T ByteBuffer::Read()
	{
		T r = Read<T>(m_nReadPos);
		m_nReadPos += sizeof(T);
		return r;
	}

	template <typename T>
	inline T ByteBuffer::Read(size_t pos) const
	{
		assert(pos + sizeof(T) <= Size() || PrintPosError(false, pos, sizeof(T)));
		return *((T*)&m_vStorage[pos]);
	}

	inline void ByteBuffer::Read(unsigned char* dest, size_t len)
	{
		if (m_nReadPos + len <= Size())
		{
			memcpy(dest, &m_vStorage[m_nReadPos], len);
		}
		else
		{
			throw error();
		}
		m_nReadPos += len;
	}

	inline size_t ByteBuffer::Size(void) const
	{
		return m_vStorage.size();
	};

	inline void ByteBuffer::Resize(size_t newsize)
	{
		m_vStorage.resize(newsize);
		m_nReadPos = 0;
		m_nWritePos = Size();
	}

	inline void ByteBuffer::Reserve(size_t ressize)
	{
		if (ressize > Size())
			m_vStorage.reserve(ressize);
	}

	inline void ByteBuffer::PrintStorage()
	{
		printf("STORAGE_SIZE: %u\n", Size());
		for (uint32 i = 0; i < Size(); i++)
			printf("%u - ", Read<uint8>(i));
		printf("\n");
	}

	inline void ByteBuffer::TextLike()
	{
		printf("STORAGE_SIZE: %u\n", Size());
		for (uint32 i = 0; i < Size(); i++)
			printf("%c", Read<uint8>(i));
		printf("\n");
	}

	inline void ByteBuffer::Hexlike()
	{
		uint32 j = 1, k = 1;
		printf("STORAGE_SIZE: %u\n", Size());
		for (uint32 i = 0; i < Size(); i++)
		{
			if ((i == (j * 8)) && ((i != (k * 16))))
			{
				if (Read<uint8>(i) < 0x0F)
				{
					printf("| 0%X ", Read<uint8>(i));
				}
				else
				{
					printf("| %X ", Read<uint8>(i));
				}

				j++;
			}
			else if (i == (k * 16))
			{
				if (Read<uint8>(i) < 0x0F)
				{
					printf("\n0%X ", Read<uint8>(i));
				}
				else
				{
					printf("\n%X ", Read<uint8>(i));
				}

				k++;
				j++;
			}
			else
			{
				if (Read<uint8>(i) < 0x0F)
				{
					printf("0%X ", Read<uint8>(i));
				}
				else
				{
					printf("%X ", Read<uint8>(i));
				}
			}
		}
		printf("\n");
	}

	inline bool ByteBuffer::PrintPosError(bool add, size_t pos, size_t esize) const
	{
		printf("ERROR: Attempt %s in ByteBuffer (pos: %u size: %u) value with size: %u", (add ? "put" : "get"), pos, Size(), esize);

		// assert must fail after function call
		return false;
	}

	inline void ByteBuffer::Append(const std::string& str)
	{
		Append((uint8*)str.c_str(), str.size() + 1);
	}

	inline void ByteBuffer::Append(const char* src, size_t cnt)
	{
		return Append((const uint8*)src, cnt);
	}

	inline void ByteBuffer::Append(const ByteBuffer& buffer)
	{
		if (buffer.Size()) Append(buffer.Contents(), buffer.Size());
	}
}