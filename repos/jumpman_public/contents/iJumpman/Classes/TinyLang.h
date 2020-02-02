/*
 *  TinyLang.h
 *  iJumpman
 *
 *  Created by Andi McClure on 7/4/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
using namespace std;

extern string lang;

void InitLang();

const char *_(const char *in);
string _(const string &in);