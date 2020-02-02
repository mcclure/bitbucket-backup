#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL/SDL_opengl.h>

void drawRect(int x, int y, int w, int h);

GLuint createTexture(int width, int height);
GLuint createTextureFromImage(const char* path);
GLuint loadImageToTexture(const char* path, GLuint tex);
void deleteTexture(GLuint tex);
void drawTexture(int x, int y, int w, int h, GLuint tex);

void initFont(const char* path);
void cleanupFont(void);
void printText(int x, int y, const char* string);

void initFramebuffer(void);
void cleanupFramebuffer(void);
void activateFramebuffer(void);
void deactivateFramebuffer(void);
void attachFramebufferTexture(GLuint tex);
void deattachFramebufferTexture(void);
void validateFramebuffer(void);

#endif // GRAPHICS_H
