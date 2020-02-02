#include <cassert>
#include <cstring>

#include "../util.h"


using namespace plaid;



//************************************************************************
//    THE FUNCTION PLAINS
//************************************************************************


namespace plaid
{
	/*
		The interface for Functions.
	*/
	class Transcoder::Function
	{
	public:
		virtual ~Function() {}
		virtual void transcode(Sint32 *const* src, Sint32 *const* dest,
			Uint32 samples) = 0;
	};

	class MonoCopy : public Transcoder::Function
	{
	public:
		MonoCopy(Uint32 _outs) : outs(_outs) {}
		virtual ~MonoCopy() {}

		virtual void transcode(Sint32 *const* src, Sint32 *const* dest,
			Uint32 samples)
		{
			for (Uint32 i = 0; i < outs; ++i)
				std::memcpy((void*) dest[i], (void*) src[0], samples*4);
		}

		const Uint32 outs;
	};

	class MonoMix : public Transcoder::Function
	{
	public:
		MonoMix(Uint32 _ins) : ins(_ins) {}
		virtual ~MonoMix() {}

		virtual void transcode(Sint32 *const* src, Sint32 *const* dest,
			Uint32 samples)
		{
			//Copy first channel
			std::memcpy((void*) dest[0], (void*) src[0], samples*4);

			Sint32 *line, *out, *end;
			for (Uint32 i = 1; i < ins; ++i)
			{
				line = src[i];
				out = dest[0];
				end = out+samples;

				//Final pass averages values
				if (i < ins-1) while (out != end) {*(out++) += *(line++);}
				else while (out != end) {*out = (*out+*(line++))/ins; ++out;}
			}
		}

		const Uint32 ins;
	};

	class MultiMod : public Transcoder::Function
	{
	public:
		MultiMod(Uint32 _ins, Uint32 _outs) : ins(_ins), outs(_outs) {}
		virtual ~MultiMod() {}

		virtual void transcode(Sint32 *const* src, Sint32 *const* dest,
			Uint32 samples)
		{
			for (Uint32 i = 0; i < outs; ++i)
				std::memcpy((void*) dest[i], (void*) src[i%ins], samples*4);
		}

		const Uint32 ins, outs;
	};
}


//************************************************************************
//    THE SWITCHYARD
//************************************************************************


Transcoder::Transcoder(Signal _source, AudioFormat _dest) :
	source(_source), dest(_dest)
{
	if (!source) reportError("Transcoder got null signal...");

	if (dest.rate == 0)
		reportError("BAD DEST SAMPLERATE IN TRANSCODER");
	if (source.format().rate == 0)
		reportError("BAD SOURCE SAMPLERATE IN TRANSCODER");

	ratio = float(_dest.rate) / float(_source.format().rate);
	offset = 0.0f;


	Uint32 srcChannels = _source.format().channels;

	if (srcChannels == dest.channels)
	{
		function = NULL;
	}
	else if (srcChannels == 1)
	{
		function = new MonoCopy(dest.channels);
	}
	else if (dest.channels == 1)
	{
		function = new MonoMix(srcChannels);
	}
	else
	{
		function = new MultiMod(srcChannels, dest.channels);
	}
}



//************************************************************************
//    THE IMPORTANT BIT
//************************************************************************


Transcoder::~Transcoder()
{
	delete function;
}

bool Transcoder::exhausted()
{
	return source.exhausted();
}

void Transcoder::tick(Uint64 frame)
{
	source.tick(frame);
}

AudioFormat Transcoder::format()
{
	return dest;
}

void Transcoder::pull(AudioChunk &chunk)
{
	if (function)
	{
		//Size the buffer
		AudioFormat src = source.format();

		//Allocate temp buffer
		AudioChunk tmp(chunk, chunk.length(), src);
		if (!tmp.ok()) {chunk.silence(); return;}

		//Transcode...
		source.pull(tmp);
		function->transcode(tmp.data(), chunk.data(), chunk.length());
	}
	else source.pull(chunk);
}
