#ifndef _BRIDGE_H
#define _BRIDGE_H

#include "program.h"

/*
 *  bridge.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 12/26/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

// Limitations in create_lua_library mean static or non-object methods cannot be called from LUA.
// This is an empty object that does nothing but contain staticky methods that Lua can actually see.

struct type_automaton;

struct StringArray {
	vector<string> contents;
	int size() { return contents.size(); }
	String get(int at) { return at >= 0 && at < contents.size() ? String(contents[at]) : String(); }
};

struct VectorArray {
	vector<Vector3> contents;
	int size() { return contents.size(); }
	Vector3 get(int at) { return at >= 0 && at < contents.size() ? contents[at] : Vector3(0,0,0); }
};

struct EntityArray {
	vector<SceneEntity *> contents;
	int size() { return contents.size(); }
	SceneEntity *get(int at) { return at >= 0 && at < contents.size() ? contents[at] : NULL; }
};

struct project_bridge {
	Screen *room_screen();
	PhysicsScene *room_scene();
	StringArray *room_objs();
	SceneEntity *room_id(String _at);
	String room_name(SceneEntity *of);
	SceneEntity *room_a();
	SceneEntity *room_b();
	void room_remove(SceneEntity *obj);
	void load_room(String from);
	void clear();
	ScreenMesh *meshFor(Polycode::Polygon *p);
	String saved_level();
	void set_saved_level(int priority);
	String help(String _path);
	String localf(String _path);
	void fake();
	SceneEntity *collides(SceneEntity *obj, Number inset);
	Matrix4 mmult(Matrix4 a, Matrix4 b);
	Quaternion qmult(Quaternion a, Quaternion b);
	Quaternion Slerp(Number fT, const Quaternion& rkP, const Quaternion& rkQ, bool shortestPath=false);
	Quaternion Squad(Number fT, const Quaternion& rkP, const Quaternion& rkA, const Quaternion& rkB, const Quaternion& rkQ, bool shortestPath);
	Vector3 bBox(Entity *e);
	project_bridge() {}
	Vector3 playerAt();
	VectorArray *diamondsAt();
	EntityArray *carpetsAt();
	SceneEntity *echoBlock();
	String custEntityType(Entity *obj);
	type_automaton *col40();
	String charCode(InputEvent *e);
	void playback_index(); // Only useful in debug mode
	void playback_from(int idx);
	void crash();
};

struct SceneEcho : SceneEntity {
	SceneEcho(Entity *_copying);
	virtual void transformAndRender();
	Entity *getEntity();
protected:
	Entity *copying;
};

#endif /* _BRIDGE_H */