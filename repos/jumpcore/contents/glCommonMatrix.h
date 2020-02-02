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
void jcOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void jcPerspective(GLfloat fovy, GLfloat aspect, GLfloat near, GLfloat far);
void jcRotatef(GLfloat,GLfloat,GLfloat,GLfloat);
void jcTranslatef(GLfloat,GLfloat,GLfloat);
void jcMultMatrixf(const GLfloat *f);
void jcPopMatrix();
void jcPushMatrix();
void jcMatrixFetch(GLfloat *projection, GLfloat *modelview, GLint *viewport);
void jcMatrixDump(const char *why = 0);
void jcMatrixGlmIn(const glm::mat4 &v);

#endif /* JUMPCORE_GL_COMMON_MATRIX */