#ifndef _FILE_THREADPOOL_H
#define _FILE_THREADPOOL_H

#include <map>
#include <algorithm>
#include <vector>
#include <assert.h>
#include <Windows.h>
#include "SyncObjs.h"
#include "Heap.h"
#include "ByteBuffer.h"

namespace sbase
{
	///////////////////////////////////////////////////////////////////////////////
	class ITask
	{
	public:
		virtual ~ITask();
		virtual uint32 DoTask(void* pvParam) = 0;
	};

	class CMyWorker
	{
	public:
		typedef DWORD_PTR RequestType;

		CMyWorker() : m_dwExecs(0)
		{
			m_Task.clear();
		}

		~CMyWorker()
		{
			m_CritSec.Lock();
			std::vector<ITask*>::iterator itor = m_Task.begin();
			for (; itor != m_Task.end();)
			{
				delete* itor;
				itor = m_Task.erase(itor);
			}
			m_CritSec.Unlock();
		}

		virtual BOOL Initialize(void* pvParam)
		{
			printf("[%d]: CMyWorker.Initialize(%d)\n", (DWORD_PTR)::GetCurrentThreadId(), (DWORD_PTR)pvParam);
			return TRUE;
		}

		virtual void Terminate(void* /*pvParam*/)
		{
			printf("CMyWorker #%d exec'd %d times.\n", m_lId, m_dwExecs);
		}

		void AddTask(RequestType dw)
		{
			m_CritSec.Lock();
			ITask* pTask = (ITask*)(DWORD_PTR)dw;
			m_Task.push_back(pTask);
			m_CritSec.Unlock();
		}

		void Execute(RequestType dw, void* pvParam, OVERLAPPED* pOverlapped) throw()
		{
			m_CritSec.Lock();
			assert(pvParam != NULL);

			//printf("[%d] CMyWorker::Execute(dw=%d, pvParam=%d, pOverlapped=%d\n",
			//	::GetCurrentThreadId(), dw, (DWORD_PTR)pvParam, (DWORD_PTR)pOverlapped);

			ITask* pTask = (ITask*)(DWORD_PTR)dw;
			pTask->DoTask(pvParam);

			std::vector<ITask*>::iterator itor = find(m_Task.begin(), m_Task.end(), pTask);
			if (itor != m_Task.end())
			{
				delete* itor;
				m_Task.erase(itor);
			}

			m_dwExecs++;
			m_CritSec.Unlock();
		}

		virtual BOOL GetWorkerData(DWORD /*dwParam*/, void** /*ppvData*/)
		{
			return FALSE;
		}

	protected:
		std::vector<ITask*> m_Task;
		DWORD	m_dwExecs;
		LONG	m_lId;
		CCriticalSection m_CritSec;
	}; // CMyWorker

	template<typename Worker>
	class CThreadPool
	{
		static const int  MAX_WAIT = 36000;
	public:
		CThreadPool() throw() :
			m_hRequestQueue(NULL),
			m_pvWorkerParam(NULL),
			m_dwMaxWait(MAX_WAIT),
			m_bShutdown(FALSE),
			m_dwThreadEventId(0),
			m_dwStackSize(0)
		{
		}

		~CThreadPool() throw()
		{
			Shutdown();
		}

		// Shutdown the thread pool
		// This function posts the shutdown request to all the threads in the pool
		// It will wait for the threads to shutdown a maximum of dwMaxWait MS.
		// If the timeout expires it just returns without terminating the threads.
		void Shutdown(__in DWORD dwMaxWait = 0) throw()
		{
			if (!m_hRequestQueue)   // Not initialized
				return;

			m_CritSec.Lock();

			if (dwMaxWait == 0)
				dwMaxWait = m_dwMaxWait;

			HRESULT hr = InternalResizePool(0, dwMaxWait);

			// If the threads have not returned, then something is wrong
			std::map<DWORD, HANDLE>::iterator itor = m_ThreadMap.begin(),
				itore = m_ThreadMap.end();

			for (; itor != m_ThreadMap.end(); ++itor)
			{
				HANDLE hThread = itor->second;
				DWORD dwExitCode;
				GetExitCodeThread(hThread, &dwExitCode);
				if (dwExitCode == STILL_ACTIVE)
				{
#pragma warning(push)
#pragma warning(disable: 6258)
					/* deliberate design choice to use TerminateThread here in extremis */
					TerminateThread(hThread, 0);
#pragma warning(pop)
				}
				CloseHandle(hThread);
			}

			// Close the request queue handle
			CloseHandle(m_hRequestQueue);

			// Clear the queue handle
			m_hRequestQueue = NULL;

			assert(m_ThreadMap.size() == 0);

			// Uninitialize the critical sections
			m_CritSec.Unlock();
			CloseHandle(m_hThreadEvent);
		}

		HRESULT SetSize(__in int nNumThreads) throw()
		{
			if (nNumThreads == 0)
				nNumThreads = -2;

			if (nNumThreads < 0)
			{
				SYSTEM_INFO si;
				GetSystemInfo(&si);
				nNumThreads = (int)(-nNumThreads) * si.dwNumberOfProcessors;
			}

			return InternalResizePool(nNumThreads, m_dwMaxWait);
		}

		HRESULT InternalResizePool(int nNumThreads, int dwMaxWait) throw()
		{
			if (!m_hRequestQueue)
				return E_FAIL;

			m_CritSec.Lock();

			int nCurThreads = m_ThreadMap.size();
			if (nNumThreads == nCurThreads)
			{
				return S_OK;
			}
			else if (nNumThreads < nCurThreads)
			{
				int nNumShutdownThreads = nCurThreads - nNumThreads;
				for (int nThreadIndex = 0; nThreadIndex < nNumShutdownThreads; nThreadIndex++)
				{
					ResetEvent(m_hThreadEvent);

					InterlockedExchange(&m_bShutdown, TRUE);
					PostQueuedCompletionStatus(m_hRequestQueue, 0, 0, ((OVERLAPPED*)((__int64)-1)));
					DWORD dwRet = ::WaitForSingleObject(m_hThreadEvent, dwMaxWait);

					if (dwRet == WAIT_TIMEOUT)
					{
						LONG bResult = InterlockedExchange(&m_bShutdown, FALSE);
						if (bResult) // Nobody picked up the shutdown message
						{
							return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
						}
					}
					else if (dwRet != WAIT_OBJECT_0)
					{
						return sbase::HresultFromLastError();
					}

					std::map<DWORD, HANDLE>::iterator itor = m_ThreadMap.find(m_dwThreadEventId);

					if (itor != m_ThreadMap.end())
					{
						HANDLE hThread = itor->second;
						if (::WaitForSingleObject(hThread, 60000) == WAIT_OBJECT_0)
						{
							CloseHandle(hThread);
							m_ThreadMap.erase(itor);
						}
						else
						{
							return E_FAIL;
						}
					}
				}
			}
			else
			{
				int nNumNewThreads = nNumThreads - nCurThreads;

				for (int nThreadIndex = 0; nThreadIndex < nNumNewThreads; nThreadIndex++)
				{
					DWORD dwThreadID;
					ResetEvent(m_hThreadEvent);
					HANDLE hdlThread = CreateThread(NULL, m_dwStackSize, WorkerThreadProc, (LPVOID)this, 0, &dwThreadID);

					if (!hdlThread)
					{
						return sbase::HresultFromLastError();
					}

					DWORD dwRet = WaitForSingleObject(m_hThreadEvent, dwMaxWait);
					if (dwRet != WAIT_OBJECT_0)
					{
						if (dwRet == WAIT_TIMEOUT)
						{
							return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
						}
						else
						{
							return sbase::HresultFromLastError();
						}
					}

					m_ThreadMap[dwThreadID] = hdlThread;
				}
			}
			return S_OK;
		}

		static DWORD WINAPI WorkerThreadProc(LPVOID pv) throw()
		{
			CThreadPool* pThis =
				reinterpret_cast<CThreadPool*>(pv);

			return pThis->ThreadProc();
		}

		DWORD ThreadProc() throw()
		{
			DWORD dwBytesTransfered;
			ULONG_PTR dwCompletionKey;

			OVERLAPPED* pOverlapped;

			// this block is to ensure theWorker gets destructed before the
			// thread handle is closed
			{
				if (theWorker.Initialize(m_pvWorkerParam) == FALSE)
				{
					return 1;
				}

				SetEvent(m_hThreadEvent);
				// Get the request from the IO completion port
				while (GetQueuedCompletionStatus(m_hRequestQueue, &dwBytesTransfered, &dwCompletionKey, &pOverlapped, INFINITE))
				{
					if (pOverlapped == ((OVERLAPPED*)((__int64)-1))) // Shut down
					{
						LONG bResult = InterlockedExchange(&m_bShutdown, FALSE);
						if (bResult) // Shutdown has not been cancelled
							break;

						// else, shutdown has been cancelled -- continue as before
					}
					else										// Do work
					{
						Worker::RequestType request = (Worker::RequestType)dwCompletionKey;

						// Process the request.  Notice the following:
						// (1) It is the worker's responsibility to free any memory associated
						// with the request if the request is complete
						// (2) If the request still requires some more processing
						// the worker should queue the request again for dispatching
						theWorker.Execute(request, m_pvWorkerParam, pOverlapped);
					}
				}

				theWorker.Terminate(m_pvWorkerParam);
			}

			m_dwThreadEventId = GetCurrentThreadId();
			SetEvent(m_hThreadEvent);

			return 0;
		}

		HANDLE GetQueueHandle() throw()
		{
			return m_hRequestQueue;
		}

		// QueueRequest adds a request to the thread pool
		// it will be picked up by one of the threads and dispatched to the worker
		// in WorkerThreadProc
		BOOL QueueRequest(typename Worker::RequestType request) throw()
		{
			assert(m_hRequestQueue != NULL);

			theWorker.AddTask(request);

			if (!PostQueuedCompletionStatus(m_hRequestQueue, 0, (ULONG_PTR)request, NULL))
				return FALSE;

			return TRUE;
		}

		int GetNumThreads() throw()
		{
			return m_ThreadMap.size();
		}

		HRESULT Initialize(void* pvWorkerParam = NULL, int nNumThreads = 0,
			DWORD dwStackSize = 0, HANDLE hCompletion = INVALID_HANDLE_VALUE) throw()
		{
			m_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (!m_hThreadEvent)
			{
				return sbase::HresultFromLastError();
			}

			m_hRequestQueue = CreateIoCompletionPort(hCompletion, NULL, 0, nNumThreads);
			if (m_hRequestQueue == NULL)
			{
				CloseHandle(m_hThreadEvent);
				return sbase::HresultFromLastError();
			}

			m_pvWorkerParam = pvWorkerParam;
			m_dwStackSize = dwStackSize;

			HRESULT hr = SetSize(nNumThreads);
			if (hr != S_OK)
			{
				// Close the request queue handle
				CloseHandle(m_hRequestQueue);

				// Clear the queue handle
				m_hRequestQueue = NULL;

				// Uninitialize the critical sections
				m_CritSec.~CCriticalSection();
				CloseHandle(m_hThreadEvent);

				return hr;
			}

			return S_OK;
		}

	protected:
		std::map<DWORD, HANDLE> m_ThreadMap;
		DWORD m_dwThreadEventId;

		CCriticalSection m_CritSec;
		DWORD m_dwStackSize;
		DWORD m_dwMaxWait;

		void* m_pvWorkerParam;
		LONG m_bShutdown;

		HANDLE m_hThreadEvent;
		HANDLE m_hRequestQueue;

		// We instantiate an instance of the worker class on the stack
		// for the life time of the thread.
		Worker theWorker;
	private:
	};
}

#endif