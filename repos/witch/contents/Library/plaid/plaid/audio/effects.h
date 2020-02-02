#ifndef PLAIDGADGET_AUDIO_EFFECTS_H
#define PLAIDGADGET_AUDIO_EFFECTS_H


#include <iostream>  //TODO remove the sanity check
#include <cmath>
#include <vector>

#include "audio.h"

#include "../thread/lockfree.h"


namespace plaid
{
	/*
		AudioStream_Effect:  has a single input stream which it modulates and
			outputs; its exhaustion and format are generally that of the source.

		'Settings' is the datatype which represents the per-frame settings of
			this effect stream.  Each frame, the value of 'settings' will be
			queued; it will be used as the 'b' value for that frame and the 'a'
			value of the next, facilitating easy frame-to-frame interpolation.
	*/
	template<typename Settings>
	class AudioEffect : public AudioStream
	{
	protected:
		AudioEffect(Signal _source, Settings defaults) :
			source(_source), settings(defaults), _a(defaults), _b(defaults) {}
		virtual ~AudioEffect() {}

		//Override these variants
		virtual void pull(AudioChunk &chunk,
			const Settings &a, const Settings &b) = 0;
		virtual void effectTick(Uint64 frame) = 0;

		//Interpolate between two Settings, for split renders
		virtual Settings interpolate(const Settings &a, const Settings &b,
			float mid) = 0;

		//Tick and pull
		virtual void pull(AudioChunk &chunk)
			{if (chunk.first()) {_a = _b; if(!queue.pull(_b)) {
			#if STARVEWARN
				std::cout << "AUDIO EFFECT QUEUE STARVED" << std::endl;
			#endif
				}}
			Settings pm = _m;
			//std::cout << chunk.a() << "/" << chunk.b() << std::endl;
			pull(chunk, chunk.extra() ? _b : (chunk.first() ? _a : pm),
				chunk.last() ? _b : (_m = interpolate(_a,_b,chunk.b())));}
		virtual void tick(Uint64 frame)
			{source.tick(frame); queue.push(settings); effectTick(frame);}

		//Base this stuff on the source stream, generally
		virtual bool exhausted()        {return source.exhausted();}
		virtual AudioFormat format()    {return source.format();}

	protected:
		Signal source;

		//Do not access this in the pull method!
		Settings settings;

	private:
		LockFreeQueue<Settings> queue;
		Settings _a, _b, _m;
	};


	class Amp_Node {public: Amp_Node(float v=1.0f,Uint32 r=0):vol(v),ramp(r) {}
		float vol; Uint32 ramp;};
	/*
		Amp:  Used to alter the "loudness" of sound.

		Volume range is typically 0.0 - 1.0 but can be higher or negative.
			(negative values cause problems with decibel adjustment!)
		0.0 decibels is equal to volume 1; learn more about decibels online...
	*/
	class Amp : public AudioEffect<Amp_Node>
	{
	public:
		//Bind to signal
		Amp(Signal source, float volume = 1.0f);
		virtual ~Amp();

		//Set ramping mode; default is linear
		void rampLinear();
		void rampExponential();  //Will revert to linear when volume <= 0

		//Interpolate to the given volume.
		void volume(float volume)    {settings.vol = std::max(volume,.0f);}
		void decibels(float db)      {settings.vol = Decibels(db);}

		//Query current volume
		float volume()               {return settings.vol;}
		float decibels()             {return ToDecibels(settings.vol);}

	protected:
		virtual void pull(AudioChunk &chunk,
			const Amp_Node &a, const Amp_Node &b);
		virtual Amp_Node interpolate(
			const Amp_Node &a, const Amp_Node &b, float mid);

		virtual void effectTick(Uint64) {}
	};


	class Pitch_Node {public: Pitch_Node(float r=1.0f, Uint32 a=0) :
		rate(r), alg(a) {} float rate; Uint32 alg;};
	/*
		Pitch:  Allows volume/pitch modulation and pausing of a stream.

		Frequency modulation can be constant, based on another

		WARNING:  Pitch takes in audio from the source stream at an altered
			rate; thus, performance issues will arise from the use of very
			high Pitches as large amounts of data are consumed, and certain
			streams such as Microphones will be unable to keep up with demand.
	*/
	class Pitch : public AudioEffect<Pitch_Node>
	{
	public:
		//Interpolation styles
		enum RESAMPLER
			{
			NONE=0, //Bottom-of-the-barrel
			LINEAR=1, //Low-quality linear
			HERMITE=2, HERMITE_43=2, //4-point 3rd order hermite.
			//Coming: sinc interpolation
			};

	public:
		//Bind to signal
		Pitch(Signal source, float rate=1.0f, Uint32 alg=HERMITE);
		virtual ~Pitch();

		//Choose the resampling algorithm.  Default is HERMITE.
		void resampling(Uint32 alg);

		//Set and interpolate to the given pitch.  "Note" is in semitones.
		void rate(float factor)   {settings.rate = std::max(factor,.0f);}
		void note(float note)     {settings.rate = Semitones(note);}

		//Query current pitch
		float rate()              {return settings.rate;}
		float note()              {return Semitones(settings.rate);}

	protected:
		virtual void pull(AudioChunk &chunk,
			const Pitch_Node &a, const Pitch_Node &b);
		virtual Pitch_Node interpolate(
			const Pitch_Node &a, const Pitch_Node &b, float mid);

		virtual void effectTick(Uint64) {}

	private:
		float offset;
		std::vector<Sint32> mem;
	};


	class Pan_Node {public: Pan_Node(float l=1.0f,float r=1.0f,Uint32 p=0) :
		left(l), right(r), ramp(p) {} float left, right; Uint32 ramp;};
	/*
		Pan:  A stereo amp used to alter the left/right balance of a sound.
			Turns mono sounds stereo while adjusting balance in each channel.
			Separately amps the channels of stereo sounds.

		Pan of -1.0 is left; 0.0 is center; 1.0 is right.
			left volume  = level * clamp(1-pan, 0, 1);
			right volume = level * clamp(1+pan, 0, 1);

		Volume/level values are equivalent to those in Pan.
	*/
	class Pan : public AudioEffect<Pan_Node>
	{
	public:
		//Bind to signal
		Pan(Sound source, float panning = 0.0f, float level = 1.0f);
		virtual ~Pan();

		//Set ramping mode; default is linear
		void rampLinear();
		void rampExponential();  //Will revert to linear when volume <= 0

		//Change settings...
		void pan(float pan);
		void level(float level);
		void left(float leftVolume);
		void right(float rightVolume);

		//Query settings
		float pan();
		float level();
		float left();
		float right();

	protected:
		virtual void pull(AudioChunk &chunk,
			const Pan_Node &a, const Pan_Node &b);
		virtual Pan_Node interpolate(
			const Pan_Node &a, const Pan_Node &b, float mid);

		virtual void effectTick(Uint64) {}
	};


	/*
		A simple but efficient and easily-customized delay line reverberator.
			Also suffices for echoes; see the preset function.
	*/
	class Reverb : public AudioEffect<int>
	{
	public:
		//Bind to signal
		Reverb(Signal source, float dry=1.0f, float wet=.5f, float depth=30.0f);
		virtual ~Reverb();

		/*
			Usually you'll want to use this to set up your reverb.
				dry: strength of original sound (default 1)
				wet: base strength of reverb (default .5)
				depth: "bigness" of space, in meters (default 30.0f)

			Reverbs will last longer with higher "wet" and "depth".
			Low "dry" makes sounds seem faint and faraway.
			"wet" at or over 1 will be disastrously loud and terrible.
		*/
		void reverb(float dry, float wet, float depth);

		/*
			An echo at the given distance (speed of sound 343 m/s) and decay.
		*/
		void echo(float distance, float decay);


		/*
			Use these functions to set up custom delay lines for the reverb.
				Delay line 0 is the dry signal.
				Adding twice to the same delay will combine, not overwrite.
		*/
		void clearDelays();
		void addDelaySeconds(float seconds, float amp);
		void addDelaySamples(Uint32 samples, float amp);

	protected:
		virtual void pull(AudioChunk &chunk, const int &a, const int &b);
		virtual int interpolate(const int &a, const int &b, float mid)
			{return b;}

		virtual void effectTick(Uint64) {}

	private:
		std::vector<Sint32> delays;
		std::vector<Sint32> echoes;
		std::vector<Sint16> memory;
		Sint32 memoryPos;
	};


	struct Filter_Node
		{float lp, hp; Filter_Node(float l=0,float h=0) : lp(l), hp(h) {}};
	/*
		A filter which might be a highpass, lowpass, bandpass or bandstop.
	*/
	class Filter : public AudioEffect<Filter_Node>
	{
	public:
		//cutoff frequencies well beyond perceptibility
		static const float BOTTOM, TOP;

	public:
		//Bind to signal
		Filter(Signal source, float lowPass=0.0f, float highPass=0.0f);
		virtual ~Filter();

		//Change to the given settings over one frame or <rampTime> seconds.
		//  Use frequency 0 to "turn off" the corresponding effect.
		//  Use a rampTime of zero to change instantly.
		void bandpass(float lowFreq, float highFreq);
		void bandstop(float lowFreq, float highFreq);
		void lowpass (float cutoff);
		void highpass(float cutoff);
		void off();

	protected:
		virtual void pull(AudioChunk &chunk,
			const Filter_Node &a, const Filter_Node &b);
		virtual Filter_Node interpolate(
			const Filter_Node &a, const Filter_Node &b, float mid);

		virtual void effectTick(Uint64) {}

	private:
		//Resonant state information
		float lps[PG_MAX_CHANNELS], hps[PG_MAX_CHANNELS];
	};
}


#endif // PLAIDGADGET_AUDIO_EFFECTS_H
