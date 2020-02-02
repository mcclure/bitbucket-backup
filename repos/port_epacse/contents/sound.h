#ifndef SOUND_H
#define SOUND_H

typedef enum {
	SOUND_NONE = 0,
	SOUND_JUMP,
	SOUND_LAND,
	SOUND_BUMP,
	SOUND_SPLASH
} soundTrack;

void initSound(void);
void cleanupSound(void);

void playSound(soundTrack track);
void playSoundPan(soundTrack track, float panning);

void updateSound(void);

#endif // SOUND_H
