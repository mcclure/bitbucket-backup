uniform sampler2D screenColorBuffer;
uniform float aspect, radius;

#define SQRT2 0.707106781

vec4 readtex(vec2 st) {
	float b = radius;
	vec2 off = vec2(b*aspect,0);
    return
		( texture2D(screenColorBuffer, st)
		 +texture2D(screenColorBuffer, st + off )
		 +texture2D(screenColorBuffer, st + off*-1.0 )
		 +texture2D(screenColorBuffer, st + off*2.0 )
		 +texture2D(screenColorBuffer, st + off*-2.0 )
		 +texture2D(screenColorBuffer, st + off*4.0 )
		 +texture2D(screenColorBuffer, st + off*-4.0 )
		 +texture2D(screenColorBuffer, st + off*8.0 )
		 +texture2D(screenColorBuffer, st + off*-8.0 )
		 
		   ) / 9.0; 
}

void main(void)
{
	vec2 st = gl_TexCoord[0].st;
	gl_FragColor = readtex(st);
}
