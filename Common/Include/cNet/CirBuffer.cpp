//========================================================
//
//    Copyright (c) 2007,欢乐连线工作室
//    All rights reserved.
//
//    文件名称：CirBuffer.cpp
//    摘  要：  CirBuffer封装类
//
//    当前版本：1.01
//    作   者： 李锋军
//    完成日期：2007-12-15
//    说   明： TCP粘包处理
//
//========================================================
#ifdef _WIN32
#pragma warning(disable:4786)
#endif

#include "stdafx.h"
#include <string>
#include "CirBuffer.h"

namespace cnet
{
	CircularBuffer::CircularBuffer(size_t size) :buf(new char[size]), m_max(size), m_q(0), m_b(0), m_t(0), m_count(0)
	{
		Initnalize();
	}

	CircularBuffer::~CircularBuffer()
	{
		delete[] buf;
	}

	void CircularBuffer::Initnalize()
	{
		Clear();
	};

	void CircularBuffer::Clear()
	{
		m_q = 0;
		m_b = 0;
		m_t = 0;
		m_count = 0;
	}

	bool CircularBuffer::Write(const char* s, size_t l)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_q + l > m_max)
		{
			SYSTEMTIME stLocalTime;
			GetLocalTime(&stLocalTime);
			printf("cnet buffer overflow，%d,%d,[%d-%d-%d %d:%d:%d:%d]\n", m_q, l,
				stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
				stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
				stLocalTime.wMilliseconds);

			return false;
		}
		m_count += (unsigned long)l;
		if (m_t + l > m_max)
		{
			size_t l1 = m_max - m_t;
			memcpy(buf + m_t, s, l1);
			memcpy(buf, s + l1, l - l1);
			m_t = l - l1;
			m_q += l;
		}
		else
		{
			memcpy(buf + m_t, s, l);
			m_t += l;
			if (m_t >= m_max)
				m_t -= m_max;
			m_q += l;
		}

		// 	::LeaveCriticalSection(&m_CriSec);
		return true;
	}

	bool CircularBuffer::Read(char* s, size_t l)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (l > m_q)
		{
			// 	::LeaveCriticalSection(&m_CriSec);
			   //没有足够的缓冲
			return false;
		}
		if (m_b + l > m_max)
		{
			size_t l1 = m_max - m_b;
			if (s)
			{
				memcpy(s, buf + m_b, l1);
				memcpy(s + l1, buf, l - l1);
			}
			m_b = l - l1;
			m_q -= l;
		}
		else
		{
			if (s)
			{
				memcpy(s, buf + m_b, l);
			}
			m_b += l;
			if (m_b >= m_max)
				m_b -= m_max;
			m_q -= l;
		}
		if (!m_q)
		{
			m_b = m_t = 0;
		}

		// 	::LeaveCriticalSection(&m_CriSec);

		return true;
	}

	bool CircularBuffer::SoftRead(char* s, size_t l)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (l > m_q)
		{
			// 	::LeaveCriticalSection(&m_CriSec);
			return false;
		}
		if (m_b + l > m_max)
		{
			size_t l1 = m_max - m_b;
			if (s)
			{
				memcpy(s, buf + m_b, l1);
				memcpy(s + l1, buf, l - l1);
			}
		}
		else
		{
			if (s)
			{
				memcpy(s, buf + m_b, l);
			}
		}

		// 	::LeaveCriticalSection(&m_CriSec);
		return true;
	}

	bool CircularBuffer::Remove(size_t l)
	{
		return Read(NULL, l);
	}
}