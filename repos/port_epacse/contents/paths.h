#ifndef PATHS_H
#define PATHS_H

#define PATH_SPRITEBASE "data/entity"
#define PATH_SOUNDBASE "data/sound"
#define PATH_LEVELBASE "data/level"
#define PATH_SAVEBASE "save"

#define PATH_SETTINGS "settings.conf"
#define PATH_LEVELINIT PATH_LEVELBASE"/init.conf"
#define PATH_TEMPORARY "t.emp"

#define PATH_FONT "data/font/font.png"

#define PATH_PLAYERSPRITE PATH_SPRITEBASE"/player.png"

#define PATH_SOUNDJUMP PATH_SOUNDBASE"/jump.wav"
#define PATH_SOUNDLAND PATH_SOUNDBASE"/land.wav"
#define PATH_SOUNDBUMP PATH_SOUNDBASE"/bump.wav"
#define PATH_SOUNDSPLASH PATH_SOUNDBASE"/splash.wav"

#define PATH_LEVELDIR_TEMPLATE        PATH_LEVELBASE"/999_999"
#define PATH_LEVELSTATIC_TEMPLATE     PATH_LEVELBASE"/999_999/static.png"
#define PATH_LEVELDYNAMIC_TEMPLATE    PATH_LEVELBASE"/999_999/dynamic.png"
#define PATH_LEVELBACKGROUND_TEMPLATE PATH_LEVELBASE"/999_999/background.png"
#define PATH_LEVELFOREGROUND_TEMPLATE PATH_LEVELBASE"/999_999/foreground.png"
#define PATH_LEVELDATA_TEMPLATE       PATH_LEVELBASE"/999_999/data.conf"
#define PATH_LEVELDIR                 PATH_LEVELBASE"/%d_%d"
#define PATH_LEVELSTATIC              PATH_LEVELBASE"/%d_%d/static.png"
#define PATH_LEVELDYNAMIC             PATH_LEVELBASE"/%d_%d/dynamic.png"
#define PATH_LEVELBACKGROUND          PATH_LEVELBASE"/%d_%d/background.png"
#define PATH_LEVELFOREGROUND          PATH_LEVELBASE"/%d_%d/foreground.png"
#define PATH_LEVELDATA                PATH_LEVELBASE"/%d_%d/data.conf"

#define PATH_LEVELSAVE_EXT "els"
#define PATH_LEVELSAVE_TEMPLATE PATH_SAVEBASE"/999_999."PATH_LEVELSAVE_EXT
#define PATH_LEVELSAVE PATH_SAVEBASE"/%d_%d."PATH_LEVELSAVE_EXT

#endif // PATHS_H
