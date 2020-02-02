#include "../util.h"


using namespace plaid;



Splitter::Splitter(Signal source, Uint32 capacity)
{
	common = new Common(source, capacity);
}

Splitter::Common::Common(Signal _source, Uint32 size) :
	source(_source)
{
	length = 0;
	call = 0;
	capacity = size;
	Uint32 chans = source.format().channels;
	for (Uint32 i = 0; i < PG_MAX_CHANNELS; ++i)
	{
		if (i < chans) data[i] = new Sint32[capacity];
		else data[i] = NULL;
	}
}
Splitter::Common::~Common()
{
	for (Uint32 i = 0; i < PG_MAX_CHANNELS; ++i) if (data[i]) delete data[i];
}

Splitter::Splitter(Splitter *other)
{
	common = other->common;
}

Splitter *Splitter::copy()
{
	return new Splitter(this);
}

AudioFormat Splitter::format()
{
	return common->source.format();
}
void Splitter::pull(AudioChunk &chunk)
{
	//Possibly pull audio from source
	if (common->call != chunk.call())
	{
		//Reassign data, length, frame, a
		common->call = chunk.call();
		common->length = std::min(common->capacity, chunk.length());
		AudioChunk chunk(chunk.scratch(), chunk.format(),
			common->data, common->length, chunk.sync);
		common->source.pull(chunk);
	}

	//Write audio to output
	for (Uint32 i = 0; i < chunk.format().channels; ++i)
	{
		std::memcpy((void*) chunk.start(i),
			(void*) common->data[i], 4*std::min(chunk.length(),common->length));
	}
}
bool Splitter::exhausted()
{
	return common->source.exhausted();
}
void Splitter::tick(Uint64 frame)
{
}
