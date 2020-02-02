/*
 *  internalfile.cpp
 *  Jumpman
 *
 *  Created by Andi McClure on 2/16/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "kludge.h"
#include "internalfile.h"

// Provides internalPath, userPath on Windows
// Provides only userPath on Mac
// Provides for everyone

#ifndef __APPLE__

// Gets an fopen-compatible path for a file located in the package resources.
// dst assumed at least 512 bytes long.
void internalPath(char *dst, const char *fmt, int arg1, int arg2) {
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, fmt, arg1, arg2);
    snprintf(dst, FILENAMESIZE, "Internal/%s", filename); // LOOK HOW SIMPLE THAT WAS
}

void userPath(char *dst, const char *fmt, int arg1, int arg2) {
	snprintf(dst, FILENAMESIZE, fmt, arg1, arg2);
}

#endif

void liveInternalPath(char *dst, const char *fmt, int arg1, int arg2) {
#if defined(SELF_EDIT) && defined(TARGET_DESKTOP) && defined(__APPLE__)
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, fmt, arg1, arg2);
	snprintf(dst, FILENAMESIZE, "../../Internal/%s", filename);
	watch_file(dst);
#else
	internalPath(dst, fmt, arg1, arg2);
#endif
}

#if defined(SELF_EDIT) && defined(__APPLE__)

#include <sys/stat.h>

// Notice this is brittle; if you change the signature of program_init, you'll get a link error here
void program_init(bool reinit);

struct watched_file {
	timespec updated;
	string name;
	watched_file(const string &_name = string()) : name(_name) { 
		memset( &updated, 0, sizeof(updated) );
	}
};

vector<watched_file> watching_files;

int last_check_updates = 0;

void watch_file(const string &filename) {
	ERR("WATCHING FILE: [%s]\n", filename.c_str());
	watching_files.push_back(watched_file(filename));
}

void check_updates() {
	int now = SDL_GetTicks();
	if (now - last_check_updates < 1000)
		return;
	last_check_updates = now;
	
	bool changes = false;
	
	for(int c = 0; c < watching_files.size(); c++) {
		timespec old_updated = watching_files[c].updated;
		struct stat stats;
		
		int found = stat(watching_files[c].name.c_str(), &stats);
		
		watching_files[c].updated = stats.st_mtimespec;
		
		if (found >= 0 && old_updated.tv_sec &&
			(old_updated.tv_sec != watching_files[c].updated.tv_sec || 
			 old_updated.tv_nsec != watching_files[c].updated.tv_nsec)) {
			ERR("CHANGED: [%s] at %d\n", watching_files[c].name.c_str(), (int)watching_files[c].updated.tv_sec);
			changes = true;
		}
	}
	
	if (changes)
		program_init(true);
}

#endif
