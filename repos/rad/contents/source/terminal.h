#ifndef _TERMINAL_H
#define _TERMINAL_H

/*
 *  terminal.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 11/22/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

#include "program.h"
#include <fstream>

// TODO: VISIBLE_TERMINAL without TERMINAL_LOG
#define ASCII_TEST 0
#define VISIBLE_TERMINAL _DEBUG
#define TERMINAL_LOG 0
struct lua_State; struct terminal_auto;	

struct storedfunc {
	static int key_generator;
	terminal_auto *parent;
	int key;
	storedfunc(terminal_auto*);
	~storedfunc();
	bool execute();
};

struct terminal_auto : public automaton { // Overlay
#if VISIBLE_TERMINAL
	ScreenLabel *entry, **output;
#if ASCII_TEST
	ScreenLabel *ascii;
#endif
	string entry_text, history_text, pending_text;
	Color pending_color;
	int output_line;
	bool want_reset;
#endif
#if TERMINAL_LOG
	ofstream console_log;
#endif
	double line_height, line_descender;
	int max_width, max_height;
	terminal_auto();
	virtual ~terminal_auto();
	virtual void insert();
	virtual void die();
	virtual void handleEvent(Event *e);
	virtual void tick();
	void output_cycle();
	void init_interpreter();
	bool execute(const string &p);
	storedfunc *compile(const string &p);
	void process_entry();
	void print_line(const string &line, const Color &color);
	void print_lines(const string &lines, const Color &color, int limit = -1);
	void print_errorcode(int code, bool ran);
	void lua_require(const char *package);
	void reset();
	void setup_events();
	void inject_global(string name, int value);
	void inject_global(string name, double value);
	void inject_global(string name, string value);
	lua_State *lua() { return L; }
	static terminal_auto *singleton() { return _singleton; }
protected:
	lua_State *L;
	std::vector<String> loadedModules;
	static terminal_auto *_singleton;
};

#endif