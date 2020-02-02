#include "../util.h"


using namespace plaid;



Splicer::Splicer(AudioFormat format)
{
	output = format;
	current = NULL;
}
Splicer::Splicer(Sound first)
{
	current = NULL;

	if (first.null()) reportError("Cannot create Splicer from NULL Sound...");
	signals.push_back(Signal(first));
	play.push(&signals.back());
	output = signals.back().format();
}
Splicer::Splicer(Sound first, Sound second)
{
	current = NULL;

	if (first.null()) reportError("Cannot create Splicer from NULL Sound...");
	signals.push_back(Signal(first));
	play.push(&signals.back());
	output = signals.back().format();

	add(second);
}
Splicer::~Splicer()
{
}

void Splicer::add(Sound source)
{
	if (!source.null())
	{
		signals.push_back(Signal(source, output));
		play.push(&signals.back());
	}
}

AudioFormat Splicer::format()
{
	return output;
}
void Splicer::pull(AudioChunk &chunk)
{
	Uint32 left = chunk.length(), chans = chunk.format().channels;

	Sint32 *data[PG_MAX_CHANNELS];
	for (Uint32 i = 0; i < chans; ++i) data[i] = chunk.start(i);

	//Query exhausted each loop to refresh the value of "current".
	while (!exhausted())
	{
		//Pull data from next stream
		AudioChunk sub(chunk.scratch(), output, data, left, chunk.sync);
		current->pull(sub);

		//Partial advance
		if (current->exhausted())
		{
			Uint32 cut = sub.cutoff();
			for (Uint32 i = 0; i < chans; ++i) data[i] += cut;
			left -= cut;
			current = NULL;
			if (left) continue;
		}
		return;
	}

	//The Splicer is exhausted!
	chunk.cutoff(data[0] - chunk.start(0));
}
bool Splicer::exhausted()
{
	if (!current)
	{
		Signal *next;
		if (play.pull(next)) current = next;
	}
	return !current;
}
void Splicer::tick(Uint64 frame)
{
	Signal *stopped;
	while (stop.pull(stopped))
	{
		signals.pop_front();
	}
}
