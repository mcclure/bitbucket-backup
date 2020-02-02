#ifndef ZPIPE_H
#define ZPIPE_H

#include <stdio.h>

int def(FILE *source, FILE *dest, int level); // Decompress
int inf(FILE *source, FILE *dest); // Compress
const char* zerr(int ret); // Return an error message

#endif // ZPIPE_H
