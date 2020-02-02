// DISPLAY CODE

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2009 Andrew McClure
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// File contains code from Chipmunk "MoonBuggy" demo; notice applies to that code only:
/* Copyright (c) 2007 Scott Lembcke
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "kludge.h"

#include "chipmunk.h"
#include "lodepng.h"
#include "internalfile.h"
#include <queue>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "tinyxml.h"
#include "program.h"
#include "display.h"
#include "color.h"

#include "glCommon.h"
#include "glCommonMatrix.h"

// #define DRAW_DEBUG 1

// Display state: You probably want to keep all of this:

bool optDebug = true;

// Puts screen in orthographic projection where the screen top is 1.0, bottom is -1.0, left is -aspect, right is aspect
// Note: This is used by ControlBase, so if you mess with it that may stop working.
// If you need a different orthographic projection I recommend duplicating this function rather than altering it.
void goOrtho() {
	jcMatrixMode(GL_PROJECTION);
	jcLoadIdentity();
	jcOrtho(-1.0f, 1.0f, -1.0, 1.0, -1.0, 5.0);
	jcScalef(1.0*aspect, 1.0, 1.0);
	jcMatrixMode(GL_MODELVIEW);
}

void orthoText() { // Puts screen in a 1:1 pixel:point relationship. Needed by drawText.
	jcMatrixMode(GL_PROJECTION);
	jcLoadIdentity();
	jcOrtho(-surfacew/2, surfacew/2, -surfaceh/2, surfaceh/2, -1.0, 5.0);
	jcMatrixMode(GL_MODELVIEW);
}

#if 0
void goPerspective() { // An example projective ... projection matrix. Not used anywhere right now
	jcMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1/aspect, 0.5, 5);
	jcMatrixMode(GL_MODELVIEW);
}
#endif

#if DRAW_DEBUG
void drawCollisions(void *ptr, void *data)
{
	cpArbiter *arb = (cpArbiter*)ptr;
	int i;
	for(i=0; i<arb->numContacts; i++){
		cpVect v = arb->contacts[i].p;
		glVertex2f(v.x, v.y);
	}
}
#endif

// The default display() iterates over each shape in the hash and calls a drawObject() on each one.
// This forwards to one of the following methods, depending on what kind of shape it is:
void drawCircle(cpFloat x, cpFloat y, cpFloat r, cpFloat a)
{
#if DRAW_DEBUG
	const int segs = 15;
	const cpFloat coef = 2.0*M_PI/(cpFloat)segs;
	
	int n;
	
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_LINE_LOOP); {
		for(n = 0; n < segs; n++){
			cpFloat rads = n*coef;
			glVertex2f(r*cos(rads + a) + x, r*sin(rads + a) + y);
		}
		glVertex2f(x,y);
	} glEnd();
#endif
}

void drawCircleShape(cpShape *shape)
{
#if DRAW_DEBUG
	cpBody *body = shape->body;
	cpCircleShape *circle = (cpCircleShape *)shape;
	cpVect c = cpvadd(body->p, cpvrotate(circle->c, body->rot));
	drawCircle(c.x, c.y, circle->r, body->a);
#endif
}

void drawSegmentShape(cpShape *shape)
{
#if DRAW_DEBUG
	cpBody *body = shape->body;
	cpSegmentShape *seg = (cpSegmentShape *)shape;
	cpVect a = cpvadd(body->p, cpvrotate(seg->a, body->rot));
	cpVect b = cpvadd(body->p, cpvrotate(seg->b, body->rot));
	
	glBegin(GL_LINES); 
	glColor3f(0.5, 0.5, 0.5);{
		glVertex2f(a.x, a.y);
		glVertex2f(b.x, b.y);
	} glEnd();
#endif
}

void drawPolyShape(cpShape *shape, void *_data)
{
#if 0
	cpBody *body = shape->body;
	
	switch(shape->collision_type) {
		default:break;
	}
#endif
	
#if DRAW_DEBUG
	if (optDebug) {
        cpBody *body = shape->body;
        
		cpPolyShape *poly = (cpPolyShape *)shape;
		int num = poly->numVerts;
		cpVect *verts = poly->verts;
		int i;
		glBegin(GL_LINE_LOOP);
		glColor3f(0.5f,0.5f,0.5f);
		for(i=0; i<num; i++){
			cpVect v = cpvadd(body->p, cpvrotate(verts[i], body->rot));
			glVertex3f(v.x, v.y, 0);
		} glEnd();
	}
#endif
}

void drawObject(void *ptr, void *data)
{
	cpShape *shape = (cpShape*)ptr;
	switch(shape->klass->type){
		case CP_CIRCLE_SHAPE:
			drawCircleShape(shape);
			break;
		case CP_SEGMENT_SHAPE:
			drawSegmentShape(shape);
			break;
		case CP_POLY_SHAPE:
			drawPolyShape(shape, data);
			break;
		default:
			ERR("Bad enumeration in drawObject().\n");
	}
}

// "IJUMPMAN"

void pile::push(cpVect vertex, bool usecolor, float r, float g, float b, float a, float pointSize, float *texverts) {
    push_back(vertex.x);
    push_back(vertex.y);
    if (usecolor) {
        unsigned int color = packColor(r, g, b, a);
        push_back(*((float *)&color));
    }
    if (pointSize) {
        push_back(pointSize);
    }
    if (texverts) { // Do I like this? It's only ever used by display_init/traceBox in ijumpman.
        push_back(texverts[0]);
        push_back(texverts[1]);
    }
}

void quadpile::push(float x1, float y1, float x2, float y2, bool usecolor, float r, float g, float b, float a, float *texverts) {
	cpVect vertices[4] =   { cpv(x1, y2), cpv(x1, y1), cpv(x2,y1), cpv(x2,y2), };
	unsigned int color;
    if (usecolor) color = packColor(r, g, b, a);
	for(int c = 0; c < 4; c++) {
		cpVect &v = vertices[c];
		push_back(v.x);
		push_back(v.y);
        if (usecolor)
            push_back(*((float *)&color));
        if (texverts) { // Do I like this? It's only ever used by display_init/traceBox.
            push_back(texverts[c*2]);
            push_back(texverts[c*2+1]);
        }
	}			
}

void linepile::push(float x1, float y1, float x2, float y2, bool usecolor, float r, float g, float b, float a) {
    push(cpv(x1, y1), cpv(x2, y2), usecolor, r, g, b, a);
}

void linepile::push(cpVect v1, cpVect v2, bool usecolor, float r, float g, float b, float a) {
    cpVect vertices[2] =   { v1, v2 };
	unsigned int color;
    if (usecolor) color = packColor(r, g, b, a);
	for(int c = 0; c < 2; c++) {
		cpVect &v = vertices[c];
		push_back(v.x);
		push_back(v.y);
        if (usecolor)
            push_back(*((float *)&color));
	}		    
}

void quadpile::push(float x1, float y1, float x2, float y2, bool usecolor, float r, float g, float b, float a, subtexture_slice *s) {
    float texes[8] = { s->coords[0], s->coords[3], s->coords[0], s->coords[1], s->coords[2], s->coords[1], s->coords[2], s->coords[3] };
    push(x1, y1, x2, y2, usecolor, r,g,b,a, texes);
}

vector<unsigned short> quadpile::megaIndex;
void quadpile::megaIndexEnsure(int count) {
	int current = megaIndex.size()/6;
	for(int c = current; c < count; c++) {
		megaIndex.push_back(c*4);
		megaIndex.push_back(c*4+3);
		megaIndex.push_back(c*4+1);
		megaIndex.push_back(c*4+1);
		megaIndex.push_back(c*4+3);
		megaIndex.push_back(c*4+2);
	}
}

// Calls BombBox with a message stating "filename" could not be fou
void FileBombBox(string filename) {
	string because;
	if (filename.empty())
		because = "An internal file could not be opened.";
	else
		because = "Could not open file:\n";
	BombBox(because + filename + "\n");
}

// Give the interface library the chance to respond to a click or touch; return true if the interface "ate" the click.
// Lives in util_display because it uses matrix stuff
// Note: Button should always be 0 on mobile
bool interface_attempt_click(cpVect screen_coord, int button) {
	goOrtho();
	void jcLoadIdentity(); jcLoadIdentity();
	cpVect at = screenToGL(screen_coord.x, screen_coord.y, 0);
	
	clickConnected = false;
	
	if (!(button == 2 || button == 4 || button == 5)) { // Left click only
		cpSpacePointQuery(workingUiSpace(), at, CP_ALL_LAYERS, CP_NO_GROUP, clicked, NULL);
	} else { // Scroll wheel
		int dir = 0;
		if (button == 4) dir = 1;
		if (button == 5) dir = -1;
		cpSpacePointQuery(workingUiSpace(), at, CP_ALL_LAYERS, CP_NO_GROUP, wheeled, (void *)dir);
	}
	
	if (clickConnected) { // Somewhere above, a control event has occurred.
		program_interface();
	}
	
	return clickConnected;
}