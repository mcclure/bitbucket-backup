#ifndef AYNL_TYPES_H_INCLUDED
#define AYNL_TYPES_H_INCLUDED

/*
	If stdint.h cannot be found then download an appropriate version for the
		compiler being used.
*/
#include <stdint.h>
#include <string>
#include <sstream>


namespace plaid
{
	typedef uint8_t      Uint8;
    typedef int8_t       Sint8;

    typedef uint16_t     Uint16;
    typedef int16_t      Sint16;

    typedef uint32_t     Uint32;
    typedef int32_t      Sint32;

    typedef uint64_t     Uint64;
    typedef int64_t      Sint64;


    //Plaidgadget uses wide strings
	typedef wchar_t              Char;
	typedef std::wstring         String;
	typedef std::wstringstream   StringStream;
	typedef std::wostringstream  OStringStream;
	typedef std::wistringstream  IStringStream;


	//String conversion
	std::string ToStdString(const String &str);
	String ToPGString(const std::string &str);
}


#endif // AYNL_TYPES_H_INCLUDED
