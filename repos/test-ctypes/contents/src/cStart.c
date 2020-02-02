// Caml wrap
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

extern void caml_startup(char **argv);

static bool initialized = false;

void emilyStartupArgs(char **argv)
{
	if (initialized)
		return;
	initialized = true;

	caml_startup(argv);
}

void emilyStartup()
{
	char **empty = {NULL};
	emilyStartupArgs(empty);
}

void emilyStartupCountArgs(int argc, char **argv)
{
	int word = sizeof(char *);
	char** terminated = malloc(word*(argc+1));
	memcpy(terminated, argv, argc*word);
	terminated[argc] = NULL;

	emilyStartupArgs(terminated);

	free(terminated);
}
