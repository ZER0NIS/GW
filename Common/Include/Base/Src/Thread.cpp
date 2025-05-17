#include "..\inc\Thread.h"

#include <mmsystem.h>
#include <process.h>
#include <new>

namespace sbase
{
	//CThread::CThread(IThreadEvent& event)
	//	: m_event(event), m_hThread(NULL), m_hExitEvent(NULL), m_hWorkEvent(NULL)
	//{
	//	m_id = 0;
	//	m_dwWorkInterval = 0;
	//	m_nStatus = STATUS_INIT;
	//}

	//CThread::~CThread()
	//{
	//	if (m_nStatus == STATUS_INIT)
	//		return;

	//	// Reanudar si está suspendido
	//	if (m_nStatus == STATUS_SUSPEND)
	//		this->Resume();

	//	// Cerrar el hilo si no está ya en proceso de cierre
	//	if (m_nStatus != STATUS_CLOSING)
	//		this->Close();

	//	if (m_hThread)
	//	{
	//		::WaitForSingleObject(m_hThread, TIME_WAITINGCLOSE);
	//		::CloseHandle(m_hThread);
	//		m_hThread = NULL;
	//	}

	//	if (m_hWorkEvent)
	//	{
	//		::CloseHandle(m_hWorkEvent);
	//		m_hWorkEvent = NULL;
	//	}

	//	if (m_hExitEvent)
	//	{
	//		::CloseHandle(m_hExitEvent);
	//		m_hExitEvent = NULL;
	//	}

	//	m_id = 0;
	//}

	//bool CThread::Close()
	//{
	//	if (m_hExitEvent && ::SetEvent(m_hExitEvent))
	//	{
	//		m_nStatus = STATUS_CLOSING;
	//		return true;
	//	}

	//	return false;
	//}

	//bool CThread::Init(bool bSuspend, DWORD dwWorkInterval)
	//{
	//	if (m_nStatus != STATUS_INIT)
	//		return false;

	//	// Crear evento de salida
	//	m_hExitEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	//	if (!m_hExitEvent)
	//		return false;

	//	// Crear evento de trabajo
	//	m_hWorkEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	//	if (!m_hWorkEvent)
	//	{
	//		::CloseHandle(m_hExitEvent);
	//		m_hExitEvent = nullptr;
	//		return false;
	//	}

	//	// Crear el hilo
	//	DWORD dwCreationFlags = bSuspend ? CREATE_SUSPENDED : 0;
	//	m_hThread = ::CreateThread(nullptr, 0,
	//		(LPTHREAD_START_ROUTINE)CThread::RunThread,
	//		this, dwCreationFlags, &m_id);
	//	if (!m_hThread)
	//	{
	//		::CloseHandle(m_hExitEvent);
	//		m_hExitEvent = nullptr;
	//		::CloseHandle(m_hWorkEvent);
	//		m_hWorkEvent = nullptr;
	//		return false;
	//	}

	//	m_dwWorkInterval = dwWorkInterval;
	//	m_nStatus = bSuspend ? STATUS_SUSPEND : STATUS_RUNNING;
	//	return true;
	//}

	//void CThread::Resume(void)
	//{
	//	if (m_nStatus == STATUS_SUSPEND && m_hThread)
	//	{
	//		if (::ResumeThread(m_hThread) != (DWORD)-1)
	// m_nStatus = STATUS_RUNNING;
	//	}
	//}

	//void CThread::Suspend(void)
	//{
	//	if (m_nStatus == STATUS_RUNNING && m_hThread)
	//	{
	//		if (::SuspendThread(m_hThread) != (DWORD)-1)
	// m_nStatus = STATUS_SUSPEND;
	//	}
	//}

	//bool CThread::SetEvent()
	//{
	//	if (m_hWorkEvent)
	//		return ::SetEvent(m_hWorkEvent) == TRUE;
	//	return false;
	//}

	//CThread* CThread::CreateNew(IThreadEvent& refEvent, bool bSuspend, DWORD dwWorkInterval)
	//{
	//	CThread* pThread = new (std::nothrow) CThread(refEvent);
	//	if (!pThread)
	//		return NULL;

	//	if (!pThread->Init(bSuspend, dwWorkInterval))
	//	{
	//		delete pThread;
	//		return NULL;
	//	}

	//	return pThread;
	//}

	//DWORD WINAPI CThread::RunThread(LPVOID pThreadParameter)
	//{
	//	CThread* pThread = static_cast<CThread*>(pThreadParameter);
	//	if (!pThread)
	//		return 1;

	//	// Inicialización
	//	if (pThread->m_event.OnThreadCreate() == -1)
	//		return 2;

	//	// Bucle principal
	//	HANDLE hEvent[2] = { pThread->m_hExitEvent, pThread->m_hWorkEvent };
	//	bool bRunning = true;

	//	while (bRunning)
	//	{
	//		DWORD dwResult = ::WaitForMultipleObjects(2, hEvent, FALSE, pThread->m_dwWorkInterval);

	//		switch (dwResult)
	//		{
	//		case WAIT_OBJECT_0: // Exit event
	// bRunning = false;
	// break;

	//		case WAIT_OBJECT_0 + 1: // Work event
	// if (pThread->m_event.OnThreadEvent() == -1)
	// 	bRunning = false;
	// break;

	//		case WAIT_TIMEOUT: // Timeout
	// if (pThread->m_event.OnThreadProcess() == -1)
	// 	bRunning = false;
	// break;

	//		default: // Error
	// bRunning = false;
	// break;
	//		}
	//	}

	//	pThread->m_nStatus = STATUS_CLOSED;
	//	return pThread->m_event.OnThreadDestroy();
	//}
}