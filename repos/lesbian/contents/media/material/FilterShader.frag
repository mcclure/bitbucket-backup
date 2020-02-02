uniform sampler2D screenColorBuffer;
uniform float aspect, span; // Note aspect is opposite of my normal definition

#define SQRT2 0.707106781

vec4 readtex(vec2 st) {
	return texture2D(screenColorBuffer, st-vec2(mod(st.s, span*aspect), mod(st.t, span)));
/*
	float b = intensity * l*l*l;
	float b2 = b*SQRT2;
	vec2 off = vec2(b*aspect,b);
	vec2 off2 = vec2(b2*aspect,b2); 
    return
		( texture2D(screenColorBuffer, st)
		 +texture2D(screenColorBuffer, st + off2*vec2(-1.0, 1.0) )
		 +texture2D(screenColorBuffer, st + off2*vec2( 1.0, 1.0) )
		 +texture2D(screenColorBuffer, st + off2*vec2(-1.0,-1.0) )
		 +texture2D(screenColorBuffer, st + off2*vec2( 1.0,-1.0) )
		 +texture2D(screenColorBuffer, st + off* vec2(-1.0, 0.0) )
		 +texture2D(screenColorBuffer, st + off* vec2( 1.0, 0.0) )
		 +texture2D(screenColorBuffer, st + off* vec2( 0.0,-1.0) )
		 +texture2D(screenColorBuffer, st + off* vec2( 0.0, 1.0) )
		 
		   ) / 9.0; 
		   */
}

void main(void)
{
	vec2 st = gl_TexCoord[0].st;
	gl_FragColor = readtex(st);
}
