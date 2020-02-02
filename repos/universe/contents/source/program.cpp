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

room_auto *room_auto::_singleton = NULL;

class roomloader : public svgloader {
	room_auto *owner;
	virtual bool loadRootXml(TiXmlElement *xml, const svgtransform &parent_transform);
	virtual bool addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform);
public:
	roomloader(room_auto *_owner, Screen *__screen = NULL) : svgloader(__screen), owner(_owner) {}
};

room_auto::room_auto(const string &_filename) : automaton(), screen(NULL), filename(_filename), a(NULL), b(NULL), onUpdate(NULL) {
}

room_auto::~room_auto() {
	for(hash_map<void *, storedfunc *>::iterator i = onCollide.begin(); i != onCollide.end(); i++)
		delete i->second;
	delete onUpdate;
}

void room_auto::insert() {
	automaton::insert();
	
	// TODO: Move outside of .mm file?
	screen = new Screen();
	owned_screen.push_back(screen);
	
	scene = new Scene();
	owned_screen.push_back(scene);
	
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYDOWN);

	roomloader svg(this, screen);
	svg.load(filename);
	
	_singleton = this;
	
	if (!onLoad.empty()) {
		terminal_auto::singleton()->execute(onLoad);
		onLoad.clear(); // Will never be used again (valid assumption?)
	}
}

void room_auto::die() {
	automaton::die();
	
	cor->getInput()->removeAllHandlers(); // Remove EVERYTHING, not just our listeners, in case Lua did something
	terminal_auto::singleton()->setup_events();
}

void room_auto::tick() {
	automaton::tick();

	terminal_auto::singleton()->inject_global("ticks", frame);
	
	if (onUpdate)
		onUpdate->execute();
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
						die();
						(new room_auto(filename))->insert();
#else
						Quit();
#endif
					}
				}
				break;
		}
		
	} else if (e->getDispatcher() == screen) {
		PhysicsScreenEvent *pe = (PhysicsScreenEvent*)e;
		switch(e->getEventCode()) {
			case PhysicsScreenEvent::EVENT_NEW_SHAPE_COLLISION: {
				terminal_auto::singleton()->inject_global("impact", pe->impactStrength);
				if (onCollide.count(pe->entity1)) {
					doCollide(pe->entity1, pe->entity2, true);
				}
				if (onCollide.count(pe->entity2)) {
					doCollide(pe->entity2, pe->entity1, false);
				}
			}
		}
	}
}
							  
void room_auto::doCollide(ScreenEntity *acting, ScreenEntity *against, bool) { // Assumes count > 0
	a = acting; b = against;
	onCollide[acting]->execute();
	a = NULL; b = NULL;
}

bool roomloader::loadRootXml(TiXmlElement *xml, const svgtransform &parent_transform) {
	const char *temp = xml->Attribute("polycode:onLoad");
	if (temp)
		owner->onLoad = temp;
	
	temp = xml->Attribute("polycode:onUpdate");
	if (temp)
		owner->onUpdate = terminal_auto::singleton()->compile(temp);
	
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

save_file save;