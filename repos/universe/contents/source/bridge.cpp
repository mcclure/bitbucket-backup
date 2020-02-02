/*
 *  bridge.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 12/26/11.
 *  Copyright 2011 Tomatogon. All rights reserved.
 *
 */

#include "bridge.h"
#include "terminal.h"
#include "physfs.h"

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

ScreenEntity *project_bridge::room_id(String _at) {
	string at = _at.getSTLString();
	room_auto *room = room_auto::singleton();
	if (room->obj.count(at))
		return room->obj[at];
	return NULL;
}

String project_bridge::room_name(ScreenEntity *of) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(of))
		return String(room->obj_name[of]);
	return String();
}

ScreenEntity *project_bridge::room_a() {
	return room_auto::singleton()->a;
}
ScreenEntity *project_bridge::room_b() {
	return room_auto::singleton()->b;
}

void project_bridge::room_remove(ScreenEntity *obj) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(obj)) {
		const string &name = room->obj_name[obj];
		if (room->obj.count(name))
			room->obj.erase(name);
		room->obj_name.erase(obj); // "name" now invalid
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

Number project_bridge::getSpin(PhysicsScreen *screen, ScreenEntity *entity) {
	PhysicsScreenEntity *pe = screen->getPhysicsByScreenEntity(entity);
	return pe->body->GetAngularVelocity();
}

String project_bridge::saved_level() {
	return save.file;
}
void project_bridge::set_saved_level(int priority) {
	if (save.priority < priority) {
		save.priority = priority;
		save.file = room_auto::singleton()->filename;
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

SceneEcho::SceneEcho(Entity *_copying) : Entity(), copying(_copying) {}

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