#ifndef _FILE_BYTEBUFFER_H_
#define _FILE_BYTEBUFFER_H_

#include <vector>
#include <iostream>
#include "Types.h"

namespace sbase
{
	namespace msg
	{
		class error
		{
		};

		class ByteBuffer
		{
		public:
 const static size_t DEFAULT_SIZE = 0x1000;

 ByteBuffer();

 ByteBuffer(size_t res);

 ByteBuffer(const ByteBuffer& buf);

 void Clear(void);

 template <typename T>
 void Append(T value);

 template <typename T>
 void Put(size_t pos, T value);

 ByteBuffer& operator<<(bool value);

 ByteBuffer& operator<<(uint8 value);

 ByteBuffer& operator<<(uint16 value);

 ByteBuffer& operator<<(uint32 value);

 ByteBuffer& operator<<(uint64 value);

 ByteBuffer& operator<<(float value);

 ByteBuffer& operator<<(double value);

 ByteBuffer& operator<<(const std::string& value);

 ByteBuffer& operator<<(const char* str);

 ByteBuffer& operator>>(bool& value);

 ByteBuffer& operator>>(uint8& value);

 ByteBuffer& operator>>(uint16& value);

 ByteBuffer& operator>>(uint32& value);

 ByteBuffer& operator>>(uint64& value);

 ByteBuffer& operator>>(float& value);

 ByteBuffer& operator>>(double& value);

 ByteBuffer& operator>>(std::string& value);

 uint8 operator[](size_t pos);

 size_t ReadPos();

 size_t SetReadPos(size_t rpos);

 size_t WritePos();

 size_t SetWritePos(size_t wpos);

 template <typename T>
 T Read();

 template <typename T>
 T Read(size_t pos) const;

 void Read(unsigned char* dest, size_t len);

 const uint8* Contents(void) const { return &m_vStorage[0]; };

 size_t Size(void) const;

 void Resize(size_t newsize);

 void Reserve(size_t ressize);

 void PrintStorage();

 void TextLike();

 void Hexlike();

		protected:

 bool PrintPosError(bool add, size_t pos, size_t esize) const;

 size_t m_nReadPos, m_nWritePos;

 std::vector<uint8> m_vStorage;

		private:

 void Append(const std::string& str);

 void Append(const char* src, size_t cnt);

 void Append(const uint8* src, size_t cnt);

 void Append(const ByteBuffer& buffer);

 void Put(size_t pos, const uint8* src, size_t cnt);
		};
	}

#include "ByteBuffer.inl"
}

#endif
