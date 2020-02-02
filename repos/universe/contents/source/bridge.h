#ifndef _BRIDGE_H
#define _BRIDGE_H

#include "program.h"

/*
 *  bridge.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 12/26/11.
 *  Copyright 2011 Tomatogon. All rights reserved.
 *
 */

// Limitations in create_lua_library mean static or non-object methods cannot be called from LUA.
// This is an empty object that does nothing but contain staticky methods that Lua can actually see.

struct StringArray {
	vector<string> contents;
	int size() { return contents.size(); }
	String get(int at) { return at >= 0 && at < contents.size() ? String(contents[at]) : String(); }
};

struct project_bridge {
	Screen *room_screen();
	Scene *room_scene();
	StringArray *room_objs();
	ScreenEntity *room_id(String _at);
	String room_name(ScreenEntity *of);
	ScreenEntity *room_a();
	ScreenEntity *room_b();
	void room_remove(ScreenEntity *obj);
	void load_room(String from);
	void clear();
	ScreenMesh *meshFor(Polycode::Polygon *p);
	Number getSpin(PhysicsScreen *screen, ScreenEntity *entity);
	String saved_level();
	void set_saved_level(int priority);
	String help(String _path);
	project_bridge() {}
};

struct SceneEcho : Entity {
	SceneEcho(Entity *_copying);
	virtual void transformAndRender();
	Entity *getEntity();
protected:
	Entity *copying;
};

#endif /* _BRIDGE_H */