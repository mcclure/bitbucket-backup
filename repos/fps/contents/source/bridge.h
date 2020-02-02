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

struct BridgeRayTestResult {
	SceneEntity *_entity;
	Vector3 _normal;
	Vector3 _position;
	SceneEntity *entity() { return _entity; }
	Vector3 normal() { return _normal; }
	Vector3 position() { return _position; }
};

struct project_bridge {
	Screen *room_screen();
	Scene *room_scene();
	StringArray *room_objs();
	Entity *room_id(String _at);
	String room_name(SceneEntity *of);
	Entity *room_a();
	Entity *room_b();
	void room_remove_scene(SceneEntity *obj);
	void room_remove_screen(ScreenEntity *obj);
	void load_room(String from);
	void load_room_txt(String from);
	void clear();
	ScreenMesh *meshFor(Polycode::Polygon *p);
	String saved_level();
	void set_saved_level(int priority);
	String filedump(String _path);
	String help(String _path);
	void fake();
	void rebirth();
	Matrix4 mmult(Matrix4 a, Matrix4 b);
	Quaternion qmult(Quaternion a, Quaternion b);
	Vector3 qapply(Quaternion q, Vector3 v);
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
	
	// fps
	Vector3 rotateXzByAngle(Vector3 _v, Number angle);
	void setAngularFactor(PhysicsSceneEntity *e, Number f);
	void setLinearFactor(PhysicsSceneEntity *e, Number f);
	void setAngularVelocity(PhysicsSceneEntity *e, Vector3 v);
	void activate(PhysicsSceneEntity *e);
	void setGradient(SceneMesh *m, Number r1, Number g1, Number b1, Number a1, Number r2, Number g2, Number b2, Number a2);
	Image *subImage(Image *from, int xo, int yo, int xd, int yd);
	Texture *textureFromImage(Image *from);
	BridgeRayTestResult getFirstEntityInRay (PhysicsScene *s, const Vector3 *origin, const Vector3 *dest);
	void warpCursor(Number x, Number y);
	Number getFriction(PhysicsSceneEntity *e);
	void setFriction(PhysicsSceneEntity *e, Number friction);
	SceneMesh *normalPlane(Number x, Number y);
	SceneMesh *uprightBox(Number x, Number y, Number z);
};

// Draws a "copy" of a scene object without having to duplicate its resources.
struct SceneEcho : SceneEntity {
	SceneEcho(Entity *_copying);
	virtual void transformAndRender();
	Entity *getEntity();
protected:
	Entity *copying;
};

string filedump(string &path);
string fromMatrix4(const Matrix4 &mat);
Vector3 matrixApply(const Matrix4 &m, Vector3 &v2);

#endif /* _BRIDGE_H */