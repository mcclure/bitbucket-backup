#pragma once
#include <iostream>
#include <SFML/Graphics.hpp>
#include "constants.h"
#include "Particle.h"
#include "globals.h"

struct Processor {
	Processor *next;
	Processor(Processor *_next = NULL) : next(_next) {}
	virtual void preBuffer() { if (next) next->preBuffer(); }
	virtual void drawDot(float x, float y, Pixel p);
	virtual void drawLine(float x1, float y1, float x2, float y2, Pixel p);
	virtual void postBuffer() { if (next) next->postBuffer(); }
};

extern Processor *currentProcessor;

struct FadeProcessor : public Processor {
	FadeProcessor(Processor *_next = NULL) : Processor(_next) {}
	void preBuffer();
};

struct BleedProcessor : public Processor {
	BleedProcessor(Processor *_next = NULL) : Processor(_next) {}
	void drawLine(float x1, float y1, float x2, float y2, Pixel p) {}
	void postBuffer();
};

struct LightingProcessor : public Processor {
	LightingProcessor(Processor *_next = NULL) : Processor(_next) {}
	void preBuffer();
	virtual void drawDot(float x, float y, Pixel p);
	virtual void drawLine(float x1, float y1, float x2, float y2, Pixel p);
	void postBuffer();
};

struct BlurProcessor : public Processor {
	BlurProcessor(Processor *_next = NULL) : Processor(_next) {}
	void postBuffer();
};