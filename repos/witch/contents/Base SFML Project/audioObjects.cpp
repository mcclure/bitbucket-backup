#include "globals.h"
#include "audioObjects.h"
#include "scene.h"

#include <plaid/audio.h>
#include <plaid/audio/synth.h>
#include <plaid/audio/effects.h>
#include <plaid/audio/util.h>

using namespace plaid;
Ref<Audio> audio;

// SUPPORT CLASSES

#define FDEF 440.0
#define ADEF 1.0

// I DON'T WANT TO WRITE STEREO
void autofillChannel(AudioChunk &chunk) {
	//Copy signal into all other channels
	for (uint32_t c = chunk.channels()-1; c > 0; --c)
	{
		std::memcpy(chunk.start(c), chunk.start(0), 4*chunk.length());
	}
}

struct SimpleSynth : public AudioSynth<_OscillatorState> {
	SimpleSynth(float amp = ADEF, float frequency = FDEF, AudioFormat format = audio->format()) :
		AudioSynth<State>(AudioFormat(1, format.rate), State(amp, frequency, 0.0f)) {}
	
	float frequency() const       {return state.freq;}
	void frequency(float freq)    {state.freq = freq;}

	typedef _OscillatorState State;
	virtual State interpolate(const State &a, const State &b, float mid) {
		return b;
	}
	virtual void gen(int32_t *begin, int32_t *end) = 0;
	virtual void pull(AudioChunk &chunk, const State &a, const State &b) {
		gen(chunk.start(0), chunk.end(0));
		autofillChannel(chunk);
	}
};

class SimpleNode { public: SimpleNode(float p=1.0f) : param(p) {} float param; };
class SimpleEffect : public AudioEffect<SimpleNode>
{
public:
	//Bind to signal
	SimpleEffect(Signal _source, float param = 1.0f) : AudioEffect<SimpleNode>(_source, SimpleNode(param)) {}
	virtual ~SimpleEffect() {}

protected:
	virtual void gen(int32_t *i, int32_t *e) = 0;
	virtual void pull(AudioChunk &chunk, const SimpleNode &a, const SimpleNode &b) {
		source.pull(chunk);
		gen(chunk.start(0), chunk.end(0));
		autofillChannel(chunk);
	}
	virtual SimpleNode interpolate(const SimpleNode &a, const SimpleNode &b, float mid) {
		return b;
	}
	virtual void effectTick(Uint64) {}
};

class MergeEffect : public SimpleEffect
{
public:
	MergeEffect(Signal _source, Signal _source2, float param = 1.0f) : SimpleEffect(_source, param), source2(_source2) {}
	virtual ~MergeEffect() {}

	virtual void gen(int32_t*,int32_t*) {} // DEAD CODE
	virtual void gen(int32_t *i, int32_t *e, int32_t *i2, int32_t *e2) {}

	virtual void pull(AudioChunk &chunk, const SimpleNode &a, const SimpleNode &b) {
		AudioChunk tmp(chunk, chunk.length(), source.format()); // For source2 pull
		if (!tmp.ok()) {chunk.silence(); return;}
	
		source.pull(chunk);
		source2.pull(tmp);
		
		gen(chunk.start(0), chunk.end(0), tmp.start(0), tmp.end(0));
		autofillChannel(chunk);
	}
protected:
	Signal source2;
};

// GENERATION CLASSES

struct NoiseSynth : public SimpleSynth
{
	NoiseSynth(float amp = ADEF, float frequency = FDEF) :
		SimpleSynth(amp, frequency) {}

	void gen(int32_t *i, int32_t *e) { 
		while (i != e) {
			*i = float(random())/RANDOM_MAX * AudioFormat::INT24_CLIP;
			i++;
		}
	}
};

struct CrushEffect : public SimpleEffect
{
	float theta;
	int32_t last;

	CrushEffect(Signal _source, float param = 1.0f) :
		SimpleEffect(_source, param), theta(300000), last(0) {}
	
	void gen(int32_t *i, int32_t *e) {
		while (i != e) {
			theta = theta + 1;
			if (theta >= settings.param) {
				theta = fmod(theta, settings.param);
				last = *i;
			}
			*i = last;
			i++;
		}
	}
};

struct ModulateEffect : public MergeEffect
{
	ModulateEffect(Signal _source, Signal _source2, float param = 0.0f) : MergeEffect(_source, _source2, param) {}
	virtual ~ModulateEffect() {}
	
	void gen(int32_t *i, int32_t *e, int32_t *i2, int32_t *e2) {
		while (i != e) {
			float carrier = *i; float m = float(*i2)/(AudioFormat::INT24_CLIP*2) + 0.5;
			*i = carrier*m;
			i++, i2++;
		}
	}

};

// NETWORK SETUPS

// SYSTEM

// --- PARAMS
float audioparam[AUDIOPARAMS] = {1,1,1};
int selected_param;
const float semitone = pow(2, 1.0/12);
void debug_audioparam_ptr(int dir) {
	selected_param = (selected_param + dir + AUDIOPARAMS) % AUDIOPARAMS;
	fprintf(stderr, "%d selected\n", selected_param);
}
void debug_audioparam_incr(int dir) {
	audioparam[selected_param] *= pow(semitone, dir);
	fprintf(stderr, "Param %d = %f\n", selected_param, audioparam[selected_param]);
}

// -- ACTUALLY DO STUFF

vector<AudioStream *> playing;
void pressPlay(AudioStream *s) {
	playing.push_back(s);
	audio->play(s);
}

#define MINIMAL 0
#define BUZZER (scene->song == SONG_BUZZER)
#define WANDER (scene->song == SONG_WANDER)


#define PARAM_BUZZ_BASE  0
#define PARAM_BUZZ_WHINE 1
#define PARAM_BUZZ_TREM  2

#define BUZZ_UNSAFE (AUDIOPARAMS <= PARAM_BUZZ_TREM)

Pitch *basePitch = NULL, *whinePitch = NULL, *tremPitch = NULL;

void setupAudio_minimalTest() {
	Amp *a = new Amp(new Oscillator(audio->format(), 440, Oscillator::SINE, 1), 0.5);
	pressPlay( a );
}

void setupAudio_buzzer() {
	if (BUZZ_UNSAFE) return; // Not enough params for this
	
	Mixer *m = new Mixer(audio->format());
	Amp *a;

	// Slow ring modulator
	a = new Amp( new ModulateEffect(
		basePitch = new Pitch(new Oscillator(audio->format(), scene->songFrequency, Oscillator::SINE, 1), 1),
		new Oscillator(audio->format(), 0.5, Oscillator::SINE, 1)
	), 0.25);
	m->play(a);

	// Fancy ring modulator
	a = new Amp( new ModulateEffect(
		whinePitch = new Pitch(new Oscillator(audio->format(), scene->songFrequency, Oscillator::SQUARE, 1), 1),
		tremPitch = new Pitch(new Oscillator(audio->format(), 50, Oscillator::SQUARE, 1), 1)
	), 0.25);
	m->play(a);
	
	pressPlay( new CrushEffect(m, 4) );
}

void updateAudio_buzzer() {
	if (BUZZ_UNSAFE) return; // Not enough params for this

	if (PARAM_BUZZ_BASE >= 0) {
		basePitch->rate(audioparam[PARAM_BUZZ_BASE]);
	}
	if (PARAM_BUZZ_WHINE >= 0) {
		whinePitch->rate(audioparam[PARAM_BUZZ_WHINE]);
	}
	if (PARAM_BUZZ_TREM >= 0) {
		tremPitch->rate(audioparam[PARAM_BUZZ_TREM]);
	}
}

#define WANDERERS (scene->songWanderers)
#define WANDA (0.75/WANDERERS)
const float WANDSTEP = pow(semitone, 1/12.0f);
#define WANDLOW 0.25
#define WANDHIGH 4

#define WANDPROB(n) 1
#define WANDFREQ(n) audioparam[n*2]
#define WANDSTEPMOD(n) audioparam[n*2+1]

#define WANDER_UNSAFE (AUDIOPARAMS < WANDERERS*2)

vector< Pitch * > wanderers;
void setupAudio_wander() {
	if (WANDER_UNSAFE) return; // Not enough params for this
	
	Mixer *m = new Mixer(audio->format());
	
	for(int c = 0; c < WANDERERS; c++) {
		WANDFREQ(c) = 1;
		WANDSTEPMOD(c) = WANDSTEP;
	
		Pitch *p = new Pitch( new Oscillator(audio->format(), scene->songFrequency, Oscillator::SINE, 1), 1);
		wanderers.push_back( p );
		m->play(p);
	}
	
	pressPlay( m );
}

void updateAudio_wander() {
	if (WANDER_UNSAFE) return; // Not enough params for this

	for(int c = 0; c < WANDERERS; c++) {
		int move = random()%3 - 1;
		if (move)
			WANDFREQ(c) *= pow(WANDSTEPMOD(c), move);
		wanderers[c]->rate( WANDFREQ(c) );
	}
}

// SWITCHER

void setupAudio() {
	audio = new Audio();

	if (MINIMAL)
		setupAudio_minimalTest();

	if (BUZZER)
		setupAudio_buzzer();
	
	if (WANDER)
		setupAudio_wander();
}

void updateAudio() {
	if (BUZZER)
		updateAudio_buzzer();
	
	if (WANDER)
		updateAudio_wander();

	audio->update();
}

void teardownAudio() {
	FOREACH( AudioStream *s, playing )
		audio->stop( s );
	}
	playing.clear();
	wanderers.clear();
	audio = NULL;
}