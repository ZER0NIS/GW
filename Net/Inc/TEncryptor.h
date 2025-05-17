#ifndef _ENCRYPTOR_H_
#define _ENCRYPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ISocket.h"
//#include "../../Lib_Base/inc/Heap.h"


namespace net
{
	// <a1,  b1,   c1,   fst1, a2,   b2,   c2,   fst2>
	// 0xEE, 0x17, 0x33, 0x50, 0x82, 0x23, 0x61, 0x33

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// TEncryptClient
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned char a1, unsigned char b1, unsigned char c1, unsigned char fst1, 
		unsigned char a2, unsigned char b2, unsigned char c2, unsigned char fst2>
	class TEncryptClient : public IEncryptor
	{
	public:
		TEncryptClient()	{ this->Init(); }
		//TEncryptClient(TEncryptClient & cEncrypt);

		// Interface
	public:
		virtual void ChangeCode	(DWORD dwData);
		virtual void ChangeCode	(const char* pszKey);

		virtual void Encrypt		(unsigned char * bufMsg, int nLen);
		virtual void Decrypt		(unsigned char* buf, int nLen);
		virtual void Rencrypt	(unsigned char* buf, int nLen)		{ return; }

		virtual IEncryptor*		Duplicate	(void)  		{ return new TEncryptClient; }
		virtual unsigned long	Release		(void)  		{ delete this; return 0; }

	protected:
		void Init		(void);
		int 	m_nPos1;
		int 	m_nPos2;

		// EncryptCode
		unsigned char m_bufEncrypt1[256];
		unsigned char m_bufEncrypt2[256];

		// heap manage
	public:
		//MYHEAP_DECLARATION(s_heap)
	};

	// Init
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template <unsigned char a1, unsigned char b1, unsigned char c1, unsigned char fst1, 
		unsigned char a2, unsigned char b2, unsigned char c2, unsigned char fst2>
		inline void 
		TEncryptClient<a1, b1, c1, fst1, a2, b2, c2, fst2>::Init(void)
	{
		m_nPos1 = m_nPos2 = 0;

		try{
 unsigned char	nCode = fst1;
 for(int i = 0; i < 256; i++)
 {
 	m_bufEncrypt1[i] = nCode;
 	//printf("%02X ", nCode);
 	nCode = (a1*nCode*nCode + b1*nCode + c1) % 256;
 }
 //printf("[%02X]\n", nCode);
 assert(nCode == fst1);

 nCode = fst2;
 for(int i = 0; i < 256; i++)
 {
 	m_bufEncrypt2[i] = nCode;
 	nCode = (a2*nCode*nCode + b2*nCode + c2) % 256;
 }
 assert(nCode == fst2);
		}catch(...){ sbase::LogSave("Net", "Encryptor Init fail."); }
	}


	// Encrypt
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template <unsigned char a1, unsigned char b1, unsigned char c1, unsigned char fst1, 
		unsigned char a2, unsigned char b2, unsigned char c2, unsigned char fst2>
		inline void 
		TEncryptClient<a1, b1, c1, fst1, a2, b2, c2, fst2>::Encrypt(unsigned char * bufMsg, int nLen)
	{
		bool bMove = true;
		try{
 int		nOldPos1 = m_nPos1;
 int		nOldPos2 = m_nPos2;
 for(int i = 0; i < nLen; i++)
 {
 	bufMsg[i] ^= m_bufEncrypt1[1];//[m_nPos1];
 	bufMsg[i] ^= m_bufEncrypt2[1];//[m_nPos2];
 	if(++m_nPos1 >= 256)
 	{
 		m_nPos1 = 0;

 	}
 	if(++m_nPos2 >= 256)
 		m_nPos2 = 0;

 	assert(m_nPos1 >=0 && m_nPos1 < 256);
 	assert(m_nPos2 >=0 && m_nPos2 < 256);
 }
 
 if(!bMove)
 {
 	// 恢复指针
 	m_nPos1 = nOldPos1;
 	m_nPos2 = nOldPos2;
 }
		}catch(...){ sbase::LogSave("Net", "Encryptor Encrypt fail."); }
	}

	// Decrypt
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template <unsigned char a1, unsigned char b1, unsigned char c1, unsigned char fst1, 
 unsigned char a2, unsigned char b2, unsigned char c2, unsigned char fst2>
	inline void 
	TEncryptClient<a1, b1, c1, fst1, a2, b2, c2, fst2>::Decrypt(unsigned char * bufMsg, int nLen)
	{
		this->Encrypt(bufMsg, nLen);
	}


	// ChangeCode
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned char a1, unsigned char b1, unsigned char c1, unsigned char fst1, 
 unsigned char a2, unsigned char b2, unsigned char c2, unsigned char fst2>
	inline void 
	TEncryptClient<a1, b1, c1, fst1, a2, b2, c2, fst2>::ChangeCode(DWORD dwData)
	{
		try{
 DWORD	dwData2 = dwData*dwData;
 for(int i = 0; i < 256; i += 4)
 {
 	*(DWORD*)(&m_bufEncrypt1[i]) ^= dwData;
 	*(DWORD*)(&m_bufEncrypt2[i]) ^= dwData2;
 }
		}catch(...){ sbase::LogSave("Net", "Encryptor ChangeCode fail."); }
	}

	// ChangeCode
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned char a1, unsigned char b1, unsigned char c1, unsigned char fst1, 
 unsigned char a2, unsigned char b2, unsigned char c2, unsigned char fst2>
	inline void 
	TEncryptClient<a1, b1, c1, fst1, a2, b2, c2, fst2>::ChangeCode(const char* pszKey)
	{
		if (!pszKey)
 return;
		
		try{
 DWORD dwData = (DWORD) atoi(pszKey);
 
 DWORD	dwData2 = dwData*dwData;
 for(int i = 0; i < 256; i += 4)
 {
 	*(DWORD*)(&m_bufEncrypt1[i]) ^= dwData;
 	*(DWORD*)(&m_bufEncrypt2[i]) ^= dwData2;
 }
		}catch(...){ sbase::LogSave("Net", "Encryptor ChangeCode fail."); }
	}



	////////////////////////////////////////////////////////////////////////////////////////////////////
	// 3个随机数, 用于隐藏服务端INI中的SERVER KEY。不变
	////////////////////////////////////////////////////////////////////////////////////////////////////
	#define		aa	0x7E
	#define		bb	0x33
	#define		cc	0xA1


	// #define LOGIN_KEY1 	0xa61fce5e	// A = 0x20, B = 0xFD, C = 0x07, first = 0x1F, key = a61fce5e
	// #define LOGIN_KEY2 	0x443ffc04	// A = 0x7A, B = 0xCF, C = 0xE5, first = 0x3F, key = 443ffc04
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// TEncryptServer
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned long key1, unsigned long key2>
	class TEncryptServer : public IEncryptor
	{
	public:
		TEncryptServer()		{ this->Init(); }
		//TEncryptServer(TEncryptServer & cEncrypt);

		// Interface
	public:
		virtual void ChangeCode	(DWORD dwData);
		virtual void ChangeCode	(const char* pszKey);
		
		virtual void Encrypt		(unsigned char * bufMsg, int nLen);
		virtual void Decrypt		(unsigned char* buf, int nLen);
		virtual void Rencrypt	(unsigned char* buf, int nLen)		{ return; }
		
		virtual IEncryptor*		Duplicate	(void)  		{ return new TEncryptServer; }
		virtual unsigned long	Release		(void)  		{ delete this; return 0; }
		
	protected:
		void Init		(void);
		int 	m_nPos1;
		int 	m_nPos2;

		// EncryptCode
		unsigned char m_bufEncrypt1[256];
		unsigned char m_bufEncrypt2[256];
		
		// heap manage
	public:
		MYHEAP_DECLARATION(s_heap)
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Implemention
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned long key1, unsigned long key2>
	sbase::CHeap TEncryptServer<key1, key2>::s_heap;

	//template <unsigned long key1, unsigned long key2>
	//TEncryptServer<key1, key2>::CEncryptCode	TEncryptServer<key1, key2>::m_cGlobalEncrypt;


	// Init
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned long key1, unsigned long key2>
	inline void 
	TEncryptServer<key1, key2>::Init(void)
	{
		m_nPos1 = m_nPos2 = 0;
		
		try{
 // 生成 ABC
 int	a1, b1, c1, fst1;
 a1		= ((key1 >> 0) & 0xFF) ^ aa;
 b1		= ((key1 >> 8) & 0xFF) ^ bb;
 c1		= ((key1 >> 24) & 0xFF) ^ cc;
 fst1	= (key1 >> 16) & 0xFF;
 
 int	a2, b2, c2, fst2;
 a2		= ((key2 >> 0) & 0xFF) ^ aa;
 b2		= ((key2 >> 8) & 0xFF) ^ bb;
 c2		= ((key2 >> 24) & 0xFF) ^ cc;
 fst2	= (key2 >> 16) & 0xFF;
 
 unsigned char	nCode = fst1;
 for(int i = 0; i < 256; i++)
 {
 	m_bufEncrypt1[i] = nCode;
 	//printf("%02X ", nCode);
 	nCode = (a1*nCode*nCode + b1*nCode + c1) % 256;
 }
 //printf("[%02X]\n", nCode);
 assert(nCode == fst1);
 
 nCode = fst2;
 for(int  i = 0; i < 256; i++)
 {
 	m_bufEncrypt2[i] = nCode;
 	nCode = (a2*nCode*nCode + b2*nCode + c2) % 256;
 }
 assert(nCode == fst2);
		}catch(...){ sbase::LogSave("Net", "Encryptor init fail."); }
	}
	
	// Encrypt
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned long key1, unsigned long key2>
	inline void 
	TEncryptServer<key1, key2>::Encrypt(unsigned char * bufMsg, int nLen)
	{
		bool bMove = true;
		try{
 int		nOldPos1 = m_nPos1;
 int		nOldPos2 = m_nPos2;
 for(int i = 0; i < nLen; i++)
 {
 	bufMsg[i] ^= m_bufEncrypt1[1];//[m_nPos1];
 	bufMsg[i] ^= m_bufEncrypt2[1];//[m_nPos2];
 	if(++m_nPos1 >= 256)
 	{
 		m_nPos1 = 0;
 		if(++m_nPos2 >= 256)
  m_nPos2 = 0;
 	}
 	assert(m_nPos1 >=0 && m_nPos1 < 256);
 	assert(m_nPos2 >=0 && m_nPos2 < 256);
 }
 
 if(!bMove)
 {
 	// 恢复指针
 	m_nPos1 = nOldPos1;
 	m_nPos2 = nOldPos2;
 }
		}catch(...){ sbase::LogSave("Net", "Encryptor Encrypt fail."); }
	}

	// Decrypt
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned long key1, unsigned long key2>
	inline void TEncryptServer<key1, key2>::Decrypt(unsigned char * bufMsg, int nLen)
	{
		this->Encrypt(bufMsg, nLen);
	}
	
	
	// ChangeCode
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned long key1, unsigned long key2>
	inline void TEncryptServer<key1, key2>::ChangeCode(DWORD dwData)
	{
		try{
 DWORD	dwData2 = dwData*dwData;
 for(int i = 0; i < 256; i += 4)
 {
 	*(DWORD*)(&m_bufEncrypt1[i]) ^= dwData;
 	*(DWORD*)(&m_bufEncrypt2[i]) ^= dwData2;
 }
		}catch(...){ sbase::LogSave("Net", "Encryptor ChangeCode fail."); }
	}
	
	
	// ChangeCode
	////////////////////////////////////////////////////////////////////////////////////////////////////
	template<unsigned long key1, unsigned long key2>
	inline void 
	TEncryptServer<key1, key2>::ChangeCode(const char* pszKey)
	{
		if (!pszKey)
 return;
		
		try{
 DWORD dwData = (DWORD) atoi(pszKey);
 
 DWORD	dwData2 = dwData*dwData;
 for(int i = 0; i < 256; i += 4)
 {
 	*(DWORD*)(&m_bufEncrypt1[i]) ^= dwData;
 	*(DWORD*)(&m_bufEncrypt2[i]) ^= dwData2;
 }
		}catch(...){ sbase::LogSave("Net", "Encryptor ChangeCode fail."); }
	}
}

#endif // _ENCRYPTOR_H_

