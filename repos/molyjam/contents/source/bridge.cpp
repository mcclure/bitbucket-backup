/*
 *  bridge.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 12/26/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

#include "bridge.h"
#include "terminal.h"
#include "physics.h"
#include "dos.h"
#include "physfs.h"

Screen *project_bridge::room_screen() {
	return room_auto::singleton()->screen;
}

PhysicsScene *project_bridge::room_scene() {
	return room_auto::singleton()->scene;
}

StringArray *project_bridge::room_objs() {
	room_auto *room = room_auto::singleton();
	StringArray *result = new StringArray();
	for(obj_iter i = room->obj.begin(); i != room->obj.end(); i++)
		result->contents.push_back(i->first);
	return result;
}

SceneEntity *project_bridge::room_id(String _at) {
	string at = _at.getSTLString();
	room_auto *room = room_auto::singleton();
	if (room->obj.count(at))
		return room->obj[at];
	return NULL;
}

String project_bridge::room_name(SceneEntity *of) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(of))
		return String(room->obj_name[of]);
	return String();
}

SceneEntity *project_bridge::room_a() {
	return room_auto::singleton()->a;
}
SceneEntity *project_bridge::room_b() {
	return room_auto::singleton()->b;
}

void project_bridge::room_remove(SceneEntity *obj) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(obj)) {
		const string &name = room->obj_name[obj];
		if (room->obj.count(name))
			room->obj.erase(name);
		room->obj_name.erase(obj); // "name" now invalid
		room->scene->removeEntity(obj);
	}
}

void project_bridge::load_room(String from) {
	room_auto *room = room_auto::singleton();
	clear();
	if (room)
		room->die();
	(new room_auto("media/title.svg", from.getSTLString()))->insert();
}

void project_bridge::clear() {
	terminal_auto::singleton()->reset();
}

ScreenMesh *project_bridge::meshFor(Polycode::Polygon *p) { // Lua bridge doesn't work with ScreenMesh, so...
	bool triangle = (p->getVertexCount() == 3);
	ScreenMesh *s = (triangle ? new ScreenMesh(Mesh::TRI_MESH) : new ScreenMesh(Mesh::TRIFAN_MESH));
	Mesh *m = s->getMesh();
	m->addPolygon(p);
	return s;
}

// This doesn't actually do anything
String project_bridge::saved_level() {
	return save.file;
}
void project_bridge::set_saved_level(int priority) {
	if (save.priority < priority) {
		save.priority = priority;
		save.file = room_auto::singleton()->svgname;
		save.save();
	}
}

string help_attempt(const string &path) {
	PHYSFS_file *f = PHYSFS_openRead(path.c_str());
	if (f) {
#define CHUNKSIZE 1000
		string result;
		char chunk[CHUNKSIZE+1];
		int chunkread = 0;
		do {
			chunkread = PHYSFS_read(f, chunk, 1, CHUNKSIZE);
			if (chunkread < 0)
				chunkread = 0;
			chunk[chunkread] = '\0';
			result += chunk;
		} while (chunkread >= CHUNKSIZE);
		PHYSFS_close(f);
		return result;
	}
	return string();
}

String project_bridge::help(String _path) {
	string path = "media/help/" + _path.getSTLString();
	string sep = _path.size() ? "/" : "";
	for(int c = 0; c < path.size(); c++) {
		char &s = path[c];
		if (s == '.' || s == '\\' || s == ':')
			s = '/';
		if (s == '(' || s == ')')
			s = '\0';
	}
	string result;
	result = help_attempt(path + ".txt");
	if (result.empty()) result = help_attempt(path + sep + "index.txt");
	if (result.empty()) result = "No help found.";
	return String(result); // Not very efficient, but we don't need efficient.
}

String project_bridge::localf(String _path) {
	String path = PROGDIR; path += _path;
	FILE *f = fopen(path.c_str(),"rb");
	if (f) {
#define CHUNKSIZE 1000
		string result;
		char chunk[CHUNKSIZE+1];
		int chunkread = 0;
		do {
			chunkread = fread(chunk, 1, CHUNKSIZE, f);
			if (chunkread < 0)
				chunkread = 0;
			
			// Consume NULLs (so UTF-16 works)
			int eaten = 0;
			for(int c = 0, d = 0; c < chunkread; c++) {
				if (d < c)
					chunk[d] = chunk[c];
				if (!chunk[c]) {
					eaten++;
					continue;
				}
				d++;
			}
			
			chunk[chunkread - eaten] = '\0';
			result += chunk;
		} while (chunkread >= CHUNKSIZE);
		fclose(f);
		return result;
	}
	return string("NONE");
}

void project_bridge::fake() {
	room_auto::singleton()->rebirth(true);
}

SceneEntity *project_bridge::collides(SceneEntity *obj, Number inset) {
	return room_auto::singleton()->scene->collidesWith(obj, inset);
}

Matrix4 project_bridge::mmult(Matrix4 a, Matrix4 b) {
	return a * b;
}

Quaternion project_bridge::qmult(Quaternion a, Quaternion b) {
	return a * b;
}

Quaternion project_bridge::Slerp(Number fT, const Quaternion& rkP, const Quaternion& rkQ, bool shortestPath) {
	return Quaternion::Slerp(fT,rkP,rkQ,shortestPath);
}

Quaternion project_bridge::Squad(Number fT, const Quaternion& rkP, const Quaternion& rkA, const Quaternion& rkB, const Quaternion& rkQ, bool shortestPath) {
	return Quaternion::Squad(fT,rkP,rkA,rkB,rkQ,shortestPath);
}

Vector3 project_bridge::bBox(Entity *e) {
	return e->bBox;
}

Vector3 project_bridge::playerAt() {
	return room_auto::singleton()->playerAt;
}

VectorArray *project_bridge::diamondsAt() {
	VectorArray *v = new VectorArray();
	v->contents = room_auto::singleton()->diamondsAt;
	return v;
}

EntityArray *project_bridge::carpetsAt() {
	EntityArray *v = new EntityArray();
	v->contents = room_auto::singleton()->carpetsAt;
	return v;
}

SceneEntity *project_bridge::echoBlock() {
	return room_auto::singleton()->block;
}

String project_bridge::custEntityType(Entity *obj) {
	return obj->custEntityType;
}

type_automaton *project_bridge::col40() {
	return type_automaton::singleton();
}

String project_bridge::charCode(InputEvent *e) {
	wchar_t s[2] = {e->charCode, 0};
	return String(s);
}

void project_bridge::crash() {
	for(int *p=0;;*(p++)=0);
}

// Only do playback in debug mode-- otherwise there's no console!
#if _DEBUG
#include "playtest.h"

static playback_loader *common_playback_loader() {
	static playback_loader *__playback_loader = NULL;
	if (!__playback_loader)
		__playback_loader = new playback_loader();
	return __playback_loader;
}
#endif

void project_bridge::playback_index() { // Only useful in debug mode
#if _DEBUG
	string s = common_playback_loader()->index();
	ERR("%s\n", s.c_str());
#endif
}
void project_bridge::playback_from(int idx) {
#if _DEBUG
	common_playback_loader()->load_from(idx);
#endif
}

SceneEcho::SceneEcho(Entity *_copying) : SceneEntity(), copying(_copying) {
	bBox.x = 1;
	bBox.y = 1;
	bBox.z = 1;		
}

void SceneEcho::transformAndRender() {
	Entity *oldCopyParent = copying->getParentEntity();
	if (oldCopyParent)
		oldCopyParent->removeChild(copying);
	this->addChild(copying);
	Entity::transformAndRender();
	this->removeChild(copying);
	if (oldCopyParent)
		oldCopyParent->addChild(copying);
}

Entity *SceneEcho::getEntity() { return copying; }