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

// This is a do-nothing implementation of Sound. It is for build configurations
// where Sound is actually undesired-- but, we still need to include SOMETHING
// for the appropriate symbols, or builds will fail, lua bindings will fail, etc.

#include "PolySound.h"
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>
#undef OV_EXCLUDE_STATIC_CALLBACKS
#include "PolyString.h"
#include "PolyLogger.h"

#include "OSBasics.h"
#include <string>
#include <vector>

using namespace std;
using namespace Polycode;

Sound::Sound(const String& fileName) : sampleLength(-1) {
	setIsPositional(false);
}

Sound::Sound(const char *data, int size, int channels, int freq, int bps) : sampleLength(-1) {
	setIsPositional(false);
}

Sound::~Sound() {
}

void Sound::loadFile(String fileName) {
}

void Sound::reloadProperties() {
}

void Sound::soundCheck(bool result, const String& err) {
}

void Sound::soundError(const String& err) {
}

unsigned long Sound::readByte32(const unsigned char buffer[4]) {
	return 0;
}

unsigned short Sound::readByte16(const unsigned char buffer[2]) {
	return 0;
}

void Sound::Play(bool loop) {
}

bool Sound::isPlaying() {
	return false;
}

void Sound::setVolume(Number newVolume) {
}

Number Sound::getVolume() {
	return 0;
}

void Sound::setPitch(Number newPitch) {
}

Number Sound::getPitch() {
}

void Sound::setSoundPosition(Vector3 position) {
}

void Sound::setSoundVelocity(Vector3 velocity) {
}

void Sound::setSoundDirection(Vector3 direction) {
}

void Sound::setOffset(int off) {
}

String Sound::getFileName() {
	return String();
}

Number Sound::getPlaybackTime() {
	return 0;
}

Number Sound::getPlaybackDuration() {
	return 0;
}
		
int Sound::getOffset() {
	return 0;
}

void Sound::seekTo(Number time) {
}

int Sound::getSampleLength() {
	return 0;
}

void Sound::setPositionalProperties(Number referenceDistance, Number maxDistance) { 
}

void Sound::setReferenceDistance(Number referenceDistance) {
}

void Sound::setMaxDistance(Number maxDistance) {
}

Number Sound::getReferenceDistance() {
	return 0;
}

Number Sound::getMaxDistance() {
	return 0;
}

void Sound::setIsPositional(bool isPositional) {
	this->isPositional = isPositional;
}

ALenum Sound::checkALError(const String& operation) {
	return 0;
}

void Sound::Stop() {
}

ALuint Sound::GenSource() {
	return -1;
}

ALuint Sound::GenSource(ALuint buffer) {
	return -1;
}

ALuint Sound::loadBytes(const char *data, int size, int freq, int channels, int bps) {
	return 0;
}

ALuint Sound::loadOGG(const String& fileName) {
	return -1;
}

ALuint Sound::loadWAV(const String& fileName) {
	return -1;
}
