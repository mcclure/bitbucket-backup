/*
 *  displaycodes.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/3/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef DISPLAYCODES_H
#define DISPLAYCODES_H

enum display_code {
	D_P,
	D_C,
	D_T,
	D_CT,
	
	D_P3,
	D_C3,
	D_T3,
	D_CT3,
	D_MAX,
};

display_code display_code_unique();

// Make sure enum is hashable
namespace __gnu_cxx {
	template<> struct hash< display_code >
	{ size_t operator()( display_code x ) const { return hash< unsigned int >()( x ); } };
}

#endif