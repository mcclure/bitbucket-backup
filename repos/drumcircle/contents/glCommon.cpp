/*
 *  glCommon.cpp
 *  iJumpman
 *
 *  Created by Andi McClure on 6/14/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "kludge.h"
#include "glCommon.h"

// TODO: Get kludge.h in here
#include "internalfile.h"
#include <string>
#include <fstream>
#include <sstream>

#ifndef GLchar
typedef char GLchar; // Android needs this I guess?
#endif

#define GLSL_DEBUG

#if SELF_EDIT
void trap_glerr() {
    return;
}
#endif

// Used for GL1
const GLenum glcsEnum[GLCS_LAST] = {GL_VERTEX_ARRAY, GL_COLOR_ARRAY, GL_TEXTURE_COORD_ARRAY};
int8_t glcsIs[GLCS_LAST] = { -1, -1, -1 }; // -1 for dunno
const GLenum gleEnum[GLE_LAST] = {GL_TEXTURE_2D, GL_DEPTH_TEST};
int8_t gleIs[GLE_LAST] = {-1, -1};

// Used for GL2
bool gl2;
progpack *prog_for_code(unsigned int code);
progpack p_dummy; // Will never be initialized, just so p-> never crashes
progpack *p = &p_dummy;
unsigned int gl2_state_cache = 0xFFFFFFFF; // Input to prog_for_code
const GLuint s_invalid = -1;

// TODO: How should this work for Jumpcore? Move to display.cpp?
progpack local_progs[4];
enum { // Bitmask-- see States. // I feel like this is more complicated than it needs to be.
    s_texturebit = 1,
    s_colorbit = 2,
    s_hyperbit = 4,
};
progpack *prog_for_code(unsigned int code) {
    int idx = (code & s_texturebit ? 1 : 0) + (code & s_hyperbit ? 2:0);
    return &local_progs[idx];
}

const char *attribute_names[s_attribute_max] = {"position", "color", "texcoord"};
const char *uniform_names[s_uniform_max] = {"mvp_matrix", "texture", "a", "b", "c", "d"};

// TODO move to kludge.h or something
string string_from_internal(string name) {
    char filename[FILENAMESIZE];
    internalPath(filename, name.c_str());
    ifstream i(filename);
    std::stringstream buffer;
    buffer << i.rdbuf();
    return buffer.str();
}    

template <typename A> void stl_push(A &a, A &b) {
    a.insert(a.end(), b.begin(), b.end());
}

void single_shader::compile(const char *debug_name) { // FIXME: This is a silly place to put debug_name
    shader = glCreateShader(type); GLERR("Create (shader)");
    glShaderSource(shader, lines.size(), &lines[0], NULL); GLERR("Source");
    glCompileShader(shader); GLERR("Compile");
        
#if defined(GLSL_DEBUG)
	GLint logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        ERR("Shader %s compile log:\n%s", debug_name, log);
        free(log);
    }
#endif
}

void progpack::init(single_shader &vert, single_shader &frag, const char *debug_name) {
    vector<int> r_a = vert.required_attributes; stl_push(r_a, frag.required_attributes);
    vector<int> r_u = vert.required_uniforms; stl_push(r_u, frag.required_uniforms);
    
    prog = glCreateProgram(); GLERR("Create (program)");
    
    glAttachShader(prog, vert.shader); GLERR("Attach 1");
    glAttachShader(prog, frag.shader); GLERR("Attach 2");
    
    // Now hook up attributes:
    for(int c = 0; c < r_a.size(); c++) { // r_a[c] will correspond to an s_attribute
        if (attributes[r_a[c]] != s_invalid) continue; // Already done this one
        attributes[r_a[c]] = r_a[c];
        glBindAttribLocation(prog, r_a[c], attribute_names[r_a[c]]);
    }
    for(int c = 0; c < s_attribute_max; c++) attributes[c] = c; // This seems very wrong!
    
    glLinkProgram(prog); GLERR("Link");
    
#if defined(GLSL_DEBUG)
	GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        ERR("Program %s link log:\n%s", debug_name, log);
        free(log);
    }
#endif    
    
    glUseProgram(prog); GLERR("Use"); // Unnecessary?
    
    // Now hook up uniforms:
    for(int c = 0; c < r_u.size(); c++) { // r_a[c] will correspond to an s_attribute
        if (uniforms[r_u[c]] != s_invalid) continue; // Already done this one
        uniforms[r_u[c]] = glGetUniformLocation(prog, uniform_names[r_u[c]]);
        
        if (r_u[c] == s_texture) { // The "texture" uniform, when present, gets special handling
            glActiveTexture ( GL_TEXTURE0 ); GLERR("ActiveTexture");
            glUniform1i ( uniforms[r_u[c]], 0 );     GLERR("uniform1i");
        }
    }    
    
    glEnableVertexAttribArray(attributes[s_position]);     GLERR("attribarray");
}

void gl2Basic() {
    if (!gl2) return;

    // --- Scratch variables-- all of this exists only for the span of this function: ---
    
    single_shader position01(GL_VERTEX_SHADER), color01(GL_FRAGMENT_SHADER),
                  position11(GL_VERTEX_SHADER), color11(GL_FRAGMENT_SHADER),
                  hposition01(GL_VERTEX_SHADER), hposition11(GL_VERTEX_SHADER);
    
    string position = string_from_internal("position.vsh");
    string color = string_from_internal("color.fsh");
    const char *enable_textures = "#define USE_TEXTURE 1\n"; // TODO: Automate "options", this is awful
    const char *enable_hyper = "#define USE_HYPER 1\n";
    
    // --- Set up shader code: ---

    position01.lines.push_back(position.c_str()); 
    
    position11.lines.push_back(enable_textures);
    position11.lines.push_back(position.c_str());

    hposition01.lines.push_back(enable_hyper);
    hposition01.lines.push_back(position.c_str());
    
    hposition11.lines.push_back(enable_hyper);
    hposition11.lines.push_back(enable_textures);
    hposition11.lines.push_back(position.c_str());
    
    color01.lines.push_back(color.c_str());
    
    color11.lines.push_back(enable_textures);
    color11.lines.push_back(color.c_str());
    
    // --- Set up attributes / uniforms: ---
    
    position01.required_attributes.push_back(s_position); // position01 has position, color, mvp_matrix
    position01.required_attributes.push_back(s_color);
    position01.required_uniforms.push_back(s_mvp_matrix);
    position11.required_attributes = position01.required_attributes; // position11 is position01 plus texcoord
    position11.required_uniforms = position01.required_uniforms;
    position11.required_attributes.push_back(s_texcoord);

    hposition01.required_attributes = position01.required_attributes; // hposition01 is position01 plus a,b,c,d
    hposition01.required_uniforms = position01.required_uniforms;
    hposition01.required_uniforms.push_back(s_mobius_a);
    hposition01.required_uniforms.push_back(s_mobius_b);
    hposition01.required_uniforms.push_back(s_mobius_c);
    hposition01.required_uniforms.push_back(s_mobius_d);
    hposition11.required_attributes = position11.required_attributes; //hposition11 is p11's attributes, hp01's uniforms
    hposition11.required_uniforms = hposition01.required_uniforms;
    
    color11.required_uniforms.push_back(s_texture);
    
    // --- Compile: ---    
    position01.compile("position01"); color01.compile("color01"); // Compile each shader, with debug name
    position11.compile("position11"); color11.compile("color11");
    hposition01.compile("hposition01"); hposition11.compile("hposition11");
    
    // --- Now link programs: ---
    local_progs[0].init(position01, color01, "_");
    local_progs[1].init(position11, color11, "t");
    local_progs[2].init(hposition01, color01, "h");
    local_progs[3].init(hposition11, color11, "ht");
}

void rinseGl() {
    for(int c = 0; c < GLCS_LAST; c++)
        glcsIs[c] = -1;
    for(int c = 0; c < GLE_LAST; c++)
        gleIs[c] = -1;
    gl2_state_cache = 0xFFFFFFFF;
}

void jcColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    if (gl2) {
        glVertexAttrib4f(p->attributes[s_color], r, g, b, a); GLERR("Color4f");
    } else {
        glColor4f(r,g,b,a);
    }
}

void jcColor4ubv(GLubyte *color) {
    if (gl2) {
//        glVertexAttrib4Nub(p->s_color, 255, 255, 255, 255);
//        glVertexAttrib4ubv(p->s_color, color);
        glVertexAttrib4f(p->attributes[s_color], color[0]/255.0, color[1]/255.0, color[2]/255.0, color[3]/255.0); GLERR("Color4f-2");
    } else {
        glColor4ub(color[0], color[1], color[2], color[3]);
    }
}

void jcVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    if (gl2) {
        GLERR("Prevertexpointer");
        glVertexAttribPointer ( p->attributes[s_position], size, type, GL_FALSE, stride, pointer );
        GLERR("Vertexpointer");
    } else {
        glVertexPointer(size, type, stride, pointer);
    }
}
void jcTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    if (gl2) {
        glVertexAttribPointer ( p->attributes[s_texcoord], size, type, GL_FALSE, stride, pointer );
        GLERR("Coordpointer");
    } else {
        glTexCoordPointer(size, type, stride, pointer);
    }
}
void jcColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    if (gl2) {
        glVertexAttribPointer ( p->attributes[s_color], size, type, GL_TRUE, stride, pointer ); // We expect these in as bytes
        GLERR("Colorpointer");
    } else {
        glColorPointer(size, type, stride, pointer);
    }
}

void jcGetPointerv (GLenum pname, void **params) {
    if (gl2) {
        switch(pname) {
            case GL_VERTEX_ARRAY_POINTER:
                glGetVertexAttribPointerv(p->attributes[s_position], GL_VERTEX_ATTRIB_ARRAY_POINTER, params);
                break;
            case GL_TEXTURE_COORD_ARRAY_POINTER:
                glGetVertexAttribPointerv(p->attributes[s_texcoord], GL_VERTEX_ATTRIB_ARRAY_POINTER, params);
                break;
            case GL_COLOR_ARRAY_POINTER:
                glGetVertexAttribPointerv(p->attributes[s_color], GL_VERTEX_ATTRIB_ARRAY_POINTER, params);
                break;
        }
    } else {
        glGetPointerv(pname, params);
    }
}

// Weird(?) quirk: GL_VERTEX_ARRAY is *always* on
void jcGetBooleanv (GLenum pname, GLboolean* params) {
    if (gl2) {
        switch (pname) {
            case GL_VERTEX_ARRAY:
                *params = true;
                break;
            case GL_TEXTURE_COORD_ARRAY:
                *params = gl2_state_cache & s_texturebit;
                break;
            case GL_COLOR_ARRAY:
                *params = gl2_state_cache & s_colorbit;
                break;
        }
        *params = false;
    } else {
        glGetBooleanv(pname, params);
    }
}

void jcGetIntegerv (GLenum pname, GLint *params) {
    if (gl2) {
        bool invalid = false;
        int attribute; GLenum query;
        switch (pname) {
            case GL_VERTEX_ARRAY_TYPE: case GL_TEXTURE_COORD_ARRAY_TYPE: case GL_COLOR_ARRAY_TYPE:
                query = GL_VERTEX_ATTRIB_ARRAY_TYPE; break;
            case GL_VERTEX_ARRAY_SIZE: case GL_TEXTURE_COORD_ARRAY_SIZE: case GL_COLOR_ARRAY_SIZE:
                query = GL_VERTEX_ATTRIB_ARRAY_SIZE; break;
            case GL_VERTEX_ARRAY_STRIDE: case GL_TEXTURE_COORD_ARRAY_STRIDE: case GL_COLOR_ARRAY_STRIDE:
                query = GL_VERTEX_ATTRIB_ARRAY_STRIDE; break;
            default:
                invalid = true; break;
        }
        switch (pname) {
            case GL_VERTEX_ARRAY_TYPE: case GL_VERTEX_ARRAY_SIZE: case GL_VERTEX_ARRAY_STRIDE:
                attribute = p->attributes[s_position]; break;
            case GL_TEXTURE_COORD_ARRAY_TYPE: case GL_TEXTURE_COORD_ARRAY_SIZE: case GL_TEXTURE_COORD_ARRAY_STRIDE:
                attribute = p->attributes[s_texcoord]; break;
            case GL_COLOR_ARRAY_TYPE: case GL_COLOR_ARRAY_SIZE: case GL_COLOR_ARRAY_STRIDE:
                attribute = p->attributes[s_color]; break;
        }
        if (!invalid)
            glGetVertexAttribiv(attribute, query, params);
    } else {
        glGetIntegerv(pname, params);
    }
}

// Texture enable/disable just gets a passthrough
void jcEnable(GLenum e) {
    GLE e2 = enumToGle(e); if (e2 != GLE_INVALID) Enable(e2);
}
void jcDisable(GLenum e) { 
    GLE e2 = enumToGle(e); if (e2 != GLE_INVALID) Disable(e2);
}

void gl2SetState(unsigned int state_cache) {
    if (state_cache == gl2_state_cache)
        return;
    
    gl2_state_cache = state_cache;
    
    p = prog_for_code( gl2_state_cache );
    
    if (state_cache & s_texturebit) { // Why does one of these need to be done before useProgram and the other after?
        glEnableVertexAttribArray(p->attributes[s_texcoord]);
    } else {
        glDisableVertexAttribArray(p->attributes[s_texcoord]);
    }        
    GLERR("TextureArray");
    
    glUseProgram( p->prog );
    
    if (state_cache & s_colorbit)
        glEnableVertexAttribArray(p->attributes[s_color]);
    else
        glDisableVertexAttribArray(p->attributes[s_color]);
    GLERR("ColorArray");
    
    void mesa_sync();
    mesa_sync();    
}

// ClientState enable/disable is a little more complicated...
void jcEnableClientState(GLenum e) {
    if (gl2) {
        switch (e) {
            case GL_VERTEX_ARRAY: break;
            case GL_TEXTURE_COORD_ARRAY: gl2SetState(gl2_state_cache | s_texturebit); break;
            case GL_COLOR_ARRAY: gl2SetState(gl2_state_cache | s_colorbit); break;
        }
    } else {
        GLCS e2 = enumToGlcs(e); if (e2 != GLCS_INVALID) EnableClientState(e2);
    }
}
void jcDisableClientState(GLenum e) {
    if (gl2) {
        switch (e) {
            case GL_VERTEX_ARRAY: break;
            case GL_TEXTURE_COORD_ARRAY: gl2SetState(gl2_state_cache & ~s_texturebit); break;
            case GL_COLOR_ARRAY: gl2SetState(gl2_state_cache & ~s_colorbit); break;
        }
    } else {
        GLCS e2 = enumToGlcs(e); if (e2 != GLCS_INVALID) DisableClientState(e2);
    }
}

// So for some stupid reason you can't name variables "hyper" in mingw.
void States(const bool textures, const bool colors, const bool _hyper) {
    if (gl2) {
        GLERR("Prestates");
        
        unsigned int state_cache = (textures?s_texturebit:0) + (colors?s_colorbit:0) + (_hyper?s_hyperbit:0);
        
        gl2SetState( state_cache );
        
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
