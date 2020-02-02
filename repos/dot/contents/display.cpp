// DISPLAY CODE

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2009 Andi McClure
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
#include "slice.h"

#include "lodepng.h"
#include "internalfile.h"
#include "FTGLTextureFont.h"
#include <queue>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "tinyxml.h"
#include "program.h"
#include "color.h"

// Display state: You probably want to keep all of this:

#define DRAW_DEBUG 1

#define SQUARE_SIZE 1
#define SQUARE_SPAN 17

FTFont* uiFont = NULL;
GLuint ftex;
float uiFontHeight = 0;

// Puts screen in orthographic projection where the screen top is 1.0, bottom is -1.0, left is -aspect, right is aspect
// Note: This is used by ControlBase, so if you mess with it that may stop working.
// If you need a different orthographic projection I recommend duplicating this function rather than altering it.
void goOrtho() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    float d = SQUARE_SPAN + 0.5;
	glOrtho(-d, d, -d, d, -1.0, 5.0);
	glScalef(1.0*aspect, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
}

void orthoText() { // Puts screen in a 1:1 pixel:point relationship. Needed by drawText.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-surfacew/2, surfacew/2, -surfaceh/2, surfaceh/2, -1.0, 5.0);
	glMatrixMode(GL_MODELVIEW);
}

void goPerspective() { // An example projective ... projection matrix. Not used anywhere right now
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1/aspect, 0.5, 5);
	glMatrixMode(GL_MODELVIEW);
}


enum GLCS { 
    GLCS_VERTEX,
    GLCS_TEXTURE,
    GLCS_COLOR,
    GLCS_LAST
};

enum GLE {
    GLE_TEXTURE,
    GLE_DEPTH,
    GLE_LAST
};

GLenum glcsEnum[GLCS_LAST] = {GL_VERTEX_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_COLOR_ARRAY};
int8_t glcsIs[GLCS_LAST] = { -1, -1, -1 }; // -1 for dunno
GLenum gleEnum[GLE_LAST] = {GL_TEXTURE_2D, GL_TEXTURE_COORD_ARRAY};
int8_t gleIs[GLE_LAST] = {-1, -1};

inline void Enable(GLE gle) {
    if (1 != gleIs[gle]) {
        gleIs[gle] = 1;
        glEnable(gleEnum[gle]);
    }
}

inline void Disable(GLE gle) {
    if (0 != gleIs[gle]) {
        gleIs[gle] = 0;
        glDisable(gleEnum[gle]);
    }
}

inline void EnableClientState(GLCS glcs) {
    if (1 != glcsIs[glcs]) {
        glcsIs[glcs] = 1;
        glEnableClientState(glcsEnum[glcs]);
    }
}

inline void DisableClientState(GLCS glcs) {
    if (0 != glcsIs[glcs]) {
        glcsIs[glcs] = 0;
        glDisableClientState(glcsEnum[glcs]);
    }
}

// Used by font stuff:

static float tempTexture[] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

float textWidth(string str) {
	float llx, lly, llz, urx, ury, urz;
	uiFont->BBox(str.c_str(), llx, lly, llz, urx, ury, urz);
	return (urx - llx)/(surfaceh/2);	
}
float textHeight(string s = string()) {
	return uiFontHeight/(surfaceh/2);	
}
float centerOff(const char *str) {
	float llx, lly, llz, urx, ury, urz;
	uiFont->BBox(str, llx, lly, llz, urx, ury, urz);
	return (urx - llx)/2;
}
void initFont(int size) {
	uiFont->FaceSize(size);
	uiFontHeight = uiFont->LineHeight();
	
	glGenTextures(1, &ftex);
    glBindTexture(GL_TEXTURE_2D, ftex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_FLOAT, tempTexture);
}

// x and y are in range -1..1, -aspect..aspect, rot is in range 0..360
void drawText(string text, double x, double y, double rot = 0, bool xcenter = true, bool ycenter = true) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-surfacew/2, surfacew/2, -surfaceh/2, surfaceh/2, -1.0, 5.0);
	glMatrixMode(GL_MODELVIEW);
	
	glPushMatrix();	
	glLoadIdentity();
	glColor3f(0.0,0.0,0.0);	
	glTranslatef(x*surfaceh/2, y*surfaceh/2, 0);
	if (rot)
		glRotatef(rot,0,0,1);
	glTranslatef(xcenter?-centerOff(text.c_str()):0, ycenter?-uiFontHeight/2:0, 0);
		
	glEnable( GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ftex);
	uiFont->Render(text.c_str());
	glDisable( GL_TEXTURE_2D); 	

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

// .

#define STATIC_STRIDE 3

vector<thing *> acting;
quadpile drawing_square;
quadpile drawing_thing;

inline void quadpile::push(float x1, float y1, float x2, float y2, uint32_t color) {
	cpVect vertices[4] =   { cpv(x1, y2), cpv(x1, y1), cpv(x2,y1), cpv(x2,y2), };
	for(int c = 0; c < 4; c++) {
		cpVect &v = vertices[c];
		push_back(v.x);
		push_back(v.y);
		push_back(*((float *)&color));
	}			
}

inline void quadpile::push(float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
    push(x1, y1, x2, y2, packColor(r, g, b, a));
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

inline void squareCenteredAt(quadpile &to, cpVect at, uint32_t color) {
    to.push(at.x-0.5, at.y-0.5, at.x+0.5, at.y+0.5, color);
}

void plain_square::draw(int x, int y) {
    squareCenteredAt( drawing_square, cpvsub( cpv(x,y), chassis.offset ), color );
}

void dot_thing::draw(int x, int y) {
    squareCenteredAt( drawing_thing, cpvsub( cpvadd(cpv(x,y), offset), chassis.offset ), color );
}

// Callbacks follow:

// This is called once each time the display surface is initialized. It's a good place to do things like initialize
// fonts and textures. (Note it could be called more than once if the window size ever changes.)
void display_init() {
    for(int c = 0; c < GLE_LAST; c++)
        gleIs[c] = -1;
    for(int c = 0; c < GLCS_LAST; c++)
        glcsIs[c] = -1;
    
	if (uiFont)
		delete uiFont;
	
	char fontfile[FILENAMESIZE];
	internalPath(fontfile, "DroidSans.ttf");
	uiFont = new FTGLTextureFont( fontfile);
	
	if ( !uiFont ){
		REALERR("Couldn't open font file: %s\n", fontfile);
		Quit(1);
	}
	
	if ( uiFont->Error() ){
		REALERR("Couldn't open font file (error %d): %s\n", (int)uiFont->Error(), fontfile);
		Quit(1);
	}
	
	float fontSize = 18;
	fontSize *= (surfaceh/800.0); // Scale the font size so that it will be 18 point on a 1024x800 screen.
	
	initFont(fontSize);
}

void displayAt(square *at, int x, int y, int d, int r) {
    if (r<=0 || !at)
        return;
    
    at->draw(x, y);
    for(list<thing *>::iterator b = at->anchor.begin(); b != at->anchor.end(); b++) {
        (*b)->draw(x, y);
        acting.push_back(*b);
    }
    
    if (x == 0 || y == 0) {
        int dist = iabs( x ? x : y );
        int d1 = (d-1+4)%4, d2 = (d+1)%4;
        displayAt(at->nesw[d1], xe(x, d1), ye(y, d1), d1, dist);
        displayAt(at->nesw[d2], xe(x, d2), ye(y, d2), d2, dist-1);
    }
    displayAt(at->nesw[d], xe(x, d), ye(y, d), d, r-1);
}

// This is called when it is time to draw a new frame.
void display(void)
{	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
    EnableClientState(GLCS_VERTEX);
    EnableClientState(GLCS_COLOR);

//	glTranslatef(0, 0, -1); // So we can draw at 0,0,0

	goOrtho();
	
	// DRAW THINGS
    
    acting.clear();
    drawing_square.clear();
    drawing_thing.clear();
    
    displayAt(chassis.anchored, 0, 0, 0, 1);
    for(int c = 0; c < 4; c++)
        displayAt(chassis.anchored->nesw[c], xe(0, c), ye(0, c), c, SQUARE_SPAN);
    
    {
        float *megaVertex;
        int drawTo;
        
        megaVertex = &drawing_square[0];
        
        glVertexPointer(2, GL_FLOAT, STATIC_STRIDE*sizeof(float), megaVertex);
        glColorPointer(4, GL_UNSIGNED_BYTE, STATIC_STRIDE*sizeof(float), megaVertex+2);
        
        drawTo = drawing_square.size()*6/STATIC_STRIDE/4;
        quadpile::megaIndexEnsure(drawTo);
        
        glDrawElements(GL_TRIANGLES, drawTo, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);

        
        megaVertex = &drawing_thing[0];
        
        glVertexPointer(2, GL_FLOAT, STATIC_STRIDE*sizeof(float), megaVertex);
        glColorPointer(4, GL_UNSIGNED_BYTE, STATIC_STRIDE*sizeof(float), megaVertex+2);
        
        drawTo = drawing_thing.size()*6/STATIC_STRIDE/4;
        quadpile::megaIndexEnsure(drawTo);
        
        glDrawElements(GL_TRIANGLES, drawTo, GL_UNSIGNED_SHORT, &quadpile::megaIndex[0]);
    }
        
	SDL_GL_SwapBuffers();
	ticks++;
	
	program_update();
}

// A maybe-overcomplicated function for displaying a fatal error (given as "why"). All event handling will halt and a
// window will go up explaining the error; on the next keypress after this, the program will quit. This uses the global
// "uiFont" font (but is not dependent on drawText()). See also FileBombBox
void BombBox(string why) {
	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	orthoText();
	
	float leftMargin = -centerOff("(Press a key or gamepad button to set the highlighted control)");
	int lineCount = -1;
	
	glColor3f(1.0, 1.0, 1.0);
	
	glPushMatrix();
	glTranslatef(leftMargin-72, 200, 0);
	glRotatef(180,0,1,0);
	//	glScalef(1.5, 1.5, 1.0);
	
	glPopMatrix();
	
	glEnable( GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ftex);
	glPushMatrix();
	
	glTranslatef(leftMargin, 200, 0);
	
	glPushMatrix();
	uiFont->Render("FATAL ERROR");
	glPopMatrix();
	glTranslatef(0, -uiFontHeight, 0);
	glTranslatef(0, -uiFontHeight, 0);
	while (!why.empty()) { // Wraps text. I can't believe I have to do simple stuff like this myself..
		char filename2[FILENAMESIZE+1]; memset(filename2, 0, FILENAMESIZE+1);
		int size = why.size(); if (size>FILENAMESIZE) size = FILENAMESIZE;
		int c = 0;
		for(; c < size; c++) {
			char next = why[c];
			if ('\n' == next) { 
				if (c > 0 && c+1 == why.size()) // Special case: '\n' comes at end of string
					c--;
				break;
			}
			filename2[c] = next;
			if (c > 0 && -centerOff(filename2) < leftMargin-36) {
				filename2[c] = '\0';
				c--;
				break;
			}
		}
		glPushMatrix();
		uiFont->Render(filename2);
		glPopMatrix();
		glTranslatef(0, -uiFontHeight, 0);
		lineCount++;
		
		why.replace(0, c+1, "");
	}
	glPushMatrix();
	uiFont->Render("The program must shut down.");
	glPopMatrix();
	glTranslatef(0, -uiFontHeight, 0);
	glTranslatef(0, -uiFontHeight, 0);
	glPushMatrix();
	uiFont->Render("Press any key to continue.");
	glPopMatrix();
	
	glDisable( GL_TEXTURE_2D); 
	glPopMatrix();
	
	glBegin(GL_LINE_LOOP);
	glVertex2f( leftMargin-72*2, 200+72);
	glVertex2f(-leftMargin+72*2, 200+72);
	glVertex2f(-leftMargin+72*2, 100-72 - lineCount*uiFontHeight);
	glVertex2f( leftMargin-72*2, 100-72 - lineCount*uiFontHeight);
	glEnd();
	
	SDL_GL_SwapBuffers();		
	
	while(1) { SDL_Event event;
		while ( SDL_PollEvent(&event) ) {
			//		ERR("OK event %d (vs %d) ... axis %d value %d\n", (int) event.type, (int)SDL_JOYAXISMOTION, (int)event.jaxis.axis, (int)event.jaxis.value);
			if ( event.type == SDL_QUIT || event.type == SDL_KEYDOWN) {
				Quit(1);
			}
		}
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

// Audio state:

#define NO_AUDIO 0

FILE *audiolog = NULL; // If you find yourself needing to debug your audio, fopen() this somewhere and all audio will start getting copied to this file as 16-bit pcm

int tapping = 0; // Used by typewriter demo
double lastWhiteNoiseValue = 0; // Used by typewriter demo

// Audio code:

void audio_callback(void *userdata, uint8_t *stream, int len) {
	int16_t *samples = (int16_t *)stream;
	int slen = len / sizeof(int16_t);

	for(int c = 0; c < slen; c++) {
		double value = 0;

		// MUSIC GOES HERE
		
#if NO_AUDIO
		value = 0;
#endif
		samples[c] = value*SHRT_MAX;
	}
	if (audiolog) { // For debugging audio
		fwrite(stream, sizeof(short), slen, audiolog);
	}
}