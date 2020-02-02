/*
 *  plaidext.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 12/23/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _PLAIDEXT_H
#define _PLAIDEXT_H

// Jumpcore specific plaid stuff

#include "kludge.h"
#include <plaid/audio.h>
#include <plaid/audio/synth.h>
#include <plaid/audio/effects.h>
#include <plaid/audio/util.h>

using namespace plaid;
extern Ref<Audio> audio;

#define FDEF 440.0
#define ADEF 1.0

// I DON'T WANT TO WRITE STEREO
inline void autofillChannel(AudioChunk &chunk) {
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
	virtual void effectTick(plaid::Uint64) {}
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
	float baseValue, valueScale;
	ModulateEffect(Signal _source, Signal _source2, float param = 0.0f, float _minAmp = 0.0f) : MergeEffect(_source, _source2, param) {
		_minAmp = ::min<float>(_minAmp, 1);
		baseValue = _minAmp;
		valueScale = (1.0-_minAmp);
	}
	virtual ~ModulateEffect() {}
	
	void gen(int32_t *i, int32_t *e, int32_t *i2, int32_t *e2) {
		while (i != e) {
			float carrier = *i;
			float m = (float(*i2)/(AudioFormat::INT24_CLIP*2) + 0.5)*valueScale + baseValue;
			*i = carrier*m;
			i++, i2++;
		}
	}

};

#endif