#ifndef PLAIDGADGET_AUDIO_IMP_H
#define PLAIDGADGET_AUDIO_IMP_H

#include "audio.h"
#include "util.h"
#include "scratch.h"

namespace plaid
{
	/*
		The 'back end' of the Audio module.

		Audio implementations patch their render callbacks to this.
	*/
	class AudioScheduler
	{
	public:
		AudioScheduler(Audio &audio);
		~AudioScheduler();

		/*
			The main render callback.
			Provide input and output buffers (equal size) for the render.

			If mike input is unsupported, give an empty buffer for the latter.
			Mono mike data must be converted to stereo.

			'time' should correspond to AudioImp::time().
			'capTime' should suggest the 'latest' time the callback should exit.

			This HAS to be called to avert a pileup of audio data; in the case
				of dummy behavior ask for zero bytes in AudioImp::update().
		*/
		void render(Sint32 **speaker, const Sint32 *mic, Uint32 moments,
			double time, double capTime);

	private:
		friend class Audio;
		friend class MikeStream;

		void tick(double time);
		void setupFront(AudioFormat format, double time);
		void setupBack(double time);

		Sound microphone();

		Audio      &audio;
		AudioImp   *imp;
		AudioFormat format;
		Mixer      *master;
		Signal      signal;


		struct Front;
		struct Back;
		Front *front;
		Back  *back;

		bool monitor;
	};


	/*
		Audio implementations must extend this interface.
	*/
	class AudioImp
	{
	public:
		AudioImp(Audio &a, AudioScheduler &s) : audio(a), scheduler(s) {}
		virtual ~AudioImp() {}

		/*
			The flow of callbacks should be started with this function.
		*/
		virtual void startStream() = 0;

		/*
			This should return a time corresponding to the audio I/O streams.
			Try to provide the most granular timer possible.

			It should be specified in seconds but need not start at zero.
		*/
		virtual double time() = 0;

		//Return the CPU usage of the audio callback, where 1.0 is 100%.
		virtual float load() = 0;

		//Return the format for input and output.
		virtual AudioFormat format() = 0;

		//Optional per-frame functionality (useful for dummy behavior)
		virtual void update() {}

#if PLAIDGADGET
		//Optionally implement implementation-specific commands and help
		virtual void handle(Command &command) {}
		virtual void commandHelp() {}
#endif

	protected:
		Audio          &audio;
		AudioScheduler &scheduler;
	};


	/*
		Declare a global static instance of your AudioDriver subclass in its
			implementation file.

		Its stream() function must be safe to use from multiple threads;
			as long as you don't reference any global data you should be safe.
	*/
	class AudioDriver
	{
	public:
		//Give your driver a unique name.
		//  Recommended format: "APIName version", eg "portaudio v17.0"
		AudioDriver(String name, float preference = 0.0f);
		virtual ~AudioDriver() {}
		static void LockRegistry();

		//Choose the most preferred audio driver.
		static AudioDriver *Default();

		//Look up a driver by name.
		static AudioDriver *Find(const String &name);

		//Use this function to instantiate a driver.
		struct DriverParams
		{
			Audio          &audio;
			AudioScheduler &scheduler;
		};
		virtual AudioImp *create(const DriverParams&) = 0;

	public:
		String name;
	};


	/*
		Declare a global static instance of your AudioCodec subclass in its
			implementation file.

		Its stream() function must be safe to use from multiple threads;
			as long as you don't reference any global data you should be safe.
	*/
	class AudioCodec
	{
	public:
		//Takes a list of case-insensitive extensions separated by semicolons.
		//  E.G.  L"ogg,ogv"
		AudioCodec(String extensions);
		virtual ~AudioCodec() {}
		static void LockRegistry();

		static AudioCodec *Find(const String &ext);

		//Use this function to instantiate a file-stream.
		struct StreamParams
		{
			const String &file;
			bool          loop;
		};
		virtual Sound stream(const StreamParams&) = 0;
	};
}

#endif // PLAIDGADGET_AUDIO_IMP_H
