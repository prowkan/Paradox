#pragma once

template<typename T>
class ThreadSafeQueue
{
	public:

		void Push(T& Item)
		{
			QueueMutex.lock();
			Queue.push(Item);
			QueueMutex.unlock();
		}

		bool Pop(T& Item)
		{
			QueueMutex.lock();
			if (Queue.size() == 0)
			{
				QueueMutex.unlock();
				return false;
			}
			else
			{
				Item = Queue.front();
				Queue.pop();
				QueueMutex.unlock();
				return true;
			}
		}

	private:

		queue<T> Queue;
		mutex QueueMutex;
};