#include <stdlib.h>

int (*callbackTest)(int x) = NULL;

void emilyInternalPopulate(int (*_callbackTest)(int x))
{
	callbackTest = _callbackTest;
}