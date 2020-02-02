// Caml wrap
#include <stdlib.h>
#include <string.h>

void caml_startup(char** args); // "EXTERN C"

void caml_startup_wrap(int argc, char **argv)
{
    int word = sizeof(char *);
    char** terminated = malloc(word*(argc+1));
    memcpy(terminated, argv, argc*word);
    terminated[argc] = NULL;

    caml_startup(terminated);

    free(terminated);
}

int main(int argc, char **argv)
{
    caml_startup_wrap(argc, argv);

    return 0;
}

// Unit test functions

int TESTincrement3(int a)
    { return a + 3; }
int TESTsubtract(char *a, int b)
    { return atoi(a) - b; }
int TESTmultThenSubtract(int a, double b, char *c)
    { return a * ((int)b) - atoi(c); }
int TESTmultThenSubtractMult(int a, double b, char *c, int d)
    { return a * ((int)b) - atoi(c) * d; }
int TESTmultSubtractMultSubtract(int a, double b, char *c, int d, char *e)
    { return a * ((int)b) - atoi(c) * d - atoi(e); }
int TESTmultSubtractMultSubtractMult(int a, double b, char *c, int d, char *e, double f)
    { return a * ((int)b) - atoi(c) * d - atoi(e) * ((int)f); }
int TESTmultSubtractMultSubtractMultSubtract(int a, double b, char *c, int d, char *e, double f, int g)
    { return a * ((int)b) - atoi(c) * d - atoi(e) * ((int)f) - g; }
int TESTmultSubtractMultSubtractMultSubtractMult(int a, double b, char *c, int d, char *e, double f, int g, double h)
    { return a * ((int)b) - atoi(c) * d - atoi(e) * ((int)f) - g * ((int)h); }

