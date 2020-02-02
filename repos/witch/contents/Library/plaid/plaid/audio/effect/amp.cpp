#include <cmath>

#include "../effects.h"

#if PLAIDGADGET
#include "../bindings.h"
#endif


using namespace plaid;


#if PLAIDGADGET

static Amp *ampFac2(Sound source, float volume)
	{Amp *a = new Amp(source, volume); a->_retain(); return a;}
static Amp *ampFac1(Sound source)
	{Amp *a = new Amp(source); a->_retain(); return a;}

static int ampBindings(Scripts &scripts, asIScriptEngine *engine)
{
	AS_AUDIO_CLASS("Amp", Amp);

	//Factories
	AS_FACTORY("Amp", "Amp@ f(Sound)",       asFUNCTION(ampFac1), PGC_FUNCTION);
	AS_FACTORY("Amp", "Amp@ f(Sound,float)", asFUNCTION(ampFac2), PGC_FUNCTION);

	//Property access
	AS_METH("Amp", "float get_volume()",
		asMETHODPR(Amp, volume,   (), float));
	AS_METH("Amp", "float get_decibels()",
		asMETHODPR(Amp, decibels, (), float));
	AS_METH("Amp", "void set_volume(float)",
		asMETHODPR(Amp, volume,   (float), void));
	AS_METH("Amp", "void set_decibels(float)",
		asMETHODPR(Amp, decibels, (float), void));

	//Mode switching
	AS_METH("Amp", "void rampLinear()",      asMETHOD(Amp, rampLinear));
	AS_METH("Amp", "void rampExponential()", asMETHOD(Amp, rampExponential));

	AS_FUNC("float Decibels(float)",   asFUNCTION(Decibels));
	AS_FUNC("float ToDecibels(float)", asFUNCTION(ToDecibels));

	return 0;
}
static Bindings bindings(L"Audio_Amp", ampBindings);

#endif


Amp::Amp(Signal source, float volume) :
	AudioEffect<Amp_Node>(source, Amp_Node(volume, 0))
{
}
Amp::~Amp()
{
}

void Amp::rampLinear()
{
	settings.ramp = false;
}
void Amp::rampExponential()
{
	settings.ramp = true;
}

void Amp::pull(AudioChunk &chunk, const Amp_Node &a, const Amp_Node &b)
{
	//Pull source data for in-place adjustment
	source.pull(chunk);

	float vol, inc;
	Sint32 iVol, iInc, iInit; // iSamp, iLow, iMid, iHigh, iVA, iVB, iSA, iSB; // Unused
	Uint32 chan = source.format().channels;

	if (a.vol == b.vol)
	{
		//No ramping needed
		vol = a.vol;
		for (Uint32 i = 0; i < chan; ++i)
		{
			Sint32 *pos = chunk.start(i), *end = chunk.end(i);
			while (pos < end) *(pos++) *= vol;
		}
	}
	else if (b.ramp && (a.vol*b.vol) > 0.0f)
	{
		//Exponential ramping
		inc = std::pow(b.vol / a.vol, 1.0f/float(chunk.length()));

		for (Uint32 i = 0; i < chan; ++i)
		{
			Sint32 *pos = chunk.start(i), *end = chunk.end(i);
			vol = a.vol;
			while (pos < end)
			{
				vol *= inc;
				*(pos++) *= vol;
			}
		}

		/*float error = (vol - b.vol) / std::abs(b.vol - a.vol);
		if (error > .001f || error < -.001f)
		{
			std::cout << (error*100.0f) << "% error in amp exponential ramp "
				<< "(mode " << b.ramp << ") "
				<< a.vol << " -> " << b.vol << " over " << chunk.length()
				<< std::endl;
		}*/
	}
	else
	{
		//Linear ramping
		float inc = (b.vol - a.vol) / float(chunk.length());
		iInit = 65536.0f * a.vol;
		iInc = 65536.0f * inc;

		for (Uint32 i = 0; i < chan; ++i)
		{
			Sint32 *pos = chunk.start(i), *end = chunk.end(i);
			vol = a.vol;
			iVol = iInit;
			while (pos < end)
			{
				/*iSamp = *pos;
				iVA = (iVol >>16);  iVB = (iVol &0xFFFF);
				iSA = (iSamp>>16);  iSB = (iSamp&0xFFFF);
				iLow = iVB*iSB;
				iMid = iVA*iSB + iVB*iSA;
				iHigh = iVA*iSA;
				*(pos++) = (iHigh << 16) + iMid + (iLow >> 16);
				iVol += iInc;*/

				*(pos++) *= vol;
				vol += inc;
			}
		}

		/*float error = (vol - b.vol) / std::abs(b.vol - a.vol);
		if (error > .001f || error < -.001f)
		{
			std::cout << (error*100.0f) << "% error in amp linear ramp "
				<< a.vol << " -> " << b.vol << " over " << chunk.length()
				<< std::endl;
		}*/
	}
}

Amp_Node Amp::interpolate(const Amp_Node &a, const Amp_Node &b, float mid)
{
	if (b.ramp && (a.vol*b.vol) > 0.0f)
		return Amp_Node(a.vol * std::pow(b.vol/a.vol, mid), 1);
	else
		return Amp_Node(a.vol + mid*(b.vol-a.vol), 0);
}
