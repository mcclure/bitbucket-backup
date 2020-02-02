// Textured, vary color

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highp
#endif

attribute highp vec2 position;

#ifdef USE_MATRICES
uniform highp mat4 mvp_matrix;
#else
uniform float aspect;
#endif

#ifdef MINIMAL
varying vec2 dist;
#endif

#ifndef MINIMAL
attribute vec4 color;
varying vec4 v_color;
#endif

#ifdef USE_TEXTURE
attribute vec2 texcoord;
varying vec2 v_texcoord;
#endif

void main()
{
#ifndef MINIMAL
    v_color = color;
#endif

#ifdef USE_TEXTURE
    v_texcoord = texcoord;
#endif

#define POSITION position

#ifdef MINIMAL
	dist = position;
#endif

    gl_Position = 
#ifdef USE_MATRICES
			mvp_matrix * vec4(POSITION, 0, 1);
#else
			vec4(POSITION.x*aspect, POSITION.y, 0, 1);
#endif
}