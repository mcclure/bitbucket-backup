#ifndef PLAIDGADGET_AUDIO_SCRATCH_H
#define PLAIDGADGET_AUDIO_SCRATCH_H

#include <vector>
#include <map>

#include "stream.h"

/*
	This header defines some scratch allocators for audio rendering.
		Generally these are used by schedulers and other processes that are
		initiating audio consumption.
*/

namespace plaid
{
	/*
		A constant-time stack allocator well-suited to use in audio callbacks.
			It can run out of space, in which case it will wait until it is
			empty to expand its reserve.
	*/
	class AudioScratch::Stack : public AudioScratch
	{
	public:
		Stack();
		virtual ~Stack();

		void alloc(Sint32 **ptr, Uint32 length, Uint32 channels);
		void release(Sint32 **ptr);

	private:
		typedef std::vector<Sint32> Buffer;
		typedef std::map<Sint32*, Uint32> Allocs;
		typedef Allocs::value_type Alloc;

		struct Back
		{
			Buffer buffer;
			Allocs allocs;
			Uint32 resize;
		};
		Back *back;
	};

	/*
		A wrapper for the system allocator; potentially slow but never fails.
			Unsafe to use in audio callbacks due to unbounded execution time.
			Reccommended for preloader processes and used in AudioClip::load.
	*/
	class AudioScratch::Heap : public AudioScratch
	{
	public:
		Heap();
		virtual ~Heap();

		void alloc(Sint32 **ptr, Uint32 length, Uint32 channels);
		void release(Sint32 **ptr);
	};
}

#endif // PLAIDGADGET_AUDIO_SCRATCH_H
