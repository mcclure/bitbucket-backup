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
 
#include <vector>
 
#define PROJ_EXT 1

// FIXME: Globals are safe?
enum s_attribute {
    s_position = 0,
    s_color,
    s_texcoord,
    s_attribute_max
};

__attribute__((unused)) static const char *attribute_names[s_attribute_max]
	= {"position", "color", "texcoord"};

enum s_uniform {
    s_mvp_matrix = 0,
    s_texture,
#ifdef PROJ_EXT
    s_px,
    s_py,
	s_width,
	s_height,
	s_brightness,
#endif
    s_uniform_max
};

__attribute__((unused)) static const char *uniform_names[s_uniform_max] = {"mvp_matrix", "texture",
#if PROJ_EXT
	"px", "py", "width", "height", "brightness",
#endif
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
	string debug_name;
    void init(single_shader &vert, single_shader &frag, const char *debug_name = "");
    progpack() : prog(-1)
    {
        reset();
    }
    void reset() {
        for(int c = 0; c < s_attribute_max; c++) attributes[c] = s_invalid;
        for(int c = 0; c < s_uniform_max; c++) uniforms[c] = s_invalid;            
		debug_name = "";
    }
};

extern progpack *p;

void gl2Basic();

// ------------- OPENGL WRAPPERS ------------------

void States(const bool textures, const bool colors);

void Special(int which);

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
    
#ifdef SELF_EDIT

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
#define jcImmediateColor4ub ftglColor4ub
#define jcEnd ftglEnd

#else

#define jcBegin glBegin
#define jcVertex3f glVertex3f
#define jcVertex2f glVertex2f
#define jcTexCoord2f glTexCoord2f
#define jcImmediateColor4f jcColor4f
static inline void jcImmediateColor4ub(GLubyte r, g, b, a) { GLubyte rgba[4] = {r,g,b,a}; jcColor4ubv(rgba); }
#define jcEnd glEnd

#endif

static inline void jcImmediateColorWord(uint32_t rgba) { GLubyte *_ = (GLubyte *)&rgba; jcImmediateColor4ub(_[0],_[1],_[2],_[3]); }

// Handle framebuffer/renderbuffer having different func names on iPhone
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
