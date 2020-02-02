#ifndef JUMPCORE_GL_COMMON_MATRIX
#define JUMPCORE_GL_COMMON_MATRIX

/*
 *  glCommonMatrix.h
 *  iJumpman
 *
 *  Created by Andi McClure on 6/14/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "glCommon.h"

void jcMatrixInit();
void jcMatrixMode(GLenum mode);
void jcLoadIdentity();
void jcScalef(GLfloat x, GLfloat y, GLfloat z);
void jcFrustumf(GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zNear, GLfloat zFar);
void jcOrtho(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat);
void jcRotatef(GLfloat,GLfloat,GLfloat,GLfloat);
void jcTranslatef(GLfloat,GLfloat,GLfloat);
void jcPopMatrix();
void jcPushMatrix();
void jcMatrixFetch(GLfloat *projection, GLfloat *modelview, GLint *viewport);
void jcMatrixDump(const char *why = 0);

#endif /* JUMPCORE_GL_COMMON_MATRIX */