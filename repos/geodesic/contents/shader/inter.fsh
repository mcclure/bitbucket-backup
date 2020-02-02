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
	vec2 use_texcoord;
	if (mod(v_texcoord.y/py, 2.0) < 1.0) {
		use_texcoord = v_texcoord;
	} else {
		use_texcoord = vec2( mod(v_texcoord.x + brightness, 1.0), v_texcoord.y );
	}
	gl_FragColor = texture2D(texture, use_texcoord);
}