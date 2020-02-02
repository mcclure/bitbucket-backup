#include <cmath>
#include <iostream>

#include "../effects.h"

#if PLAIDGADGET
#include "../bindings.h"
#endif


using namespace plaid;


#if PLAIDGADGET

static Pitch *pitchFac2(Sound source, float rate)
	{Pitch *p = new Pitch(source, rate); p->_retain(); return p;}
static Pitch *pitchFac1(Sound source)
	{Pitch *p = new Pitch(source); p->_retain(); return p;}

static int pitchBindings(Scripts &scripts, asIScriptEngine *engine)
{
	AS_AUDIO_CLASS("Pitch", Pitch);

	//Factories
	AS_FACTORY("Pitch", "Pitch@ f(Sound)",
		asFUNCTION(pitchFac1), PGC_FUNCTION);
	AS_FACTORY("Pitch", "Pitch@ f(Sound,float)",
		asFUNCTION(pitchFac2), PGC_FUNCTION);

	//Property access
	AS_METH("Pitch", "float get_rate()",
		asMETHODPR(Pitch, rate, (), float));
	AS_METH("Pitch", "float get_note()",
		asMETHODPR(Pitch, note, (), float));
	AS_METH("Pitch", "void set_rate(float)",
		asMETHODPR(Pitch, rate, (float), void));
	AS_METH("Pitch", "void set_note(float)",
		asMETHODPR(Pitch, note, (float), void));

	//TODO algorithm choice?

	AS_FUNC("float Semitones(float)",   asFUNCTION(Semitones));
	AS_FUNC("float ToSemitones(float)", asFUNCTION(ToSemitones));

	return 0;
}
static Bindings bindings(L"Audio_Pitch", pitchBindings);

#endif


Pitch::Pitch(Signal source, float rate, Uint32 alg) :
	AudioEffect<Pitch_Node>(source, Pitch_Node(rate, alg))
{
	offset = 0.0f;
	for (int i = 4*source.format().channels; i--;) mem.push_back(0);
}
Pitch::~Pitch()
{
}

void Pitch::resampling(Uint32 alg)
{
	settings.alg = alg;
}


void Pitch::pull(AudioChunk &chunk, const Pitch_Node &a, const Pitch_Node &b)
{
	Uint32 length = chunk.length(), chan = source.format().channels;

	/*{
		static float sanity = 0.0f;
		if (a.rate != sanity)
			std::cout << "TIME BREAK " << sanity << " -> " << a.rate << std::endl;
		sanity = b.rate;
	}*/

	//Compute rate and offset
	float rate = a.rate, inc = std::pow(b.rate/a.rate, 1.0f/float(length));
	float off = offset;

	//"simulate" the render to figure out how many samples are needed.
	Uint32 count = 0, steps;
	for (Uint32 i = length; i--;)
	{
		off += rate; rate *= inc;
		steps = off; off -= steps;
		count += steps;
	}

	//Allocate temp buffer from scratch pool; account for failure
	AudioChunk sub(chunk, (4 + count), source.format());
	if (!sub.ok()) {chunk.silence(); return;}

	//Pull source audio into temp buffer
	{
		//Copy from memory to beginning of buffer
		Sint32 *forward[PG_MAX_CHANNELS];
		for (Uint32 i=0; i<chunk.format().channels; ++i)
		{
			std::memcpy((void*) sub.start(i), (void*) &mem[4*i], 4*4);
			forward[i]=sub.start(i)+4;
		}

		//Fill rest of buffer with new data from source stream
		AudioChunk pull(chunk.scratch(), chunk.format(), forward, count,
			chunk.sync);
		source.pull(pull);

		//Copy from end of buffer to memory
		for (Uint32 i=0; i<chunk.format().channels; ++i)
		{
			std::memcpy((void*) &mem[4*i], (void*) (sub.end(i)-4), 4*4);
		}
	}

	//Resample
	for (Uint32 i = 0; i < chan; ++i)
	{
		float c1, c2, c3;
		rate = a.rate;
		off = offset;
		Sint32 *in = sub.start(i), *out = chunk.start(i), *end = chunk.end(i);

		while (out < end)
		{
			//4-point, 3rd order hermite resampling
			off += rate; rate *= inc;
			steps = off; off -= steps;
			in += steps;

			//*data++ = s0;

			/*sampL = l1*off + l0*(1.0f-off);
			sampR = r1*off + r0*(1.0f-off);
			*data++ = Sint16(sampL + .5f - (sampL < 0.0f));
			*data++ = Sint16(sampR + .5f - (sampR < 0.0f));*/

			c1 = .5f*(in[2]-in[0]);
			c2 = in[0] - 2.5f*in[1] + 2.0f*in[2] - .5f*in[3];
			c3 = .5f*(in[3]-in[0]) + 1.5f*(in[1]-in[2]);
			*out = in[1] + ((c3*off+c2)*off+c1)*off;

			++out;
		}
	}

	offset = off;
}

Pitch_Node Pitch::interpolate(const Pitch_Node &a, const Pitch_Node &b,
	float mid)
{
	return Pitch_Node(a.rate*std::pow(b.rate/a.rate, mid), b.alg);
}
