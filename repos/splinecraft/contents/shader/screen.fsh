// Arbitrary screen shader

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highp
#endif

uniform sampler2D texture;
varying vec2 v_texcoord;

uniform float px, py, width, height;

void main(void)
{	
    gl_FragColor = texture2D(texture, v_texcoord);;
}