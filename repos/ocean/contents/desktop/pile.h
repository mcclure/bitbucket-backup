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


struct pile;

struct stateoid { virtual void set(pile &) = 0; };

struct elementoid { virtual void draw(pile &) = 0; };

struct state_basic : public stateoid {
	virtual void set(pile &);
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
	void draw(); // Implemented in util_display.cpp
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
	void push4(float x1, float y1, float x2, float y2, unsigned int color = WHITE, float *texes = NULL);
	void push4(float x1, float y1, float x2, float y2, unsigned int color, texture_slice *s);
	void push4(float x1, float y1, float x2, float y2, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0, float *texes = NULL);
	void push4(float x1, float y1, float x2, float y2, float r, float g, float b, float a, texture_slice *s);
	void push4(cpVect *vertices, unsigned int color = WHITE, float *texes = NULL);
	void push4(cpVect *vertices, unsigned int color, texture_slice *s);
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

struct linepile : public vector<float> {
	void push(float x1, float y1, float x2, float y2, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0);
    void push(cpVect v1, cpVect v2, bool usecolor = false, float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0);
};

#endif