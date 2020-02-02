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
#include "dos.h"
#include "physfs.h"
#include "vox.h"
#include <sstream>

Screen *project_bridge::room_screen() {
	return room_auto::singleton()->screen;
}

Scene *project_bridge::room_scene() {
	return room_auto::singleton()->scene;
}

StringArray *project_bridge::room_objs() {
	room_auto *room = room_auto::singleton();
	StringArray *result = new StringArray();
	for(obj_iter i = room->obj.begin(); i != room->obj.end(); i++)
		result->contents.push_back(i->first);
	return result;
}

Entity *project_bridge::room_id(String _at) {
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

Entity *project_bridge::room_a() {
	return room_auto::singleton()->a;
}
Entity *project_bridge::room_b() {
	return room_auto::singleton()->b;
}

void project_bridge::room_remove_scene(SceneEntity *obj) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(obj)) {
		const string &name = room->obj_name[obj];
		if (room->obj.count(name))
			room->obj.erase(name);
		room->obj_name.erase(obj); // "name" now invalid
		
		// Wait, is this in the Screen or the Scene? Eh, I guess it doesn't matter.
		if (room->scene)
			room->scene->removeEntity(obj);
	}
}

void project_bridge::room_remove_screen(ScreenEntity *obj) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(obj)) {
		const string &name = room->obj_name[obj];
		if (room->obj.count(name))
			room->obj.erase(name);
		room->obj_name.erase(obj); // "name" now invalid
		
		// Wait, is this in the Screen or the Scene? Eh, I guess it doesn't matter.
		if (room->screen)
			room->screen->removeChild(obj);
	}
}

void project_bridge::load_room(String from) {
	room_auto *room = room_auto::singleton();
	clear();
	if (room)
		room->die();
	(new room_auto(from.getSTLString()))->insert();
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
		save.file = room_auto::singleton()->spec;
		save.save();
	}
}

string help_attempt(const string &path) {
	return ::filedump(path);
}

String project_bridge::filedump(String _path) {
	return ::filedump(_path.getSTLString());
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

void project_bridge::fake() {
	room_auto::singleton()->rebirth(true);
}

Matrix4 project_bridge::mmult(Matrix4 a, Matrix4 b) {
	return a * b;
}

Quaternion project_bridge::qmult(Quaternion a, Quaternion b) {
	return a * b;
}

Vector3 project_bridge::qapply(Quaternion q, Vector3 v) {
	Quaternion result = q * Quaternion(0,v.x,v.y,v.z) * q.Inverse();
	return Vector3(result.x,result.y,result.z);
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

Sound *project_bridge::soundFromValues(NumberArray *values, int channels, int freq, int bps) {
	int s = values->contents.size();
	uint16_t *data = new uint16_t[s];
	for(int c = 0; c < s; c++) {
		data[c] = values->contents[c] * SHRT_MAX;
	}
	Sound *sound = new Sound((char *)data, s*sizeof(uint16_t), channels, freq, bps);
	delete[] data;
	return sound;
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
	load_room(::filedump("media/init.txt")); // Restart from true beginning
	common_playback_loader()->load_from(idx);
#endif
}

Vox *project_bridge::loadVoxel(String filename) {
	return Vox::load(filename.getSTLString());
}

void project_bridge::setSceneClearColor(Scene *scene, int r, int g, int b, int a) {
	scene->useClearColor = true;
	scene->clearColor = Color(r,g,b,a);
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

string fromMatrix4(const Matrix4 &mat) {
    ostringstream o;
    for(int y = 0; y < 4; y++) {
        o << (0==y?"[":" ") << "[";
        for(int x = 0; x < 4; x++) {
            o << mat.m[y][x] << (3==x?"":",\t");
        }
        o << "]" << (3==y?"]":"\n");
    }
    return o.str();
}