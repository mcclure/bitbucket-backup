#if PLAIDGADGET
#include "../../geometry/2d.h"
#else
#define PI 3.141592654f
#endif

#include "../effects.h"


using namespace plaid;
using namespace std;


const float Filter::BOTTOM = 20.0f / 1024.0f, Filter::TOP = 20000.0f * 1024.0f;


Filter::Filter(Signal source, float low, float high) :
	AudioEffect<Filter_Node>(source, Filter_Node(low, high))
{
	for (Uint32 i = 0; i < PG_MAX_CHANNELS; ++i) hps[i] = lps[i] = 0.0f;
}
Filter::~Filter()
{
}


void Filter::bandpass(float lowFreq, float highFreq)
{
	volatile float f;
	f = lowFreq;
	if (f != f || f > 1e18f)
	{
		std::cout << "Bad highpass frequency: " << f << std::endl;
		lowFreq = 0.0f;
	}
	f = highFreq;
	if (f != f || f > 1e18f)
	{
		std::cout << "Bad lowpass frequency: " << f << std::endl;
		highFreq = 0.0f;
	}

	//Adjust frequencies
	settings.lp = highFreq;
	settings.hp = lowFreq;
}
void Filter::bandstop(float lowFreq, float highFreq)
{
	//Adjust frequencies
	settings.lp = -lowFreq;
	settings.hp = -highFreq;
}
void Filter::lowpass(float freq)   {bandpass(0.0f, freq);}
void Filter::highpass(float freq)  {bandpass(freq, 0.0f);}
void Filter::off()                 {bandpass(0.0f, 0.0f);}

static const float RCCONV = .5f / PI;

static const float CLIP = float(AudioFormat::INT24_CLIP);

void Filter::pull(AudioChunk &chunk,
	const Filter_Node &a, const Filter_Node &b)
{
	//Pull source data
	source.pull(chunk);

	//Calculate RC multipliers
	float
		al = RCCONV / ((a.lp>0.0f)?a.lp:TOP),
		ah = RCCONV / ((a.hp>0.0f)?a.hp:BOTTOM),
		bl = RCCONV / ((b.lp>0.0f)?b.lp:TOP),
		bh = RCCONV / ((b.hp>0.0f)?b.hp:BOTTOM);
	float
		lpRC = al, lpM = pow(bl/al, 1.0f / float(chunk.length())),
		hpRC = ah, hpM = pow(bh/ah, 1.0f / float(chunk.length())),
		lpA, hpA, samp,
		dt = 1.0f / float(chunk.format().rate);

	//Apply effect!
	Uint32 chan = source.format().channels;

	for (Uint32 i = 0; i < chan; ++i)
	{
		Sint32 *pos = chunk.start(i), *end = chunk.end(i);
		float &lpc = lps[i], &hpc = hps[i];

		//Temporary bugfix...
		//lpc = std::min(std::max(lpc, -CLIP), CLIP);
		//hpc = std::min(std::max(hpc, -CLIP), CLIP);

		while (pos < end)
		{
			//Interpolate settings
			lpA = dt / (lpRC + dt); lpRC *= lpM;
			hpA = dt / (hpRC + dt); hpRC *= hpM;

			/*if (lpA < 0.0f || !(lpA < 1.0f))
			{
				std::cout << "Aberrant lowpass alpha: " << lpA << std::endl;
				std::cout << "  Lowpass RC: " << lpRC
					<< " (" << al << " -> " << bl << ")"
					<< " (" << a.lp << "hz -> " << b.lp << "hz)"
					<< std::endl;
			}
			if (hpA < 0.0f || !(hpA < 1.0f))
			{
				std::cout << "Aberrant highpass alpha: " << hpA << std::endl;
				std::cout << "  Highpass RC: " << hpRC
					<< " (" << ah << " -> " << bh << ")"
					<< " (" << a.hp << "hz -> " << b.hp << "hz)"
					<< std::endl;
			}*/

			//Get samples
			samp = float(*pos);

			//Lowpass
			lpc += lpA * (samp-lpc);

			//Highpass (implemented as subtractive lowpass)
			hpc += hpA * (lpc-hpc);

			//Mix
			samp = lpc - hpc;

			//Set samples
			*pos = Sint32(samp);
			++pos;
		}
	}
}

Filter_Node Filter::interpolate(
	const Filter_Node &a, const Filter_Node &b, float mid)
{
	if (mid == 0.0f) return a;
	if (mid == 1.0f) return b;
	float
		al = ((a.lp>0.0f)?a.lp:TOP),
		ah = ((a.hp>0.0f)?a.hp:BOTTOM),
		bl = ((b.lp>0.0f)?b.lp:TOP),
		bh = ((b.hp>0.0f)?b.hp:BOTTOM);
	return Filter_Node(al*std::pow(bl/al,mid),ah*std::pow(bh/ah,mid));
}
