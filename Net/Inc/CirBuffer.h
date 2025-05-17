//========================================================
//
//    文件名称：CirBuffer.h
//    摘   要： CirBuffer封装类
//    说    明： TCP粘包处理
//
//========================================================
#ifndef _FILE_CIRBUFFER_H
#define _FILE_CIRBUFFER_H

class CircularBuffer
{
public:
    CircularBuffer(size_t size);
    ~CircularBuffer();
    bool Write(const char *p,size_t l);
    bool Read(char *dest,size_t l);
    bool SoftRead(char *dest, size_t l);
    bool Remove(size_t l);
    size_t GetLength() { return m_q; }
    char *GetStart() { return buf + m_b; }
    size_t GetL() { return (m_b + m_q > m_max) ? m_max - m_b : m_q; }
    size_t Space() { return m_max - m_q; }
    unsigned long ByteCounter() { return m_count; }

private:
    CircularBuffer(const CircularBuffer& s)  {}
    CircularBuffer& operator=(const CircularBuffer& ) { return *this; }
    char *buf;
    size_t m_max;
    size_t m_q;
    size_t m_b;
    size_t m_t;
    unsigned long m_count;
};
#endif                                          
