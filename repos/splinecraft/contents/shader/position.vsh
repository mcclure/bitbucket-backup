// Textured, vary color

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highp
#endif

attribute highp vec3 position;

#ifdef USE_MATRIX
uniform highp mat4 mvp_matrix;
#endif

//#ifdef USE_COLOR
attribute vec4 color;
varying vec4 v_color;
//#endif

#ifdef USE_TEXTURE
attribute vec2 texcoord;
varying vec2 v_texcoord;
#endif

void main()
{
#ifdef USE_COLOR
    v_color = color;
#endif

#ifdef USE_TEXTURE
    v_texcoord = texcoord;
#endif

    gl_Position =
#ifdef USE_MATRIX
		mvp_matrix * 
#endif
		vec4(position, 1);
}