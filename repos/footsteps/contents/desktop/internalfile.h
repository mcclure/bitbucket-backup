// Cross-platform "internal file" header
 
#include <stdio.h>
#include <string.h>

#define FILENAMESIZE 512

// A function that in a cross-platform way produces the absolute path to an "internal file"--
// On Windows, this means a file in the directory named "Internal" that accompanies the .exe.
// On the mac, this means a file in the "Resources" folder inside the application package.
void internalPath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0);

inline void userPath(char *dst, const char *src) {
	strncpy(dst, src, FILENAMESIZE);
}

// This last thing is a mechanism I use for development. Anytime UNLESS you're on a mac and
// in debug mode, it's an alias for internalPath. On the mac in debug mode, it instead loads
// the file out of the source code directory and also adds the file to a list of "watched"
// files. Watched files are checked once a second to see if they've changed; if so, the program
// resets using program_init(true). This is so you can see resource changes in realtime.
void liveInternalPath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0);

#if SELF_EDIT && defined(TARGET_DESKTOP)
void watch_file(const std::string &filename);
void check_updates();
#else
inline void watch_file(const std::string &) {}
inline void check_updates() {}
#endif