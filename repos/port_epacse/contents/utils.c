#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h> // A wild POSIX appeared!
#include "error.h"

// Apparently this gives a proper distribution...
// I have no idea how it works either.
unsigned randint(unsigned N){
	int val;
	while(N <= (val = rand() / (RAND_MAX/N)));
	return val;
}

void removeMatchingFiles(const char* directory, const char* substr){
	DIR* dir;
	struct dirent* file;
	char* fullname;
	
	dir = opendir(directory);
	if(dir == NULL) errorarg(ERROR_DIROPEN, directory);
	while(1){
		file = readdir(dir);
		if(file == NULL) break;
		if(strstr(file->d_name, substr) != NULL){
			fullname = malloc(strlen(directory)+1+strlen(file->d_name)+1);
			strcpy(fullname, directory);
			strcat(fullname, "/");
			strcat(fullname, file->d_name);
			remove(fullname);
			free(fullname);
		}
	}
	closedir(dir);
}

unsigned char fileExists(const char* path){
	FILE* file;
	
	file = fopen(path, "rb");
	if(file == NULL)
		return 0;
	else {
		fclose(file);
		return 1;
	}
}
