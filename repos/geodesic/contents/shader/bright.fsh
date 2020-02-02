// Arbitrary screen shader

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highpw
#endif

uniform sampler2D texture;
varying vec2 v_texcoord;

uniform float px, py, brightness;

void main(void)
{	
	vec4 color = texture2D(texture, v_texcoord);
    gl_FragColor = vec4(color.rgb * brightness, color.a);
}