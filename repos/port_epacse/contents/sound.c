#include "sound.h"

#if __APPLE__
#include <fmod.h>
#include <fmod_errors.h>
#else
#include <fmodex/fmod.h>
#include <fmodex/fmod_errors.h>
#endif
#include "settings.h"
#include "paths.h"
#include "error.h"

FMOD_SYSTEM* device;
FMOD_CHANNEL* channel = 0;
FMOD_RESULT result;
FMOD_SOUND* sounds[5];
unsigned char enabled = 0;

static void errcheck(FMOD_RESULT result){
	if(result != FMOD_OK)
		errorarg("Sound subdevice error", FMOD_ErrorString(result));
}

void initSound(void){
	result = FMOD_System_Create(&device);
	if(result != FMOD_OK){
		warningarg(WARNING_SOUNDINIT, FMOD_ErrorString(result));
		return;
	}
	result = FMOD_System_Init(device, 32, FMOD_INIT_NORMAL, NULL);
	if(result != FMOD_OK){
		warningarg(WARNING_SOUNDINIT, FMOD_ErrorString(result));
		return;
	}
	enabled = 1;
	
	sounds[0] = NULL;
	result = FMOD_System_CreateSound(device, PATH_SOUNDJUMP, FMOD_SOFTWARE, 0, &(sounds[1]));
	errcheck(result);
	result = FMOD_System_CreateSound(device, PATH_SOUNDLAND, FMOD_SOFTWARE, 0, &(sounds[2]));
	errcheck(result);
	result = FMOD_System_CreateSound(device, PATH_SOUNDBUMP, FMOD_SOFTWARE, 0, &(sounds[3]));
	errcheck(result);
	result = FMOD_System_CreateSound(device, PATH_SOUNDSPLASH, FMOD_SOFTWARE, 0, &(sounds[4]));
	errcheck(result);
}

void cleanupSound(void){
	int i;
	
	if(enabled == 0) return;
	for(i = 1; i < 5; ++i){
		result = FMOD_Sound_Release(sounds[i]);
		errcheck(result);
	}
	result = FMOD_System_Close(device);
    errcheck(result);
    result = FMOD_System_Release(device);
    errcheck(result);
}

void playSound(soundTrack track){
	if(enabled == 0) return;
	result = FMOD_System_PlaySound(device, FMOD_CHANNEL_FREE, sounds[track], 0, &channel);
	errcheck(result);
}

void playSoundPan(soundTrack track, float panning){
	if(enabled == 0) return;
	result = FMOD_System_PlaySound(device, FMOD_CHANNEL_FREE, sounds[track], 0, &channel);
	FMOD_Channel_SetPan(channel, panning*SOUND_PANNINGSCOPE);
}

void updateSound(void){
	FMOD_System_Update(device);
}
