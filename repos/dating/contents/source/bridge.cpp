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
#include "tinyxml.h"
#include <sstream>

extern "C" {	
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// Small lua-interface utilities

// Pull out a LUSERDATA pointer from the table at the given idx in the field with the given name.
static void *lightPointerFieldFromObject(lua_State *L, int idx, const char *name) {
	lua_getfield (L, idx, name);
	void *result = (void *)lua_topointer(L, -1);
	lua_pop(L, 1);
	return result;
}

// Pull out a USERDATA pointer (a handle) from the table at the given idx in the field with the given name.
static PolyBase *pointerFieldFromObject(lua_State *L, int idx, const char *name) {
	lua_getfield (L, idx, name);
	luaL_checktype(L, -1, LUA_TUSERDATA);
	PolyBase *result = *((PolyBase**)lua_touserdata(L, -1));
	lua_pop(L, 1);
	return result;
}

// Pull out the __ptr field of the table at the given idx.
static PolyBase *ptrFromObject(lua_State *L, int idx) {
	return pointerFieldFromObject(L, idx, "__ptr");
}

// Methods intended to be called from Lua

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

StringArray *project_bridge::room_onclick_objs() {
	room_auto *room = room_auto::singleton();
	StringArray *result = new StringArray();
	for(handler_iter i = room->onClick.begin(); i != room->onClick.end(); i++)
		result->contents.push_back(room->obj_name[i->first]);
	return result;
}

StringArray *project_bridge::room_oncollide_objs() {
	room_auto *room = room_auto::singleton();
	StringArray *result = new StringArray();
	for(handler_iter i = room->onCollide.begin(); i != room->onCollide.end(); i++)
		result->contents.push_back(room->obj_name[i->first]);
	return result;
}

Entity *project_bridge::room_id(String _at) {
	const string &at = _at.getSTLString();
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

void project_bridge::room_remove_screen(ScreenEntity *obj, bool doRemove) {
	room_auto *room = room_auto::singleton();
	if (room->obj_name.count(obj)) {
		const string &name = room->obj_name[obj];
		if (room->obj.count(name))
			room->obj.erase(name);
		room->obj_name.erase(obj); // "name" now invalid
	}
	room->onClick.erase(obj); // as are handlers
	room->onCollide.erase(obj);
	
	// We don't know for a fact it's in the screen, but over-removing is harmless
	if (doRemove && room->screen)
		room->screen->removeChild(obj);
}

void project_bridge::load_room(String from) {
	room_auto *room = room_auto::singleton();
	clear();
	if (room)
		room->die();
	(new room_auto(from.getSTLString()))->insert();
}

void project_bridge::load_room_txt(String from) {
	this->load_room(filedump(from));
}

Screen *project_bridge::standalone_screen(String svg, String objectPath, bool isPhysics) {
	Object overlay;
	room_auto *room = room_auto::singleton();
	if (objectPath.size()) {
		loadFromTree(overlay, objectPath.getSTLString());
	}
#if PHYSICS2D
	string loadClass = isPhysics ? "PhysicsScreen" : "Screen";
	Screen *into = isPhysics ? new PhysicsScreen() : new Screen();
#else
	string loadClass = "Screen";
	Screen *into = new Screen();
#endif
	
	room->loadSvgFromFile(overlay, svg.getSTLString(), into, loadClass, false);
	return into;
}

// Expectation is screen will be deleted right after
void project_bridge::room_remove_standalone_screen_all(Screen *s) {
#if 0
	// RNTODO
	for(int c = 0; c < s->getNumChildren(); c++) {
		room_remove_screen( s->getChild(c), false );
	}
#endif
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

String project_bridge::filedump_external(String _path) {
	_path = PROGDIR + _path;
	return ::filedump_external(_path.getSTLString());
}

void project_bridge::filedump_external_out(String _path, String data) {
	_path = PROGDIR + _path;
	FILE *f = fopen(_path.c_str(), "w");
	fwrite(data.c_str(), 1, data.size(), f);
	fclose(f);
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

void project_bridge::rebirth() {
	room_auto::singleton()->rebirth(false);
}

void project_bridge::Quit() {
	::Quit();
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

void project_bridge::setSceneClearColor(Scene *scene, int r, int g, int b, int a) {
	scene->useClearColor = true;
	scene->clearColor = Color(r,g,b,a);
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

int project_bridge::luaTestAddOne(lua_State *L) {
	int got = lua_tointeger(L, 2);
	lua_pop(L,1);
	ERR("HERE GOT %d\n", got);
	lua_pushinteger(L, got+1);
	return 1;
}

void pop_objectEntry(lua_State *L, ObjectEntry *o) {
	int ty = lua_type (L, -1);
	switch (ty) {
		case LUA_TNIL: case LUA_TLIGHTUSERDATA: case LUA_TFUNCTION: case LUA_TUSERDATA: case LUA_TTHREAD: default: 
			// TODO: WTF to do here? Let's just make an empty container.
			o->type = ObjectEntry::CONTAINER_ENTRY;
			break;
		case LUA_TBOOLEAN:
			o->type = ObjectEntry::BOOL_ENTRY;
			o->boolVal = lua_toboolean(L,-1);
			break;
		case LUA_TNUMBER:
			o->type = ObjectEntry::FLOAT_ENTRY; // TODO: Or int?
			o->NumberVal = lua_tonumber(L,-1);
			break;
		case LUA_TSTRING:
			o->type = ObjectEntry::STRING_ENTRY;
			o->stringVal = lua_tostring(L,-1);
			break;
		case LUA_TTABLE: {
			int t = lua_gettop(L);
			o->type = ObjectEntry::CONTAINER_ENTRY;
			
			lua_pushnil(L);  // first key
			while (lua_next(L, t) != 0) {
				// uses 'key' (at index -2) and 'value' (at index -1)
				int key_type = lua_type (L, -2);
				
				ObjectEntry *child = new ObjectEntry();
				if (key_type == LUA_TSTRING) {
					child->name = lua_tostring(L,-2);
				} else {
					// Parent has integer indices and therefore must be treated as array.
					o->type = ObjectEntry::ARRAY_ENTRY;
				}
				pop_objectEntry(L, child); // Will pop value, leave key for next call
				o->children.push_back(child);
				o->length++;
			}
		} break;
	}
	
	lua_pop(L, 1); // Pop item
}

static void push_objectentry(lua_State *L, ObjectEntry *o) {
//	ERR("%p has name %s type %d value %s\n", o, o->name.c_str(), (int)o->type, o->stringVal.c_str());
	if (!o) {
		lua_pushnil(L);
		return;
	}
	switch(o->type) {
		case ObjectEntry::UNKNOWN_ENTRY:
			lua_pushnil(L);
			break;
		case ObjectEntry::FLOAT_ENTRY:
			lua_pushnumber(L, o->NumberVal);
			break;
		case ObjectEntry::INT_ENTRY:
			lua_pushinteger(L, o->intVal);
			break;
		case ObjectEntry::BOOL_ENTRY:
			lua_pushboolean(L, o->boolVal);
			break;
		case ObjectEntry::STRING_ENTRY:
			lua_pushstring(L, o->stringVal.c_str());
			break;
		case ObjectEntry::ARRAY_ENTRY:
			lua_newtable(L);
			for(int c = 0; c < o->length; c++) { // Makes more sense to do "size"?
				lua_pushinteger(L, c+1);
				push_objectentry(L, o->children[c]);
				lua_settable(L, -3);
			}
			break;
		case ObjectEntry::CONTAINER_ENTRY:
			lua_newtable(L);
			for(int c = 0; c < o->children.size(); c++) {
				ObjectEntry *n = o->children[c];
				lua_pushstring(L, n->name.c_str());
				push_objectentry(L, n);
				lua_settable(L, -3);
			}
			break;
	}
}

// Arguments: ObjectEntry to save into, lua item to save 
// Returns: None
int project_bridge::saveTableIntoObject(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);
	ObjectEntry *o = (ObjectEntry*)ptrFromObject(L, 2);
	
	o->Clear();
	pop_objectEntry(L, o);
	
	return 0;
}

// Arguments: ObjectEntry to load from
// Returns: Lua equivalent
int project_bridge::loadTableFromObject(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);
	ObjectEntry *o = (ObjectEntry*)ptrFromObject(L, 2);
	
	push_objectentry(L, o);
	
	return 1;
}

// Arguments: File path to save into, root name, lua table to save
// Returns: None
int project_bridge::saveTableIntoFile(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
	luaL_checktype(L, 3, LUA_TSTRING);
	luaL_checktype(L, -1, LUA_TTABLE);
	
	// TODO: Check for empty filename, yield
	string filename = lua_tostring(L, 2);
	if (!filename.size())
		return 0;
	if (filename[0] != '/')
		filename = string(PROGDIR) + filename;
	Object o;
	o.root.name = lua_tostring(L, 3);
	
	pop_objectEntry(L, &o.root);
	o.saveToXML(filename);
	
	return 0;
}

static void pcall(lua_State *L, int nargs, int nresults) {
	if(lua_pcall(L, nargs, nresults, 0) != 0) {
		const char *msg = lua_tostring(L, -1);
		lua_getfield(L, LUA_GLOBALSINDEX, "__customError");
		lua_pushstring(L, msg);
		lua_call(L, 1, 0);
	}	
}

// Arguments: File path to load from, "external path?" bool
// Returns: Lua equivalent
int project_bridge::loadTableFromFile(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
//	luaL_checktype(L, 3, LUA_TBOOLEAN); // Don't bother, we can "cast to boolean"
	string filename;
	Object o;
	
	if (lua_toboolean(L, 3)) { // Check directory with exe
		string filename = lua_tostring(L, 2);
		if (!filename.size())
			return 0;
		if (filename[0] != '/')
			filename = string(PROGDIR) + filename;
		o.loadFromXMLString(::filedump_external(filename));
	} else { // Check PhysFS
		filename += lua_tostring(L, 2);
		o.loadFromXML(filename);
	}
	
	push_objectentry(L, &o.root);
	
	return 1;
}

// Arguments: root name, lua table to save
// Returns: XML equivalent
int project_bridge::saveTableIntoXml(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
	luaL_checktype(L, -1, LUA_TTABLE);
	Object o;
	o.root.name = lua_tostring(L, 2);
	
	pop_objectEntry(L, &o.root);
	
	// Nothing for this in Object, so I'll build it myself.
	TiXmlDocument doc;  	
	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );  
	doc.LinkEndChild( decl ); 
	
	TiXmlElement * rootElement = o.createElementFromObjectEntry(&o.root);
	doc.LinkEndChild(rootElement);
	
	TiXmlPrinter printer;
	printer.SetStreamPrinting();
	doc.Accept(&printer);
	lua_pushstring(L, printer.CStr());
	
	return 1;
}

// Arguments: XML string
// Returns: Lua equivalent
int project_bridge::loadTableFromXml(lua_State *L) {
	luaL_checktype(L, 2, LUA_TSTRING);
	string content = lua_tostring(L, 2);
	Object o;
	
	o.loadFromXMLString(content);
	push_objectentry(L, &o.root);
	
	return 1;
}

// Arguments: Screen, x coordinate, y coordinate
int project_bridge::getChildAtScreenPosition(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_checktype(L, 3, LUA_TNUMBER);
	luaL_checktype(L, 4, LUA_TNUMBER);

	Screen *s = (Screen *)ptrFromObject(L, 2);
	Number x = lua_tonumber(L,3);
	Number y = lua_tonumber(L,4);
	
	lua_newtable(L);
	
#if 0
	// RNTODO
	int target = lua_gettop(L);
	
	int pushi = 1;
	for(int i=0; i < s->getNumChildren(); i++) {
		ScreenEntity *child = s->getChild(i);
		if(child->hitTest(x,y)) {
			lua_getfield(L, LUA_GLOBALSINDEX, "__ptrToTable");
			lua_pushstring(L, "ScreenEntity");
			lua_pushlightuserdata(L, child);
			pcall(L,2,1); // FIXME: pcall or raw call? Or just yield? We're technically inside a lua call at this point.
			
			lua_rawseti(L, target, pushi);
			pushi++;
		}
	}	
#endif
	
	return 1;
}

bool project_bridge::getVisible(Entity *e) {
	return e->visible;
}

void project_bridge::setVisible(Entity *e, bool visible) {
	e->visible = visible;
}

CoreServices *project_bridge::coreServices() {
	return CoreServices::getInstance();
}

int project_bridge::userSettings(lua_State *L) {
	push_objectentry(L, ::userSettings);

	return 1;
}

int project_bridge::debugSettings(lua_State *L) {
	push_objectentry(L, ::debugSettings);
	
	return 1;
}

void project_bridge::register_room_name(SceneEntity *of, String _name) {
	room_auto *room = room_auto::singleton();
	const string &name = _name.getSTLString();
	room->obj[name] = of;
	room->obj_name[of] = name;
}

// Arguments: SceneEntity, function
// Returns: none
int project_bridge::register_room_onCollision(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_checktype(L, 3, LUA_TFUNCTION);
	void *of = ptrFromObject(L, 2);
	storedfunc *f = terminal_auto::singleton()->funcFromStack(3);
	room_auto *room = room_auto::singleton();
	if (room->onCollide.count(of)) {
		delete room->onCollide[of];
	}
	room->onCollide[of] = f;
	return 0;
}

int project_bridge::register_room_onClick(lua_State *L) { // Factor this and above function into common function?
	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_checktype(L, 3, LUA_TFUNCTION);
	void *of = ptrFromObject(L, 2);
	storedfunc *f = terminal_auto::singleton()->funcFromStack(3);
	room_auto *room = room_auto::singleton();
	if (room->onClick.count(of)) {
		delete room->onClick[of];
	}
	room->onClick[of] = f;
	return 0;
}

String project_bridge::pwd() {
	return PROGDIR;
}

void project_bridge::setColorObj(Entity *e, Color *c) {
	e->setColor(*c);
}

SceneMesh *project_bridge::normalPlane(Number w, Number h, Number wscale, Number hscale, bool no_backface) {
	SceneMesh *e = new SceneMesh(Mesh::QUAD_MESH);
	Mesh *m = e->getMesh();
	
	SceneMesh *e2 = new SceneMesh(Mesh::QUAD_MESH); 
	Mesh *m2 = e2->getMesh();
	m2->createBox(w,h,1);
	
	Polycode::Polygon *imagePolygon = new Polycode::Polygon();
	imagePolygon->addVertex(0,h,0, 0     , hscale);
	imagePolygon->addVertex(w,h,0, wscale, hscale);
	imagePolygon->addVertex(w,0,0, wscale, 0);
	imagePolygon->addVertex(0,0,0, 0     , 0);
	m->addPolygon(imagePolygon);
	
	if (!no_backface) {
		imagePolygon = new Polycode::Polygon();
		imagePolygon->addVertex(0,0,0, wscale, 0);
		imagePolygon->addVertex(w,0,0, 0     , 0);
		imagePolygon->addVertex(w,h,0, 0     , hscale);
		imagePolygon->addVertex(0,h,0, wscale, hscale);
		m->addPolygon(imagePolygon);
	}
	
	for(int i=0; i < m->getPolygonCount(); i++) {
		for(int j=0; j < m->getPolygon(i)->getVertexCount(); j++) {
			Vertex *v = m->getPolygon(i)->getVertex(j);
			v->x = v->x - (w/2.0f);
			v->y = v->y - (h/2.0f);
			v->normal = m2->getPolygon(2)->getVertex(j)->normal; // Act like side #3 of a cube
		}
	}
	
	//	m->calculateNormals();
	m->calculateTangents();
	m->arrayDirtyMap[RenderDataArray::VERTEX_DATA_ARRAY] = true;
	m->arrayDirtyMap[RenderDataArray::COLOR_DATA_ARRAY] = true;
	m->arrayDirtyMap[RenderDataArray::TEXCOORD_DATA_ARRAY] = true;
	m->arrayDirtyMap[RenderDataArray::NORMAL_DATA_ARRAY] = true;
	m->arrayDirtyMap[RenderDataArray::TANGENT_DATA_ARRAY] = true;
	
	e->bBox.x = w;
	e->bBox.y = h;
	e->bBox.z = 0.1;
	
	delete e2;
	
	return e;
}

SceneMesh *project_bridge::uprightBox(Number w, Number d, Number h, bool stretch) {
	SceneMesh *e = new SceneMesh(Mesh::QUAD_MESH);
	Mesh *m = e->getMesh();
	
	Number tw, td, th;
	if (stretch) {
		tw = 1; td = 1; th = 1;
	} else {
		tw = w; td = d; th = h;
	}
	
	Polycode::Polygon *polygon = new Polycode::Polygon(); // "Bottom"
	polygon->addVertex(w,0,h, 0, tw);
	polygon->addVertex(0,0,h, 0, 0);
	polygon->addVertex(0,0,0, th, 0);
	polygon->addVertex(w,0,0, th, tw);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon(); // "Top"
	polygon->addVertex(w,d,h, tw, 0);
	polygon->addVertex(w,d,0, tw, th);
	polygon->addVertex(0,d,0, 0, th);
	polygon->addVertex(0,d,h, 0, 0);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon(); // Visible when facing Z+
	polygon->addVertex(0,d,0, tw, td);
	polygon->addVertex(w,d,0, 0, td);
	polygon->addVertex(w,0,0, 0, 0);
	polygon->addVertex(0,0,0, tw, 0);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon(); // Visible when facing Z-
	polygon->addVertex(0,0,h, 0, 0);
	polygon->addVertex(w,0,h, tw, 0);
	polygon->addVertex(w,d,h, tw, td);
	polygon->addVertex(0,d,h, 0, td);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon(); // Visible when facing X+
	polygon->addVertex(0,0,h, td, 0);
	polygon->addVertex(0,d,h, td, th);
	polygon->addVertex(0,d,0, 0, th);
	polygon->addVertex(0,0,0, 0, 0);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon(); // Visible when facing X-
	polygon->addVertex(w,0,h, 0, 0);
	polygon->addVertex(w,0,0, th, 0);
	polygon->addVertex(w,d,0, th, td);
	polygon->addVertex(w,d,h, 0, td);
	m->addPolygon(polygon);
	
	for(int i=0; i < m->getPolygonCount(); i++) {
		for(int j=0; j < m->getPolygon(i)->getVertexCount(); j++) {
			m->getPolygon(i)->getVertex(j)->x = m->getPolygon(i)->getVertex(j)->x - (w/2.0f);
			m->getPolygon(i)->getVertex(j)->y = m->getPolygon(i)->getVertex(j)->y - (d/2.0f);
			m->getPolygon(i)->getVertex(j)->z = m->getPolygon(i)->getVertex(j)->z - (h/2.0f);	
		}
	}
	
	m->calculateNormals();
	m->calculateTangents();
	m->arrayDirtyMap[RenderDataArray::VERTEX_DATA_ARRAY] = true;		
	m->arrayDirtyMap[RenderDataArray::COLOR_DATA_ARRAY] = true;				
	m->arrayDirtyMap[RenderDataArray::TEXCOORD_DATA_ARRAY] = true;						
	m->arrayDirtyMap[RenderDataArray::NORMAL_DATA_ARRAY] = true;		
	m->arrayDirtyMap[RenderDataArray::TANGENT_DATA_ARRAY] = true;									
	
	e->bBox.x = w;
	e->bBox.y = d;
	e->bBox.z = h;
	return e;
}

void project_bridge::term_setOverride(bool v) { terminal_auto::singleton()->setOverride(v); }
void project_bridge::term_setHidden(bool v) { terminal_auto::singleton()->minireset(); terminal_auto::singleton()->set_hidden(v); }
void project_bridge::term_setEntry(String str) { terminal_auto::singleton()->setEntry(str.getSTLString()); }
void project_bridge::term_setLine(int y, String str) { terminal_auto::singleton()->setLine(y, str.getSTLString()); }
void project_bridge::term_setColor(int y, Color *c) { terminal_auto::singleton()->setColor(y, *c); }
void project_bridge::term_reset() { terminal_auto::singleton()->minireset(); }
int project_bridge::term_height() { return terminal_auto::singleton()->max_height; }
int project_bridge::term_width() { return terminal_auto::singleton()->max_width; }
bool project_bridge::term_busy() { return !terminal_auto::singleton()->hidden && !terminal_auto::singleton()->override; }

String project_bridge::editor_dir() {
#if SELF_EDIT
	extern string selfedit_dir;
	return selfedit_dir + "/media";
#else
	return "mod/media"; // PROGDIR will be added automatically
#endif
}

Number project_bridge::labelWidth(ScreenLabel *label) {
	return label->getLabel()->getTextWidth();
}
Number project_bridge::labelHeight(ScreenLabel *label) {
	return label->getLabel()->getTextHeight();
}

void project_bridge::paramSetNumber(LocalShaderParam *param, Number x) { memcpy(param->data, &x, sizeof(x)); }
void project_bridge::paramSetVector2(LocalShaderParam *param, Vector2 x) { memcpy(param->data, &x, sizeof(x)); }
void project_bridge::paramSetVector3(LocalShaderParam *param, Vector3 x) { memcpy(param->data, &x, sizeof(x)); }
void project_bridge::paramSetColor(LocalShaderParam *param, Color x) { memcpy(param->data, &x, sizeof(x)); }



// Pull a ScreenEntity out; shove it back in.
void project_bridge::stackthrash(Screen *s, ScreenEntity *e) {
//	bool ownsChildren = s->ownsChildren; // RNTODO
//	s->ownsChildren = false;
	s->removeChild(e);
	s->addChild(e);
//	s->ownsChildren = ownsChildren;
}

int project_bridge::getTextHeight(Label *l) {
	return l->getTextHeight();
}

ScreenEcho::ScreenEcho(Entity *_copying) : ScreenEntity(), copying(_copying) {
	bBox.x = 1;
	bBox.y = 1;
	bBox.z = 1;		
}

void ScreenEcho::transformAndRender() {
	Entity *oldCopyParent = copying->getParentEntity();
	if (oldCopyParent)
		oldCopyParent->removeChild(copying);
	this->addChild(copying);
	Entity::transformAndRender();
	this->removeChild(copying);
	if (oldCopyParent)
		oldCopyParent->addChild(copying);
	else
		copying->setParentEntity(NULL);
}

Entity *ScreenEcho::getEntity() { return copying; }

// Arguments: Polycode bound object
// Returns: Underlying pointer / unique identifier
int project_bridge::bindingId(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);

	PolyBase *ptr = ptrFromObject(L, 2);
	
	lua_pushlightuserdata(L, ptr);
	
	return 1;
}

// Arguments: Polycode bound object
// Returns: nothing
int project_bridge::unlinkgc(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTABLE);
	
	lua_getfield (L, 2, "__ptr");
	luaL_checktype(L, -1, LUA_TUSERDATA);
	PolyBase **result = ((PolyBase**)lua_touserdata(L, -1));
	*result = NULL;
	lua_pop(L, 1);
	
	return 0;
}

// Arguments: gfxcontainer, x, y, dx, dy
// Returns: any opaque, all opaque
int project_bridge::gfxscan(lua_State *L) {
	gfxcontainer *g = (gfxcontainer *)ptrFromObject(L, 2);
	int bx = lua_tonumber(L,3);
	int by = lua_tonumber(L,4);
	int dx = lua_tonumber(L,5);
	int dy = lua_tonumber(L,6);
	
	bool allopq = true, anyopq = false;
	
	for(int y = 0; y < dy; y++) {
		for(int x = 0; x < dx; x++) {
			int srcx = bx + x, srcy = by + y;
			unsigned char *src = (unsigned char *)&g->px(srcx,srcy);
			unsigned char a = src[3];
			if (a > 0)  anyopq = true;
			if (a == 0) allopq = false;
		}
	}
	
	lua_pushboolean(L, anyopq);
	lua_pushboolean(L, allopq);
	return 2;
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

void project_bridge::Audio1() {
	PaError err;
	err = Pa_Initialize();
	if (err != paNoError) ERR("INIT FAIL %d = %s\n", (int)err,  Pa_GetErrorText( err ));

}

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData )
{
     float *out = (float*)outputBuffer;
     unsigned long i;
 
     (void) timeInfo; /* Prevent unused variable warnings. */
     (void) statusFlags;
     (void) inputBuffer;
     
     for( i=0; i<framesPerBuffer; i++ )
     {
         *out++ = random()/float(RAND_MAX);  /* left */
         *out++ = random()/float(RAND_MAX);  /* right */
     }
     
		ERR("DEF\n");
	
     return paContinue;
}

void project_bridge::Audio2() {
	PaError err;
	PaStream *stream;
	PaStreamParameters outputParameters;
	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
     if (outputParameters.device == paNoDevice) {
       ERR("Error: No default output device.\n");
		 return;
     }
     outputParameters.channelCount = 2;       /* stereo output */
     outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
     outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
     outputParameters.hostApiSpecificStreamInfo = NULL;
 
     err = Pa_OpenStream(
               &stream,
               NULL, /* no input */
               &outputParameters,
               SAMPLE_RATE,
               FRAMES_PER_BUFFER,
               paClipOff,      /* we won't output out of range samples so don't bother clipping them */
               patestCallback,
               this );
     if (err != paNoError) ERR("INIT FAIL %d = %s\n", (int)err,  Pa_GetErrorText( err ));
	ERR("ABC\n");
	err = Pa_StartStream( stream );
	if (err != paNoError) ERR("START FAIL %d = %s\n", (int)err,  Pa_GetErrorText( err ));
}
void project_bridge::Audio3() {
}


#define AUDIODUMP 0

#if AUDIODUMP
FILE *audiolog = NULL; // If you find yourself needing to debug your audio, fopen() this somewhere and all audio will start getting copied to this file as 16-bit pcm
#endif

#if AUDIODUMP
#define DUMP_QUEUE 2048
static short dumpqueue[DUMP_QUEUE];
static int dumpat = 0;
void debug_audio_init() {
	if (!audiolog) audiolog = fopen("/tmp/OUTAUDIO", "w");
}
inline void debug_dump(float v) {
	short s = ::max(::min(v,1.0f),-1.0f)*SHRT_MAX;
	dumpqueue[dumpat++] = s;
	
	if (dumpat >= DUMP_QUEUE && audiolog) { // For debugging audio
		fwrite(dumpqueue, sizeof(short), DUMP_QUEUE, audiolog);
		dumpat = 0;
	}
}
#else
#define debug_audio_init(...)
#define debug_dump(...)
#endif

BSound::BSound() : PSound(), pitch(440), volume(1) {
	for(int c = 0; c < BPARAMCOUNT; c++) {
		param[c] = 0;
	}
	for(int c = 0; c < BLAYERS; c++) {
		theta[c] = 0;
		thetaoff[c] = 0;
		on[c] = false;
	}
	ptheta = 0;
	debug_audio_init();
}

Number BSound::getPitch() {
	return pitch;
}
	
void BSound::setPitch(Number newPitch) {
	pitch = newPitch;
	double ta = pitch / 44100.0 * M_PI*2;
	double offs = pow( pow(2, 1/12.0), 4 );
	for(int c = 0; c < BLAYERS; c++) {
		thetaoff[c] = ta;
		ta = ta * offs;
	}
}

Number BSound::getVolume() {
	return volume;
}

void BSound::setVolume(Number newVolume) {
	volume = newVolume;
}

Number BSound::getParam(int idx) {
	return param[idx];
}

void BSound::setParam(int idx, Number v) {
	param[idx] = v;
}

#define BPPERIOD (param[0])
#define BPINTENS (param[1])

int BSound::soundCallback( const void *, void *outputBuffer,
						  unsigned long framesPerBuffer,
						  const PaStreamCallbackTimeInfo*,
						  PaStreamCallbackFlags)
{
	float *out = (float*)outputBuffer;
	
	for(unsigned long i=0; i<framesPerBuffer; i++)
	{
		double v = 0;
		
		ptheta++;
		if (ptheta > BPPERIOD) {
			for(int c = 0; c < BPINTENS; c++) {
				bool &target = on[random()%BLAYERS];
				target = !target;
			}
			ptheta = 0;
		}
		
		for(int c = 0; c < BLAYERS; c++) {
			if (on[c])
				v += sin(theta[c])*volume;
			theta[c] += thetaoff[c];
		}
		debug_dump(v);
		
		*out++ = v;  /* left */
		*out++ = v;  /* right */
	}
	
	return paContinue;
}


WhiteSound::WhiteSound() : PSound(), pitch(440), volume(1) {
	ptheta = 0; last = 0;
	debug_audio_init();
}

Number WhiteSound::getPitch() {
	return pitch;
}

void WhiteSound::setPitch(Number newPitch) {
	pitch = newPitch;
}

Number WhiteSound::getVolume() {
	return volume;
}

void WhiteSound::setVolume(Number newVolume) {
	volume = newVolume;
}

int WhiteSound::soundCallback( const void *, void *outputBuffer,
						  unsigned long framesPerBuffer,
						  const PaStreamCallbackTimeInfo*,
						  PaStreamCallbackFlags)
{
	float *out = (float*)outputBuffer;
	
	for(unsigned long i=0; i<framesPerBuffer; i++)
	{		
		ptheta++;
		if (ptheta > pitch) {
			last = (float(random())/RANDOM_MAX * 2 - 1) * volume ;
			ptheta = 0;
		}
		
		debug_dump(v);
		
		*out++ = last;  /* left */
		*out++ = last;  /* right */
	}
	
	return paContinue;
}