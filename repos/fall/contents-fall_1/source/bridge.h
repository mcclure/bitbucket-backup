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

// Limitations in create_lua_library mean static or non-object methods cannot be called from Lua.
// This is an empty object that does nothing but contain staticky methods that Lua can actually see.

struct type_automaton;
struct lua_State;

struct NumberArray {
	NumberArray() {}
	vector<Number> contents;
	int size() { return contents.size(); }
	Number get(int at) { return at >= 0 && at < contents.size() ? contents[at] : 0.0; }
	void push_back(Number value) { contents.push_back(value); }
};

struct StringArray {
	vector<string> contents;
	int size() { return contents.size(); }
	String get(int at) { return at >= 0 && at < contents.size() ? String(contents[at]) : String(); }
	void push_back(String value) { contents.push_back(value.getSTLString()); }
};

struct VectorArray {
	vector<Vector3> contents;
	int size() { return contents.size(); }
	Vector3 get(int at) { return at >= 0 && at < contents.size() ? contents[at] : Vector3(0,0,0); }
	void push_back(Vector3 value) { contents.push_back(value); }
};

struct EntityArray {
	vector<Entity *> contents;
	int size() { return contents.size(); }
	Entity *get(int at) { return at >= 0 && at < contents.size() ? contents[at] : NULL; }
	void push_back(Entity *value) { contents.push_back(value); }
};

struct project_bridge {
	Screen *room_screen();
	Scene *room_scene();
	StringArray *room_objs();
	StringArray *room_oncollide_objs();
	StringArray *room_onclick_objs();
	Entity *room_id(String _at);
	String room_name(SceneEntity *of);
	Entity *room_a();
	Entity *room_b();
	void room_remove_scene(SceneEntity *obj);
	void room_remove_screen(ScreenEntity *obj, bool doRemove = true);
	void load_room(String from);
	void load_room_txt(String from);
	Screen *standalone_screen(String svg, String objectPath = String(), bool isPhysics = false);
	void room_remove_standalone_screen_all(Screen *s);
	void clear();
	ScreenMesh *meshFor(Polycode::Polygon *p);
	String saved_level();
	void set_saved_level(int priority);
	String filedump(String _path);
	String filedump_external(String _path);
	String help(String _path);
	void fake();
	void rebirth();
	void Quit();
	Matrix4 mmult(Matrix4 a, Matrix4 b);
	Quaternion qmult(Quaternion a, Quaternion b);
	Quaternion Slerp(Number fT, const Quaternion& rkP, const Quaternion& rkQ, bool shortestPath=false);
	Quaternion Squad(Number fT, const Quaternion& rkP, const Quaternion& rkA, const Quaternion& rkB, const Quaternion& rkQ, bool shortestPath);
	Vector3 bBox(Entity *e);
	void setSceneClearColor(Scene *scene, int r, int g, int b, int a);
	project_bridge() {}
	String custEntityType(Entity *obj);
	type_automaton *col40();
	String charCode(InputEvent *e);
	Sound *soundFromValues(NumberArray *values, int channels = 1, int freq = 44100, int bps = 16);
	void playback_index(); // Only useful in debug mode
	void playback_from(int idx);
	int luaTestAddOne(lua_State *);
	int saveTableIntoObject(lua_State *);
	int loadTableFromObject(lua_State *);
	int saveTableIntoFile(lua_State *);
	int loadTableFromFile(lua_State *);
	int saveTableIntoXml(lua_State *);
	int loadTableFromXml(lua_State *);
	int getChildAtScreenPosition(lua_State *);
	bool getVisible(Entity *e);
	void setVisible(Entity *e, bool visible);
	CoreServices *coreServices();
	int userSettings(lua_State *L);
	int debugSettings(lua_State *L);
	void register_room_name(SceneEntity *of, String name);
	int register_room_onCollision(lua_State *L);
	int register_room_onClick(lua_State *L);
	SceneLine *slbv(Vector3 a, Vector3 b, Number lineWidth = 1.0);
};

// Draws a "copy" of a scene object without having to duplicate its resources.
struct SceneEcho : SceneEntity {
	SceneEcho(Entity *_copying);
	virtual void transformAndRender();
	Entity *getEntity();
protected:
	Entity *copying;
};

string fromMatrix4(const Matrix4 &mat);
Vector3 matrixApply(const Matrix4 &m, Vector3 &v2);

#endif /* _BRIDGE_H */