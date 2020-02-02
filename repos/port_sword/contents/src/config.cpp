/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Jacob's Matrix Development
 *
 * NAME:        config.cpp ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 */

#include <libtcod.hpp>

#include "config.h"

#include <fstream>
using namespace std;

CONFIG *glbConfig = 0;

CONFIG::CONFIG()
{
}

CONFIG::~CONFIG()
{
}

void
CONFIG::load(const char *fname)
{
    TCODParser		parser;
    TCODParserStruct	*flame, *music, *screen;

    flame = parser.newStructure("blood");
    flame->addProperty("dynamic", TCOD_TYPE_BOOL, true);

    music = parser.newStructure("music");
    music->addProperty("enable", TCOD_TYPE_BOOL, true);
    music->addProperty("file", TCOD_TYPE_STRING, true);
    music->addProperty("volume", TCOD_TYPE_INT, true);

    screen = parser.newStructure("screen");
    screen->addProperty("full", TCOD_TYPE_BOOL, true);

    parser.run(fname, 0);

    myBloodDynamic = parser.getBoolProperty("blood.dynamic");

    myMusicEnable = parser.getBoolProperty("music.enable");
    myMusicFile = parser.getStringProperty("music.file");
    myMusicVolume = parser.getIntProperty("music.volume");

    myFullScreen = parser.getBoolProperty("screen.full");
}

void
CONFIG::save(const char *fname)
{
    // So TCOD has a parser that does thet hard part,
    // of trying to read in a file.

    // But it has no way to write out a changed version?

    // Argh!
    ofstream		os(fname);

    os << "blood" << endl;
    os << "{" << endl;
    os << "    dynamic = " << (myBloodDynamic ? "true" : "false") << endl;
    os << "}" << endl;
    os << endl;

    os << "music" << endl;
    os << "{" << endl;
    os << "    enable = " << (myMusicEnable ? "true" : "false") << endl;
    // Heaven help you if you have " in your file as we don't escape here.
    os << "    file = \"" << myMusicFile << "\"" << endl;
    os << "    volume = " << myMusicVolume << endl;
    os << "}" << endl;
    os << endl;

    os << "screen" << endl;
    os << "{" << endl;
    os << "    full = " << (myFullScreen ? "true" : "false") << endl;
    os << "}" << endl;
    os << endl;
}
