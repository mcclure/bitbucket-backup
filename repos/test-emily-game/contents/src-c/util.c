#include "util.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Don't try to work with strings over 500MB.
#define SILLY_STRING (1024*1024*500)

char *concat(char *a, char *b)
{
    if (!a || !b) return NULL;

    int aLen = strlen(a);
    int bLen = strlen(b);
    if (aLen > SILLY_STRING || bLen > SILLY_STRING) return NULL;

    int cLen = aLen+bLen;
    char *c = malloc(cLen+1);
    if (!c) return NULL;

    memcpy(c,      a, aLen);
    memcpy(c+aLen, b, bLen);
    c[cLen] = '\0';
    return c;
}
