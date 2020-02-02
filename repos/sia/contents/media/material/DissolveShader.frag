uniform sampler2D screenColorBuffer;
uniform float xd, yd, thresh;

#define SQRT2 0.707106781

vec4 dissolve(vec2 st, vec4 c) {
	float a = mod( (st.x/xd * st.y/yd)/8.0, 1.0 );
	float b;
	if (a < thresh)
		b = 1.0;
	else
		b = 0.0;

	return vec4(c.rgb, b);
}

void main(void)
{
	vec2 st = gl_TexCoord[0].st;
	gl_FragColor = dissolve(st, texture2D(screenColorBuffer, st) );
}
