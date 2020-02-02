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

#define TEXTUREPROCESS 1
#define DOUBLEPROCESS  0
#define PROCESSSAFE (gl2)

// FIXME: Globals are safe?
enum s_attribute {
    s_position = 0,
    s_color,
    s_texcoord,
    s_attribute_max
};

enum s_uniform {
    s_aspect = 0,
	s_mvp_matrix,
    s_texture,
#ifdef TEXTUREPROCESS
	s_blur,
	s_blurtexture,
#endif
    s_uniform_max
};

extern const GLuint s_invalid;

// In order for any of these functions to work, you must set this flag
// at startup, based on whatever it is you do at startup to determine
// opengl 2 / opengl es 2 is available.
extern bool gl2;

// Describes one shader along with the attributes and uniforms it requires.
struct single_shader {
    vector<const char *> lines;
    vector<int> required_attributes;
    vector<int> required_uniforms;
    GLuint type;
    GLuint shader;
    
    single_shader(GLuint _type) : type(_type), shader(s_invalid) {}
    void compile(const char *debug_name = "");
};

// Bundles together a compiled shader program along with the attributes and uniforms it knows about.
struct progpack {
    GLuint attributes[s_attribute_max]; // TODO: Make accessors, do type safety
    GLuint uniforms[s_uniform_max];
    GLuint prog;
    void init(single_shader &vert, single_shader &frag, const char *debug_name = "");
    progpack() : prog(-1)
    {
        for(int c = 0; c < s_attribute_max; c++) attributes[c] = s_invalid;
        for(int c = 0; c < s_uniform_max; c++) uniforms[c] = s_invalid;            
    }
	
	inline bool m() { return uniforms[s_mvp_matrix] != s_invalid; }
};

extern progpack *p;

void gl2Basic();

// ------------- OPENGL WRAPPERS ------------------

// Could I also eliminate BindTextures?

enum { // Bitmask-- see States. // I feel like this is more complicated than it needs to be.
    s_texturebit = 1,
    s_colorbit = 2,
	s_matrixbit = 4,
	s_blurbit = 8,
};

enum GLCS { 
    GLCS_VERTEX,
    GLCS_COLOR,
    GLCS_TEXTURE,
    GLCS_LAST,
    GLCS_INVALID = -1

};

enum GLE {
    GLE_TEXTURE,
    GLE_DEPTH,
    GLE_LAST,
    GLE_INVALID = -1
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
        if (!gl2)
            glEnableClientState(glcsEnum[glcs]);
    }
}

inline void DisableClientState(GLCS glcs) {
    if (0 != glcsIs[glcs]) {
        glcsIs[glcs] = 0;
        if (!gl2)
            glDisableClientState(glcsEnum[glcs]);
    }
}


// Awkwardness to glue against FTGL ES glue
inline GLE enumToGle(GLenum e) { for(int c = 0; c < GLE_LAST; c++) if (gleEnum[c] == e) return (GLE)c; return GLE_INVALID; }
inline GLCS enumToGlcs(GLenum e) { for(int c = 0; c < GLCS_LAST; c++) if (glcsEnum[c] == e) return (GLCS)c; return GLCS_INVALID; }

void gl2SetState(unsigned int state_cache);
void States(const bool textures, const bool colors, const bool matrix = false);

void rinseGl();

void jcColor4f(GLfloat r, GLfloat b, GLfloat g, GLfloat a);

void jcColor4ubv(GLubyte *color);

void jcVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void jcTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void jcColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

// ONLY SUPPORTS SOME GLENUMS-- INTENDED TO SUPPORT FTGL ES GLUE
void jcGetPointerv (GLenum pname, void **params);
void jcGetBooleanv (GLenum pname, GLboolean* params);
void jcGetIntegerv (GLenum pname, GLint *params);

void jcEnable(GLenum e);
void jcDisable(GLenum e);
void jcEnableClientState(GLenum e);
void jcDisableClientState(GLenum e);

#if SELF_EDIT

void trap_glerr();

#define GLERR(x) \
for ( GLenum Error = glGetError( ); ( GL_NO_ERROR != Error ); Error = glGetError( ) )\
{printf("GLERR %s: %u\n", x, Error);trap_glerr();}

#else
#define GLERR(...)
#endif

// Just do this next batch of stuff with macros, since we will never have to make a decision at runtime
// Note: Use ImmediateColor instead of Color to color ftglbegin...ftglend *OR* FTGLES rendering
#if 1
//#ifdef OPENGL_ES // TODO: Fall through to native opengl when available

#include "ftglesGlue.h"

#define jcBegin ftglBegin
#define jcVertex3f ftglVertex3f
#define jcVertex2f ftglVertex2f
#define jcTexCoord2f ftglTexCoord2f
#define jcImmediateColor4f ftglColor4f
#define jcImmediateColor4ubv(x) ftglColor4ub(((GLubyte *)(x))[0],((GLubyte *)(x))[1],((GLubyte *)(x))[2],((GLubyte *)(x))[3])
#define jcEnd ftglEnd

#else

#define jcBegin glBegin
#define jcVertex3f glVertex3f
#define jcVertex2f glVertex2f
#define jcTexCoord2f glTexCoord2f
#define jcImmediateColor4f jcColor4f
#define jcImmediateColor4ubv jcColor4ubv
#define jcEnd glEnd

#endif

// TODO: Are these sufficient? Would a more opaque "create a renderbuffer" be more appropriate given
// how weird iPhone renderbuffers are? Like renderbufferStorage:fromDrawable: should get called.
#ifdef OPENGL_ES

#define jcGenRenderbuffers glGenRenderbuffersOES
#define jcBindRenderbuffer glBindRenderbufferOES
#define jcGenFramebuffers glGenFramebuffersOES
#define jcBindFramebuffer glBindFramebufferOES
#define jcRenderbufferStorage glRenderbufferStorageOES
#define jcDeleteRenderbuffers glDeleteRenderbuffersOES
#define jcFramebufferRenderbuffer glFramebufferRenderbufferOES
#define jcFramebufferTexture2D glFramebufferTexture2DOES
#define jcCheckFramebufferStatusEXT glCheckFramebufferStatusOES
#define jcDeleteFramebuffers glDeleteFramebuffersOES

#else

#define jcGenRenderbuffers glGenRenderbuffersEXT
#define jcBindRenderbuffer glBindRenderbufferEXT
#define jcGenFramebuffers glGenFramebuffersEXT
#define jcBindFramebuffer glBindFramebufferEXT
#define jcRenderbufferStorage glRenderbufferStorageEXT
#define jcDeleteRenderbuffers glDeleteRenderbuffersEXT
#define jcFramebufferRenderbuffer glFramebufferRenderbufferEXT
#define jcFramebufferTexture2D glFramebufferTexture2DEXT
#define jcCheckFramebufferStatusEXT glCheckFramebufferStatusEXT
#define jcDeleteFramebuffers glDeleteFramebuffersEXT

#endif

#endif /* JUMPCORE_GL_COMMON */
