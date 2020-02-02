#ifndef ERROR_H
#define ERROR_H

#define WARNING_SOUNDINIT "Sound will not be available"
#define ERROR_SDLINIT "Failed to initialize SDL"
#define ERROR_VIDEOINIT "Failed to initialize video mode"
#define ERROR_MEMALLOC "Could not allocate memory"
#define ERROR_FILEREAD "Failed to open a file for reading"
#define ERROR_FILEWRITE "Failed to open a file for writing"
#define ERROR_DIROPEN "Failed to open a directory"
#define ERROR_IMGOPEN "Failed to open image"
#define ERROR_SOUNDOPEN "Failed to open sound"
#define ERROR_SYNTAX "Syntax error in file"

void warning(const char* message);
void warningarg(const char* message, const char* arg);
void error(const char* message);
void errorarg(const char* message, const char* arg);
void errorarg2(const char* message, const char* arg1, const char* arg2);

#endif // ERROR_H
