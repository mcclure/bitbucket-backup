uniform sampler2D screenColorBuffer;
uniform float x3, x2, x1, x0, y3, y2, y1, y0;
void main(void)
{
	float x = gl_TexCoord[0].s, y = gl_TexCoord[0].t;
	vec2 coord = 
		vec2( mod(x + y3*y*y*y + y2*y*y + y1*y + y0, 1.0),
		      mod(y + x3*x*x*x + x2*x*x + x1*x + x0, 1.0) );
	gl_FragColor = texture2D(screenColorBuffer,coord);
	gl_FragColor.a = 1.0;
}
