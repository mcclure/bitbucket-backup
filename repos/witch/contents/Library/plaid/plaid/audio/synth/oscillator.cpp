#include <math.h>
#include <complex>

#if PLAIDGADGET
#include "../../geometry/2d.h"
#else
#define PI 3.141592654f
#endif

#include "../synth.h"


using namespace plaid;


Oscillator::Oscillator(AudioFormat format, float frequency, Uint32 _type,
	float amp, float _phase) :
	AudioSynth<State>(AudioFormat(1, format.rate),
		State(amp, frequency, 0.0f)), type(_type), phase(_phase)
{

}
Oscillator::~Oscillator()
{

}

void Oscillator::pull(AudioChunk &chunk, const State &a, const State &b)
{
	//Phase goes from -1 to 1.  Wierd?  Maybe.
	while (phase >= 1.0f) phase -= 2.0f;
	while (phase < -1.0f) phase += 2.0f;

	Sint32 samp;
	float v, x, sq;

	float step = (b.freq / output.rate) * 2.0f,
		amp = b.amp * AudioFormat::INT24_CLIP,
		ph = phase;

	Sint32 *i = chunk.start(0), *e = chunk.end(0);
	switch (type)
	{
	case SINE:
		while (i != e)
		{
			//Simple sine wave
			samp = amp*sin(PI * ph);
			ph += step;
			*i = samp; ++i;
			ph -= 2.0f*int(ph); //Restrict to [-1, 1]
		}
		break;

	case SQUARE:
		while (i != e)
		{
			samp = amp * ((ph>=0.0f) ? 1.0f : -1.0f);
			ph += step;
			*i = samp; ++i;
			ph -= 2.0f*int(ph); //Restrict to [-1, 1]
		}
		break;

	case TRIANGLE:
		while (i != e)
		{
			samp = amp * (2.0f*std::abs(ph) - 1.0f);
			ph += step;
			*i = samp; ++i;
			ph -= 2.0f*int(ph); //Restrict to [-1, 1]
		}
		break;

	case SAWTOOTH:
		while (i != e)
		{
			samp = amp * ph;
			ph += step;
			*i = samp; ++i;
			ph -= 2.0f*int(ph); //Restrict to [-1, 1]
		}
		break;

	case KLAXON:
	default:
	{
		const float PISQ = PI*PI;
		while (i != e)
		{
			//Sine wave approximation gone wrong
			sq = PISQ*ph*ph; x = PI*ph*amp;
			v = x * 1.02394347; //Makes the endpoints line up
			x *= sq; v -= x/6.0f;
			x *= sq; v += x/120.0f;
			x *= sq; v -= x/5040.0f;
			samp = x;
			ph += step;
			*i = samp; ++i;

			ph -= 2.0f*int(ph); //Restrict to [-1, 1]
		}
	}
		break;
	}

	phase = ph;

	//Copy signal into all other channels
	for (Uint32 c = chunk.channels()-1; c > 0; --c)
	{
		std::memcpy(chunk.start(c), chunk.start(0), 4*chunk.length());
	}
}
Oscillator::State Oscillator::interpolate(const State&a,const State&b,float mid)
{
	return b;
}
