#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#include <SDL/SDL.h>

#include "../core.h"


using namespace std;
using namespace plaid;


/*#ifdef _WIN32
	#include "ned/nedmalloc.h"
	#define pgmalloc nedmalloc
	#define pgfree nedfree
#else*/
	#define pgmalloc malloc
	#define pgfree free
//#endif



#undef new
#undef delete

#ifdef PG_MEMORY_WATCH

MemoryUntracked_Type Memory_Track::MemoryUntracked;

void* operator new(size_t size, const char *where)
{
    void *p = pgmalloc(size);
    Memory_Add(p, where, size);
    return p;
}

void* operator new[](size_t size, const char *where)
{
    void *p = pgmalloc(size);
    Memory_Add(p, where, size);
    return p;
}

void* Memory_Track::operator new(size_t size, MemoryUntracked_Type untracked)
{
	void *p = pgmalloc(size);
	return p;
}

#endif


#if PG_CUSTOM_NEW
    void* operator new(size_t size)
    {
        void *p = pgmalloc(size);
    #ifdef PG_MEMORY_WATCH
        Memory_Add(p, "?", size);
    #endif
        return p;
    }

    void* operator new[](size_t size)
    {
        void *p = pgmalloc(size);
    #ifdef PG_MEMORY_WATCH
        Memory_Add(p, "?", size);
    #endif
        return p;
    }

    void operator delete(void *p)
    {
    #ifdef PG_MEMORY_WATCH
        Memory_Remove(p);
    #endif
        pgfree(p);
    }

    void operator delete[](void *p)
    {
    #ifdef PG_MEMORY_WATCH
        Memory_Remove(p);
    #endif
        pgfree(p);
    }
#endif



//------------------

namespace plaid
{
	typedef pair<const char*, size_t> mem_info;
	typedef map<void*, mem_info> mem_map;
	typedef mem_map::iterator mem_iter;
	typedef pair<void*, mem_info> mem_pair;

	class WatchLock
	{
	public:
		static unsigned on;
		operator bool() {return on^1;}
		static SDL_mutex* mutex()
		{
			static SDL_mutex* _mutex = SDL_CreateMutex();
			return _mutex;
		}
		WatchLock() {SDL_LockMutex(mutex()); ++on;}
		~WatchLock() {--on; SDL_UnlockMutex(mutex());}
	};
	unsigned WatchLock::on = 0;

	static mem_map &memory()
	{
		static mem_map mem;
		return mem;
	}

	static mem_map &deleted()
	{
		static mem_map del;
		return del;
	}

	void Memory_Add(void *p, const char *where, size_t amt)
	{
		WatchLock lock;
		if (lock) return;
		memory().insert(mem_pair(p, mem_info(where, amt)));
	}

	void Memory_Remove(void *p)
	{
		static bool err;
		{
			WatchLock lock;
			if (lock) return;
			mem_iter pos = memory().find(p);
			if (pos != memory().end())
			{
				deleted().insert(*pos);
				memory().erase(p);
				return;
			}
			else
			{
				mem_iter dpos = deleted().find(p);
				err = dpos == deleted().end();
			}
		}
		if (err) reportError("Deleted unallocated memory!");
		else reportError("Double deletion of memory!");
	}

	void Memory_Report()
	{
		WatchLock lock;
		size_t total = 0;
		typedef pair<unsigned long, size_t> counts;
		typedef map<const char*, counts> actives_map;
		typedef pair<const char*, counts> actives_pair;
		typedef actives_map::iterator actives_iter;
		actives_map actives;
		for(mem_iter i = memory().begin(); i != memory().end(); i++)
		{
			total += i->second.second;
			const char* s = i->second.first;
			actives_iter iter = actives.find(s);
			if (iter != actives.end())
			{
				iter->second.first++;
				iter->second.second += i->second.second;
			}
			else
			{
				actives.insert(actives_pair(s, counts(1, i->second.second)));
			}
		}

		typedef map<size_t, const char*> ordered_map;
		typedef ordered_map::reverse_iterator ordered_iter;
		typedef pair<size_t, const char*> ordered_pair;
		ordered_map ordered;
		for (actives_iter i = actives.begin(); i != actives.end(); ++i)
			ordered.insert(ordered_pair(i->second.second, i->first));

		struct
		{
			void operator()(size_t s)
			{
				for (int i = 0; i < 5; ++i, s >>= 10)
				{
					if (s < 10000)
					{
						cout << s << " KMGT"[i] << 'B';
						return;
					}
				}
			}
		} PrintSize;

		cout << "---------------" << endl;
		cout << "Active Memory: "; PrintSize(total); cout << endl;
		cout << "---------------" << endl;
		for(ordered_iter i = ordered.rbegin(); i != ordered.rend(); ++i)
		{
			PrintSize(i->first);
			cout << " in " << actives[i->second].first << " allocs: "
				<< i->second << endl;
		}
		cout << "---------------" << endl;
	}
}
