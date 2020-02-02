#include <iostream>

#include "scratch.h"


using namespace plaid;


AudioScratch::Stack::Stack()
{
	back = NULL;
}

AudioScratch::Stack::~Stack()
{
	if (back) delete back;
}

void AudioScratch::Stack::alloc(Sint32 **ptr, Uint32 length, Uint32 channels)
{
	Uint32 size = length*channels;
	if (!size) return;

	//Possibly initialize
	if (!back)
	{
		back = new Back();
		back->buffer.resize(1024*128); //1 MB, about 2.7 seconds at stereo 48K
		back->resize = 0;
	}

	//Find starting position, after most recent alloc
	Uint32 start = 0, end;
	if (back->allocs.size()) start = back->allocs.rbegin()->second;
	end = start+size;

	//Is there enough space?
	if (end > back->buffer.size())
	{
		if (end <= back->resize) return;

		std::cout << back->allocs.size()
			<< " stacked allocs -- Expanding audio-scratch to "
			<< ((end+1023)/1024) << " KB";

		if (!start)
		{
			//Buffer is free; we can resize now
			std::cout << " (immediate)" << std::endl;
			back->buffer.resize(end);
		}
		else
		{
			//Unsafe to resize with buffer in use
			std::cout << " (delayed)" << std::endl;
			back->resize = std::max(back->resize, end);
			return;
		}
	}

	//Allocate
	for (Uint32 i = 0; i < channels; ++i)
		ptr[i] = &back->buffer[start + i*length];
	back->allocs.insert(Alloc(ptr[0], end));
}
void AudioScratch::Stack::release(Sint32 **ptr)
{
	Allocs::iterator i = back->allocs.find(ptr[0]);
	if (i == back->allocs.end())
	{
		std::cout << "WARNING: NO SUCH ALLOC ON AUDIO SCRATCH" << std::endl;
		return;
	}
	back->allocs.erase(i);
	if (back->allocs.size() == 0 && back->resize)
	{
		back->buffer.resize(back->resize);
		back->resize = 0;
	}
}


AudioScratch::Heap::Heap() {}
AudioScratch::Heap::~Heap() {}

void AudioScratch::Heap::alloc(Sint32 **ptr, Uint32 length, Uint32 channels)
{
	Uint32 size = length*channels;
	Sint32 *alloc = new Sint32[size];
	for (Uint32 i = 0; i < channels; ++i) ptr[i] = (alloc + i*length);
}
void AudioScratch::Heap::release(Sint32 **ptr)
{
	delete[] *ptr;
}
