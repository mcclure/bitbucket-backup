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

using namespace plaid;
extern Ref<Audio> audio;

struct BaseSynth : public AudioSynth<_OscillatorState>
{
	BaseSynth(AudioFormat format, float frequency, float amp) :
		AudioSynth<State>(AudioFormat(1, format.rate), State(amp, frequency, 0.0f)) {}

	float frequency() const       {return state.freq;}
	void frequency(float freq)    {state.freq = freq;}

	typedef _OscillatorState State;
	virtual State interpolate(const State &a, const State &b, float mid) {
		return b;
	}
	virtual void pull(AudioChunk &chunk, const State &a, const State &b);
};

struct NoiseSynth : public BaseSynth
{
	NoiseSynth(AudioFormat format, float frequency, float amp) :
		BaseSynth(format, frequency, amp) {}
		
	virtual void pull(AudioChunk &chunk, const State &a, const State &b);
};

#endif