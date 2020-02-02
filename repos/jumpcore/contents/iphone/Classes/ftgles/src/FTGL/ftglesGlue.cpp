/*
 
 Copyright (c) 2010 David Petrie
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 
 */

#include "ftglesGlue.h"
#include "glCommon.h"

struct Vertex 
{
	float xyz[3];
	float st[2];
	GLubyte c[4];
};

#define MAX_VERTS 16384

typedef struct Vertex Vertex;
Vertex immediate[MAX_VERTS];
Vertex vab;
short quad_indexes[MAX_VERTS * 3 / 2];
int curr_vertex;
GLenum curr_prim;
bool initted = false;
bool immediateTexture = false;

GLvoid ftglInitImmediateModeGL() 
{
}

GLvoid ftglNormal3f(float x, float y, float z) {
#if defined(glNormal3f)
	glNormal3f(x, y, z);
#endif
}

GLvoid ftglBegin(GLenum prim) 
{
	if (!initted)
	{
		for (int i = 0; i < MAX_VERTS * 3 / 2; i += 6) 
		{
			int q = i / 6 * 4;
			quad_indexes[i + 0] = q + 0;
			quad_indexes[i + 1] = q + 1;
			quad_indexes[i + 2] = q + 2;
			
			quad_indexes[i + 3] = q + 0;
			quad_indexes[i + 4] = q + 2;
			quad_indexes[i + 5] = q + 3;
		}
		initted = true;
	}
	curr_vertex = 0;
	curr_prim = prim;
	immediateTexture = false; // Must be at least one texcoord between Begin() and End(), or we don't draw texture
}


GLvoid ftglVertex3f(float x, float y, float z) 
{
	assert(curr_vertex < MAX_VERTS);
	vab.xyz[0] = x;
	vab.xyz[1] = y;
	vab.xyz[2] = z;
	immediate[curr_vertex] = vab;
	curr_vertex++;
}


GLvoid ftglVertex2f(float x, float y) 
{
	assert(curr_vertex < MAX_VERTS);
	vab.xyz[0] = x;
	vab.xyz[1] = y;
	vab.xyz[2] = 0.0f;
	immediate[curr_vertex] = vab;
	curr_vertex++;
}


GLvoid ftglColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) 
{
	vab.c[0] = r;
	vab.c[1] = g;
	vab.c[2] = b;
	vab.c[3] = a;
}


GLvoid ftglColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) 
{
	vab.c[0] = (GLubyte) (r * 255);
	vab.c[1] = (GLubyte) (g * 255);
	vab.c[2] = (GLubyte) (b * 255);
	vab.c[3] = (GLubyte) (a * 255);
}


GLvoid ftglTexCoord2f(GLfloat s, GLfloat t) 
{
	vab.st[0] = s;
	vab.st[1] = t;
	immediateTexture = true;
}

// Note: FTGL ES vanilla has this neat feature where you can haul off and draw something
// with ftglBegin/ftglEnd and it will automatically restore your vertex pointers and such
// afterward. In Jumpcore this is broken if you're using OpenGL 2.0.
GLvoid ftglEnd() 
{	
	GLboolean vertexArrayEnabled;
	GLboolean texCoordArrayEnabled;
	GLboolean colorArrayEnabled;
	
	GLvoid * vertexArrayPointer;
	GLvoid * texCoordArrayPointer;
	GLvoid * colorArrayPointer;
	
	GLint vertexArrayType, texCoordArrayType, colorArrayType;
	GLint vertexArraySize, texCoordArraySize, colorArraySize;
	GLsizei vertexArrayStride, texCoordArrayStride, colorArrayStride;
	
	bool resetPointers = false;
	
	bool okayRestoreAfter = false; // !gl2;
	
	if (!okayRestoreAfter) { // In theory, the other path should work fine. But I don't trust it yet with gl2, and there's a problem where it leaves textures on
		rinseGl();
		States(immediateTexture,true);
		jcVertexPointer(3, GL_FLOAT, sizeof(Vertex), immediate[0].xyz);
		if (immediateTexture)
			jcTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), immediate[0].st);
		jcColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), immediate[0].c);
	} else { // Store all state so we can leave it the way we found it:
		jcGetPointerv(GL_VERTEX_ARRAY_POINTER, &vertexArrayPointer);
		jcGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER, &texCoordArrayPointer);
		jcGetPointerv(GL_COLOR_ARRAY_POINTER, &colorArrayPointer);

		jcGetBooleanv(GL_VERTEX_ARRAY, &vertexArrayEnabled);
		jcGetBooleanv(GL_TEXTURE_COORD_ARRAY, &texCoordArrayEnabled);
		jcGetBooleanv(GL_COLOR_ARRAY, &colorArrayEnabled);

		if (!vertexArrayEnabled)
		{
			jcEnableClientState(GL_VERTEX_ARRAY);
		}
		
		if (vertexArrayPointer != &immediate[0].xyz)
		{
			jcGetIntegerv(GL_VERTEX_ARRAY_TYPE, &vertexArrayType);
			jcGetIntegerv(GL_VERTEX_ARRAY_SIZE, &vertexArraySize);
			jcGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &vertexArrayStride);
			if (texCoordArrayEnabled)
			{
				jcGetIntegerv(GL_TEXTURE_COORD_ARRAY_TYPE, &texCoordArrayType);
				jcGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE, &texCoordArraySize);
				jcGetIntegerv(GL_TEXTURE_COORD_ARRAY_STRIDE, &texCoordArrayStride);
			}	
			if (colorArrayEnabled)
			{
				jcGetIntegerv(GL_COLOR_ARRAY_TYPE, &colorArrayType);
				jcGetIntegerv(GL_COLOR_ARRAY_SIZE, &colorArraySize);
				jcGetIntegerv(GL_COLOR_ARRAY_STRIDE, &colorArrayStride);
			}	
			jcVertexPointer(3, GL_FLOAT, sizeof(Vertex), immediate[0].xyz);
			if (immediateTexture)
				jcTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), immediate[0].st);
			jcColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), immediate[0].c);
			
			resetPointers = true;
		}
		
		if (!texCoordArrayEnabled && immediateTexture)
			jcEnableClientState(GL_TEXTURE_COORD_ARRAY);

		if (texCoordArrayEnabled && !immediateTexture)
			jcDisableClientState(GL_TEXTURE_COORD_ARRAY);
		
		if (!colorArrayEnabled)
			jcEnableClientState(GL_COLOR_ARRAY);
	}
	
	if (curr_vertex == 0) 
	{
		curr_prim = 0;
		return;
	}
	
	void mesa_sync();
	mesa_sync();
	
	if (curr_prim == GL_QUADS) 
	{
		glDrawElements(GL_TRIANGLES, curr_vertex / 4 * 6, GL_UNSIGNED_SHORT, quad_indexes);
	} 
	else 
	{
		glDrawArrays(curr_prim, 0, curr_vertex);
	}
	curr_vertex = 0;
	curr_prim = 0;
	
	if (okayRestoreAfter) {
		if (resetPointers)
		{
			if (vertexArrayEnabled)
			{
				jcVertexPointer(vertexArraySize, vertexArrayType, 
								vertexArrayStride, vertexArrayPointer);	
			}
			if (texCoordArrayEnabled)
			{
				jcTexCoordPointer(texCoordArraySize, texCoordArrayType, 
								  texCoordArrayStride, texCoordArrayPointer);
			}
			if (colorArrayEnabled)
			{
				jcColorPointer(colorArraySize, colorArrayType, 
							   colorArrayStride, colorArrayPointer);
			}
		}
		
		if (!vertexArrayEnabled)
			jcDisableClientState(GL_VERTEX_ARRAY);
		
		if (!texCoordArrayEnabled && immediateTexture)
			jcDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if (texCoordArrayEnabled && !immediateTexture)
			jcEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		if (!colorArrayEnabled)
			jcDisableClientState(GL_COLOR_ARRAY);
	}
}


GLvoid ftglError(const char *source)
{
	GLenum error = glGetError();
	 
	switch (error) {
		case GL_NO_ERROR:
			break;
		case GL_INVALID_ENUM:
			printf("GL Error (%x): GL_INVALID_ENUM. %s\n\n", error, source);
			break;
		case GL_INVALID_VALUE:
			printf("GL Error (%x): GL_INVALID_VALUE. %s\n\n", error, source);
			break;
		case GL_INVALID_OPERATION:
			printf("GL Error (%x): GL_INVALID_OPERATION. %s\n\n", error, source);
			break;
		case GL_STACK_OVERFLOW:
			printf("GL Error (%x): GL_STACK_OVERFLOW. %s\n\n", error, source);
			break;
		case GL_STACK_UNDERFLOW:
			printf("GL Error (%x): GL_STACK_UNDERFLOW. %s\n\n", error, source);
			break;
		case GL_OUT_OF_MEMORY:
			printf("GL Error (%x): GL_OUT_OF_MEMORY. %s\n\n", error, source);
			break;
		default:
			printf("GL Error (%x): %s\n\n", error, source);
			break;
	}
}