#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>

void warning(const char* message){
	fprintf(stderr, "Warning: %s\n", message);
}

void warningarg(const char* message, const char* arg){
	fprintf(stderr, "Warning: %s: %s\n", message, arg);
}

void error(const char* message){
	if(SDL_WasInit(0) != 0) SDL_Quit();
	fprintf(stderr, "Error: %s", message);
	getchar();
	exit(1);
}

void errorarg(const char* message, const char* arg){
	if(SDL_WasInit(0) != 0) SDL_Quit();
	fprintf(stderr, "Error: %s: %s", message, arg);
	getchar();
	exit(1);
}

void errorarg2(const char* message, const char* arg1, const char* arg2){
	if(SDL_WasInit(0) != 0) SDL_Quit();
	fprintf(stderr, "Error: %s: %s: %s", message, arg1, arg2);
	getchar();
	exit(1);
}
