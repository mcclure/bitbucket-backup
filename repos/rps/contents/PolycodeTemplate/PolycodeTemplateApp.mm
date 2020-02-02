//
// Polycode template. Write your code here.
// 

#include "PolycodeTemplateApp.h"
#include "program.h"
#include "terminal.h"
#include "playtest.h"
#include "physfs.h"

#if SELF_EDIT
#include <unistd.h>
#endif

// File prefix?
#ifdef __APPLE__
#define FP ""
#else
#define FP "Internal/"
#endif

#if SELF_EDIT || WIN_CONSOLE
#define FULLSCREEN 0
#else
#define FULLSCREEN 0
#endif

#if FULLSCREEN
#define SCREENMOD 1
#else
#define SCREENMOD 0.75
#endif

#define CALC_ASPECT (1280.0/800)

#define FPS 60

#define RESOURCE_PATH "media/"

PolycodeView *appView = NULL;

Object userSettingsObject;
ObjectEntry *userSettings = NULL;
bool debugMode = false;
Object debugSettingsObject;
ObjectEntry *debugSettings = NULL;

#if SELF_EDIT
string selfedit_dir;
#endif

PolycodeTemplateApp::PolycodeTemplateApp(PolycodeView *view) {
	appView = view; // Set singleton
	{ // Seed random
		time_t rawtime;
		time ( &rawtime );
		srandom(rawtime);
	}
	
#if defined(__APPLE__)
	// This will be done in CocoaCore, but before CocoaCore we still need to do some fopen-ing.
	chdir([[[[NSBundle mainBundle] bundlePath] stringByAppendingString:@"/Contents/Resources"] UTF8String]);
#endif
	
	// Check for user settings files
	FILE *f;
	f = fopen(PROGDIR "settings.xml", "r");
	if (f && userSettingsObject.loadFromXMLString(filedrain(f))) {
		userSettings = &userSettingsObject.root;
	}
	
#if _DEBUG
	debugMode = true;
#endif
	f = fopen(PROGDIR "debug.xml", "r");
	if (f) {
		debugMode = true;
		if (debugSettingsObject.loadFromXMLString(filedrain(f))) {
			debugSettings = &debugSettingsObject.root;
		}
	}
	
	// Size screen-- and check user settings
	bool fullscreen = FULLSCREEN;
	bool aa = true;
	int temp;
	Core::getScreenInfo(&fullscreen_width, &fullscreen_height, NULL);
	
	temp = intCheck(userSettings, "fullscreen", -1);
	if (temp >= 0) fullscreen = temp;
	
	temp = intCheck(userSettings, "aa", -1);
	if (temp >= 0) aa = temp;
	
	if (fullscreen) {
		surface_width = fullscreen_width;
		surface_height = fullscreen_height;
	} else {
		surface_width = 1120;
		surface_height = 700;
	}
	
	temp = intCheck(userSettings, "width");
	if (temp) surface_width = temp;
	
	temp = intCheck(userSettings, "height");
	if (temp) surface_height = temp;
	
	sbound = cpbounds(cpvzero,cpv(surface_width, surface_height));

#if defined(__APPLE__)
	core = new CocoaCore(view, surface_width,surface_height,fullscreen,aa, 0,0,FPS);
#elif defined(_WINDOWS)
	core = new Win32Core(view, surface_width,surface_height,fullscreen,aa, 0,0,FPS);
#else
	core = new SDLCore(view, surface_width,surface_height,fullscreen,aa, 0,0,FPS);
#endif
	cor = core;

	// Do this first to allow overriding anything: Load user mod
	PHYSFS_addToSearchPath (PROGDIR"mod", true);
	CoreServices::getInstance()->getResourceManager()->addArchive(PROGDIR"mod.zip");
	
	CoreServices::getInstance()->getResourceManager()->addArchive(FP"default.pak");
	CoreServices::getInstance()->getResourceManager()->addDirResource("default");

	CoreServices::getInstance()->getResourceManager()->addArchive(FP"api.pak");
	CoreServices::getInstance()->getResourceManager()->addArchive(FP"project.pak");

#if PHYSICS2D
	CoreServices::getInstance()->getResourceManager()->addArchive(FP"physics2d.pak");
#endif
#if PHYSICS3D
	CoreServices::getInstance()->getResourceManager()->addArchive(FP"physics3d.pak");
#endif
#if UI
	CoreServices::getInstance()->getResourceManager()->addArchive(FP"UI.pak");
#endif
	
#if SELF_EDIT
	{
// How many `cd ..`s between debug executable and project root?
#if __APPLE__
#define MEDIA_DEPTH 5
#else
#define MEDIA_DEPTH 3
#endif
		// In SELF_EDIT mode, rather than loading the media.pak archive we will attempt to load live data directly
		// out of media/. We do this by assuming that relative to the repository root, we are running in
		// "build/Debug/PolycodeTemplate.app/Contents/Resources"; we strip off five levels of dirs from the cwd
		// to get to the repository root, then add the result to the PhysFS search paths.
		char *cwd = getcwd(NULL, NULL);
		int slashes = 0;
		ERR("Full path %s\n", cwd);
		for(int c = strlen(cwd)-1; c >= 0; c--) {
			if (cwd[c] == '/')
				slashes++;
			if (slashes >= MEDIA_DEPTH) {
				cwd[c] = '\0';
				break;
			}
		}
		ERR("Will look for media dir in %s\n", cwd);
		selfedit_dir = cwd;
		PHYSFS_addToSearchPath (cwd, true);
		free(cwd);
	}
#else
	CoreServices::getInstance()->getResourceManager()->addArchive(FP"media.pak");
#endif	
	
	CoreServices::getInstance()->getResourceManager()->addDirResource("media/material", false);
	
#if UI
	CoreServices::getInstance()->getResourceManager()->addArchive(FP"UIThemes.pak");
	CoreServices::getInstance()->getConfig()->loadConfig("Polycode", "UIThemes/default/theme.xml");
#endif

	save.load(); // Game save file
	
	(new terminal_auto())->insert();
	
	terminal_auto *terminal = terminal_auto::singleton();
	terminal->inject_global("surface_height", surface_height);
	terminal->inject_global("surface_width",  surface_width);
	terminal->inject_global("scale",		  surface_height/800.0);
	terminal->inject_global("line_height",	  terminal->line_height);
	terminal->inject_global("max_width",	  terminal->max_width);
	terminal->inject_global("max_height",	  terminal->max_height);
	terminal->inject_global("FPS",			  FPS);
	terminal->inject_global("TPF",			  1.0/FPS);

	if (debugMode)
		terminal->inject_global("_DEBUG",		  1);
		
#if PLAYTEST_RECORD
	(new recorder_auto())->insert();
#endif
	
	(new room_auto(filedump("media/init.txt")))->insert();
}

PolycodeTemplateApp::~PolycodeTemplateApp() {
}

// Function

list<automaton *> automata;
vector<auto_iter> dying_auto;
Core *cor;
int ticks = 0;
int fullscreen_width, fullscreen_height, surface_width, surface_height;
cpRect sbound = cpr(cpvzero,cpvzero);

automaton::automaton() : EventHandler(), done(false), born(ticks), frame(0), state(0), rollover(1) {}

automaton::~automaton() {
	for(int c = 0; c < owned_screen.size(); c++)
		delete owned_screen[c];
}

void automaton::tick() {
	if (done) return;
	
    if (age() > 0) {
        frame++;
    }
    if (frame > rollover) {
        state++;
        rollover = 0;
    }
}

void automaton::die() {
	done = true;
	
    dying_auto.push_back(anchor);
}

bool PolycodeTemplateApp::Update() {
	ticks++;
	
	// TODO: Can all this be done with the polycode primitives?
	for(auto_iter i = automata.begin(); i != automata.end(); i++) {
        (*i)->tick();
    }            
	if (!dying_auto.empty()) {
        for(int c = 0; c < dying_auto.size(); c++) {
            automaton *a = *dying_auto[c];
            automata.erase(dying_auto[c]);
            delete a;
        }
        dying_auto.clear();
    }    
    
    bool result = cor->Update();
    cor->Render();
    return result;
}

void Quit() {
#ifdef _WINDOWS
	PostMessage(appView->hwnd, WM_CLOSE, NULL, NULL);
#else
	exit(0);
#endif
}
