#pragma once

#include <windows.h>
#include <mmsystem.h>

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>

namespace sbase {
	class IThreadEvent {
	public:
		virtual ~IThreadEvent() = default;
		virtual int OnThreadCreate() = 0;
		virtual int OnThreadEvent() = 0;
		virtual int OnThreadProcess() = 0;
		virtual int OnThreadDestroy() = 0;
	};

	class CThread {
	public:
		enum Status {
			STATUS_INIT,
			STATUS_RUNNING,
			STATUS_SUSPEND,
			STATUS_CLOSING,
			STATUS_CLOSED
		};

		CThread(IThreadEvent& event)
			: m_event(event), m_status(STATUS_INIT), m_exit(false), m_workRequested(false), m_workIntervalMs(0) {
		}

		~CThread() {
			Close();
			if (m_thread.joinable())
				m_thread.join();
		}

		bool Init(bool suspend, unsigned int workIntervalMs) {
			if (m_status != STATUS_INIT)
				return false;

			m_workIntervalMs = workIntervalMs;
			m_exit = false;
			m_workRequested = false;
			m_status = suspend ? STATUS_SUSPEND : STATUS_RUNNING;

			m_thread = std::thread(&CThread::ThreadMain, this);

			if (suspend)
				Suspend();

			return true;
		}

		static CThread* CreateNew(IThreadEvent& event, bool suspend, unsigned int interval) {
			CThread* t = new (std::nothrow) CThread(event);
			if (!t || !t->Init(suspend, interval)) {
				delete t;
				return nullptr;
			}
			return t;
		}

		void Resume() {
			if (m_status == STATUS_SUSPEND) {
				m_status = STATUS_RUNNING;
				m_cv.notify_all();
			}
		}

		void Suspend() {
			if (m_status == STATUS_RUNNING) {
				m_status = STATUS_SUSPEND;
			}
		}

		//void SetEvent() {
		//	{
		//		std::lock_guard<std::mutex> lock(m_mutex);
		//		m_workRequested = true;
		//	}
		//	m_cv.notify_all();
		//}

		bool SetEvent() {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_workRequested = true;
			}
			m_cv.notify_all();

			return true;
		}

		bool Close() {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_exit = true;
			}
			m_cv.notify_all();
			m_status = STATUS_CLOSING;
			return true;
		}

	private:
		void ThreadMain() {
			if (m_event.OnThreadCreate() == -1) {
				m_status = STATUS_CLOSED;
				return;
			}

			while (true) {
				std::unique_lock<std::mutex> lock(m_mutex);

				if (m_exit)
					break;

				if (m_status == STATUS_SUSPEND) {
					m_cv.wait(lock, [this] { return m_status != STATUS_SUSPEND || m_exit; });
					if (m_exit) break;
				}

				if (m_workRequested) {
					m_workRequested = false;
					lock.unlock();
					if (m_event.OnThreadEvent() == -1)
						break;
				}
				else {
					if (m_cv.wait_for(lock, std::chrono::milliseconds(m_workIntervalMs)) == std::cv_status::timeout) {
						lock.unlock();
						if (m_event.OnThreadProcess() == -1)
							break;
					}
				}
			}

			m_status = STATUS_CLOSED;
			m_event.OnThreadDestroy();
		}

	private:
		IThreadEvent& m_event;
		std::thread m_thread;
		std::mutex m_mutex;
		std::condition_variable m_cv;
		std::atomic<Status> m_status;
		bool m_exit;
		bool m_workRequested;
		unsigned int m_workIntervalMs;
	};
} // namespace sbase
