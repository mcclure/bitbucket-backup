uniform sampler2D screenColorBuffer;
uniform sampler2D screenTexture;
uniform float px, py, thresh;

// Birth: 34678 Survive: 3678
vec2 wrap(vec2 c) {
	return vec2(mod(c.x, 1.0), c.y);
}

void main(void)
{
	vec2 st = gl_TexCoord[0].st;
	vec2 p = vec2(px,py);

	float current = texture2D(screenTexture, wrap(st + p*vec2(-1.0, 1.0)) ).r;
	
	bool upleft = texture2D(screenTexture, wrap(st + p*vec2(-1.0, 1.0)) ).r > 0.5;
	bool upcent = texture2D(screenTexture, wrap(st + p*vec2( 0.0, 1.0)) ).r > 0.5;
	bool uprite = texture2D(screenTexture, wrap(st + p*vec2( 1.0, 1.0)) ).r > 0.5;
	
	bool outp = ( upleft && !upcent && !uprite )
			 || (!upleft &&  upcent &&  uprite )
			 || (!upleft &&  upcent && !uprite )
			 || (!upleft && !upcent &&  uprite );
	float outval = current*(1.0-thresh) + (outp ? thresh : 0.0);
	
	vec4 lastframe = vec4(outval, outval, outval, 1.0);
	vec4 thisframe = texture2D(screenColorBuffer, st);
	gl_FragColor = thisframe.a > 0.9 ? thisframe : lastframe;
}
