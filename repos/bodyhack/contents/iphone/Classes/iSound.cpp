/*
 *  iSound.cpp
 *  iJumpman
 *
 *  Created by Andi McClure on 3/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <stdio.h>

#include "CAXException.h"
#include "CAStreamBasicDescription.h"

// BE WARNED: Audio does not work in the simulator.

// Audio debugging can be verbose, so keep it distinct from ERR
#if SELF_EDIT
#define ARR(...) printf (__VA_ARGS__)
#else
#define ARR(...)
#endif

// Defined in display.cpp
void audio_callback(void *userdata, uint8_t *stream, int len);

#define CALLDEBUG 0
// Intended to prevent accidental audio wakeup while phone is asleep
// TODO not hooked up in EAGLView currently
bool fastAsleep = false;
void AudioHalt();

struct AudioTools {
	AudioUnit					rioUnit;
	AURenderCallbackStruct		outputProc;
	CAStreamBasicDescription	thruFormat;
	Float64						hwSampleRate;
	bool						mute;
};

static AudioTools audioTools;

int SetupRemoteIO (AudioUnit& inRemoteIOUnit, AURenderCallbackStruct inRenderProc, CAStreamBasicDescription& outFormat);

void propListener(	void *                  inClientData,
				  AudioSessionPropertyID	inID,
				  UInt32                  inDataSize,
				  const void *            inData)
{
	AudioTools *THIS = (AudioTools*)inClientData;
	if (inID == kAudioSessionProperty_AudioRouteChange)
	{
		try {
			// if there was a route change, we need to dispose the current rio unit and create a new one
			XThrowIfError(AudioComponentInstanceDispose(THIS->rioUnit), "couldn't dispose remote i/o unit");		
			
			SetupRemoteIO(THIS->rioUnit, THIS->outputProc, THIS->thruFormat);
			
			UInt32 size = sizeof(THIS->hwSampleRate);
			XThrowIfError(AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &size, &THIS->hwSampleRate), "couldn't get new sample rate");
			
            if (!fastAsleep) {
                XThrowIfError(AudioOutputUnitStart(THIS->rioUnit), "couldn't start unit");
            } else {
                ARR("STAY ASLEEP\n");
            }

			
		} catch (CAXException e) {
			char buf[256];
			fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
		}
		
	}
}

inline void SWAP16(int16_t &x) {
	x = ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8);
}

static OSStatus	PerformThru(
							void						*inRefCon, 
							AudioUnitRenderActionFlags 	*ioActionFlags, 
							const AudioTimeStamp 		*inTimeStamp, 
							UInt32 						inBusNumber, 
							UInt32 						inNumberFrames, 
							AudioBufferList 			*ioData)
{
	AudioTools *THIS = (AudioTools *)inRefCon;
	//OSStatus err = AudioUnitRender(THIS->rioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData);
	//if (err) { printf("PerformThru: error %d\n", (int)err); return err; }
	
//	printf("frames %d buffers %d channels %d size %d\n", inNumberFrames, ioData->mNumberBuffers, ioData->mBuffers[0].mNumberChannels, ioData->mBuffers[0].mDataByteSize); 
	
	UInt32 outNumberFrames = ioData->mBuffers[0].mDataByteSize/sizeof(short);
	
	int16_t *left = (int16_t *)ioData->mBuffers[0].mData;
	int16_t *right = ioData->mNumberBuffers > 1 ? (int16_t *)ioData->mBuffers[1].mData : 0; 
	
    audio_callback(NULL, (uint8_t *)left, ioData->mBuffers[0].mDataByteSize);
    
    // Stereo-ify // TODO: Jumpcore release should let callback do stereo
    if (right) {
        for(int c = 0; c < outNumberFrames; c++)
            right[c] = left[c];
	}
		
	return 0;//err;
}

#define kOutputBus 0
#define kInputBus 1

int SetupRemoteIO (AudioUnit& inRemoteIOUnit, AURenderCallbackStruct inRenderProc, CAStreamBasicDescription& outFormat)
{	
	try {		
//		AudioStreamBasicDescription audioFormat;
		// Open the output unit
		AudioComponentDescription desc;
		desc.componentType = kAudioUnitType_Output;
		desc.componentSubType = kAudioUnitSubType_RemoteIO;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
//		desc.componentSubType = kAudioUnitSubType_GenericOutput; // Shouldn't it be?
//		desc.componentManufacturer = 'awk_';
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;
		
		AudioComponent comp = AudioComponentFindNext(NULL, &desc);
		
		XThrowIfError(AudioComponentInstanceNew(comp, &inRemoteIOUnit), "couldn't open the remote I/O unit");

		UInt32 one = 0; // lol
		XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &one, sizeof(one)), "couldn't disable input on the remote I/O unit");
		one = 1;
		XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &one, sizeof(one)), "couldn't enable output on the remote I/O unit");

//		XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &inRenderProc, sizeof(inRenderProc)), "couldn't set remote i/o render callback");
		XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, kOutputBus, &inRenderProc, sizeof(inRenderProc)), "couldn't set remote i/o render callback");
		
		outFormat.mSampleRate			= 44100.00;
		outFormat.mFormatID			= kAudioFormatLinearPCM;
		outFormat.mFormatFlags		= kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
		outFormat.mFramesPerPacket	= 1;
		outFormat.mChannelsPerFrame	= 1;
		outFormat.mBitsPerChannel		= 16;
		outFormat.mBytesPerPacket		= 2;
		outFormat.mBytesPerFrame		= 2; // What?		
		outFormat.Print(); 
		
		XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &outFormat, sizeof(outFormat)), "couldn't set the remote I/O unit's input client format");
		
		XThrowIfError(AudioUnitInitialize(inRemoteIOUnit), "couldn't initialize the remote I/O unit");
	}
	catch (CAXException &e) {
//		char buf[256];
//		fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
		return 1;
	}
	catch (...) {
		fprintf(stderr, "An unknown error occurred\n");
		return 1;
	}	
	
	return 0;
}

// WHAT DOES ANY OF THIS EVEN MEAN
void rioInterruptionListener(	void	*inUserData,
							 UInt32	inInterruption)
{
	printf("Session interrupted! --- %s ---", inInterruption == kAudioSessionBeginInterruption ? "Begin Interruption" : "End Interruption");
	
	AudioUnit *remoteIO = (AudioUnit*)inUserData;
	
	if (inInterruption == kAudioSessionEndInterruption)
	{
		// make sure we are again the active session
		AudioSessionSetActive(true);
		AudioOutputUnitStart(*remoteIO);
	}
	
	if (inInterruption == kAudioSessionBeginInterruption)
		AudioOutputUnitStop(*remoteIO);		
}

enum AudioMode {
    NO_AUDIO,       // No audio besides callback
    LOCAL_AUDIO,    // Use if you plan to use an AVAudioPlayer
    SHUFFLE_AUDIO   // Use if you plan to use a MPMusicPlayerController
};

bool anyLastMode = false;
AudioMode lastMode = NO_AUDIO, audioMode = NO_AUDIO;
bool audioIsHalted = false;

void AudioHalt() {
    ARR("TRY TO HALT\n");
    if (AudioOutputUnitStop(audioTools.rioUnit))
        printf("couldn't stop remote i/o unit\n");
    else audioIsHalted = true;
}
void AudioResume() {
    ARR("TRY TO RESUME\n");
    if (AudioOutputUnitStart(audioTools.rioUnit))
        printf("couldn't start remote i/o unit\n");
    else audioIsHalted = false;
}

// This is a remnant of a project where I needed to repeatedly change
// audio categories so that I could support different iPhone media
// classes. Jumpcore users can safely ignore it.
OSStatus SetAudioCategory(AudioMode mode) {
    UInt32 audioCategory;
    OSStatus e;
    
    ARR("AUDIO?\n");
    if (mode == LOCAL_AUDIO) // THIS! IS! STUPID!
        mode = NO_AUDIO;
    if (anyLastMode && lastMode == mode)
        return 0;
    anyLastMode = true;
    lastMode = mode;
    
    AudioHalt();
    
    ARR("YES. AUDIO CATEGORY %d\n", (int)mode);
    if (mode == SHUFFLE_AUDIO) {
        audioCategory = kAudioSessionCategory_MediaPlayback;
        e = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory);
        if (e) return e;
        ARR("...1...\n");
        audioCategory = 1; // Bah
        e = AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryMixWithOthers, sizeof(audioCategory), &audioCategory);
        if (e) return e;
    } else {
        audioCategory = 0; // Bah
        e = AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryMixWithOthers, sizeof(audioCategory), &audioCategory);
        ARR("...2...\n");
        audioCategory = kAudioSessionCategory_SoloAmbientSound;
        e = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory);
        if (e) return e;
    }
    ARR("SUCCESS\n", (int)mode);
    
    AudioResume();
    
    return 0;
}

void sound_init() {
	audioTools.outputProc.inputProc	= PerformThru; // IT'S AN OUTPUT PROC NOT AN INPUT PROC. WHY MUST YOU MAKE ME LIE	
	audioTools.outputProc.inputProcRefCon = &audioTools;	
//	CFURLRef url = NULL;
	try {	
		// Initialize and configure the audio session
		XThrowIfError(AudioSessionInitialize(NULL, NULL, rioInterruptionListener, &audioTools), "couldn't initialize audio session");
		XThrowIfError(AudioSessionSetActive(true), "couldn't set audio session active\n");
		
        XThrowIfError(SetAudioCategory(audioMode), "couldn't set one of the audio category properties");
		XThrowIfError(AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, propListener, &audioTools), "couldn't set property listener");
        
		Float32 preferredBufferSize = .010; // .005 IS NOT A SIZE, WTF APPLE
		XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");
		
		UInt32 size = sizeof(audioTools.hwSampleRate); // FIXME: Man I don't need no 64-bit floats
		XThrowIfError(AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &size, &audioTools.hwSampleRate), "couldn't get hw sample rate");
		
		XThrowIfError(SetupRemoteIO(audioTools.rioUnit, audioTools.outputProc, audioTools.thruFormat), "couldn't setup remote i/o unit");
				
		XThrowIfError(AudioOutputUnitStart(audioTools.rioUnit), "couldn't start remote i/o unit");
		
		size = sizeof(audioTools.thruFormat);
		XThrowIfError(AudioUnitGetProperty(audioTools.rioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &audioTools.thruFormat, &size), "couldn't get the remote I/O unit's output client format");
		audioTools.thruFormat.Print();
	}
	catch (CAXException &e) {
		char buf[256];
		fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
	}
	catch (...) {
		fprintf(stderr, "An unknown error occurred\n");
	}
	
}

