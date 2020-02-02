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
	vec2 use_texcoord  = vec2(dmod(v_texcoord.x + 1.0*brightness), v_texcoord.y);
	vec2 use_texcoord2 = vec2(dmod(v_texcoord.x + 2.0*brightness), v_texcoord.y);
	vec2 use_texcoord3 = vec2(dmod(v_texcoord.x + 3.0*brightness), v_texcoord.y);
	vec4 red   = texture2D(texture, use_texcoord);
	vec4 green = texture2D(texture, use_texcoord2);
	vec4 blue  = texture2D(texture, use_texcoord3);
	gl_FragColor = vec4(red.r, green.g, blue.b, 1.0);
}

