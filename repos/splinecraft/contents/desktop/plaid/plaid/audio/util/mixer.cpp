#include <iostream>
#include <cstring>

#include "../util.h"


using namespace plaid;




void Mixer::play()
{
	if (!extPlay)
	{
		actions.push(Action(GPLAY, NULL, true), extFrame);
		extPlay = true;
	}
}
void Mixer::pause()
{
	if (extPlay)
	{
		actions.push(Action(GPLAY, NULL, false), extFrame);
		extPlay = false;
	}
}
void Mixer::volume(float volume)
{
	if (volume < 0.0f) volume = 0.0f;
	if (extVolume != volume)
	{
		actions.push(Action(GVOLUME, NULL, volume), extFrame);
		extVolume = volume;
	}
}

Signal *Mixer::findOrAdd(Sound sound)
{
	//Find
	Signals::iterator it = signals.find(sound);
	if (it != signals.end()) return &it->second;

	//...Or add
	Signal sig(sound, output);
	if (sig.null()) return NULL;
	Signal *p = &signals.insert(SignalsEntry(sound, sig)).first->second;
	external.insert(ChannelsEntry(p, Channel()));

	//Notify pull thread
	actions.push(Action(ADD, p, 1.0f), extFrame);

	return p;
}

bool Mixer::add(Sound sound)
{
	//Return false if we've already got it
	if (signals.find(sound) != signals.end()) return false;
	return bool(findOrAdd(sound));
}

void Mixer::drop(Sound sound)
{
	//Find the thing
	Signals::iterator it = signals.find(sound);
	if (it == signals.end()) return;

	//Mark it terminal
	Channel &c = external[&it->second];
	if (c.drop) return;
	c.drop = true;

	//Notify pull thread
	actions.push(Action(DROP, &it->second, 0.0f), extFrame);
}

void Mixer::play(Sound sound)
{
	//Autobind
	Signal *p = findOrAdd(sound);
	if (!p) return;
	Channel &c = external[p];

	//State change
	if (!c.play)
	{
		actions.push(Action(PLAY, p, true), extFrame);
		c.play = true;
	}
}
void Mixer::pause(Sound sound)
{
	//Autobind
	Signal *p = findOrAdd(sound);
	if (!p) return;
	Channel &c = external[p];

	//State change
	if (c.play)
	{
		actions.push(Action(PLAY, p, false), extFrame);
		c.play = false;
	}
}
void Mixer::volume(Sound sound, float volume)
{
	if (volume < 0.0f) volume = 0.0f;

	//Autobind
	Signal *p = findOrAdd(sound);
	if (!p) return;
	Channel &c = external[p];

	//State change
	if (c.volume != volume)
	{
		actions.push(Action(VOLUME, p, volume), extFrame);
		c.volume = volume;
	}
}

bool Mixer::has(Sound sound)
{
	return (signals.find(sound) != signals.end());
}
bool Mixer::playing(Sound sound)
{
	Signals::iterator it = signals.find(sound);
	if (it == signals.end()) return false;
	return external[&it->second].play;
}
float Mixer::volume(Sound sound)
{
	Signals::iterator it = signals.find(sound);
	if (it == signals.end()) return 0.0f;
	return external[&it->second].volume;
}

/*bool Mixer::clipping(bool reset)
{
}*/


AudioFormat Mixer::format()
{
	return output;
}


Mixer::Mixer(AudioFormat format, bool exhausts) :
	output(format), exhaustible(exhausts)
{
	extVolume = intVolume = 1.0f;
	extPlay = intPlay = true;
	extFrame = 0;
}

Mixer::~Mixer()
{
}

bool Mixer::exhausted()
{
	return exhaustible && !internal.size();
}

void Mixer::tick(Uint64 frame)
{
	//All pushes after the tick are in the frame AFTER this one.
	extFrame = frame+1;

	//Handle drops
	Signal *drop;
	while (drops.pull(drop))
	{
		if (!external.erase(drop))        reportError("Mixer drop fail A");
		if (!signals.erase(Sound(*drop))) reportError("Mixer drop fail B");
	}

	//Update playing streams
	int count = 0;
	if (extPlay)
	{
		for (Signals::iterator i = signals.begin(); i != signals.end(); ++i)
			if (external[&i->second].play)
		{
			i->second.tick(frame);
			++count;
		}
	}

	/*static int cycle = 0;
	if (++cycle & 32)
	{
		std::cout << count << " playing sounds." << std::endl;
		cycle = 0;
	}*/
}

void Mixer::pull(AudioChunk &chunk)
{
	//First handle state changes
	if (chunk.first())
	{
		//std::cout << "Mixer:" << now << std::endl;
		Action act;
		Channels::iterator i, e = internal.end();
		//if (pend.frame > now) std::cout << " wait: " << pend.frame << std::endl;
		while (actions.pull(act, chunk.frame()))
		{
			i = internal.find(act.signal);
			switch (act.code)
			{
			case GPLAY:   intPlay =   act.value; break;
			case GVOLUME: intVolume = act.value; break;
			case PLAY:    if (i!=e) i->second.play =   act.value; break;
			case VOLUME:  if (i!=e) i->second.volume = act.value; break;
			case ADD:
				//std::cout << "Mixer added a sound!" << std::endl;
				if (i==e) internal.insert(ChannelsEntry(act.signal,Channel()));
				break;
			case DROP:
				//std::cout << "Mixer stopped a sound!" << std::endl;
				if (i!=e) {internal.erase(i); drops.push(act.signal);}
				break;
			}
			/*std::cout << "  " << "0PVadpv"[act.code] << " "
				<< " " << act.signal << " " << act.value << " "
				<< "/" << now << std::endl;*/
		}
	}


	//Prep mix
	AudioChunk temp(chunk, chunk.length());
	bool tempOK = temp.ok();

	Uint32 count = chunk.length(), chans = chunk.format().channels;

	bool first = true;

	//GET ON WITH THE MIXIN'
	if (intPlay)
		for (Channels::iterator i=internal.begin(); i!=internal.end();)
	{
		//Skip paused sounds
		if (!i->second.play) {++i; continue;}

		//Compute volume
		float v = i->second.volume*intVolume;
		Sint32 mult = Sint32(256.0f * v);

		if (first && mult == 256)
		{
			//Shortcut!
			i->first->pull(chunk);
			first = false;
		}
		else
		{
			if (!tempOK) {++i; continue;}

			//Render channel chunk
			i->first->pull(temp);

			//Compute volume and (if not silent) mix!
			if (mult)
			{
				for (Uint32 chan = 0; chan < chans; ++chan)
				{
					//Core mixer code lives here!
					Sint32 *ip = temp.start(chan), *op = chunk.start(chan);
					Uint32 blocks = count/16, mod = count%16;
		#define UNROLLED(X) do {while (mod--) {X;} \
			while (blocks--) {X;X;X;X;X;X;X;X;X;X;X;X;X;X;X;X;}} while(0)
					if (first)
					{
						//First case should never happen in practice
						if (mult == 256) std::memcpy((void*)op, (void*)ip, 4*count);
						else UNROLLED(*(op++) =  (mult*(*(ip++)))>>8);
					}
					else
					{
						if (mult == 256) UNROLLED(*(op++) += *(ip++));
						else UNROLLED(*(op++) += (mult*(*(ip++)))>>8);
					}
		#undef UNROLLED
				}
				first = false;
			}
		}

		//Drop if exhausted
		if (i->first->exhausted())
		{
			drops.push(i->first);
			internal.erase(i++);
		}
		else ++i;
	}

	if (first)
	{
		chunk.silence();
	}
}
