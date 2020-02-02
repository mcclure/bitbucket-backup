uniform sampler2D screenColorBuffer;
uniform sampler2D screenTexture;
uniform float aspect, intensity, radius;
uniform vec2 v1, v2, v3, v4;

#define SQRT2 0.707106781

vec2 filterSpot(vec2 v, vec2 c) {
	vec2 offs = c - v;
	float l = length(offs);
	offs = offs / l * pow(l, radius);
//	float dist = length(offs * vec2(aspect,1.0));
	return -offs*intensity;
}

void main(void)
{
	vec2 st = gl_TexCoord[0].st;
	vec4 new = texture2D(screenColorBuffer, st);
	if (new.a > 0.1) {
		gl_FragColor = new;
	} else {
		vec2 st2 = st + filterSpot(st, v1) + filterSpot(st, v2)
			+ filterSpot(st, v3) + filterSpot(st, v4);
		vec4 old = texture2D(screenTexture, st2);
		gl_FragColor = old;
	}
}
