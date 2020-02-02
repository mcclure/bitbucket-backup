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

const float pi = 3.1415926;

float dmod(float f) {
	return mod(f + 1.0, 2.0) - 1.0;
}

void main(void)
{
	vec2 use_texcoord = vec2(v_texcoord.x, dmod(v_texcoord.y +  sin((v_texcoord.x)*pi*5.0)*brightness));
	gl_FragColor = texture2D(texture, use_texcoord);
}

