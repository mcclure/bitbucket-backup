/*
 Copyright (C) 2011 by Ivan Safrin
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#include "PolyPortSound.h"
#include <vorbis/vorbisfile.h>
#include "PolyString.h"
#include "PolyLogger.h"

#include "OSBasics.h"
#include <string>
#include <vector>

#define PORTAUDIO_DEBUG 1

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

using namespace std;
using namespace Polycode;

bool PSound::globalInitialized = false;
void PSound::globalInit() {
	if (!globalInitialized) {
		PaError err;
		err = Pa_Initialize();
#if PORTAUDIO_DEBUG
		if (err != paNoError) fprintf(stderr,"Polycode::Sound global init fail: %d = %s\n", (int)err,  Pa_GetErrorText( err ));
#endif
		
		globalInitialized = true;
	}
}

PSound::PSound() {
	setIsPositional(false);
	stream = NULL;
}

PSound::PSound(const String& fileName) : sampleLength(-1) {
	setIsPositional(false);
	stream = NULL;
}

PSound::PSound(const char *data, int size, int channels, int freq, int bps) : sampleLength(-1) {
	setIsPositional(false);
	stream = NULL;
}

PSound::~PSound() {
	if (stream) {
		PaError err = Pa_CloseStream( stream );
#if PORTAUDIO_DEBUG
		if (err != paNoError) fprintf(stderr,"Polycode::Sound close fail: %d = %s\n", (int)err,  Pa_GetErrorText( err ));
#endif
	}
}

void PSound::soundCheck(bool result, const String& err) {
}

void PSound::soundError(const String& err) {
}

unsigned long PSound::readByte32(const unsigned char buffer[4]) {
	return 0;
}

unsigned short PSound::readByte16(const unsigned char buffer[2]) {
	return 0;
}

void PSound::initOutputParameters(PaStreamParameters &outputParameters) {
	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	outputParameters.channelCount = 2;       /* stereo output */
	outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
	const PaDeviceInfo *deviceInfo= Pa_GetDeviceInfo( outputParameters.device );
	if (deviceInfo)
		outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
}

static int paBounceCallback( const void *inputBuffer, void *outputBuffer,
						  unsigned long framesPerBuffer,
						  const PaStreamCallbackTimeInfo* timeInfo,
						  PaStreamCallbackFlags statusFlags,
						  void *userData )
{
	PSound *target = (PSound *)userData;
	return target->soundCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

int PSound::soundCallback( const void *, void *outputBuffer,
						  unsigned long framesPerBuffer,
						  const PaStreamCallbackTimeInfo*,
						  PaStreamCallbackFlags)
{
	
	float *out = (float*)outputBuffer;
	
	for(unsigned long i=0; i<framesPerBuffer; i++)
	{
		*out++ = 0;  /* left */
		*out++ = 0;  /* right */
	}
	
	return paContinue;
}

void PSound::Play(bool loop) {
	globalInit();
	
	if (isPlaying())
		Stop();

	PaError err;
	PaStreamParameters outputParameters;

	initOutputParameters(outputParameters);
	if (outputParameters.device == paNoDevice) {
#if PORTAUDIO_DEBUG
		if (err != paNoError) fprintf(stderr,"Polycode::Sound play fail: No output device. Error %d = %s\n", (int)err,  Pa_GetErrorText( err ));
#endif
		return;
	}	

	err = Pa_OpenStream(
						&stream,
						NULL, /* no input */
						&outputParameters,
						SAMPLE_RATE,
						FRAMES_PER_BUFFER,
						paClipOff,      /* we won't output out of range samples so don't bother clipping them */
						paBounceCallback,
						this );
#if PORTAUDIO_DEBUG
	if (err != paNoError) fprintf(stderr,"Polycode::Sound play fail: %d = %s\n", (int)err,  Pa_GetErrorText( err ));
#endif
	
	err = Pa_StartStream( stream );
#if PORTAUDIO_DEBUG
	if (err != paNoError) fprintf(stderr,"Polycode::Sound startstream fail %d = %s\n", (int)err,  Pa_GetErrorText( err ));
#endif
}

bool PSound::isPlaying() {
	return stream != NULL;
}

void PSound::Stop() {
	if (stream) {
		PaError err = Pa_StopStream( stream );
#if PORTAUDIO_DEBUG
		if (err != paNoError) fprintf(stderr,"Polycode::Sound stop fail: %d = %s\n", (int)err,  Pa_GetErrorText( err ));
#endif
		stream = NULL;
	}
}

void PSound::setVolume(Number newVolume) {
}

void PSound::setPitch(Number newPitch) {
}

void PSound::setSoundPosition(Vector3 position) {
}

void PSound::setSoundVelocity(Vector3 velocity) {
}

void PSound::setSoundDirection(Vector3 direction) {
}

void PSound::setOffset(int off) {
}


Number PSound::getPlaybackTime() {
	return 0;
}

Number PSound::getPlaybackDuration() {
	return 0;
}
		
int PSound::getOffset() {
	return 0;
}

void PSound::seekTo(Number time) {
}

int PSound::getSampleLength() {
	return 0;
}

void PSound::setPositionalProperties(Number referenceDistance, Number maxDistance) { 
}

void PSound::setIsPositional(bool isPositional) {
	this->isPositional = isPositional;
}

ALuint PSound::loadBytes(const char *data, int size, int freq, int channels, int bps) {
	return 0;
}

ALuint PSound::loadOGG(const String& fileName) {
	return -1;
}

ALuint PSound::loadWAV(const String& fileName) {
	return -1;
}
