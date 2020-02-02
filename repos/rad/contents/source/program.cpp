/*
 *  Program.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 11/22/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

#include "program.h"
#include "svgloader.h"
#include "tinyxml.h"
#include "terminal.h"
#include "physics.h"
#include "voxel_loader.h"
#include "physfs.h"

#if 0 // Hyperverbal
#define HYPERERR ERR
#else
#define HYPERERR EAT
#endif

#define BAIL 1

room_auto *room_auto::_singleton = NULL;

class roomloader : public svgloader {
	room_auto *owner;
	virtual bool loadRootXml(TiXmlElement *xml, const svgtransform &parent_transform);
	virtual bool addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform);
public:
	roomloader(room_auto *_owner, Screen *__screen = NULL) : svgloader(__screen), owner(_owner) {}
};

room_auto::room_auto(const string &_svgname, const string &_overlayname, bool _fake) : automaton(), screen(NULL), scene(NULL), block(NULL), svgname(_svgname), overlayname(_overlayname), fake(_fake), a(NULL), b(NULL), onUpdate(NULL) {
}

room_auto::~room_auto() {
	clear_stored_functions();
}

void room_auto::clear_stored_functions() {
	for(hash_map<void *, storedfunc *>::iterator i = onCollide.begin(); i != onCollide.end(); i++)
		delete i->second;
	onCollide.clear();
	delete onUpdate;
	onUpdate = NULL;
}

void room_auto::insert() {
	automaton::insert();
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYDOWN); // Handle esc
	if (fake) return;
	
	// TODO: Move outside of .mm file?
	screen = new Screen();
	owned_screen.push_back(screen);
	
	scene = new ExtCollisionScene();
	owned_screen.push_back(scene);
	
	block = new SceneEntity();
	owned_screen.push_back(block);

	Object overlay;
	if (!overlayname.empty()) {
		ERR("Loading overlay from %s\n", overlayname.c_str());
		loadFromTree(overlay, overlayname);
		
		physics_voxel_loader v(scene, this);
		v.load(overlayname + "/level.png");
	}

	if (!svgname.empty()) {
		roomloader svg(this, screen);
		svg.load(svgname);
	}
	
	loadFromOverlay(overlay);
	Clear(overlay);
	
	_singleton = this;
	
	if (!onLoad.empty()) {
		bool success = terminal_auto::singleton()->execute(onLoad);
		if (BAIL && !success)
			clear_stored_functions();
		onLoad.clear(); // Will never be used again (valid assumption?)
	}
}

void room_auto::die() {
	if (done) return;
	
	if (!onClose.empty()) {
		terminal_auto::singleton()->execute(onClose); // Don't bother clearing, we're dying
	}	
	automaton::die();
	
	cor->getInput()->removeAllHandlers(); // Remove EVERYTHING, not just our listeners, in case Lua did something
	terminal_auto::singleton()->setup_events();
}

void room_auto::tick() {
	if (done) return;
	automaton::tick();

	terminal_auto::singleton()->inject_global("ticks", frame);
	
	if (onUpdate) {
		bool success = onUpdate->execute();
		if (BAIL && !success)
			clear_stored_functions();
	}
}

void room_auto::rebirth(bool fake) {
	die();
	(new room_auto(svgname, overlayname, fake))->insert();
}

void room_auto::handleEvent(Event *e) {
	if (done) return;
	if(e->getDispatcher() == cor->getInput()) {
		InputEvent *inputEvent = (InputEvent*)e;
		
		switch(e->getEventCode()) {
			case InputEvent::EVENT_KEYDOWN:
				int code = inputEvent->keyCode();
				if (!IS_MODIFIER(code)) {
					if (code == KEY_ESCAPE) {
#if SELF_EDIT
						rebirth();
#else
						Quit();
#endif
					}
				}
				break;
		}
		
	} else if (e->getDispatcher() == screen) {
		PhysicsSceneEvent *pe = (PhysicsSceneEvent*)e;
		switch(e->getEventCode()) {
			case PhysicsSceneEvent::COLLISION_EVENT: {
				if (onCollide.count(pe->entityA)) {
					terminal_auto::singleton()->inject_global("impact", pe->appliedImpulse);
					doCollide(pe->entityA->getSceneEntity(), pe->entityB->getSceneEntity(), true);
				}
				if (onCollide.count(pe->entityB)) {
					terminal_auto::singleton()->inject_global("impact", pe->appliedImpulse);
					doCollide(pe->entityB->getSceneEntity(), pe->entityA->getSceneEntity(), false);
				}
			}
		}
	}
}
							  
void room_auto::doCollide(SceneEntity *acting, SceneEntity *against, bool) { // Assumes count > 0
	a = acting; b = against;
	bool success = onCollide[acting]->execute();
	if (BAIL && !success)
		clear_stored_functions();
	a = NULL; b = NULL;
}

void room_auto::loadFromOverlay(Object &overlay) {
	ObjectEntry *temp;
	
	if (temp = overlay.root["onLoad"]) {
		onLoad = temp->stringVal.getSTLString();
	}
	if (temp = overlay.root["onUpdate"]) {
		delete onUpdate; // Could be overriding something, hypothetically
		onUpdate = terminal_auto::singleton()->compile(temp->stringVal.getSTLString());
	}
	if (temp = overlay.root["onClose"]) {
		onClose = temp->stringVal.getSTLString();
	}
}

bool roomloader::loadRootXml(TiXmlElement *xml, const svgtransform &parent_transform) {
	const char *temp = xml->Attribute("polycode:onLoad");
	if (temp)
		owner->onLoad = temp;
	
	temp = xml->Attribute("polycode:onUpdate");
	if (temp)
		owner->onUpdate = terminal_auto::singleton()->compile(temp);
	
	temp = xml->Attribute("polycode:onClose");
	if (temp)
		owner->onClose = temp;
	
	return svgloader::loadRootXml(xml, parent_transform);
}

bool roomloader::addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform) {
	svgloader::addChild(shape, xml, kind, transform);
	
	return true;
}

save_file::save_file() : priority(0) {}

#define SAVEFILE_NAME ( PROGDIR "save.xml" )

void save_file::load() {
	Object f; ObjectEntry *t;
	f.loadFromXML(SAVEFILE_NAME);
	t = f.root["priority"]; if (t) priority = t->intVal;
	t = f.root["file"];		if (t) file = t->stringVal.getSTLString();
}

void save_file::save() {
	Object f;
	f.root.name = "luanauts_save";
	f.root.addChild("priority", priority);
	f.root.addChild("file", file);
	f.saveToXML(SAVEFILE_NAME);
}

void getBaseName(string &basename) {
	while (basename.size() && basename[basename.size()-1] == '/') // Strip all /s from the end
		basename = basename.substr(0,basename.size()-1);
	size_t slash = basename.rfind("/"); // Strip everything left of rightmost /
	if (slash != string::npos)
		basename = basename.substr(slash+1);
	size_t dot = basename.rfind("."); // Strip everything right of rightmost .
	if (dot != string::npos)
		basename = basename.substr(0, dot);	
}

void loadFromTree(ObjectEntry &o, const string &path) {
	const char *cpath = path.c_str();
	string basename = path;
	getBaseName(basename);
	o.name = String(basename);
	HYPERERR("Node '%s':", basename.c_str());
	
	if (PHYSFS_isDirectory(cpath)) {
		o.type = ObjectEntry::CONTAINER_ENTRY;
		char **rc = PHYSFS_enumerateFiles(cpath);
		HYPERERR("recurse [\n");
		
		for (char **i = rc; *i != NULL; i++) { // For directories, iterate files and recurse.
			ObjectEntry *e = new ObjectEntry;
			string recurseName = path + "/" + *i;
			loadFromTree(*e, recurseName);
			o.addChild(e);
		}
		
		HYPERERR("]\n");
		PHYSFS_freeList(rc);
	} else {
		o.type = ObjectEntry::STRING_ENTRY; // TODO: Load numbers, too
		
		#define LOADTREE_CHUNK 1024
		PHYSFS_file* efile = PHYSFS_openRead(cpath);
		
		char temp[LOADTREE_CHUNK];
		int length_read;
		while (0 < (length_read = PHYSFS_read (efile, temp, 1, LOADTREE_CHUNK))) {
			o.stringVal += String(temp, length_read);
		}
		
		HYPERERR("value '%s'\n", o.stringVal.c_str());
		
		PHYSFS_close(efile);
	}
}

void loadFromTree(Object &o, const string &path) {
	Clear(o);
	loadFromTree(o.root, path);
}

save_file save;