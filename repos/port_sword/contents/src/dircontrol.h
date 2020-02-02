/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:	dircontrol.h ( Directory Control Library, C++)
 *
 * COMMENTS:
 *	This is the control routines to handle directories in a platform
 *	independent manner.
 */

#ifdef __APPLE__
#include <limits.h>
#endif

#if defined(LINUX) || defined(__APPLE__)
#include <dirent.h>
#else
#include <windows.h>
#endif

class DIRECTORY
{
public:
    DIRECTORY();
    ~DIRECTORY();
    
    // Open the directory, returns 0 for success.
    int		opendir(const char *name);

    // Multiple calls of readdir will return successive files
    // in the specified directory.
    // . and .. are ignored.
    // The const char is stored in myLastPath, so will not be
    // preserved across calls.
    const char *readdir();
    void	closedir();
    
protected:
#if defined(LINUX) || defined(__APPLE__)
    DIR			*myDir;
    char		 myLastPath[NAME_MAX + 1];

#else
    WIN32_FIND_DATA	myData;
    HANDLE		myHandle;
    char		myLastPath[MAX_PATH+1];
#endif
};
