// Textured, vary color

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highp
#endif

uniform sampler2D texture, blur_texture;
varying vec2 v_texcoord;

uniform float blur;
varying vec2 dist;

const vec4 cr = vec4(1.0, 0.5, 0.0, 0.5);
const vec4 cg = vec4(0.0, 1.0, 1.0, 0.0);
const vec4 cb = vec4(0.0, 0.0, 1.0, 1.0);

void main(void)
{
	vec4 i = texture2D(texture, v_texcoord );
		  
	float a = 0.5*(2.0*i.r - i.g - i.b);
	float b = 0.8660254037844386 * (i.g - i.b); // sqrt(3)/2
	
	float h = atan(b, a);
	float c = sqrt(a*a+b*b);

	if (c < 0.1) {
		gl_FragColor = vec4(0.0,0.0,0.0,i.a);
	} else if (c > 0.45) {
		gl_FragColor = vec4(1.0,1.0,1.0,i.a);
	} else {				 
		int idx = int(mod(h/0.78539816339744828 + 6.0, 4.0)); // PI/4
		
		gl_FragColor = vec4(cr[idx], cg[idx], cb[idx], i.a);
	}
}
