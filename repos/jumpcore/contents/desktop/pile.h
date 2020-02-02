#ifndef _PILE_H
#define _PILE_H

/*
 *  pile.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 12/2/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "displaycodes.h"

#define YFLIP 0

// Drawing consists 

struct pile;
struct drawing;

// A Thing You Can Draw in this system consists of three things.
// A pile -- a list of isolated vertices. It references a stateoid and an elementoid.
// A stateoid -- A thing that sets OpenGL state in preparation for a draw.
// An elementoid -- A thing that actually performs a draw.

struct stateoid { virtual void set(pile &, drawing *) = 0; };

struct elementoid { virtual void draw(pile &) = 0; };

// Would "state_matrix" be a better name?
struct state_common : public stateoid {
	virtual void set(pile &, drawing *d);
};

struct state_basic : public state_common {
	virtual void set(pile &, drawing *d);
	static state_basic *single;
};

// Basic vector array that is generalized into various things you "actually" want to draw.
struct pile : public vector<float> {
	bool useThree, useTexture, useColor; GLint texture; unsigned int baseColor; bool immortal;
	stateoid *stateSetter; elementoid *elementDrawer;
	pile(elementoid *_elementDrawer, bool _useThree = false, bool _useTexture = false, bool _useColor = false, GLint _texture = 0, unsigned int _baseColor = WHITE)
		: vector<float>(), useThree(_useThree), useTexture(_useTexture), useColor(_useColor), texture(_texture), baseColor(_baseColor), immortal(false), elementDrawer(_elementDrawer), stateSetter(state_basic::single) {}
	
    void push(float x, float y, float z, unsigned int color = WHITE, float *texes = NULL);
	int stride() const;
	int vertices() const;
	void draw(drawing *);
};

struct element_plain : public elementoid {
	GLenum mode;
	element_plain(GLenum _mode) : mode(_mode) {}
	void draw(pile &);
	static element_plain *points;
	static element_plain *triangles;
};

struct element_quad : public elementoid {
	void draw(pile &);
	static vector<unsigned short> megaIndex;
	static void megaIndexEnsure(int count);
	static element_quad *single;
};

struct quadpile : public pile {
	quadpile(bool _useTexture = true, bool _useColor = false, GLint _texture = 0, unsigned int _baseColor = WHITE)
		: pile(element_quad::single, false, _useTexture, _useColor, _texture, _baseColor) {}
	void push4(float x1, float y1, float x2, float y2, unsigned int color = WHITE, float *texes = NULL, float z=0);
	void push4(float x1, float y1, float x2, float y2, unsigned int color, texture_slice *s, float z=0);
	void push4(float x1, float y1, float x2, float y2, float r, float g = 1.0, float b = 1.0, float a = 1.0, float *texes = NULL, float z=0);
	void push4(float x1, float y1, float x2, float y2, float r, float g, float b, float a, texture_slice *s, float z=0);
	void push4(cpVect *vertices, unsigned int color = WHITE, float *texes = NULL, float z=0);
	void push4(cpVect *vertices, unsigned int color, texture_slice *s, float z=0);
};

struct quadpile3 : public pile {
	quadpile3(bool _useTexture = true, bool _useColor = false, GLint _texture = 0, unsigned int _baseColor = WHITE)
		: pile(element_quad::single, true, _useTexture, _useColor, _texture, _baseColor) {}
	void xysheet(cpVect a, cpVect b, cpFloat z1, cpFloat z2, unsigned int color = WHITE, float *texes = NULL);
	void xzsheet(cpVect a, cpVect b, cpFloat y1, cpFloat y2, unsigned int color = WHITE, float *texes = NULL);
	void yzsheet(cpVect a, cpVect b, cpFloat x1, cpFloat x2, unsigned int color = WHITE, float *texes = NULL);
	void xyplane(cpVect a, cpVect b, cpFloat z, unsigned int color = WHITE, float *texes = NULL);
	void xzplane(cpVect a, cpVect b, cpFloat y, unsigned int color = WHITE, float *texes = NULL);
	void yzplane(cpVect a, cpVect b, cpFloat x, unsigned int color = WHITE, float *texes = NULL);
};

struct tilepile : public quadpile {
	cpVect pixel, size, offset; int w; float z;
	tilepile(texture_slice *s, int x, int y, cpVect _size = cpv(1,1), cpVect _offset=cpvzero, bool xyIsPixels = true, bool _useColor = false, unsigned int _baseColor = WHITE);
	void tile(int ix, int iy, int x, int y, cpVect offset = cpvzero, unsigned int color=WHITE);
	void tile(int i, int x, int y, cpVect _offset = cpvzero, unsigned int color = WHITE);
	void zat(float _z) { z = _z; useThree = true; }
};

// Elementoid that tracks arbitrary vertex ordering
struct element_free : public elementoid {
	element_free(GLenum _kind = GL_TRIANGLES) : elementoid(), kind(_kind) {}
	GLenum kind;
	vector<unsigned short> index;
	void push(unsigned short i) {index.push_back(i);}
	void draw(pile &q) {
		glDrawElements(kind, index.size(), GL_UNSIGNED_SHORT, &index[0]);
	}
};

// Fancy stuff

struct element_quad_wire : public elementoid {
	static vector<unsigned short> megaIndex;
	static void megaIndexEnsure(int count);
	void draw(pile &q);
	static element_quad_wire *single;
};

struct state_blackout : public state_common {
	virtual void set(pile &, drawing *);
	static state_blackout *single;
};

// Special state for drawing a 2-pass occluding wireframe shape
struct state_doubled : public stateoid {
	elementoid *pass1e, *pass2e;
	stateoid *pass1s, *pass2s;
	state_doubled(elementoid *_pass1e = element_quad::single, elementoid *_pass2e = element_quad_wire::single, stateoid *_pass1s = state_blackout::single, stateoid *_pass2s = state_basic::single)
		: pass1e(_pass1e), pass2e(_pass2e), pass1s(_pass1s), pass2s(_pass2s) {}
	virtual void set(pile &q, drawing *d) {
		uint32_t color = q.baseColor;
	
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1., 1./(float)0x1);
		q.elementDrawer = pass1e;
		q.stateSetter = pass1s;
		q.baseColor = BLACK;
		q.draw(d); // Does matrix need to be re-uploaded?
		glDisable(GL_POLYGON_OFFSET_FILL);
		
		q.elementDrawer = pass2e;
		q.baseColor = color;
		pass2s->set(q, d);
		q.stateSetter = this;
	}
};

// Context

typedef hash_map<display_code, pile *> quad_db;
typedef quad_db::iterator quad_db_iter;

struct drawing { // Should track framebuffer
	float w, h;
	glm::mat4 mvp;
	
	drawing(float width = 1/aspect, float height = 1);
	drawing(texture_slice *target);
	drawing(drawing *target);
	~drawing();
	quad_db data; // TODO: Preserve order?
	pile &get_pile(display_code code = D_C, elementoid *_elementDrawer = NULL, bool _useThree = false, bool _useTexture = true, bool _useColor = false, GLint _texture = 0, unsigned int _baseColor = WHITE);
	quadpile &get_quadpile(display_code code = D_C, bool _useTexture = true, bool _useColor = false, GLint _texture = 0, unsigned int _baseColor = WHITE);
	quadpile3 &get_quadpile3(display_code code = D_C3, bool _useTexture = true, bool _useColor = false, GLint _texture = 0, unsigned int _baseColor = WHITE);
	tilepile &get_tilepile(texture_slice *s, int x, int y, cpVect _size = cpv(1,1), cpVect _offset=cpvzero, bool xyIsPixels = true, bool _useColor = false, unsigned int _baseColor = WHITE);
	template <class T>
	T &insert_pile(display_code code, T *p) {
		data[code] = p;
		return *p;
	}

	void dupe(drawing *target);
	void size(float width, float height);
	void size(texture_slice *target);
	void execute(); // Actually draw everything in data

	void matrix_upload();
	void matrix_jcupload();
};

struct matrix_source {
	glm::mat4 matrix;
	bool matrix_literal; // True if "to be taken literally", false if to be overlaid
	matrix_source() : matrix_literal(false) {}
};

#endif