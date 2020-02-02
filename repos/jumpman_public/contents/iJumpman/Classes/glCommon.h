#ifndef JUMPCORE_GL_COMMON
#define JUMPCORE_GL_COMMON

/*
 *  glCommon.h
 *  iJumpman
 *
 *  Created by Andi McClure on 6/14/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#define FOG 0
#define FOG_DEBUG 0 // SET BOTH

// FIXME: Globals are safe?

// In order for any of these functions to work, you must set this flag
// at startup, based on whatever it is you do at startup to determine
// opengl 2 / opengl es 2 is available.
extern bool es2;

struct progpack {
    GLuint prog, s_position, s_color, s_mvp_matrix, s_texcoord, s_texture;
#if FOG
    GLuint s_fogfactor;
#endif
    void init(const char * vsh, const char *fsh, bool textures, bool colors, bool fog = true);
    progpack() : prog(-1), s_position(0), s_color(1), s_mvp_matrix(-1), s_texcoord(2), s_texture(-1)
#if FOG
    , s_fogfactor(-1)
#endif
    {}
};

extern progpack p_basic;
extern progpack *p;

void es2Basic(bool fog);

// ------------- OPENGL WRAPPERS ------------------

// Could I also eliminate BindTextures?

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

extern GLenum glcsEnum[GLCS_LAST];
extern int8_t glcsIs[GLCS_LAST];
extern GLenum gleEnum[GLE_LAST];
extern int8_t gleIs[GLE_LAST];

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
        if (!es2)
            glEnableClientState(glcsEnum[glcs]);
    }
}

inline void DisableClientState(GLCS glcs) {
    if (0 != glcsIs[glcs]) {
        glcsIs[glcs] = 0;
        if (!es2)
            glDisableClientState(glcsEnum[glcs]);
    }
}

void States(const bool textures, const bool colors, const bool fog = true);

void rinseGl();

void jcColor4f(GLfloat r, GLfloat b, GLfloat g, GLfloat a);

void jcVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void jcTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void jcColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

#if FOG
void jcFogFactor(GLfloat f);
#endif

#endif /* JUMPCORE_GL_COMMON */
