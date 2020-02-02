#ifndef PLAIDGADGET_LOCKFREE_H
#define PLAIDGADGET_LOCKFREE_H


#include <list>

#include "../util/types.h"


namespace plaid
{
	/*
		A simple lock-free queue.

		It should be used by no more than two threads: a designated producer
			which creates and fills it, and a designated consumer which takes
			data out.

		T must have a default constructor available, and should generally be a
			cheaply-copied type that is safe to destroy in either thread.
	*/
	template<typename T>
	class LockFreeQueue
	{
	public:
		/*
			Creation and destruction should occur in 'producer' thread.
		*/
		LockFreeQueue()
			{store.push_back(T()); store.push_back(T()); head=&store.front();
			tail=&store.back(); head->next=tail; tail->next=head;}
		~LockFreeQueue() {}

		/*
			Should only be called by ONE thread, the 'producer'.
		*/
		void push(const T &t)
		{
			tail->t = t;
			if (tail->next == head)
			{
				//Need to expand; tail blocks head from entering this territory
				store.push_back(T());
				Node *next = &store.back();
				next->next = tail->next;
				tail->next = next;
			}
			tail = tail->next;
		}

		/*
			Should only be called by ONE thread, the 'consumer'.

			A successful pull overwrites t by assignment and returns true;
				an unsuccessful one leaves it unchanged and returns false.
		*/
		bool pull(T &t)
		{
			Node *next = head->next;
			if (next==tail) return false;
			t = next->t;
			head = next;
			return true;
		}

	private:
		class Node
		{
		public:
			Node(const T &_t) : t(_t) {}
			T t;
			Node *next;
		};
		typedef std::list<Node> Store;
		Store store;
		Node *head, *tail;
	};

	/*
		A TimedEventQueue is similar to a LockFreeQueue, but attaches a 64-bit
			unsigned timestamp to push-ed items and will only read items whose
			timestamps are less than or equal to the time specified to pull().

		Timestamps should be monotonically increasing; no sorting is performed.

		It was written primarily for event handling in the Audio engine.
	*/
	template<typename T>
	class TimedEventQueue
	{
	public:
		TimedEventQueue() {}

		/*
			Should only be called by producer thread, as with LockFreeQueue.
			time=0 events will be postponed to time=1.
		*/
		void push(const T &v, Uint64 time)
		{
			queue.push(Item(time+!time, v));
		}

		/*
			Should only be called by consumer thread, as with LockFreeQueue.
			pulling with time=0 will pick up time=1 events.
		*/
		bool pull(T &v, Uint64 time)
		{
			time += !time;
			while (front.time <= time)
			{
				if (front.time)
				{
					front.time = 0;
					v = front.value;
					return true;
				}
				if (!queue.pull(front)) break;
			}
			return false;
		}

	private:
		struct Item
		{
			Item()              : time(0), value(T()) {}
			Item(Uint64 t, T v) : time(t), value(v) {}
			Uint64 time; T value;
		};
		LockFreeQueue<Item> queue;
		Item front;
	};
}


#endif // PLAIDGADGET_LOCKFREE_H
