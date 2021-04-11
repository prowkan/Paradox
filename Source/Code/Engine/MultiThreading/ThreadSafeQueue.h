#pragma once

#include <Containers/Queue.h>

template<typename T>
class ThreadSafeQueue
{
	public:

		ThreadSafeQueue()
		{
			QueueEvent = CreateEvent(NULL, TRUE, FALSE, (const wchar_t*)u"QueueEvent");
		}

		~ThreadSafeQueue()
		{
			BOOL Result = CloseHandle(QueueEvent);
			QueueEvent = INVALID_HANDLE_VALUE;
		}

		void Push(T& Item)
		{
			QueueMutex.lock();
			Queue.Push(Item);
			if (Queue.GetSize() > 0) BOOL Result = SetEvent(QueueEvent);
			QueueMutex.unlock();
		}

		bool Pop(T& Item)
		{
			QueueMutex.lock();
			if (Queue.GetSize() == 0)
			{
				BOOL Result = ResetEvent(QueueEvent);
				QueueMutex.unlock();
				return false;
			}
			else
			{
				Item = Queue.Pop();
				QueueMutex.unlock();
				return true;
			}
		}

		HANDLE& GetQueueEvent() { return QueueEvent; }

	private:

		Queue<T> Queue;
		mutex QueueMutex;
		HANDLE QueueEvent;
};