#include <cstring>

#include "../analysis.h"
#include "../util.h"

using namespace plaid;



Envelope::Envelope(Signal _source, float frequency, Uint32 _logSize) :
	source(_source)
{
	filter = new Filter(new HackStream(source.format()));
	filter->bandpass(frequency/4.0f, frequency);
	filtSig = filter;

	logSize = _logSize;
	if (logSize)
	{
		log = new Feed[logSize];
		std::memset((void*) log, 0, sizeof(Feed)*logSize);
		logPos = pullPos = 0;
		frame = 0;
	}
	else log = NULL;
}

Envelope::~Envelope()
{
	if (log) delete[] log;
}

AudioFormat Envelope::format()
{
	return source.format();
}
void Envelope::pull(AudioChunk &chunk)
{
	source.pull(chunk);

	Uint32 chan = source.format().channels;
	for (Uint32 c = 0; c < chan; ++c)
	{
		for (Sint32 *i = chunk.start(c), *e = chunk.end(c); i != e; ++i)
		{
			*i *= (1-2*(*i<0));
		}
	}

	filtSig.pull(chunk);

	if (log && chunk.last())
	{
		Feed feed =
			{chunk.frame(), float(*(chunk.end(0)-1))/AudioFormat::INT24_CLIP};
		feedback.push(feed);
	}
}
bool Envelope::exhausted()
{
	return source.exhausted();
}
void Envelope::tick(Uint64 _frame)
{
	source.tick(_frame);
	filtSig.tick(_frame);

	frame = _frame;

	if (log)
	{
		Feed feed;
		while (feedback.pull(feed))
		{
			logPos = (logPos + 1) % logSize;
			log[logPos] = feed;

			//Prevent ringbuffer overflow
			pullPos = (pullPos + (pullPos == logPos)) % logSize;
		}
	}
}

bool Envelope::pull(float &amplitude)
{
	if (pullPos == logPos) return false;
	pullPos = (pullPos + 1) % logSize;
	amplitude = log[pullPos].amp;
	if (amplitude < 0.0f)
	{
		amplitude = 0.0f;
		//reportError(L"Negative envelope pull!");
	}
	return true;
}

float Envelope::amplitude()
{
	return log[logPos].amp;
}

Uint32 Envelope::latency()
{
	return frame - log[logPos].frame;
}
