#ifndef PLAIDGADGET_MEMORY_H
#define PLAIDGADGET_MEMORY_H


/*
	This header facilitates a custom memory allocator and systems for monitoring
		memory leaks, though the latter are fairly rudimentary.
*/


#if PG_DEBUG
    /*#ifdef _WIN32
	#define PG_MEMORY_WATCH 1
	#endif*/
#else
	// ...
#endif


#if PG_MEMORY_WATCH
    #define PG_CUSTOM_NEW 1
#endif


//Custom allocators:  Serious business.
#if PG_CUSTOM_NEW
    void* operator new(size_t size);
    void* operator new[](size_t size);
    void operator delete(void *p);
    void operator delete[](void *p);
#endif



//Memory monitoring
namespace plaid
{
	//Report memory that hasn't been deleted
    void Memory_Report();

	//User code usually won't need to use these
    void Memory_Add(void *p, const char *where, size_t amt);
    void Memory_Remove(void *p);
}

#ifdef PG_MEMORY_WATCH
	namespace Memory_Track
	{
		//Static linkage, you fool, I've tricked you again!
		struct MemoryUntracked_Type {};
		extern MemoryUntracked_Type MemoryUntracked;

		//Get memory without registering it
		void* operator new(size_t size, MemoryUntracked_Type untracked);
	}
	void* operator new(size_t size, const char *where);
	void* operator new[](size_t size, const char *where);
	using namespace Memory_Track;
	#undef new
	#undef delete
	#define NewUntracked new
	#define PGNEWSTRINGIFY(X) #X
	#define PGNEWTOSTRING(X) PGNEWSTRINGIFY(X)
	#define new new( __FILE__ ":" PGNEWTOSTRING(__LINE__) )
#else
	#define NewUntracked new
#endif


#endif // PLAIDGADGET_MEMORY_H
