/*
 *  glCommon.cpp
 *  iJumpman
 *
 *  Created by Andi McClure on 6/14/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#include "glCommon.h"

// TODO: Get kludge.h in here
#include "internalFile.h"
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

// JUMPMAN STUFF-- SHOULD BE FACTORED OUT INTO OTHER FILE.
#if SELF_EDIT
#define ERR(...) printf (__VA_ARGS__)

#define GLERR(x) \
for ( GLenum Error = glGetError( ); ( GL_NO_ERROR != Error ); Error = glGetError( ) )\
{printf("GLERR %s: %u\n", x, Error);}

#else
#define ERR(...)
#define GLERR(...)
#endif

#define GLSL_DEBUG

#if FOG
float fogf = -12;
#endif

bool es2;
progpack p_plain, p_texture, p_nofog;
progpack *p = &p_plain;

GLenum glcsEnum[GLCS_LAST] = {GL_VERTEX_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_COLOR_ARRAY};
int8_t glcsIs[GLCS_LAST] = { -1, -1, -1 }; // -1 for dunno
GLenum gleEnum[GLE_LAST] = {GL_TEXTURE_2D, GL_DEPTH_TEST};
int8_t gleIs[GLE_LAST] = {-1, -1};

unsigned int es2_state_cache = 0xFFFFFFFF;

GLuint initShader(const char *shader_name, GLenum type) {
    string shader;
    GLuint shad;
    
    {
        char shaderfile[FILENAMESIZE];
		internalPath(shaderfile, shader_name);
        ifstream i(shaderfile);
        std::stringstream buffer;
        buffer << i.rdbuf();
        shader = buffer.str();
    }
    
    const char *lines[] = {shader.c_str()};
    shad = glCreateShader(type); GLERR("Create (shader)");
    glShaderSource(shad, 1, lines, NULL); GLERR("Source");
    glCompileShader(shad); GLERR("Compile");
        
#if defined(GLSL_DEBUG)
	GLint logLength;
    glGetShaderiv(shad, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(shad, logLength, &logLength, log);
        ERR("Shader %s compile log:\n%s", shader_name, log);
        free(log);
    }
#endif
    
    return shad;
}

GLuint initProgram(GLuint vert, GLuint frag, bool textures, bool colors, const char *debugName = "") {
    GLuint prog = glCreateProgram(); GLERR("Create (program)");

    glAttachShader(prog, vert); GLERR("Attach 1");
    glAttachShader(prog, frag); GLERR("Attach 2");

    int s_position = 0;    // FIXME: NO MAGIC NUMBERS
    int s_color = 1;
    int s_texcoord = 2;
    
    glBindAttribLocation ( prog, s_position, "position" ); GLERR("Bind 1");
    if (colors)
        glBindAttribLocation ( prog, s_color, "color" ); GLERR("Bind 2");
    if (textures)
        glBindAttribLocation ( prog, s_texcoord, "texcoord" ); GLERR("Bind 3");    
    
    glLinkProgram(prog); GLERR("Link");

#if defined(GLSL_DEBUG)
	GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        ERR("Program %s link log:\n%s", debugName, log);
        free(log);
    }
#endif
    
    return prog;
}

void progpack::init(const char * vsh, const char *fsh, bool textures, bool colors, bool fog) {
    GLuint vert_shad = initShader(vsh, GL_VERTEX_SHADER);
    GLuint frag_shad = initShader(fsh, GL_FRAGMENT_SHADER);
    prog = initProgram(vert_shad, frag_shad, textures, colors);
    glUseProgram(prog); GLERR("Use"); // Unnecessary?
    
    s_mvp_matrix = glGetUniformLocation(prog, "mvp_matrix"); GLERR("Uniform1");
    
    if (textures) {
        s_texture = glGetUniformLocation(prog, "texture"); GLERR("Uniform4");
    
        glActiveTexture ( GL_TEXTURE0 ); GLERR("ActiveTexture");
    }
    
#if FOG
    if (fog) {
        s_fogfactor = glGetUniformLocation(prog, "fogfactor"); GLERR("Bind 5");    
        glUniform1f ( s_fogfactor, fogf );     GLERR("fog uniform1f");
    }
#endif    
    
    glUniform1i ( s_texture, 0 );     GLERR("uniform1i");
    
    glEnableVertexAttribArray(s_position);     GLERR("attribarray");
}

void es2Basic(bool fog) {
    p_plain.init("position01.vsh", "color01.fsh", false, true);
    p_texture.init("position11.vsh", "color11.fsh", true, true);
    p_nofog.init("position11nf.vsh", "color11nf.fsh", true, true, false);
}

void rinseGl() {
    for(int c = 0; c < GLCS_LAST; c++)
        glcsIs[c] = -1;
    for(int c = 0; c < GLE_LAST; c++)
        gleIs[c] = -1;
    es2_state_cache = 0xFFFFFFFF;
}

void jcColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    if (es2) {
        glVertexAttrib4f(p->s_color, r, g, b, a); GLERR("Color4f");
    } else {
        glColor4f(r,g,b,a);
    }
}

void jcVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    if (es2) {
        GLERR("Prevertexpointer");
        glVertexAttribPointer ( p->s_position, size, type, GL_FALSE, stride, pointer );
        GLERR("Vertexpointer");
    } else {
        glVertexPointer(size, type, stride, pointer);
    }
}
void jcTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    if (es2) {
        glVertexAttribPointer ( p->s_texcoord, size, type, GL_FALSE, stride, pointer );
        GLERR("Coordpointer");
    } else {
        glTexCoordPointer(size, type, stride, pointer);
    }
}
void jcColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    if (es2) {
        glVertexAttribPointer ( p->s_color, size, type, GL_TRUE, stride, pointer ); // We expect these in as bytes
        GLERR("Colorpointer");
    } else {
        glColorPointer(size, type, stride, pointer);
    }
}

#if FOG
void jcFogFactor(GLfloat f) {
    if (es2)
        glUniform1f ( p->s_fogfactor, f );
}
#endif

void States(const bool textures, const bool colors, const bool fog) {
    if (es2) {
        GLERR("Prestates");
        
        unsigned int state_cache = (textures?1:0) + (colors?2:0) + (fog?4:0);
        
        if (state_cache == es2_state_cache)
            return;
        
        es2_state_cache = state_cache;
        
        if (!fog) { // Fog only matters in ES2 mode. In normal mode we can't readily turn it off.
            p = &p_nofog;
            glEnableVertexAttribArray(p->s_texcoord);
        } else if (textures) {
            p = &p_texture;
            glEnableVertexAttribArray(p->s_texcoord);
        } else {
            p = &p_plain;
            glDisableVertexAttribArray(p->s_texcoord);
        }        
        GLERR("TextureArray");
        
        glUseProgram(p->prog);
        
        if (colors)
            glEnableVertexAttribArray(p->s_color);
        else
            glDisableVertexAttribArray(p->s_color);
        GLERR("ColorArray");

        void mesa_sync();
        mesa_sync();
        
    } else {
        if (textures) {
            Enable(GLE_TEXTURE);
            EnableClientState(GLCS_TEXTURE);
        } else {
            Disable(GLE_TEXTURE);
            DisableClientState(GLCS_TEXTURE);
        }
        if (colors)
            EnableClientState(GLCS_COLOR);
        else
            DisableClientState(GLCS_COLOR);        
    }
}