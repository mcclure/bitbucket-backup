#include <cmath>

#include "../effects.h"

#if PLAIDGADGET
#include "../bindings.h"
#endif


using namespace plaid;

using std::min;
using std::max;
using std::abs;


#if PLAIDGADGET

static Pan *panFac3(Sound source, float pan, float level)
	{Pan *a = new Pan(source, pan, level); a->_retain(); return a;}

static int panBindings(Scripts &scripts, asIScriptEngine *engine)
{
	AS_AUDIO_CLASS("Pan", Pan);

	//Factories
	AS_FACTORY("Pan", "Pan@ f(Sound, float p=0.0f, float l=1.0f)",
		asFUNCTION(panFac3), PGC_FUNCTION);

	//Property access
	AS_METH("Pan","float get_pan()",      asMETHODPR(Pan, pan,   (), float));
	AS_METH("Pan","float get_level()",    asMETHODPR(Pan, level, (), float));
	AS_METH("Pan","float get_left()",     asMETHODPR(Pan, left,  (), float));
	AS_METH("Pan","float get_right()",    asMETHODPR(Pan, right, (), float));
	AS_METH("Pan","void set_pan(float)",  asMETHODPR(Pan, pan,   (float),void));
	AS_METH("Pan","void set_level(float)",asMETHODPR(Pan, level, (float),void));
	AS_METH("Pan","void set_left(float)", asMETHODPR(Pan, left,  (float),void));
	AS_METH("Pan","void set_right(float)",asMETHODPR(Pan, right, (float),void));

	//Mode switching
	AS_METH("Pan", "void rampLinear()",      asMETHOD(Pan, rampLinear));
	AS_METH("Pan", "void rampExponential()", asMETHOD(Pan, rampExponential));

	return 0;
}
static Bindings bindings(L"Audio_Pan", panBindings);

#endif


Pan::Pan(Sound source, float pan, float level) :
	AudioEffect<Pan_Node>(Signal(source, AudioFormat(2, 0)),
	Pan_Node(
		level * min(max(1.0f-pan, 0.0f), 1.0f),
		level * min(max(1.0f+pan, 0.0f), 1.0f) ))
{
}
Pan::~Pan()
{
}

void Pan::rampLinear()
{
	settings.ramp = false;
}
void Pan::rampExponential()
{
	settings.ramp = true;
}

void Pan::pan(float pan)
{
	float _level = level();
	settings.left  = _level * min(max(1.0f-pan, 0.0f), 1.0f);
	settings.right = _level * min(max(1.0f+pan, 0.0f), 1.0f);
}
void Pan::level(float level)
{
	float _pan = pan();
	settings.left  = level * min(max(1.0f-_pan, 0.0f), 1.0f);
	settings.right = level * min(max(1.0f+_pan, 0.0f), 1.0f);
}
void Pan::left(float leftVolume)
{
	settings.left = leftVolume;
}
void Pan::right(float rightVolume)
{
	settings.right = rightVolume;
}

float Pan::pan()
{
	float d = abs(settings.left) - abs(settings.right);
	if (d == .0f) return .0f;
	if (d > .0f) return -1.0f + (settings.right / settings.left); //left pan
	else         return +1.0f - (settings.left / settings.right); //right pan
}
float Pan::level()
{
	return (abs(settings.left) > abs(settings.right)) ?
		settings.left : settings.right;
}
float Pan::left()
{
	return settings.left;
}
float Pan::right()
{
	return settings.right;
}

void Pan::pull(AudioChunk &chunk, const Pan_Node &a, const Pan_Node &b)
{
	//Pull source data for in-place adjustment
	source.pull(chunk);

	Sint32 *l = chunk.start(0), *r = chunk.start(1), *lend = chunk.end(0);
	float vl = a.left, vr = a.right, il, ir;

	if (a.left == b.left && a.right == b.right)
	{
		//No ramping
		while (l < lend)
		{
			*l *= vl; ++l;
			*r *= vr; ++r;
		}
	}
	else if (b.ramp && (a.left*b.left) > 0.0f && (a.right*b.right) > 0.0f)
	{
		//Exponential ramping
		il = std::pow(b.left  / a.left,  1.0f/float(chunk.length()));
		ir = std::pow(b.right / a.right, 1.0f/float(chunk.length()));

		while (l < lend)
		{
			vl *= il; *l *= vl; ++l;
			vr *= ir; *r *= vr; ++r;
		}
	}
	else
	{
		//Linear ramping
		il = (b.left  - a.left)  / float(chunk.length());
		ir = (b.right - a.right) / float(chunk.length());

		while (l < lend)
		{
			vl += il; *l *= vl; ++l;
			vr += ir; *r *= vr; ++r;
		}
	}
}

Pan_Node Pan::interpolate(const Pan_Node &a, const Pan_Node &b, float mid)
{
	float l, r;

	if (b.ramp && (a.left*b.left) > 0.0f && (a.right*b.right) > 0.0f)
	{
		l = a.left * std::pow(b.left/a.left, mid);
		r = a.right * std::pow(b.right/a.right, mid);
	}
	else
	{
		l = a.left + (b.left-a.left);
		r = a.right + (b.right-a.right);
	}

	return Pan_Node(l, r, 0);
}
