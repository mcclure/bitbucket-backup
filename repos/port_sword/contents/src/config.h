/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        config.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 */

#ifndef __config__
#define __config__

class CONFIG
{
public:
    CONFIG();
    ~CONFIG();

    void	load(const char *fname);
    void	save(const char *fname);

    bool	musicEnable() const { return myMusicEnable; }
    const char *musicFile() const { return myMusicFile; }
    int		musicVolume() const { return myMusicVolume; }

    bool	screenFull() const { return myFullScreen; }

    // Like why the heck do we want accessors on this flat file?
public:
    bool	myBloodDynamic;
    int 	myMusicVolume;
    bool	myMusicEnable;
    const char *myMusicFile;

    bool		myFullScreen;
};

extern CONFIG *glbConfig;


#endif
