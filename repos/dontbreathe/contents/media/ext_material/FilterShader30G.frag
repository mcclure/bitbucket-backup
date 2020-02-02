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
	
	float rcurrent = texture2D(screenTexture, wrap(st + p*vec2(-1.0, 1.0)) ).r;
	float gcurrent = texture2D(screenTexture, wrap(st + p*vec2(-1.0, 1.0)) ).g;
	float bcurrent = texture2D(screenTexture, wrap(st + p*vec2(-1.0, 1.0)) ).b;

	vec4 cupleft = texture2D(screenTexture, wrap(st + p*vec2(-1.0, 1.0)) );
	vec4 cupcent = texture2D(screenTexture, wrap(st + p*vec2( 0.0, 1.0)) );
	vec4 cuprite = texture2D(screenTexture, wrap(st + p*vec2( 1.0, 1.0)) );
		
	bool upleft = cupleft.r > 0.5 || cupleft.g > 0.5 || cupleft.b > 0.5;
	bool upcent = cupcent.r > 0.5 || cupcent.g > 0.5 || cupcent.b > 0.5;
	bool uprite = cuprite.r > 0.5 || cuprite.g > 0.5 || cuprite.b > 0.5;
	
	bool outp = ( upleft && !upcent && !uprite )
			 || (!upleft &&  upcent &&  uprite )
			 || (!upleft &&  upcent && !uprite )
			 || (!upleft && !upcent &&  uprite );
	float routval = rcurrent*(1.0-thresh) + (outp ? thresh*(cupleft.r+cupcent.r+cuprite.r) : 0.0);
	float goutval = gcurrent*(1.0-thresh) + (outp ? thresh*(cupleft.g+cupcent.g+cuprite.g) : 0.0);
	float boutval = bcurrent*(1.0-thresh) +  (outp ? thresh*(cupleft.b+cupcent.b+cuprite.b) : 0.0);
	
	vec4 lastframe = vec4(routval, goutval, boutval, 1.0);
	vec4 thisframe = texture2D(screenColorBuffer, st);
	gl_FragColor = thisframe.a > 0.9 ? thisframe : lastframe;
}
