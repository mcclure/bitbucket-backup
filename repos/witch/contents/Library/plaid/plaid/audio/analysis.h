#ifndef PLAID_AUDIO_ANALYSIS_H
#define PLAID_AUDIO_ANALYSIS_H

#include "stream.h"
#include "effects.h"

namespace plaid
{
	/*
		An Envelope gives a running approximation of a stream's amplitude.
			It is the input signal's absolute value, lowpass-filtered.
			The output signal is a positive envelope and should not be played.

		Frequencies near or below the envelope's will throw off analysis.
			Thus, filtering these out in advance might be a good idea...
	*/
	class Envelope : public AudioStream
	{
	public:
		/*
			Produce an envelope for a signal with the given lowpass frequency.
				Supply a positive logLength to enable logging.
		*/
		Envelope(Signal source, float frequency = 10.0f, Uint32 logLength = 0);
		~Envelope();

		//Change the envelope's frequency.
		void frequency(float f)   {filter->lowpass(f);}


		//Logging must be enabled for the following functions to work.

		//Pull amplitude values one by one (returns false when exhausted)
		bool   pull(float &amplitude);

		//Get the latest available amplitude value.
		float  amplitude();

		//Get the age in frames of the latest amplitude value.
		Uint32 latency();


	protected:
		virtual AudioFormat format();
		virtual void pull(AudioChunk &chunk);
		virtual bool exhausted();
		virtual void tick(Uint64 frame);

	private:
		Signal               source, filtSig;
		Ref<Filter>          filter;

		//Log data
		struct Feed {Uint32 frame; float amp;};
		LockFreeQueue<Feed>  feedback;
		Feed                *log;
		Uint32               logSize, logPos, pullPos, frame;
	};
}

#endif // PLAID_AUDIO_ANALYSIS_H
