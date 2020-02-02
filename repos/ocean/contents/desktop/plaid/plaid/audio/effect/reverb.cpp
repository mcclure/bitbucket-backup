#include "../effects.h"


using namespace plaid;


//TODO fix


Reverb::Reverb(Signal source, float dry, float wet, float depth) :
	AudioEffect<int>(source, 0)
{
	reverb(dry, wet, depth);
}
Reverb::~Reverb()
{
}

void Reverb::reverb(float dry, float wet, float depth)
{
	clearDelays();
	addDelaySamples(0, dry);

	float baseDelay = depth / 343.0f;

	addDelaySeconds(baseDelay * .14286f, -.2f * wet);
	addDelaySeconds(baseDelay * .40138f, +.3f * wet);
	addDelaySeconds(baseDelay * .38797f, +.1f * wet);
	addDelaySeconds(baseDelay * .67191f, -.4f * wet);
	addDelaySeconds(baseDelay, .1f * wet);
}

void Reverb::echo(float distance, float decay)
{
	clearDelays();

	addDelaySamples(0, 1.0f);
	addDelaySeconds(distance/343.0f, decay);
}

void Reverb::clearDelays()
{
	delays.clear();
	echoes.clear();
	delays.push_back(0);
	echoes.push_back(0);

	if (memory.size() < 32)
	{
		memoryPos = 0;
		for (int i = 32; i--;) memory.push_back(0);
	}
}

void Reverb::addDelaySeconds(float seconds, float amp)
{
	Uint32 samples = std::max(seconds * float(format().rate), 0.0f);
	addDelaySamples(samples, amp);
}

void Reverb::addDelaySamples(Uint32 _samples, float amp)
{
	_samples *= 2;

	Sint32 samples = _samples;

	//Memory is power-of-two sized; make sure it's big enough.
	while (memory.size() < _samples)
		for (size_t i=0, s=memory.size(); i<s; ++i) memory.push_back(memory[i]);

	//Possibly this is an existing delay line
	for (Uint32 i = 0; i < delays.size(); ++i)
		if (delays[i] == samples)
	{
		echoes[i] = Sint32(echoes[i] + 32768.0f * amp);
		return;
	}

	//Otherwise add it
	delays.push_back(samples);
	echoes.push_back(Sint32(32768.0f * amp));
}

void Reverb::pull(AudioChunk &chunk, const int &a, const int &b)
{
	//Pull source to destination buffer
	source.pull(chunk);

	//Sint32 *data = chunk.start(), *end = chunk.end();

	return;

	#if 0

	Sint32 memPos = memoryPos, memMask = memory.size()-1;
	Sint16 *memBuff = &memory[0];
	int curLine = 0, lines = delays.size();//lastLine = delays.size()-1;
	//int advance;
	Sint32 *echoBuff = &echoes[0], *delayBuff = &delays[0];

	//Sint16 raw, add;
	Sint32 samp;

	//This is actually a 2D loop, but it's tricky about that.
	//  (Possible this optimization is completely ineffective, but it's fun)
	while (data < end)
	{
		samp = 0;
		for (curLine = 1; curLine < lines; ++curLine)
		{
			samp += Sint32(memBuff[(memPos-delayBuff[curLine])&memMask])
				* echoBuff[curLine];
		}
		memBuff[memPos] = (samp + *data * 32768) >> 15;
		*data = (samp + (*data * *echoBuff)) >> 15;
		memPos = (memPos + 1) & memMask;
		++data;

		//
		/*advance = (curLine==lastLine);

		//Calculate this delay line
		raw = memBuff[(memPos-delayBuff[curLine])&memMask];
		add = (Sint32(raw) * echoBuff[curLine]) >> 15;

		//Mix in delay line
		*data += add;
		memBuff[memPos] += add + (raw-add)*(!curLine);

		//Advance
		curLine = (curLine + 1) * !advance;
			memPos = (memPos + advance) & memMask;
			data += advance;
			memBuff[memPos] *= !advance;
			*data *= !advance;*/
	}

	memoryPos = memPos;

	#endif
}
