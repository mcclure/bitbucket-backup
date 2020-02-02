// Cross-platform "internal file" header
 
#include <stdio.h>
#include <string>

#define FILENAMESIZE 512

// A function that in a cross-platform way produces the absolute path to an "internal file"--
// On Windows, this means a file in the directory named "Internal" that accompanies the .exe.
// On mac or iPhone, this means a file in the "Resources" folder inside the application package.
// On Android, this means a user file directory where resources have been automatically unpacked.
void internalPath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0);

// Same, but stores in a suitable "user directory"
// On desktops, this means the directory where the executable lives.
// On Mac or Android, it will be designated user file directories.
void userPath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0);

// This last thing is a mechanism I use for development. Anytime UNLESS you're on a mac and
// in debug mode, it's an alias for internalPath. On the mac in debug mode, it instead loads
// the file out of the source code directory and also adds the file to a list of "watched"
// files. Watched files are checked once a second to see if they've changed; if so, the program
// resets using program_init(true). This is so you can see resource changes in realtime.
void liveInternalPath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0);

#if defined(SELF_EDIT) && defined(__APPLE__)
void watch_file(const std::string &filename);
void check_updates();
#else
inline void watch_file(const std::string &) {}
inline void check_updates() {}
#endif