#ifndef PLAIDGADGET_AUDIO_H
#define PLAIDGADGET_AUDIO_H

#include "../util/types.h"

#include "../core.h"

#include "stream.h"

#include "clip.h"

/**
    This module facilitates audio output from Plaidgadget programs and games.
*/

namespace plaid
{
	/*
		Mathematical functions for common sound unit conversions.
	*/
	float Decibels(float db);      // Decibels to linear volume
	float ToDecibels(float mult);  // Linear volume to decibels
	float Semitones(float steps);  // Semitones to pitch multiplier
	float ToSemitones(float mult); // Pitch multiplier to semitones


	class AudioScheduler;
	class AudioImp;
	class Mixer;

#if PLAIDGADGET
	class File;
#endif

	/*
		Provides audio functionality, either through its own simple interface
			or through the highly flexible streams system which allows user-made
			DSP effects, synthesizers, samplers, et cetera.
	*/
    class Audio
#if PLAIDGADGET
		: public Module
#else
		: public RefCounted
#endif
    {
	public:

#if PLAIDGADGET
		//Instantiate as game module, possibly in headless (silent) mode.
        Audio(const ModuleSet &modules, bool headless = false);
#else
		//Standalone audio system
		explicit Audio(bool headless = false);
#endif
        virtual ~Audio();


        //Get the hardware format.
        AudioFormat format();


        //Control/query master volume
        float volume();
        void volume(float);

		//Simple controls for sounds on the master mixer.
        void play   (Sound stream);
        void play   (Sound stream, float volume);
        void pause  (Sound stream);
        void stop   (Sound stream);
        void volume (Sound stream, float volume);

        //Check sound statuses
        bool  playing (Sound stream); //True if sound is playing (not paused)
        bool  has     (Sound stream); //True if sound is playing or paused
        float volume  (Sound stream);


        //Get audio stream CPU load -- keep this well under 1.0!
        float load();


        //Buffer a file or prep it for streaming.
		//  (These return null sounds when loading fails)
        Sound stream(String filename,  bool loop = false);
#if PLAIDGADGET
        Sound stream(const File &file, bool loop = false);
#endif

        /*
			Get a sound stream of the microphone input.
				If unavailable, it will produce silence.

			Avoid pulling data from this source at a rate other than that of
				sound output.  Doing so will cause jumps and/or gaps.
		*/
        Sound microphone();


		//Updates from the main loop, one per game frame.
        virtual void update();

#if PLAIDGADGET
        //Handle console commands
        virtual void handle(Command&);
#endif

	private:
		AudioImp *imp;
		AudioScheduler *scheduler;
		Mixer *master;
		float vol;
    };
}

#endif // PLAIDGADGET_AUDIO_H
