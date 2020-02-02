#ifndef _AUTOMATON_H
#define _AUTOMATON_H

/*
 *  automaton.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 3/8/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */


#include <list>
using namespace std;
using namespace Polycode;

struct automaton;
extern int ticks; // Global ticks
typedef list<automaton *>::iterator auto_iter;
extern list<automaton *> automata;

// manages "animations"
struct automaton : EventHandler {
	bool done;
    int born;  // ticks born at
    int frame; // ticks within state
    int state; // which state are you at?
    int rollover; // what frame does state roll at?
    auto_iter anchor;
	vector<EventDispatcher *> owned_screen;
    automaton();
    inline int age() { return ticks-born; }
    virtual void tick();
    virtual void die();
    virtual void insert() {
        automata.push_back(this);
        anchor = --automata.end();
    }
    virtual ~automaton();
};

#endif /* _BRIDGE_H */