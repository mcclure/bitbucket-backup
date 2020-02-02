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
#include "physfs.h"

#if 0 // Hyperverbal
#define HYPERERR ERR
#else
#define HYPERERR EAT
#endif

// Halt on failure?
#if _DEBUG
#define BAIL 1
#else
#define BAIL 0
#endif

#define ONCLICK_ON_DOWN 0
#define ONCLICK_ON_UP 1

room_auto *room_auto::_singleton = NULL;

class roomloader : public svgloader {
protected:
	room_auto *owner;
	Object *overlay;
	const char *Attribute(TiXmlElement *xml, string attr);
	int QueryIntAttribute(TiXmlElement *xml, string attr, int &value);
	int QueryDoubleAttribute(TiXmlElement *xml, string attr, double &value);
	
	virtual bool loadRootXml(TiXmlElement *xml, const svgtransform &parent_transform);
	virtual bool localAddChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform);
	virtual bool addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform);
	virtual bool addStoredFunc(hash_map<void *,storedfunc *> &lookup, void *shape, TiXmlElement *xml, const char *attrName);
public:
	roomloader(room_auto *_owner, Object *_overlay = NULL, Screen *__screen = NULL) : svgloader(__screen), owner(_owner), overlay(_overlay) {}
};

#if PHYSICS2D
class physics_svgloader : public roomloader {
protected:
	virtual PhysicsScreen *createScreen();
	virtual bool addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform);
public:
	physics_svgloader(room_auto *_owner, Object *_overlay = NULL, PhysicsScreen *__screen = NULL);
	PhysicsScreen *physics() { return (PhysicsScreen *)screen(); }	
};
#endif

room_auto::room_auto(const string &_spec, bool _fake) : automaton(), screen(NULL), scene(NULL), spec(_spec), fake(_fake), bailed(false), clickHandler(false), a(NULL), b(NULL) {
}

room_auto::~room_auto() {
	clear_stored_functions();
	if (_singleton == this)
		_singleton = NULL;
}

void room_auto::bail() {
	bailed = true;
	clear_stored_functions();
}

void room_auto::add_update_function(const string &text) {
	storedfunc *function = terminal_auto::singleton()->compile(text);
	if (function)
		onUpdate.push_back( function );
	else if (BAIL)
		bail();
}

void room_auto::clear_stored_functions() {
	for(handler_iter i = onCollide.begin(); i != onCollide.end(); i++)
		delete i->second;
	onCollide.clear();
	
	for(handler_iter i = onClick.begin(); i != onClick.end(); i++)
		delete i->second;
	onClick.clear();
	
	for(int c = 0; c < onUpdate.size(); c++)
		delete onUpdate[c];
	onUpdate.clear();
}

void room_auto::insert() {
	automaton::insert();
	cor->getInput()->addEventListener(this, InputEvent::EVENT_KEYDOWN); // Handle esc
	_singleton = this;
	
	if (fake) return;
		
	Object overlay;
	
	// Iterate over spec string, "split" on commas
	int file_start = -1;
	for(int c = 0; c <= spec.size(); c++) { // Notice: spec.size inclusive
		if (c == spec.size() || spec[c] == '\n' || spec[c] == '\r' || spec[c] == ',') {
			if (file_start >= 0) {
				string filename = spec.substr(file_start, c-file_start);
				loadFromFile(overlay, filename); // This is the only important part
				file_start = -1;
			}
		} else {
			if (file_start < 0)
				file_start = c;
		}
	}
	if (bailed) // Make sure we didn't bail, then accidentally add more functions
		clear_stored_functions();

	Clear(overlay); // Free memory
	
	if (!onLoad.empty()) {
		terminal_auto::singleton()->inject_global("global_ticks", ticks);
		terminal_auto::singleton()->inject_global("ticks", frame);
		for(int c = 0; c < onLoad.size(); c++) {
			bool success = terminal_auto::singleton()->execute(onLoad[c]);
			if (BAIL && !success) {
				bail();
				break;
			}
		}
		onLoad.clear(); // Will never be used again (valid assumption?)
	}
}

void room_auto::die() {
	if (done) return;
	
	for(int c = 0; c < onClose.size(); c++) {
		terminal_auto::singleton()->execute(onClose[c]); // Don't bother clearing, we're dying
	}	
	automaton::die();
	
	cor->getInput()->removeAllHandlers(); // Remove EVERYTHING, not just our listeners, in case Lua did something
	terminal_auto::singleton()->setup_events(); // Let terminal_auto restore its own listeners
}

void room_auto::tick() {
	if (done) return;
	automaton::tick();
	
	terminal_auto::singleton()->inject_global("global_ticks", ticks);
	terminal_auto::singleton()->inject_global("ticks", frame);
	
	for(int c = 0; c < onUpdate.size(); c++) {
		bool success = onUpdate[c]->execute();
		if (BAIL && !success) {
			bail();
			break;
		}
	}
}

void room_auto::rebirth(bool fake) {
	CoreServices::getInstance()->getMaterialManager()->reloadProgramsAndTextures();
	
	die();
	(new room_auto(spec, fake))->insert();
}

void room_auto::handleEvent(Event *e) {
	if (done) return;
	if(e->getDispatcher() == cor->getInput()) {
		InputEvent *inputEvent = (InputEvent*)e;
		bool down = false;
		
		switch(e->getEventCode()) {
			case InputEvent::EVENT_KEYDOWN: {
				int code = inputEvent->keyCode();
				if (!IS_MODIFIER(code)) {
					if (code == KEY_ESCAPE) {
						if (SELF_EDIT || debugMode)
							rebirth();
						else
							Quit();
					}
				}
			} break;
			case InputEvent::EVENT_MOUSEDOWN: down = true; // Handle onClick
			case InputEvent::EVENT_MOUSEUP: {
				int mouseButton = inputEvent->getMouseButton();
				Vector2 pos = inputEvent->getMousePosition();
				
				if (down)
					clickTrack.clear();

				for(handler_iter i = onClick.begin(); i != onClick.end(); i++) {
					ScreenEntity *child = (ScreenEntity *)i->first;
					if (child->hitTest(pos.x,pos.y)) {
						if (down)
							clickTrack[child] = child;
						else if (!clickTrack.count(child)) // Skip ups that weren't downs
							continue;
						
						// To simplify things, maybe some programs only ever care about one?
						if ((!ONCLICK_ON_DOWN && down) || (!ONCLICK_ON_UP && !down))
							continue;
						
						terminal_auto::singleton()->inject_global("oc_down", down);
						terminal_auto::singleton()->inject_global("oc_x", pos.x);
						terminal_auto::singleton()->inject_global("oc_y", pos.y);
						
						bool success = i->second->execute();
						if (BAIL && !success) {
							bail();				
							break;
						}
					}
				}
				
				if (!down)
					clickTrack.clear();
			} break;
		}
		
	} else if (e->getDispatcher() == screen) {
#if PHYSICS2D
		PhysicsScreenEvent *pe = (PhysicsScreenEvent*)e;
		switch(e->getEventCode()) {
			case PhysicsScreenEvent::EVENT_NEW_SHAPE_COLLISION: {
				if (onCollide.count(pe->entity1)) {
					terminal_auto::singleton()->inject_global("impact", pe->impactStrength);
					doCollide(pe->entity1, pe->entity2, true);
				}
				if (onCollide.count(pe->entity2)) {
					terminal_auto::singleton()->inject_global("impact", pe->impactStrength);
					doCollide(pe->entity2, pe->entity1, false);
				}
			}
		}
#endif
	} else if (e->getDispatcher() == scene) {
#if PHYSICS3D
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
#endif
	}
}

void room_auto::needClickHandler() {
	if (!clickHandler) {
		cor->getInput()->addEventListener(this, InputEvent::EVENT_MOUSEDOWN);
		cor->getInput()->addEventListener(this, InputEvent::EVENT_MOUSEUP);
		clickHandler = true;
	}
}

void room_auto::doCollide(Entity *acting, Entity *against, bool) { // Assumes count > 0
	a = acting; b = against;
	bool success = onCollide[acting]->execute();
	if (BAIL && !success)
		bail();
	a = NULL; b = NULL;
}

void room_auto::loadFromFile(Object &overlay, const string &filename) {
	if (endsWith(filename, ".xml")) {
		overlay.loadFromXML(String(filename)); // TODO: Clears; shouldn't.
	} else if (endsWith(filename, ".svg")) {
		screen_has_class("Screen"); // Guarantee SOME screen
		loadSvgFromFile(overlay, filename, screen, screenClass);
	} else if (PHYSFS_isDirectory(filename.c_str())) {
		loadFromTree(overlay, filename); // TODO: Clears; shouldn't.
		loadFromOverlay(overlay);
	} else {
		ERR("ERROR: Don't know how to load '%s\n", filename.c_str());
	}
}

void room_auto::loadSvgFromFile(Object &overlay, const string &filename, Screen *into, const string &loadClass, bool readBackground) {
	roomloader *svg;
	if (loadClass == "PhysicsScreen") {
#if PHYSICS2D
		svg = new physics_svgloader(this, &overlay, (PhysicsScreen *)into);
#endif										
	} else {
		svg = new roomloader(this, &overlay, into);
	}
	svg->readBackground = readBackground;
	svg->load(filename);
	delete svg;	
}

void room_auto::loadFromOverlay(Object &overlay) {
	ObjectEntry *temp;
	
	if (temp = overlay.root["onLoad"]) {
		onLoad.push_back( temp->stringVal.getSTLString() );
	}
	if (temp = overlay.root["onUpdate"]) {
		add_update_function( temp->stringVal.getSTLString() );
	}
	if (temp = overlay.root["onClose"]) {
		onClose.push_back( temp->stringVal.getSTLString() );
	}
	
	if (temp = overlay.root["screen"]) {
		screen_has_class(temp->stringVal.getSTLString());
	}
	if (temp = overlay.root["scene"]) {
		scene_has_class(temp->stringVal.getSTLString());
	}
}

inline bool is_whitespace(const char &c) {
	return c == ' ' || c == '\t' || c  == '\n' || c == '\r';
}

void strip_whitespace(string &target) { // In python this would just be name = name.strip()
	int leading,trailing; 
	for(leading = 0; leading < target.size() && is_whitespace(target[leading]); leading++);
	for(trailing = 0; trailing < target.size() && is_whitespace(target[target.size()-trailing-1]); trailing++);
	if (leading || trailing)
		target = target.substr(leading, target.size()-leading-trailing);
}

void room_auto::screen_has_class(string name) {
	if (screen) return;
	strip_whitespace(name);
	
	if (name == "Screen") {
		screen = new Screen();
#if PHYSICS2D
	} else if (name == "PhysicsScreen") {
		screen = new PhysicsScreen();
		screen->addEventListener(this, PhysicsScreenEvent::EVENT_NEW_SHAPE_COLLISION);
#endif
	}
	
	if (screen) {
		screen->ownsChildren = true;
		owned_screen.push_back(screen);
		screenClass = name;
	}
}

void room_auto::scene_has_class(string name) {
	if (scene) return;
	strip_whitespace(name);
	
	if (name == "Scene") {
		scene = new Scene();
#if PHYSICS3D
	} else {
		if (name == "CollisionScene")
			scene = new CollisionScene();
		else if (name == "PhysicsScene")
			scene = new PhysicsScene();
		
		scene->addEventListener(this, PhysicsSceneEvent::COLLISION_EVENT);
#endif
	}
	
	if (scene) {
		scene->ownsChildren = true;
		owned_screen.push_back(scene);
		sceneClass = name;
	}
}

// I sure do wish I had a way to read into a tree of dictionary ObjectEntries without all these ifs.
const char *roomloader::Attribute(TiXmlElement *xml, string attr) {
	string xmlAttr = "polycode:" + attr;
	const char *result = xml->Attribute(xmlAttr.c_str());
	if (!result && overlay) { // XML takes precedence over overlay
		ObjectEntry *co = overlay->root["entity"];
		const char *id = xml->Attribute("id");
		if (co && id) {
			ObjectEntry *eo = (*co)[id];
			if (eo) {
				ObjectEntry *vo = (*eo)[attr];
				if (vo) {
					result = vo->stringVal.c_str();
				}
			}
		}
	}
	return result;
}
int roomloader::QueryIntAttribute(TiXmlElement *xml, string attr, int &value) {
	return TIXML_NO_ATTRIBUTE; 	// TODO
}
int roomloader::QueryDoubleAttribute(TiXmlElement *xml, string attr, double &value) {
	return TIXML_NO_ATTRIBUTE; 	// TODO
}

bool roomloader::loadRootXml(TiXmlElement *xml, const svgtransform &parent_transform) {
	const char *temp = xml->Attribute("polycode:onLoad");
	if (temp)
		owner->onLoad.push_back(temp);
	
	temp = xml->Attribute("polycode:onUpdate");
	if (temp)
		owner->add_update_function( temp );
	
	temp = xml->Attribute("polycode:onClose");
	if (temp)
		owner->onClose.push_back(temp);
	
	return svgloader::loadRootXml(xml, parent_transform);
}

bool roomloader::addStoredFunc(hash_map<void *,storedfunc *> &lookup, void *shape, TiXmlElement *xml, const char *attrName) {
	const char *text = Attribute(xml, attrName);
	storedfunc *func = text ? terminal_auto::singleton()->compile(text) : NULL;
	if (func)
		lookup[shape] = func;
	return func;
}

bool roomloader::localAddChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform) {	
	addStoredFunc(owner->onCollide, shape, xml, "onCollide");

	if (addStoredFunc(owner->onClick, shape, xml, "onClick"))
		owner->needClickHandler();
	
	int isStatic = true;
	xml->QueryIntAttribute("polycode:isStatic", &isStatic);	
	const char *id = xml->Attribute("id");
	if (id) {
		if (!PHYSICS2D || !isStatic || string("text") == xml->Value()) {
			owner->obj[id] = shape;
		}
		owner->obj_name[shape] = id;
	}
	
	int isVisible;
	if (TIXML_SUCCESS == xml->QueryIntAttribute("polycode:isVisible", &isVisible))
		shape->visible = isVisible;
	
	return true;
}

bool roomloader::addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform) {
	svgloader::addChild(shape, xml, kind, transform);

	localAddChild(shape, xml, kind, transform);
	
	return true;
}

#if PHYSICS2D
// TODO: Maybe should return a Scene not a Screen
physics_svgloader::physics_svgloader(room_auto *_owner, Object *_overlay, PhysicsScreen *__screen) : roomloader(_owner, _overlay, __screen) {}

PhysicsScreen *physics_svgloader::createScreen() {
	return new PhysicsScreen();
}

bool physics_svgloader::addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform)
{
	int physicsType = 0;
	int isFloater = 0;
	xml->QueryIntAttribute("polycode:isFloater", &isFloater);
	if (isFloater) kind = svg_floater;
	
	switch (kind) {
		case svg_rect:		physicsType = PhysicsScreenEntity::ENTITY_RECT; break;
		case svg_circle:	physicsType = PhysicsScreenEntity::ENTITY_CIRCLE; break;
		case svg_mesh:		physicsType = PhysicsScreenEntity::ENTITY_MESH; break;
		case svg_floater:	roomloader::addChild(shape,xml,kind,transform); return true;
		default: return false;
	}
	int isStatic = true;
	double friction = 0.25;
	double density = 1;
	double restitution = 0.25;
	int fixedRotation = false;
	int isSensor = false;
	
	xml->QueryIntAttribute("polycode:isStatic", &isStatic);
	xml->QueryDoubleAttribute("polycode:friction", &friction);
	xml->QueryDoubleAttribute("polycode:density", &density);
	xml->QueryDoubleAttribute("polycode:restitution", &restitution);
	xml->QueryIntAttribute("polycode:fixedRotation", &fixedRotation);
	xml->QueryIntAttribute("polycode:isSensor", &isSensor);
	
	physics()->addPhysicsChild(shape, physicsType, isStatic, friction, density, restitution, isSensor, fixedRotation);
	
	roomloader::localAddChild(shape, xml, kind, transform); // Add information but DON'T call screen->addChild
	
	return true;
}
#endif

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
	if (path.empty()) // An empty string is almost certainly a mistake.
		return;
	
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
		if (!efile) {
			o.type = ObjectEntry::UNKNOWN_ENTRY; // Best practice?
			ERR("ERROR: PhysFS failed to open file: %s\n", cpath);
			return;
		}
		
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

bool endsWith(const string &str, const string &suffix) {
	int sufsize = suffix.size();
	int strsize = str.size();
	if (strsize < sufsize) return false;
	return str.substr(strsize-sufsize) == suffix;
}

#define CHUNKSIZE 1024

string filedump(const string &path) {
	PHYSFS_file *f = PHYSFS_openRead(path.c_str());
	if (f) {
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

string filedrain(FILE *f) {
	if (f) {
		string result;
		char chunk[CHUNKSIZE+1];
		int chunkread = 0;
		do {
			chunkread = fread(chunk, 1, CHUNKSIZE, f);
			if (chunkread < 0)
				chunkread = 0;
			chunk[chunkread] = '\0';
			result += chunk;
		} while (chunkread >= CHUNKSIZE);
		fclose(f);
		return result;
	}
	return string();
}

// If you absolutely MUST avoid physfs
string filedump_external(const string &path) {
	FILE *f = fopen(path.c_str(), "r");
	return filedrain(f);
}

int intCheck(ObjectEntry *o, const string &key, int def) {
	o = objCheck(o, key);
	if (o && (o->type == ObjectEntry::INT_ENTRY || o->type == ObjectEntry::BOOL_ENTRY || o->type == ObjectEntry::FLOAT_ENTRY))
		return o->intVal;
	return def;
}
		
int intCheckPure(ObjectEntry *o, const string &key, int def) {
	o = objCheck(o, key);
	if (o && (o->type == ObjectEntry::INT_ENTRY || o->type == ObjectEntry::BOOL_ENTRY))
		return o->intVal;
	return def;
}

Number numCheck(ObjectEntry *o, const string &key, Number def) {
	o = objCheck(o, key);
	if (o && (o->type == ObjectEntry::FLOAT_ENTRY || o->type == ObjectEntry::BOOL_ENTRY || o->type == ObjectEntry::INT_ENTRY))
		return o->NumberVal;
	return def;}

string strCheck(ObjectEntry *o, const string &key) {
	o = objCheck(o, key);
	if (o && (o->type == ObjectEntry::STRING_ENTRY || o->type == ObjectEntry::BOOL_ENTRY || o->type == ObjectEntry::INT_ENTRY || o->type == ObjectEntry::FLOAT_ENTRY))
		return o->stringVal.contents;
	return string();
}

ObjectEntry *objCheck(ObjectEntry *o, const string &key) {
	if (o && !key.empty())
		o = (*o)[key];
	return o;
}

save_file save;