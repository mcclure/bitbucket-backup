#ifndef PLAIDGADGET_BINARY_H
#define PLAIDGADGET_BINARY_H


#include "types.h"


/*
	This header defines routines for portable encoding of numeric and text data.

	Numbers are encoded as little-endian and text as UTF-8
*/


namespace plaid
{
	//UTF-8 encoding and decoding
	std::string UTF8Encode(const String &str, char err = '?');
	String UTF8Decode(const std::string &str, Char err = L'?');

	//Sets up all the function pointers below.
	//void binary_init();
}


#endif // PLAIDGADGET_BINARY_H
