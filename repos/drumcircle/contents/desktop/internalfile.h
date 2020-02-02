// Cross-platform "internal file" header
 
#include <stdio.h>

#define FILENAMESIZE 512

// A function that in a cross-platform way produces the absolute path to an "internal file"--
// On Windows, this means a file in the directory named "Internal" that accompanies the .exe.
// On the mac, this means a file in the "Resources" folder inside the application package.
void internalPath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0);

// Same, but stores in a suitable "user directory"
void userPath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0);