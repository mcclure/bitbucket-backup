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

#include "chipmunk.h"
#include "lodepng.h"
#include "internalfile.h"
#include "FTGL/ftgles.h"
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

// Display state: You probably want to keep all of this:

// Doesn't work on ES devices right now:
#define DRAW_DEBUG 0

FTFont* uiFont = NULL;
GLuint ftex;
float uiFontHeight = 0;

// Used by font stuff:

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
}

// x and y are in range -1..1, -aspect..aspect, rot is in range 0..360
void drawText(string text, double x, double y, double rot, bool xcenter, bool ycenter) {
	jcMatrixMode(GL_PROJECTION);
	jcPushMatrix();
	jcLoadIdentity();
	jcOrtho(-surfacew/2, surfacew/2, -surfaceh/2, surfaceh/2, -1.0, 5.0);
	jcMatrixMode(GL_MODELVIEW);
	
	jcPushMatrix();	
	jcLoadIdentity();
//		jcColor4f(1.0,1.0,1.0,1.0);	// For debug
	jcTranslatef(x*surfaceh/2, y*surfaceh/2, 0);
	if (rot)
		jcRotatef(rot,0,0,1);
	jcTranslatef(xcenter?-centerOff(text.c_str()):0, ycenter?-uiFontHeight/2:0, 0);
		
	uiFont->Render(text.c_str());
		
	jcPopMatrix();
	jcMatrixMode(GL_PROJECTION);
	jcPopMatrix();
	jcMatrixMode(GL_MODELVIEW);
		
	GLERR("drawText");		
	
	// Uncomment if it turns out that FTFont::Render is changing state behind glCommon's back
//		rinseGl();
}

// Callbacks follow:

// This is called once each time the display surface is initialized. It's a good place to do things like initialize
// fonts and textures. (Note it could be called more than once if the window size ever changes.)
void text_init() {
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
	if (fontSize < 16.2) fontSize = 16.2; // Don't overdo it though I guess. TODO: Do this on iDevice only?
	ERR("FONT SIZE %f\n", fontSize);
	
	initFont(fontSize);
		
	GLERR("text_init");
}

// Rewrite this if you want to change how ControlBase items appear.
void drawButton(void *ptr, void *data)
{
	cpPolyShape *poly = (cpPolyShape *)ptr;
	ControlBase *control = (ControlBase *)poly->shape.data;
	int num = poly->numVerts;
	cpVect *verts = poly->verts;
	
	if (!control) return;
	
	// Note we do no body-specific translation because we assume this is the UI space.
#if 1 // TODO REMOVE
	if (control->bg) {
		jcColor4f(1,0,1,1);
		jcBegin(GL_QUADS);
		if (control == KeyboardControl::focus || control->highlighted)
			jcImmediateColor4f(0.5,0.5,1.0,0.5);	
		else
			jcImmediateColor4f(0.5,0.5,0.5,0.75); 
		for(int i=0; i<num; i++){
			cpVect v = verts[i];
			jcVertex3f(v.x, v.y, 0);
		}
		jcEnd();
	}
#endif
	
	bool floatOff = !control->text.empty() && control->img; // "This button has both text and an icon"
	const double border = button_height/18;
	const double slicesize = button_height - border*2; // I have no idea
	
	if (!control->text.empty()) { // Draw button text -- a little complicated because it recognizes newlines // TODO: Use FTLayout instead
		int count = 1, index = 0;
		string::size_type upto = 0, from = 0;
		while (string::npos != (upto = control->text.find('\n', upto+1))) // Assumes the first character is never a \n.
			count++;
		
		upto = 0;
		from = 0;
		do {
			upto = control->text.find('\n', upto+1);
			string sub = control->text.substr(from, string::npos == upto ? control->text.size() : upto-from);
			
			jcImmediateColor4f(1.0,1.0,1.0,1.0);	
			
			cpVect at = control->p;
			at.y += (count/2.0-index-1)*textHeight();
			at.y -= uiFont->Descender()/surfaceh;
			if (floatOff) 
				at.x += slicesize/2;

			drawText(sub, at.x, at.y, 0, true, false);
			
			index++;
			from = upto + 1;
		} while (upto != string::npos);
	} 
		
#if 1 // TODO remove
	if (control->img) {
		jcPushMatrix();		  
		jcTranslatef(control->p.x, control->p.y, 0);
		if (floatOff)
			jcTranslatef(-button_width/2 + slicesize/2 + border, 0, 0);
		glBindTexture(GL_TEXTURE_2D, control->img->texture);

		jcBegin(GL_QUADS);
		jcImmediateColor4f(1,1,1,1);
		jcTexCoord2f(0,0); jcVertex3f(-slicesize/2, slicesize/2, 0);
		jcTexCoord2f(0,1); jcVertex3f(-slicesize/2, -slicesize/2, 0);
		jcTexCoord2f(1,1); jcVertex3f(slicesize/2, -slicesize/2, 0);
		jcTexCoord2f(1,0); jcVertex3f(slicesize/2, slicesize/2, 0);
		jcEnd();

		jcPopMatrix();
	}
#endif
}

#ifdef TARGET_DESKTOP
// A maybe-overcomplicated function for displaying a fatal error (given as "why"). All event handling will halt and a
// window will go up explaining the error; on the next keypress after this, the program will quit. This uses the global
// "uiFont" font (but is not dependent on drawText()). See also FileBombBox
// I have not bothered to make this ES compatible yet because iPhone has an alternate bomb box method
void BombBox(string why) {
	glClearColor(1.0,1.0,1.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	orthoText();
	
	float leftMargin = -centerOff("(Press a key or gamepad button to set the highlighted control)");
	int lineCount = -1;
	
	glColor3f(0.0, 0.0, 0.0);
	
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
#endif