/*
 *  terminal.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 11/22/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

#include "program.h"
#include "terminal.h"
#include "OSBasics.h"
#include "physfs.h"
#include <sstream>

extern "C" {	
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "PolycodeLUA.h"
#include "ProjectLUA.h"
#if PHYSICS2D
#include "Physics2DLUA.h"
#endif
#if PHYSICS3D
#include "Physics3DLUA.h"
#endif

#define TEXT_INSET 10
#define FONT_NAME  "mono"
#define TEXT_SIZE 24

#if TERMINAL_LOG
#define LOGERR(x) console_log << "LUA ERROR: " << (x) << "\n"; console_log.flush();
#else
#define LOGERR(...)
#endif

#if VISIBLE_TERMINAL
static const Color output_color(1.0f,1.0f,0.0f,1.0f), entry_color(0.0f,1.0f,1.0f,1.0f), error_color(1.0f,0.5f,0.5f,1.0f);
#endif

#define DEFAULT_HIDDEN 1

terminal_auto *terminal_auto::_singleton = NULL;

terminal_auto::terminal_auto() : automaton(),
#if VISIBLE_TERMINAL
	entry(NULL), output(NULL), line_height(0), line_descender(0),
	max_width(0), max_height(0), output_line(0), want_reset(false), hidden(DEFAULT_HIDDEN),
#endif
#if TERMINAL_LOG
	console_log(PROGDIR "debug_log.txt", ios_base::trunc | ios_base::out),
#endif
	L(NULL)
{
	_singleton = this;
}

terminal_auto::~terminal_auto() {
	if (_singleton == this)
		_singleton = NULL;
#if VISIBLE_TERMINAL
	delete screen;
#endif
}

void terminal_auto::tick() {
#if VISIBLE_TERMINAL
	if (want_reset) {
		for(int c = 0; c < max_height; c++)
			output[c]->setText("");
		entry_text.clear(); // history_text is NOT cleared?
		entry->setText("");
		output_line = 0;
		output_cycle();
		want_reset = false;
	}
#endif
}

void terminal_auto::reset() {
#if VISIBLE_TERMINAL
	want_reset = true;
#endif
}

void terminal_auto::setup_events() {
#if VISIBLE_TERMINAL
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYDOWN);
#endif
}

void terminal_auto::insert() {
	automaton::insert();
	
	init_interpreter();
	
	FT_Face desiredFont = CoreServices::getInstance()->getFontManager()->getFontByName(FONT_NAME)->getFace();	
	FT_Set_Char_Size(desiredFont, TEXT_SIZE*64, TEXT_SIZE*64, 72, 72);
	line_height = desiredFont->size->metrics.height/64.0;
	line_descender = desiredFont->size->metrics.descender/64.0;
	
	max_height = (surface_height-TEXT_INSET*2) / line_height; max_height--;
	max_width = 67 * (surface_width/960.0); // By measurement it fits 67 chars on my screen, so...
	
#if VISIBLE_TERMINAL
	output = new ScreenLabel*[max_height];
	for(int c = 0; c < max_height; c++) {
		ScreenLabel *line = new ScreenLabel("", TEXT_SIZE, FONT_NAME);
		line->setPositionMode(ScreenEntity::POSITION_TOPLEFT);
		owned_screen.push_back(line);
		output[c] = line;
	}
	output_cycle();
	
	entry = new ScreenLabel("", TEXT_SIZE, FONT_NAME);
	entry->setPosition(TEXT_INSET,surface_height-line_height-TEXT_INSET);
	entry->setPositionMode(ScreenEntity::POSITION_TOPLEFT);
	entry->setColor(entry_color);
	owned_screen.push_back(entry);
	
#if ASCII_TEST
	ascii = new ScreenLabel("", TEXT_SIZE, FONT_NAME);
	screen->addChild(ascii);
	ascii->setPosition(TEXT_INSET,surface_height-line_height*2-TEXT_INSET);
	ascii->setPositionMode(ScreenEntity::POSITION_TOPLEFT);
	ascii->setColor(0.5,0.5,0.5,1.0);
	owned_screen.push_back(ascii);	
#endif
	
	if (!DEFAULT_HIDDEN)
		setup_screen();
	setup_events();
#endif
}

void terminal_auto::setup_screen() {
#if VISIBLE_TERMINAL
	screen = new Screen();
	for(int c = 0; c < max_height; c++)
		screen->addChild(output[c]);
	screen->addChild(entry);
#if ASCII_TEST
	screen->addChild(ascii);
#endif
#endif
}

void terminal_auto::output_cycle() {
#if VISIBLE_TERMINAL
	int scroll = output_line < max_height-1 ? 0 : output_line - (max_height-1);
	for(int c = 0; c < max_height; c++) {
		output[(c+scroll)%max_height]->setPosition(TEXT_INSET,TEXT_INSET+line_height*c);
	}
#endif
}

#if VISIBLE_TERMINAL
void terminal_auto::set_hidden(bool _hidden) {
	hidden = _hidden;
	if (hidden) {
		delete screen;
		screen = NULL;
	} else {
		setup_screen();
	}
}
#endif

void terminal_auto::die() {
	automaton::die();
#if VISIBLE_TERMINAL
	cor->getInput()->removeAllHandlersForListener(this);
#endif
}

void terminal_auto::handleEvent(Event *e) {
#if VISIBLE_TERMINAL
	if (done) return;

	if(e->getDispatcher() == cor->getInput()) {
		InputEvent *inputEvent = (InputEvent*)e;

		switch(e->getEventCode()) {
			case InputEvent::EVENT_KEYDOWN: {
				int code = inputEvent->keyCode();
				
				if (hidden && code != KEY_TAB)
					return;
				
				if (!pending_text.empty()) {
					if (code == KEY_RETURN) {
						string pending_temp = pending_text;
						pending_text.clear();
						print_lines(pending_temp, pending_color, 1);
					}
					break;
				}
				
#if ASCII_TEST
				{
					ostringstream o; o << "Code " << code << " char " << (int)inputEvent->charCode;
					ascii->setText(o.str());
				}
#endif
				
				bool entry_dirty = false, output_dirty = false;
				string output_text;
				if (code == KEY_BACKSPACE) {
					int len = entry_text.size();
					if (len) {
						entry_text.resize(len-1);
						entry_dirty = true;
					}
				} else if (code == KEY_RETURN) {
					output_dirty = true;
				} else if (IS_ARROW(code)) {
					if (code == KEY_UP) {
						if (!history_text.empty())
							entry_text = history_text;
						entry_dirty = true;
					} else if (code == KEY_DOWN) {
						entry_text.clear();
						entry_dirty = true;
					}
					// TODO: Left, right?
				} else if (code == KEY_TAB) { // Tab to show/hide terminal
					set_hidden(!hidden);
				} else if (!IS_MODIFIER(code) && code != KEY_ESCAPE && inputEvent->charCode) {					
					entry_text += inputEvent->charCode;
					entry_dirty = true;
				}
				
				if (output_dirty) {
					entry->setText("");
					process_entry();
				} else if (entry_dirty) {
					entry->setText(entry_text.size() <= max_width ?
								   entry_text :
								   entry_text.substr(entry_text.size()-max_width));
				}
			} break;
		}

	}	
#endif
}

extern "C" {
	int MyLoader(lua_State* pState)
	{		
		std::string module = lua_tostring(pState, 1);
		module += ".lua";
		
		std::string defaultPath = "API/";
		defaultPath.append(module);
		
		const char* fullPath = module.c_str();		
		Logger::log("Loading custom class: %s\n", module.c_str());
		OSFILE *inFile = OSBasics::open(module, "r");	
		
		if(!inFile) {
			inFile =  OSBasics::open(defaultPath, "r");	
		}
		
		if(inFile) {
			OSBasics::seek(inFile, 0, SEEK_END);	
			long progsize = OSBasics::tell(inFile);
			OSBasics::seek(inFile, 0, SEEK_SET);
			char *buffer = (char*)malloc(progsize+1);
			memset(buffer, 0, progsize+1);
			OSBasics::read(buffer, progsize, 1, inFile);
			luaL_loadbuffer(pState, (const char*)buffer, progsize, fullPath);		
			free(buffer);
			OSBasics::close(inFile);	
		} else {
			std::string err = "\n\tError - Could could not find ";
			err += module;
			err += ".";			
			lua_pushstring(pState, err.c_str());			
		}
		return 1;
	}
	
	static int debugPrint(lua_State *L)
	{
#if _DEBUG || VISIBLE_TERMINAL || TERMINAL_LOG
		const char *msg = lua_tostring(L, 1);
		terminal_auto *terminal = terminal_auto::singleton();
#if VISIBLE_TERMINAL
		if (terminal) { // TODO: Get object out of L instead of using singleton?
			if (msg)
				terminal->print_lines(msg, output_color);
			else
				terminal->print_line("(Unprintable)", error_color);
		}
#endif
#if TERMINAL_LOG
		terminal->console_log << msg << "\n"; terminal->console_log.flush();
#endif
		ERR("PRINT MESSAGE: %s\n", msg); // Do I want to keep these?
#endif
		return 0;
	}		
};

void terminal_auto::lua_require(const char *package) {
	lua_getfield(L, LUA_GLOBALSINDEX, "require");
	lua_pushstring(L, package);		
	lua_call(L, 1, 0);
}

void terminal_auto::lua_require_all(const string &filename) {
	PHYSFS_file *f = PHYSFS_openRead(filename.c_str());
	if (f) { // Read in letters one at a time. Implicitly tokenize.
		string filename;
		int chunkread;
		char ch;
		do {
			chunkread = PHYSFS_read(f, &ch, 1, 1);
			if (chunkread < 0 || ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
				if (filename.size()) {
					lua_require(filename.c_str());
					filename.clear();
				}
			} else {
				filename += ch;
			}
		} while (chunkread >= 1);
		PHYSFS_close(f);
	}
}

void terminal_auto::init_interpreter() {
	L=lua_open();
	luaL_openlibs(L);
	
	// Register C functions
	luaopen_Polycode(L);
	lua_pop(L, 1); // Pop from luaL_openlib
	
	// KLUDGE: create_lua_library, for reasons unknown to me, in its generated cpp contains a line where
	// for non-Polycore libraries the top value on the stack becomes the new overridden CoreServices
	// instance. Because of this we need to leave a copy of CoreServices on the stack while we load:
	lua_pushlightuserdata (L, CoreServices::getInstance());
		
	luaopen_Project(L);
	lua_pop(L, 1); // Pop from luaL_openlib
	
#if PHYSICS2D
	luaopen_Physics2D(L);
	lua_pop(L, 1); // Pop from luaL_openlib
#endif
	
#if PHYSICS3D
	luaopen_Physics3D(L);
	lua_pop(L, 1); // Pop from luaL_openlib
#endif
	
	lua_pop(L, 1); // Pop getInstance (see KLUDGE above)
	
	// Set up custom loader
	lua_getfield(L, LUA_GLOBALSINDEX, "package");	// push "package"
	lua_getfield(L, -1, "loaders");					// push "package.loaders"
	lua_remove(L, -2);								// remove "package"
	
	// Count the number of entries in package.loaders.
	// Table is now at index -2, since 'nil' is right on top of it.
	// lua_next pushes a key and a value onto the stack.
	int numLoaders = 0;
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) 
	{
		lua_pop(L, 1);
		numLoaders++;
	}
	
	lua_pushinteger(L, numLoaders + 1);
	lua_pushcfunction(L, MyLoader);
	lua_rawset(L, -3);
	
	lua_register(L, "debugPrint", debugPrint); // TODO: What if no VISIBLE_TERMINAL?
	
	lua_require("class");		
	lua_require("Polycode");
	lua_require("Project");	
	lua_require_all("media/project_classes.txt");
#if PHYSICS2D
	lua_require("Physics2D");
	lua_require_all("media/physics2d_classes.txt");
#endif	
#if PHYSICS3D
	lua_require("Physics3D");
	lua_require_all("media/physics3d_classes.txt");
#endif
	lua_require("defaults");
	lua_require("project_util"); // Note: project_util must come after defaults
	
#if 0
	for(int i=0; i < loadedModules.size(); i++) {
		String moduleName = loadedModules[i];
#ifdef _WINDOWS
		TCHAR _tempPath[4098];
		TCHAR tempPath[4098];
		GetTempPathW(4098, _tempPath);
		GetLongPathNameW(_tempPath, tempPath, 4098);
		String moduleDestPath = String(tempPath) + String("\\") + moduleName+ String(".dll");
#else
		
#if defined(__APPLE__) && defined(__MACH__)
		String moduleDestPath = String("/tmp/") + moduleName+ String(".dylib");
#else
		String moduleDestPath = String("/tmp/") + moduleName+ String(".so");
#endif
#endif
		String moduleLoadCall = String("luaopen_") + moduleName;
		lua_getfield(L, LUA_GLOBALSINDEX, "require");
		lua_pushstring(L, moduleName.c_str());		
		lua_call(L, 1, 0);
		
		printf("LOADING MODULE %s\n", moduleDestPath.c_str());
		lua_getfield(L, LUA_GLOBALSINDEX, "package");
		lua_getfield(L, -1, "loadlib");	
		lua_pushstring(L, moduleDestPath.c_str());
		lua_pushstring(L, moduleLoadCall.c_str());			
		lua_call(L, 2, 2);
		lua_setfield(L, LUA_GLOBALSINDEX, "err");								
		lua_setfield(L, LUA_GLOBALSINDEX, "f");		
		
		lua_getfield(L, LUA_GLOBALSINDEX, "print");
		lua_getfield(L, LUA_GLOBALSINDEX, "err");						
		lua_call(L, 1, 0);						
		
		printf("SETTING CORE SERVICES\n");			
		lua_getfield(L, LUA_GLOBALSINDEX, "f");
		lua_getfield(L, LUA_GLOBALSINDEX, "__core__services__instance");						
		lua_call(L, 1, 0);			
		
		printf("DONE LOADING MODULE...\n");				
	}
#endif
}

bool terminal_auto::execute(const string &p) { // Would be more efficient taking const char * here?
	int loadstring = luaL_loadstring(L, p.c_str());
	int pcall = loadstring || lua_pcall(L, 0,0,0);
	//	ERR("LOAD RESULT %d PCALL RESULT %d\n", loadstring, pcall);
	if (pcall) {
#if VISIBLE_TERMINAL || TERMINAL_LOG
		print_errorcode(loadstring ? loadstring : pcall, !loadstring);
#endif
#if VISIBLE_TERMINAL
		print_lines(lua_tostring(L, -1), error_color, 0);
#endif
		LOGERR(lua_tostring(L, -1));
		ERR("LUA ERROR: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);  /* pop error message from the stack */
	}
	return !pcall;
}

int storedfunc::key_generator = 0x33113133; // Arbitrary and large

storedfunc::storedfunc(terminal_auto *_parent) : parent(_parent), key(key_generator++) {}
storedfunc::~storedfunc() {
	lua_State *L = parent->lua();
	lua_pushnumber(L, key);  /* push value */
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);
}
bool storedfunc::execute() {
	lua_State *L = parent->lua();
	lua_pushnumber(L, key);  /* push value */
	lua_gettable(L, LUA_REGISTRYINDEX);
	int pcall = lua_pcall(L, 0,0,0);
	if (pcall) {
#if VISIBLE_TERMINAL || TERMINAL_LOG
		parent->print_errorcode(pcall, false);
#endif
#if VISIBLE_TERMINAL
		parent->print_lines(lua_tostring(L, -1), error_color, 0);
#endif
#if TERMINAL_LOG
		parent->console_log << "LUA ERROR: " << lua_tostring(L, -1) << "\n"; parent->console_log.flush(); // ew
#endif
		ERR("LUA ERROR: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);  /* pop error message from the stack */
	}
	return !pcall;
}

storedfunc *terminal_auto::compile(const string &p) {
	storedfunc *result = NULL;
	lua_pushnumber(L, storedfunc::key_generator);
	int loadstring = luaL_loadstring(L, p.c_str());
	if (!loadstring) {
		result = new storedfunc(this);
		lua_settable(L, LUA_REGISTRYINDEX);
	} else {
#if VISIBLE_TERMINAL
		print_errorcode(loadstring, true);
		print_lines(lua_tostring(L, -1), error_color, 0);
#endif
		ERR("LUA ERROR: %s\n", lua_tostring(L, -1));
		lua_pop(L, 2);  /* pop error message + key from the stack */
	}
	return result;
}

void terminal_auto::process_entry() {
#if VISIBLE_TERMINAL
#if TERMINAL_LOG
	console_log << "> " << entry_text << "\n"; console_log.flush();
#endif
	print_line("> " + entry_text, entry_color);
	execute(entry_text);
	if (!entry_text.empty())
		history_text = entry_text;
	entry_text.clear();
#endif
}

void terminal_auto::inject_global(string name, int value) {
	lua_pushinteger(L, value);
	lua_setglobal(L, name.c_str());	
}

void terminal_auto::inject_global(string name, double value) {
	lua_pushnumber(L, value);
	lua_setglobal(L, name.c_str());	
}

void terminal_auto::inject_global(string name, string value) {
	lua_pushstring(L, value.c_str());
	lua_setglobal(L, name.c_str());	
}

string readable_error(int code) {
	switch(code) {
		case LUA_YIELD:
			return "LUA_YIELD";
		case LUA_ERRRUN:
			return "LUA_ERRRUN";
		case LUA_ERRSYNTAX:
			return "LUA_ERRSYNTAX";
		case LUA_ERRERR:
			return "LUA_ERRERR";
		default:
			return "UNKNOWN";
	}
}

void terminal_auto::print_errorcode(int code, bool ran) {
#if VISIBLE_TERMINAL || TERMINAL_LOG
	ostringstream o;
	o << "Error " << (ran ? "compiling: " : "running: ") << readable_error(code);
#if VISIBLE_TERMINAL
	print_line(o.str(), error_color);
#endif
#if TERMINAL_LOG
	console_log << o.str() << "\n";
#endif
#endif
}

void terminal_auto::print_line(const string &line, const Color &color) {
#if VISIBLE_TERMINAL
	output_cycle();
	ScreenLabel *current = output[output_line%max_height];
	current->setText(line);
	current->setColor(color);
	output_line++;
#endif
}

void terminal_auto::print_lines(const string &line, const Color &color, int limit) {
#if VISIBLE_TERMINAL
	int linestart = 0;
	int lines = 0;
	if (limit < 0)
		limit = max_height;
	while(linestart < line.size()) { // TODO: Halt if too many lines printed from one string?
		bool truncated = false;
		
		if (limit && lines >= limit) {
			pending_text = line.substr(linestart);
			pending_color = color;
			entry->setText("<PRESS RETURN FOR MORE>");
			break;
		}
		
		int lineend = line.find("\n", linestart);
		if (lineend == string::npos)
			lineend = line.size();
		
		if (lineend - linestart > max_width) {
			lineend = linestart + max_width;
			truncated = true;
		}
		
		print_line(line.substr(linestart, lineend-linestart), color);
		lines++;
		linestart = lineend+!truncated; // int(true) == 1
	}
#endif
}