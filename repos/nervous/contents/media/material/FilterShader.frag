uniform sampler2D screenColorBuffer;
uniform float aspect, intensity, radius;
uniform vec3 center;

#define SQRT2 0.707106781

vec4 readtex(float l, vec2 st) {
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
}

void main(void)
{
	vec2 st = gl_TexCoord[0].st;
	float d = length((center.xy - st) * vec2(aspect,1.0));
	gl_FragColor = readtex(min(0.0,radius-d), st);
}
