/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:	dircontrol.cpp ( Directory Control Library, C++)
 *
 * COMMENTS:
 *	This is the control routines to handle directories in a platform
 *	independent manner.
 */

#include <string.h>
#include "dircontrol.h"

#if defined(LINUX) || defined(__APPLE__)

DIRECTORY::DIRECTORY()
{
    myDir = 0;
}

DIRECTORY::~DIRECTORY()
{
    closedir();
}

int
DIRECTORY::opendir(const char *name)
{
    if (myDir)
	closedir();

    myDir = ::opendir(name);

    if (myDir)
	return 0;
    else
	return -1;
}

const char *
DIRECTORY::readdir()
{
    dirent		*ent;

    if (!myDir) return 0;
    
    ent = ::readdir(myDir);

    if (!ent)
	return 0;
    strcpy(myLastPath, ent->d_name);

    // Check to see if the file is ".".  We don't want .
    if (!strcmp(myLastPath, ".") || !strcmp(myLastPath, ".."))
	return readdir();

    return myLastPath;
}

void
DIRECTORY::closedir()
{
    if (myDir)
	::closedir(myDir);
    myDir = 0;
}

#else

#define INVALID_HANDLE ((HANDLE) -1)


DIRECTORY::DIRECTORY()
{
    myHandle = INVALID_HANDLE;
}

DIRECTORY::~DIRECTORY()
{
    closedir();
}

int
DIRECTORY::opendir(const char *name)
{
    if (myHandle && myHandle != INVALID_HANDLE)
	closedir();
    
    char	dirname[MAX_PATH+1];
    strcpy(dirname, name);
    strcat(dirname, "/*");

    myHandle = FindFirstFile(dirname, &myData);

    if (myHandle && (myHandle != INVALID_HANDLE))
    {
	// Success...
	return 0;
    }
    else
	return -1;
}

const char *
DIRECTORY::readdir()
{
    if (myHandle && myHandle != INVALID_HANDLE)
    {
	// We are always working one behind, so return the last value...
	strcpy(myLastPath, myData.cFileName);

	// Get the next file...
	if (!FindNextFile(myHandle, &myData))
	{
	    // Free close dir.
	    closedir();
	}

	// Check to see if the file is ".".  We don't want .
	if (!strcmp(myLastPath, ".") || !strcmp(myLastPath, ".."))
	    return readdir();
	
	return myLastPath;
    }
    else
	return 0;		// Done.
}

void
DIRECTORY::closedir()
{
    if (myHandle && myHandle != INVALID_HANDLE)
    {
	FindClose(myHandle);
	myHandle = INVALID_HANDLE;
    }
}

#endif
