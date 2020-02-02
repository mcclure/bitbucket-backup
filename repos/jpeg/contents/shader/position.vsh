// Textured, vary color

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highp
#endif

attribute highp vec2 position;
uniform highp mat4 mvp_matrix;

attribute vec4 color;
varying vec4 v_color;

#ifdef USE_TEXTURE
attribute vec2 texcoord;
varying vec2 v_texcoord;
#endif

void main()
{
    v_color = color;

#ifdef USE_TEXTURE
    v_texcoord = texcoord;
#endif

    gl_Position = mvp_matrix * vec4(position, 0, 1);
}