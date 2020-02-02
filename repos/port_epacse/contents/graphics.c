#include "graphics.h"

#include <string.h>
#include <SDL/SDL_opengl.h>
#ifndef __APPLE__
#include <GL/glext.h>
#endif
#include <SOIL.h>
#include "error.h"
#include "settings.h"

static char fontMap[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?:;'\"/\\|-_<>[](){}+=@#$%^&*`~ ";
GLuint font;
GLuint charList;

GLuint framebuffer;

void (APIENTRY * glGenFramebuffersEXT)(GLsizei n, GLuint* ids);
void (APIENTRY * glDeleteFramebuffersEXT)(GLsizei n, GLuint* ids);
void (APIENTRY * glBindFramebufferEXT)(GLenum target, GLuint id);
void (APIENTRY * glFramebufferTexture2DEXT)(GLenum target, GLenum attachPt, GLenum texTarget, GLuint texId, GLint mipmapLvl);
GLenum (APIENTRY * glCheckFramebufferStatusEXT)(GLenum target);

void drawRect(int x, int y, int w, int h){
	glBegin(GL_QUADS);
		glVertex2i(x,   y  );
		glVertex2i(x+w, y  );
		glVertex2i(x+w, y+h);
		glVertex2i(x,   y+h);
	glEnd();
}

GLuint createTexture(int width, int height){
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

GLuint createTextureFromImage(const char* path){
	GLuint tex;
	tex = SOIL_load_OGL_texture(path, SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, 0);
	if(tex == 0) errorarg2(ERROR_IMGOPEN, path, SOIL_last_result());
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

GLuint loadImageToTexture(const char* path, GLuint tex){
	SOIL_load_OGL_texture(path, SOIL_LOAD_RGBA, tex, 0);
	if(tex == 0) errorarg2(ERROR_IMGOPEN, path, SOIL_last_result());
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

void deleteTexture(GLuint tex){
	glDeleteTextures(1, &tex);
}

void drawTexture(int x, int y, int w, int h, GLuint tex){
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2i(x, y);
		glTexCoord2f(1, 0);
		glVertex2i(x+w, y);
		glTexCoord2f(1, 1);
		glVertex2i(x+w, y+h);
		glTexCoord2f(0, 1);
		glVertex2i(x, y+h);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void initFont(const char* path){
	int i;
	float x, y;
	
	font = createTextureFromImage(path);
	
	charList = glGenLists(strlen(fontMap));
	for(i = 0; i < strlen(fontMap); ++i){
		x = (float)(i%16)/16;
		y = (float)(i/16)/16;
		
		glNewList(charList+i, GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(x, y);
				glVertex2i(0, 0);
				glTexCoord2f(x+1.0/16, y);
				glVertex2i(8, 0);
				glTexCoord2f(x+1.0/16, y+1.0/16);
				glVertex2i(8, 8);
				glTexCoord2f(x, y+1.0/16);
				glVertex2i(0, 8);
			glEnd();
		glEndList();
	}
}

void cleanupFont(void){
	glDeleteLists(charList, strlen(fontMap));
	deleteTexture(font);
}

void printText(int dx, int dy, const char* string){
	int stringPos;
	char* character;
	
	glBindTexture(GL_TEXTURE_2D, font);
	glPushMatrix();
	glTranslatef(dx-1, dy, 0);
	
	for(stringPos = 0; stringPos < strlen(string); ++stringPos){
		character = strchr(fontMap, string[stringPos]);
		if(character != NULL){
			glCallList(charList+(character-fontMap));
		}
		glTranslatef(6, 0, 0);
	}
	
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void initFramebuffer(void){
	glGenFramebuffersEXT = SDL_GL_GetProcAddress("glGenFramebuffersEXT");
	if(glGenFramebuffersEXT == NULL) error("EXT_framebuffer_object not supported");
	glDeleteFramebuffersEXT = SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
	glBindFramebufferEXT = SDL_GL_GetProcAddress("glBindFramebufferEXT");
	glFramebufferTexture2DEXT = SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
	glCheckFramebufferStatusEXT = SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
	glGenFramebuffersEXT(1, &framebuffer);
}

void cleanupFramebuffer(void){
	glDeleteFramebuffersEXT(1, &framebuffer);
}

void activateFramebuffer(void){
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
	glPushMatrix();
	glTranslatef(0, SCREEN_HEIGHT, 0);
	glScalef(1, -1, 1);
	glTranslatef(0, 0.750, 0);
}

void deactivateFramebuffer(void){
	glPopMatrix();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void attachFramebufferTexture(GLuint tex){
	GLint prevFb;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &prevFb);
	if(prevFb == 0) activateFramebuffer();
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
	if(prevFb == 0) deactivateFramebuffer();
}

void deattachFramebufferTexture(void){
	attachFramebufferTexture(0);
}

void validateFramebuffer(void){
	int returnVal;
	char errorCode[] = "12345";
	
	returnVal = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(returnVal != GL_FRAMEBUFFER_COMPLETE_EXT){
		sprintf(errorCode, "%d", returnVal);
		errorarg("Framebuffer error code", errorCode);
	}
}
