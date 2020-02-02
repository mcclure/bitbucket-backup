#pragma once

extern "C" {

#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
} // extern "C" 

#include "automaton.h"
#include "bridge.h"

using namespace std;


namespace Polycode {

static int Project_automaton_get_done(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	lua_pushboolean(L, inst->done);
	return 1;
}

static int Project_automaton_get_born(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	lua_pushinteger(L, inst->born);
	return 1;
}

static int Project_automaton_get_frame(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	lua_pushinteger(L, inst->frame);
	return 1;
}

static int Project_automaton_get_state(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	lua_pushinteger(L, inst->state);
	return 1;
}

static int Project_automaton_get_rollover(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	lua_pushinteger(L, inst->rollover);
	return 1;
}

static int Project_automaton_get_anchor(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	lua_pushlightuserdata(L, &inst->anchor);
	return 1;
}

static int Project_automaton_set_done(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	bool param = lua_toboolean(L, 2);
	inst->done = param;
	return 0;
}

static int Project_automaton_set_born(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	int param = lua_tointeger(L, 2);
	inst->born = param;
	return 0;
}

static int Project_automaton_set_frame(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	int param = lua_tointeger(L, 2);
	inst->frame = param;
	return 0;
}

static int Project_automaton_set_state(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	int param = lua_tointeger(L, 2);
	inst->state = param;
	return 0;
}

static int Project_automaton_set_rollover(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	int param = lua_tointeger(L, 2);
	inst->rollover = param;
	return 0;
}

static int Project_automaton(lua_State *L) {
	automaton *inst = new automaton();
	lua_pushlightuserdata(L, (void*)inst);
	return 1;
}

static int Project_automaton_age(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	 int *retInst = new  int();
	*retInst = inst->age();
	lua_pushlightuserdata(L, retInst);
	return 1;
}

static int Project_automaton_tick(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	inst->tick();
	return 0;
}

static int Project_automaton_die(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	inst->die();
	return 0;
}

static int Project_automaton_insert(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	inst->insert();
	return 0;
}

static int Project_delete_automaton(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	automaton *inst = (automaton*)lua_topointer(L, 1);
	delete inst;
	return 0;
}

static int Project_EntityArray_size(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	EntityArray *inst = (EntityArray*)lua_topointer(L, 1);
	lua_pushinteger(L, inst->size());
	return 1;
}

static int Project_EntityArray_get(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	EntityArray *inst = (EntityArray*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	int at = lua_tointeger(L, 2);
	void *ptrRetVal = (void*)inst->get(at);
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_EntityArray_push_back(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	EntityArray *inst = (EntityArray*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Entity* value = (Entity*)lua_topointer(L, 2);
	inst->push_back(value);
	return 0;
}

static int Project_delete_EntityArray(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	EntityArray *inst = (EntityArray*)lua_topointer(L, 1);
	delete inst;
	return 0;
}

static int Project_VectorArray_size(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	VectorArray *inst = (VectorArray*)lua_topointer(L, 1);
	lua_pushinteger(L, inst->size());
	return 1;
}

static int Project_VectorArray_get(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	VectorArray *inst = (VectorArray*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	int at = lua_tointeger(L, 2);
	Vector3 *retInst = new Vector3();
	*retInst = inst->get(at);
	lua_pushlightuserdata(L, retInst);
	return 1;
}

static int Project_VectorArray_push_back(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	VectorArray *inst = (VectorArray*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Vector3 value = *(Vector3*)lua_topointer(L, 2);
	inst->push_back(value);
	return 0;
}

static int Project_delete_VectorArray(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	VectorArray *inst = (VectorArray*)lua_topointer(L, 1);
	delete inst;
	return 0;
}

static int Project_NumberArray(lua_State *L) {
	NumberArray *inst = new NumberArray();
	lua_pushlightuserdata(L, (void*)inst);
	return 1;
}

static int Project_NumberArray_size(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	NumberArray *inst = (NumberArray*)lua_topointer(L, 1);
	lua_pushinteger(L, inst->size());
	return 1;
}

static int Project_NumberArray_get(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	NumberArray *inst = (NumberArray*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	int at = lua_tointeger(L, 2);
	lua_pushnumber(L, inst->get(at));
	return 1;
}

static int Project_NumberArray_push_back(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	NumberArray *inst = (NumberArray*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	Number value = lua_tonumber(L, 2);
	inst->push_back(value);
	return 0;
}

static int Project_delete_NumberArray(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	NumberArray *inst = (NumberArray*)lua_topointer(L, 1);
	delete inst;
	return 0;
}

static int Project_SceneEcho(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	Entity* _copying = (Entity*)lua_topointer(L, 1);
	SceneEcho *inst = new SceneEcho(_copying);
	lua_pushlightuserdata(L, (void*)inst);
	return 1;
}

static int Project_SceneEcho_transformAndRender(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	SceneEcho *inst = (SceneEcho*)lua_topointer(L, 1);
	inst->transformAndRender();
	return 0;
}

static int Project_SceneEcho_getEntity(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	SceneEcho *inst = (SceneEcho*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->getEntity();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_delete_SceneEcho(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	SceneEcho *inst = (SceneEcho*)lua_topointer(L, 1);
	delete inst;
	return 0;
}

static int Project_project_bridge_room_screen(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->room_screen();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_scene(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->room_scene();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_objs(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->room_objs();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_oncollide_objs(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->room_oncollide_objs();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_onclick_objs(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->room_onclick_objs();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_id(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING);
	String _at = String(lua_tostring(L, 2));
	void *ptrRetVal = (void*)inst->room_id(_at);
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_name(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	SceneEntity* of = (SceneEntity*)lua_topointer(L, 2);
	lua_pushstring(L, inst->room_name(of).c_str());
	return 1;
}

static int Project_project_bridge_room_a(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->room_a();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_b(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->room_b();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_remove_scene(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	SceneEntity* obj = (SceneEntity*)lua_topointer(L, 2);
	inst->room_remove_scene(obj);
	return 0;
}

static int Project_project_bridge_room_remove_screen(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	ScreenEntity* obj = (ScreenEntity*)lua_topointer(L, 2);
	bool doRemove;
	if(lua_isboolean(L, 3)) {
		doRemove = lua_toboolean(L, 3);
	} else {
		doRemove = true;
	}
	inst->room_remove_screen(obj, doRemove);
	return 0;
}

static int Project_project_bridge_load_room(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING);
	String from = String(lua_tostring(L, 2));
	inst->load_room(from);
	return 0;
}

static int Project_project_bridge_load_room_txt(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING);
	String from = String(lua_tostring(L, 2));
	inst->load_room_txt(from);
	return 0;
}

static int Project_project_bridge_standalone_screen(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING);
	String svg = String(lua_tostring(L, 2));
	String objectPath;
	if(lua_isstring(L, 3)) {
		objectPath = lua_tostring(L, 3);
	} else {
		objectPath = String ( );
	}
	bool isPhysics;
	if(lua_isboolean(L, 4)) {
		isPhysics = lua_toboolean(L, 4);
	} else {
		isPhysics = false;
	}
	void *ptrRetVal = (void*)inst->standalone_screen(svg, objectPath, isPhysics);
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_room_remove_standalone_screen_all(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Screen* s = (Screen*)lua_topointer(L, 2);
	inst->room_remove_standalone_screen_all(s);
	return 0;
}

static int Project_project_bridge_clear(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	inst->clear();
	return 0;
}

static int Project_project_bridge_meshFor(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Polycode::Polygon* p = (Polycode::Polygon*)lua_topointer(L, 2);
	void *ptrRetVal = (void*)inst->meshFor(p);
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_saved_level(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	lua_pushstring(L, inst->saved_level().c_str());
	return 1;
}

static int Project_project_bridge_set_saved_level(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	int priority = lua_tointeger(L, 2);
	inst->set_saved_level(priority);
	return 0;
}

static int Project_project_bridge_filedump(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING);
	String _path = String(lua_tostring(L, 2));
	lua_pushstring(L, inst->filedump(_path).c_str());
	return 1;
}

static int Project_project_bridge_filedump_external(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING);
	String _path = String(lua_tostring(L, 2));
	lua_pushstring(L, inst->filedump_external(_path).c_str());
	return 1;
}

static int Project_project_bridge_help(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING);
	String _path = String(lua_tostring(L, 2));
	lua_pushstring(L, inst->help(_path).c_str());
	return 1;
}

static int Project_project_bridge_fake(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	inst->fake();
	return 0;
}

static int Project_project_bridge_Quit(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	inst->Quit();
	return 0;
}

static int Project_project_bridge_mmult(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Matrix4 a = *(Matrix4*)lua_topointer(L, 2);
	luaL_checktype(L, 3, LUA_TLIGHTUSERDATA);
	Matrix4 b = *(Matrix4*)lua_topointer(L, 3);
	Matrix4 *retInst = new Matrix4();
	*retInst = inst->mmult(a, b);
	lua_pushlightuserdata(L, retInst);
	return 1;
}

static int Project_project_bridge_qmult(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Quaternion a = *(Quaternion*)lua_topointer(L, 2);
	luaL_checktype(L, 3, LUA_TLIGHTUSERDATA);
	Quaternion b = *(Quaternion*)lua_topointer(L, 3);
	Quaternion *retInst = new Quaternion();
	*retInst = inst->qmult(a, b);
	lua_pushlightuserdata(L, retInst);
	return 1;
}

static int Project_project_bridge_Slerp(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	Number fT = lua_tonumber(L, 2);
	luaL_checktype(L, 3, LUA_TLIGHTUSERDATA);
	Quaternion rkP = *(Quaternion*)lua_topointer(L, 3);
	luaL_checktype(L, 4, LUA_TLIGHTUSERDATA);
	Quaternion rkQ = *(Quaternion*)lua_topointer(L, 4);
	bool shortestPath;
	if(lua_isboolean(L, 5)) {
		shortestPath = lua_toboolean(L, 5);
	} else {
		shortestPath = false;
	}
	Quaternion *retInst = new Quaternion();
	*retInst = inst->Slerp(fT, rkP, rkQ, shortestPath);
	lua_pushlightuserdata(L, retInst);
	return 1;
}

static int Project_project_bridge_Squad(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	Number fT = lua_tonumber(L, 2);
	luaL_checktype(L, 3, LUA_TLIGHTUSERDATA);
	Quaternion rkP = *(Quaternion*)lua_topointer(L, 3);
	luaL_checktype(L, 4, LUA_TLIGHTUSERDATA);
	Quaternion rkA = *(Quaternion*)lua_topointer(L, 4);
	luaL_checktype(L, 5, LUA_TLIGHTUSERDATA);
	Quaternion rkB = *(Quaternion*)lua_topointer(L, 5);
	luaL_checktype(L, 6, LUA_TLIGHTUSERDATA);
	Quaternion rkQ = *(Quaternion*)lua_topointer(L, 6);
	luaL_checktype(L, 7, LUA_TBOOLEAN);
	bool shortestPath = lua_toboolean(L, 7);
	Quaternion *retInst = new Quaternion();
	*retInst = inst->Squad(fT, rkP, rkA, rkB, rkQ, shortestPath);
	lua_pushlightuserdata(L, retInst);
	return 1;
}

static int Project_project_bridge_bBox(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Entity* e = (Entity*)lua_topointer(L, 2);
	Vector3 *retInst = new Vector3();
	*retInst = inst->bBox(e);
	lua_pushlightuserdata(L, retInst);
	return 1;
}

static int Project_project_bridge_setSceneClearColor(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Scene* scene = (Scene*)lua_topointer(L, 2);
	luaL_checktype(L, 3, LUA_TNUMBER);
	int r = lua_tointeger(L, 3);
	luaL_checktype(L, 4, LUA_TNUMBER);
	int g = lua_tointeger(L, 4);
	luaL_checktype(L, 5, LUA_TNUMBER);
	int b = lua_tointeger(L, 5);
	luaL_checktype(L, 6, LUA_TNUMBER);
	int a = lua_tointeger(L, 6);
	inst->setSceneClearColor(scene, r, g, b, a);
	return 0;
}

static int Project_project_bridge(lua_State *L) {
	project_bridge *inst = new project_bridge();
	lua_pushlightuserdata(L, (void*)inst);
	return 1;
}

static int Project_project_bridge_custEntityType(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Entity* obj = (Entity*)lua_topointer(L, 2);
	lua_pushstring(L, inst->custEntityType(obj).c_str());
	return 1;
}

static int Project_project_bridge_charCode(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	InputEvent* e = (InputEvent*)lua_topointer(L, 2);
	lua_pushstring(L, inst->charCode(e).c_str());
	return 1;
}

static int Project_project_bridge_soundFromValues(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	NumberArray* values = (NumberArray*)lua_topointer(L, 2);
	int channels;
	if(lua_isnumber(L, 3)) {
		channels = lua_tointeger(L, 3);
	} else {
		channels = 1;
	}
	int freq;
	if(lua_isnumber(L, 4)) {
		freq = lua_tointeger(L, 4);
	} else {
		freq = 44100;
	}
	int bps;
	if(lua_isnumber(L, 5)) {
		bps = lua_tointeger(L, 5);
	} else {
		bps = 16;
	}
	void *ptrRetVal = (void*)inst->soundFromValues(values, channels, freq, bps);
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_project_bridge_playback_index(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	inst->playback_index();
	return 0;
}

static int Project_project_bridge_playback_from(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	int idx = lua_tointeger(L, 2);
	inst->playback_from(idx);
	return 0;
}

static int Project_project_bridge_luaTestAddOne(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	return inst->luaTestAddOne(L);
}

static int Project_project_bridge_saveTableIntoObject(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	return inst->saveTableIntoObject(L);
}

static int Project_project_bridge_loadTableFromObject(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	return inst->loadTableFromObject(L);
}

static int Project_project_bridge_saveTableIntoFile(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	return inst->saveTableIntoFile(L);
}

static int Project_project_bridge_loadTableFromFile(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	return inst->loadTableFromFile(L);
}

static int Project_project_bridge_saveTableIntoXml(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	return inst->saveTableIntoXml(L);
}

static int Project_project_bridge_loadTableFromXml(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	return inst->loadTableFromXml(L);
}

static int Project_project_bridge_getChildAtScreenPosition(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	return inst->getChildAtScreenPosition(L);
}

static int Project_project_bridge_getVisible(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Entity* e = (Entity*)lua_topointer(L, 2);
	lua_pushboolean(L, inst->getVisible(e));
	return 1;
}

static int Project_project_bridge_setVisible(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	Entity* e = (Entity*)lua_topointer(L, 2);
	luaL_checktype(L, 3, LUA_TBOOLEAN);
	bool visible = lua_toboolean(L, 3);
	inst->setVisible(e, visible);
	return 0;
}

static int Project_project_bridge_coreServices(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	void *ptrRetVal = (void*)inst->coreServices();
	if(ptrRetVal == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, ptrRetVal);
	}
	return 1;
}

static int Project_delete_project_bridge(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	project_bridge *inst = (project_bridge*)lua_topointer(L, 1);
	delete inst;
	return 0;
}

static int Project_StringArray_size(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	StringArray *inst = (StringArray*)lua_topointer(L, 1);
	lua_pushinteger(L, inst->size());
	return 1;
}

static int Project_StringArray_get(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	StringArray *inst = (StringArray*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TNUMBER);
	int at = lua_tointeger(L, 2);
	lua_pushstring(L, inst->get(at).c_str());
	return 1;
}

static int Project_StringArray_push_back(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	StringArray *inst = (StringArray*)lua_topointer(L, 1);
	luaL_checktype(L, 2, LUA_TSTRING);
	String value = String(lua_tostring(L, 2));
	inst->push_back(value);
	return 0;
}

static int Project_delete_StringArray(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	StringArray *inst = (StringArray*)lua_topointer(L, 1);
	delete inst;
	return 0;
}

} // namespace Polycode
