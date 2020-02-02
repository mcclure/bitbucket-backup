#ifndef PLAIDGADGET_AUDIO_SYNTH_H
#define PLAIDGADGET_AUDIO_SYNTH_H


#include <iostream>  //TODO remove the sanity check

#include "audio.h"

#include "../thread/lockfree.h"


/**
	AudioStream classes for synthesis of simple sounds.
*/
namespace plaid
{
	/*
		A convenient base class for various synthesizers.

		'State' is the datatype which represents the per-frame settings of
			this effect stream.  Each frame, the value of 'settings' will be
			queued; it will be used as the 'b' value for that frame and the 'a'
			value of the next, facilitating easy frame-to-frame interpolation.
	*/
	template<typename State>
	class AudioSynth : public AudioStream
	{
	protected:
		AudioSynth(AudioFormat f, State s) :
			output(f), state(s), _a(s), _b(s) {}
		virtual ~AudioSynth() {}

		//Override these variants
		virtual void pull(AudioChunk &chunk, const State &a, const State &b)=0;
		virtual void synthTick(Uint64 frame) {}

		//Interpolate between two Settings, for split renders
		virtual State interpolate(const State &a, const State &b, float mid)=0;

		//Tick and pull
		virtual void pull(AudioChunk &chunk)
			{if (chunk.first()) {_a = _b; if(!queue.pull(_b))
				std::cout << "SYNTH QUEUE STARVED" << std::endl;}
			State pm = _m;
			pull(chunk, chunk.first() ? _a : pm,
				chunk.last() ? _b : (_m = interpolate(_a,_b,chunk.b())));}
		virtual void tick(Uint64 frame) {queue.push(state);synthTick(frame);}

		//Base this stuff on the source stream, generally
		virtual bool exhausted()        {return false;}
		virtual AudioFormat format()    {return output;}

	protected:
		AudioFormat output;

		//Do not access this in the pull method!
		State state;

	private:
		LockFreeQueue<State> queue;
		State _a, _b, _m;
	};

	/*
		An infinite stream of silence.  For when no other sound will do.
			This stream is uniquely agnostic to output formatting.
	*/
	class Silence : public AudioStream
	{
	public:
		Silence(AudioFormat format) : output(format) {}
		~Silence() {}

	protected:
		virtual bool exhausted()             {return false;}
		virtual AudioFormat format()         {return output;}
		virtual void tick(Uint64 frame)      {}
		virtual void pull(AudioChunk &chunk) {chunk.silence();}

	private:
		AudioFormat output;
	};


	struct _OscillatorState {float amp, freq, sweep; _OscillatorState() {}
		_OscillatorState(float a,float f,float s):amp(a),freq(f),sweep(s){}};
	/*
		An oscillator that can generate simple waveforms.

		It also WILL SOON support FM and PM synthesis.
	*/
	class Oscillator : public AudioSynth<_OscillatorState>
	{
	public:
		//An oscillator's waveform cannot be changed.
		enum WAVEFORMS {
			SINE = 0,
			SQUARE = 1,
			TRIANGLE = 2,
			SAWTOOTH = 3, //"Upwards"; use a negative amplitude for opposite.

			//An alarming buzz; also plays when an invalid value is supplied.
			KLAXON = 10,
			};

	public:
		/*
			A regular oscillator with a steady (but changeable) tone.
				No tone/pitch interpolation for now.

			Phase ranges from -1 to 1 and dictates where in the waveform the
				oscillator begins.
		*/
		Oscillator(AudioFormat format, float frequency, Uint32 type = SINE,
			float amp = .25f, float phase = 0.0f);
		~Oscillator();

		float frequency() const       {return state.freq;}
		void frequency(float freq)    {state.freq = freq;}

	protected:
		typedef _OscillatorState State;
		virtual void pull(AudioChunk &chunk, const State &a, const State &b);
		virtual State interpolate(const State &a, const State &b, float mid);

	private:
		Uint32 type;
		float phase;
	};

	/*
		Produces noise in one of a few colors, defaulting to pink.
	*/
	/*class Noise : public AudioStream_Single
	{
	public:

	public:
		Noise()
	};*/

	/*
		Produces 'impulses' (clicks or pops) of customizable magnitude.
	*/
	class Impulser
	{
	};
}


#endif // PLAIDGADGET_AUDIO_SYNTH_H
