#ifndef PLAIDGADGET_AUDIO_BUFFER_H
#define PLAIDGADGET_AUDIO_BUFFER_H


#include "stream.h"

#include "../thread/lockfree.h"


namespace plaid
{
	/*
		AudioClip:  Captured or loaded audio that can be played from memory by
			an arbitrary number of player streams.

		Uses include:
			loading sound files into memory rather than streaming them
			recording microphone input or rendered audio
	*/
	class AudioClip
	{
private:
		class Link
		{
		public:
			Link() : next(NULL) {}

			static const unsigned SIZE = 4096;

			Uint8 data[SIZE];
			Link *next;
		};

		class Data : public RefCounted
		{
		public:
			Data(AudioFormat, Uint16);

			//Data formatting
			AudioFormat format;
			Uint16      type;

			//Buffer chain
			Link       *root[PG_MAX_CHANNELS], *last[PG_MAX_CHANNELS];
			Uint32      length;

			//Players and recording
			LockFreeQueue<Uint32> record;
			Uint32                locks;
		};

	public:
		/*
			An AudioClip player.

			It allows precise control over pitch, similar to a Pitch effect.
		*/
		class Player : public AudioStream
		{
		public:
			Player(const AudioClip &clip, bool loop = false);
			virtual ~Player();

		protected:
			virtual AudioFormat format();
			virtual void tick(Uint64 frame);
			virtual bool exhausted();
			virtual void pull(AudioChunk &chunk);

		private:
			struct State
			{
				float  rate;
				float  volume;
				Uint32 seek;
			};

			friend class AudioClip;

			//Basic properties
			Ref<Data>            clip;
			const bool           loop;

			//External state
			State                outer;
			LockFreeQueue<State> queue;

			//Internal state
			State                a, b;
			Link                *link[PG_MAX_CHANNELS];
			Uint32               pos;
			float                frac;

			//Stream               record;
		};

		/*
			Different sample sizes.  Each uses a bit more memory than the last.
		*/
		enum SAMPLE_TYPE
		{
			INT8  = 1, // 8-bit sample; low-quality, low memory usage
			INT16 = 2, // 16-bit sample; optimal for most purposes
			INT24 = 3, // 32-bit sample with 8 "overflow" bits; fast and costly
		};

	public:
		/*
			Create an empty AudioClip, with the given format.
		*/
		AudioClip(AudioFormat format, SAMPLE_TYPE type = INT16);

		virtual ~AudioClip();

		/*
			Create a player -- you may have as many as you like.
				Players lock the AudioClip while they exist.
		*/
		Ref<Player> player(bool loop = false);


		/*
			AudioClip is a reference object and may be null.
		*/
		AudioClip()             {}
		operator bool() const   {return data;}
		bool null() const       {return !data;}
		static AudioClip Null() {return AudioClip();}


		// ====================================================================
		// =========   Advanced functionality follows   =======================
		// ====================================================================


		/*
			Check whether the buffer is locked.
				If so, the buffer-editing functions below will not function.

			Any Player created from an AudioClip holds a lock for its lifetime.
		*/
		bool locked();

		/*
			Use these functions to access buffer data -- but only when unlocked.
		*/
		//TODO later

		/*
			"Load" an input stream by pulling data from it until it exhausts.
				Most commonly used to buffer audio files or synths.
				This happens immediately and may cause some delay.

				This will fail if the stream is locked, or malfunction if the
				source is only functional in the audio thread -- the latter
				includes the microphone, and some Splitters.

			"limit" is a safety value in seconds, which prevents hanging from
				overly large or infinite streams from being loaded.
				limit <= 0.0 means no limit -- be CAREFUL with this.

			Returns number of seconds loaded, or a negative number on failure.
		*/
		float load(Sound source, float limit);

		/*
			Advanced: record a stream's output in the audio thread.

			This can be used for recording the microphone or hybrid streaming,
				where an audio file is streamed the first time it's played
				and plays from a buffer thereafter.

				A special Player is created which records the source stream
				into the Clip while playing that audio to its own output.
				If the source exhausts, this Player will become "normal" and
				drop or loop as if it had hit the Clip's end normally.

				Note that the recording Player must have its output pulled
				in the audio thread -- you might want to play it at volume 0.0.

			"limit" is the maximum length in seconds, or <= 0.0 for infinite.

			Returns the special recording player, or a null Ref on failure.
		*/
		//Ref<Player> record(Signal source, float limit = 0.0f);


	private:
		friend class Player;

		Ref<Data> data;
	};
}

#endif // PLAIDGADGET_AUDIO_BUFFER_H
