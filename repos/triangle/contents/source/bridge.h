#ifndef _BRIDGE_H
#define _BRIDGE_H

#include "program.h"
#include "PolyPortSound.h"

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
	void filedump_external_out(String _path, String data);
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
	void stackthrash(Screen *s, ScreenEntity *e);
	int getTextHeight(Label *l);
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
	String pwd();
	void setColorObj(Entity *e, Color *c);
	SceneMesh *normalPlane(Number x, Number y, Number xs = 1, Number ys = 1, bool no_backface = true);
	SceneMesh *uprightBox(Number x, Number y, Number z, bool stretch = true);
	void term_setOverride(bool v);
	void term_setHidden(bool v);
	void term_setEntry(String str);
	void term_setLine(int y, String str);
	void term_setColor(int y, Color *c);
	void term_reset();
	int term_height();
	int term_width();
	bool term_busy();
	String editor_dir();
	Number labelWidth(ScreenLabel *label);
	Number labelHeight(ScreenLabel *label);
	void paramSetNumber(LocalShaderParam *param, Number x);
	void paramSetVector2(LocalShaderParam *param, Vector2 x);
	void paramSetVector3(LocalShaderParam *param, Vector3 x);
	void paramSetColor(LocalShaderParam *param, Color x);
	int bindingId(lua_State *L);
	int unlinkgc(lua_State *L);
	void Audio1();
	void Audio2();
	void Audio3();
    void unfocus();
	Texture *findOutTexture(String materialName, String targetId);
    String oneFilePicker(String ext);
};

// Draws a "copy" of a scene object without having to duplicate its resources.
struct SceneEcho : SceneEntity {
	SceneEcho(Entity *_copying);
	virtual void transformAndRender();
	Entity *getEntity();
protected:
	Entity *copying;
};

// Draws a "copy" of a screen object without having to duplicate its resources.
struct ScreenEcho : ScreenEntity {
	ScreenEcho(Entity *_copying);
	virtual void transformAndRender();
	Entity *getEntity();
protected:
	Entity *copying;
};

#define BPARAMCOUNT 0
#define BIPARAMCOUNT 5

#define STEPS iparam[0]
#define REPEATS iparam[1]
#define NOISESTEPS iparam[2]
#define NOISEREPEATS iparam[3]
#define RESOLUTION iparam[4]

struct transition {
    int state;
    int symbol;
    int head;
};

struct BSound : PSound {
	BSound(int _loopsize, int _symbolcount, int _statecount);
    virtual ~BSound();
	Number param[BPARAMCOUNT];
    int iparam[BIPARAMCOUNT];
    unsigned char *loop;
    float *symbols;
    int loopsize; int symbolcount; int statecount;
    int loopc; int repeatc; int noiserepeatc; 
	float last; int resc;
	virtual int soundCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags);
    
	double pitch; double theta; double volume;
	void setPitch(Number newPitch);
	Number getPitch();
	void setVolume(Number newVolume);
	Number getVolume();
	void setParam(int idx, Number v);
	Number getParam(int idx);
    void setIparam(int idx, int v);
	int getIparam(int idx);
	
	void loopScramble();
    
    // MACHINE
    transition *transitions;
    int head; int state;
    transition &transitionFor(int state, int symbol);
	int upload(lua_State *L);
    int adjust(lua_State *L);
    int current(lua_State *L);
    int setSymbols(lua_State *L);
    
    // TEXTURE DISPENSER
    void initTexture(int _width, int _height);
    int width; int height;
    int realWidth; int rowHeight;
    uint32_t *gfxTemplate;
    uint32_t *gfxScratch;
    Texture *oscTexture();
};

string fromMatrix4(const Matrix4 &mat);
Vector3 matrixApply(const Matrix4 &m, Vector3 &v2);

#endif /* _BRIDGE_H */