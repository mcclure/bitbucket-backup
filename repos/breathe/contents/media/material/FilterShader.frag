uniform sampler2D screenColorBuffer;
uniform sampler2D screenTexture;
uniform float px, py;

bool within(float test, float target, float range) {
	return test >= target-range && test <= target+range; 
}

// Birth: 34678 Survive: 3678
float snap(float c, float t) {
	return within(t, 3.0, 0.5) || within(t, 7.0, 1.5)
		|| (c > 0.5 && within(t, 4.0,0.5)) ? 1.0 : 0.0;
//	return (x >= 3.0 && x <= 4.0) || (x >= 6.0 && x <= 8.0) ? 1.0 : 0.0;
}

void main(void)
{
	vec2 st = gl_TexCoord[0].st;
	vec2 p = vec2(px,py);
	vec4 current = texture2D(screenTexture, st); 
	vec4 lifetotal = //current
		 +texture2D(screenTexture, st + p*vec2(-1.0, 1.0) )
		 +texture2D(screenTexture, st + p*vec2( 1.0, 1.0) )
		 +texture2D(screenTexture, st + p*vec2(-1.0,-1.0) )
		 +texture2D(screenTexture, st + p*vec2( 1.0,-1.0) )
		 +texture2D(screenTexture, st + p*vec2(-1.0, 0.0) )
		 +texture2D(screenTexture, st + p*vec2( 1.0, 0.0) )
		 +texture2D(screenTexture, st + p*vec2( 0.0,-1.0) )
		 +texture2D(screenTexture, st + p*vec2( 0.0, 1.0) );
	vec4 lastframe = vec4(snap(current.r,lifetotal.r),snap(current.g,lifetotal.g),snap(current.b,lifetotal.b),1.0);
	vec4 thisframe = texture2D(screenColorBuffer, st);
	gl_FragColor = thisframe.a > 0.9 ? thisframe : lastframe;
}
