#ifndef _FILE_CIRBUFFER_CLIENT_H
#define _FILE_CIRBUFFER_CLIENT_H

#include <mutex>

namespace cnet
{
	class CircularBuffer
	{
	public:

		CircularBuffer(size_t size);
		~CircularBuffer();
		bool Write(const char* p, size_t l);
		bool Read(char* dest, size_t l);
		bool SoftRead(char* dest, size_t l);
		bool Remove(size_t l);
		size_t GetLength() { return m_q; }
		char* GetStart() { return buf + m_b; }
		size_t GetL() { return (m_b + m_q > m_max) ? m_max - m_b : m_q; }
		size_t Space() { return m_max - m_q; }
		void Clear();
		unsigned long ByteCounter() { return m_count; }
		void Initnalize();

	private:
		CircularBuffer(const CircularBuffer&) = delete;
		CircularBuffer& operator=(const CircularBuffer&) = delete;

		char* buf;
		size_t m_max;
		size_t m_q;
		size_t m_b;
		size_t m_t;
		unsigned long m_count;

		std::mutex m_Mutex;
	};
}

#endif
