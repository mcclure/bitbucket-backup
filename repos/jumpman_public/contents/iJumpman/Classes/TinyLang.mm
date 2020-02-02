/*
 *  TinyLang.cpp
 *  iJumpman
 *
 *  Created by Andi McClure on 7/4/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <fstream>
#include "TinyLang.h"
#include "mutils.h"
#include <Foundation/Foundation.h>

// Ugh
#include "chipmunk.h"
#include "iJumpman.h"

string lang = "en";
#define LANGNUM 4
NSString *langnames[LANGNUM] = {@"en", @"es", @"fr", @"de"}; // Hardcode rather than pull from file?
int langi = 0;

hash_map<string, string> translate;

void InitLangChoose() {
    NSArray *langs = [NSLocale preferredLanguages];
    
    for (int i = 0; i < [langs count]; i++) {
        NSString *la = [langs objectAtIndex:i];
        //NSLog(@"%d: %@\n", i, la);
        
        for(int c = 0; c < LANGNUM; c++) {
            if (![la compare:langnames[c]]) {
                lang = toString(la);
                langi = c;
                return;
            }
        }
    }
}

// In my ad hoc format, tabs are interpreted as newlines
void fixtabs(string &str) {
    for(int c = 0; c < str.size(); c++) {
        if (str[c] == '\t')
            str[c] = '\n';
    }
}

void InitLang() {
    InitLangChoose();
    
    if (langi) {
        char filename[FILENAMESIZE];
        internalPath(filename, "jumpman_trans_all.txt");
        ifstream i(filename);
        
        while(!i.eof()) { // Eat one block-- TODO read this and base langi on it
            string line; getline(i, line); fixtabs(line);
            if (line.empty()) break;
        }
        
        while(!i.eof()) { // Eat all blocks
            string en; getline(i, en); fixtabs(en);
            int langcurrent = 1;
            
            while(!i.eof()) { // Eat one block
                string line; getline(i, line); fixtabs(line);
                if (line.empty()) break;
                if (langi == langcurrent)
                    translate[en] = line;
                langcurrent++;
            }
        }
    }
    
    ERR("\nStarted up with language %s%s\n", lang.c_str(), langi?" (translating)":" (not translating)");
}

string _(const string &in) {
    if (!langi) return in;
    if (translate.count(in))
        return translate[in];
    return in;
}

const char *_(const char *in) {
    if (!langi) return in;
    return _(string(in)).c_str();
}

NSString *_(NSString *in) {
    if (!langi) return in;
    return toNs(_(toString(in)));
}