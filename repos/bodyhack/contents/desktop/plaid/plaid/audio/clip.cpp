#include <iostream>

#include "clip.h"
#include "scratch.h"


using namespace plaid;


using std::min;
using std::max;
using std::memcpy;



AudioClip::AudioClip(AudioFormat format, SAMPLE_TYPE type) :
	data(new Data(format, type))
{
}

AudioClip::Data::Data(AudioFormat f, Uint16 t)
{
	if (t < INT8 || t > INT24)
		reportError(L"AudioClip created with invalid sample size...");

	format = f;
	type = t;
	for (int i = 0; i < PG_MAX_CHANNELS; ++i)
	{
		root[i] = NULL;
		last[i] = NULL;
	}
	length = 0;
	locks = 0;
}

AudioClip::~AudioClip()
{
	for (Uint32 c = 0; c < PG_MAX_CHANNELS; ++c)
	{
		Link *l = data->root[c], *p;
		while (l)
		{
			p = l;
			l = l->next;
			delete p;
		}
	}
}

Ref<AudioClip::Player> AudioClip::player(bool loop)
{
	if (!data) return NULL;

	return Ref<Player>(new Player(*this, loop));
}

bool AudioClip::locked()
{
	if (!data) return true;

	return data->locks;
}

float AudioClip::load(Sound sound, float limit)
{
	if (!data) return -2.0f;

	if (data->locks) return -1.0f;

	//Bind stream
	Signal source(sound, data->format);

	//Possibly create root links
	if (!data->root[0]) for (Uint32 i = 0; i < data->format.channels; ++i)
	{
		data->root[i] = data->last[i] = new Link;
	}

	//Find current links
	unsigned channels = data->format.channels;
	Link *current[PG_MAX_CHANNELS];
	for (int i = 0; i < PG_MAX_CHANNELS; ++i) current[i] = data->last[i];

	//Prep loading
	Uint64 capacity = 0, total = 0;
	if (limit <= 0.0f) capacity = ~capacity;
	else capacity = Uint64(data->format.rate * limit);
	Uint64 frame = 0;

	//Load!
	AudioScratch::Heap heap;
	while (total < capacity)
	{
		//Compute maximum chunk-size
		Uint32 amt = Link::SIZE/4, got = 0;
		if (amt > (capacity-total)) amt = (capacity-total);

		//Pull a bit of audio
		Sint32 *chanPtr[PG_MAX_CHANNELS];
		for (unsigned i=0; i<channels; ++i)
			chanPtr[i] = (Sint32*) current[i]->data;
		AudioChunk chunk(heap, data->format, chanPtr, amt,
			AudioSync(frame, frame, 0.0f, 1.0f));
		source.tick(frame);
		source.pull(chunk);
		//std::cout << ".";

		//Determine amount of audio received
		got = (source.exhausted() ? chunk.cutoff() : amt);
		data->length += got;
		total += got;

		//if (source.exhausted()) std::cout << "exhausted.";

		//Do we advance to the next link or stop?
		if (amt == Link::SIZE/4 && !source.exhausted())
		{
			for (unsigned i = 0; i < channels; ++i)
			{
				current[i] = new Link;
				data->last[i]->next = current[i];
				data->last[i] = current[i];
			}
		}
		else
		{
			break;
		}

		++frame;
	}

	//Block further editing for now
	data->locks++;

	std::cout << "loaded " << total << " samples." << std::endl;

	return total / float(data->format.rate);
}

/*Ref<Player> AudioClip::record(Signal source, float limit = 0.0f)
{
	if (data->locks) return Ref<Player>();

	if (!data->root) data->root = new Link;
}*/


AudioClip::Player::Player(const AudioClip &_clip, bool _loop) :
	loop(_loop)
{
	if (!_clip)
		reportError(L"Tried to create AudioClip::Player from null clip!");
	clip = _clip.data;

	outer.rate = 1.0f;
	outer.volume = 1.0f;
	outer.seek = 0;
	a = b = outer;

	for (Uint32 c = 0; c < PG_MAX_CHANNELS; ++c) link[c] = NULL;

	pos = 0;
	frac = 0.0f;

	++clip->locks;
}
AudioClip::Player::~Player()
{
	--clip->locks;
}

AudioFormat AudioClip::Player::format()
{
	return clip->format;
}
void AudioClip::Player::tick(Uint64 frame)
{
	queue.push(outer);
}
bool AudioClip::Player::exhausted()
{
	return (!link[0] && pos);
}
void AudioClip::Player::pull(AudioChunk &chunk)
{
	if (chunk.first())
	{
		a = b;
		queue.pull(b);
	}

	Uint32 chans = clip->format.channels;
	if (!link[0] && !pos)
	{
		for (Uint32 c = 0; c < chans; ++c) link[c] = clip->root[c];
	}

	Uint32 index = pos % (Link::SIZE/4), end = clip->length,
		have = 0, need = chunk.length();

	while (have < need)
	{
		//Read a chunk of data
		Uint32 amt = min(min(Link::SIZE/4-index, need-have), end-pos);
		for (Uint32 c = 0; c < chans; ++c)
		{
			memcpy((void*) (chunk.start(c)+have),
				(void*) (link[c]->data+4*index), 4*amt);
		}
		have += amt;

		//If we hit the end, loop or stop
		pos += amt;
		if (pos >= end)
		{
			if (loop)
			{
				pos = 0;
				index = 0;
				for (Uint32 c=0; c<chans; ++c) link[c] = clip->root[c];
				continue;
			}
			else
			{
				chunk.cutoff(have);
				for (Uint32 c=0; c<chans; ++c) link[c] = NULL;
				break;
			}
		}

		//Advance on link chain
		index += amt;
		if (index >= Link::SIZE/4)
		{
			index = 0;
			for (Uint32 c=0; c<chans; ++c) link[c] = link[c]->next;
		}
	}
}
