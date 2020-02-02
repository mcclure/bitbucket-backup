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

// FIXME: Globals are safe?
enum s_attribute {
    s_position = 0,
    s_color,
    s_texcoord,
    s_attribute_max
};

enum s_uniform {
    s_mvp_matrix = 0,
    s_texture,
    s_mobius_a,
    s_mobius_b,
    s_mobius_c,
    s_mobius_d,
    s_uniform_max
};

extern const GLuint s_invalid;

// In order for any of these functions to work, you must set this flag
// at startup, based on whatever it is you do at startup to determine
// opengl 2 / opengl es 2 is available.
extern bool gl2;
#ifdef TARGET_WEBOS
#define FORCE_ES2
#endif

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
        reset();
    }
    void reset() {
        for(int c = 0; c < s_attribute_max; c++) attributes[c] = s_invalid;
        for(int c = 0; c < s_uniform_max; c++) uniforms[c] = s_invalid;            
    }
};

extern progpack *p;

void gl2Basic();

// ------------- OPENGL WRAPPERS ------------------

// Could I also eliminate BindTextures?

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

extern const GLenum glcsEnum[GLCS_LAST];
extern int8_t glcsIs[GLCS_LAST];
extern const GLenum gleEnum[GLE_LAST];
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
		#ifndef FORCE_ES2
        if (!gl2)
            glEnableClientState(glcsEnum[glcs]);
		#endif
    }
}

inline void DisableClientState(GLCS glcs) {
    if (0 != glcsIs[glcs]) {
        glcsIs[glcs] = 0;
		#ifndef FORCE_ES2
        if (!gl2)
            glDisableClientState(glcsEnum[glcs]);
		#endif
    }
}

// Awkwardness to glue against FTGL ES glue
inline GLE enumToGle(GLenum e) { for(int c = 0; c < GLE_LAST; c++) if (gleEnum[c] == e) return (GLE)c; return GLE_INVALID; }
inline GLCS enumToGlcs(GLenum e) { for(int c = 0; c < GLCS_LAST; c++) if (glcsEnum[c] == e) return (GLCS)c; return GLCS_INVALID; }

void States(const bool textures, const bool colors);

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

// TODO: Put this in a common header or something
#ifndef TARGET_ANDROID
#define GLERR_PRINT(...) fprintf (stderr, __VA_ARGS__)
#else 
#define GLERR_PRINT(...) __android_log_print(ANDROID_LOG_ERROR,NAME_OF_THIS_PROGRAM,__VA_ARGS__)
#endif

#define GLERR(x) \
for ( GLenum Error = glGetError( ); ( GL_NO_ERROR != Error ); Error = glGetError( ) )\
{GLERR_PRINT("GLERR %s: %u\n", x, Error);trap_glerr();}

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
#define jcEnd ftglEnd

#else

#define jcBegin glBegin
#define jcVertex3f glVertex3f
#define jcVertex2f glVertex2f
#define jcTexCoord2f glTexCoord2f
#define jcImmediateColor4f jcColor4f
#define jcEnd glEnd

#endif

// Conditionally define a bunch of OpenGL ES 1.0 constants
// for the benefit of WebOS, which has header problems
#ifndef GL_STACK_OVERFLOW
#define GL_STACK_OVERFLOW 1283
#endif
#ifndef GL_STACK_UNDERFLOW
#define GL_STACK_UNDERFLOW 1284
#endif
#ifndef GL_CURRENT_COLOR
#define GL_CURRENT_COLOR 2816
#endif
#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 2848
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 2982
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 2983
#endif
#ifndef GL_LINE_SMOOTH_HINT
#define GL_LINE_SMOOTH_HINT 3154
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 5888
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 5889
#endif
#ifndef GL_VERTEX_ARRAY
#define GL_VERTEX_ARRAY 32884
#endif
#ifndef GL_COLOR_ARRAY
#define GL_COLOR_ARRAY 32886
#endif
#ifndef GL_TEXTURE_COORD_ARRAY
#define GL_TEXTURE_COORD_ARRAY 32888
#endif
#ifndef GL_VERTEX_ARRAY_SIZE
#define GL_VERTEX_ARRAY_SIZE 32890
#endif
#ifndef GL_VERTEX_ARRAY_TYPE
#define GL_VERTEX_ARRAY_TYPE 32891
#endif
#ifndef GL_VERTEX_ARRAY_STRIDE
#define GL_VERTEX_ARRAY_STRIDE 32892
#endif
#ifndef GL_COLOR_ARRAY_SIZE
#define GL_COLOR_ARRAY_SIZE 32897
#endif
#ifndef GL_COLOR_ARRAY_TYPE
#define GL_COLOR_ARRAY_TYPE 32898
#endif
#ifndef GL_COLOR_ARRAY_STRIDE
#define GL_COLOR_ARRAY_STRIDE 32899
#endif
#ifndef GL_TEXTURE_COORD_ARRAY_SIZE
#define GL_TEXTURE_COORD_ARRAY_SIZE 32904
#endif
#ifndef GL_TEXTURE_COORD_ARRAY_TYPE
#define GL_TEXTURE_COORD_ARRAY_TYPE 32905
#endif
#ifndef GL_TEXTURE_COORD_ARRAY_STRIDE
#define GL_TEXTURE_COORD_ARRAY_STRIDE 32906
#endif
#ifndef GL_VERTEX_ARRAY_POINTER
#define GL_VERTEX_ARRAY_POINTER 32910
#endif
#ifndef GL_COLOR_ARRAY_POINTER
#define GL_COLOR_ARRAY_POINTER 32912
#endif
#ifndef GL_TEXTURE_COORD_ARRAY_POINTER
#define GL_TEXTURE_COORD_ARRAY_POINTER 32914
#endif

#endif /* JUMPCORE_GL_COMMON */
