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

#define VELOCITY_MAX 0.5
#define ca 0.382683431950523*blur
#define cb 0.923879532683006*blur
#define cc 0.707106781821139*blur
#define cd 1.0*blur
#define da 0.191341715975262*blur
#define db 0.461939766341503*blur
#define dc 0.353553390910569*blur
#define dd 0.5*blur
/*
vec4 tryMotion(vec2 at) {
	vec2 reg = texture2D(blur_texture, v_texcoord + at).rg;
	if (reg.x > 0.0 || reg.y > 0.0) {
		vec2 v = ( reg - vec2(0.5,0.5) ) * VELOCITY_MAX;
		vec4 color = texture2D(texture, v_texcoord + at);
		return abs(dot(v,normalize(at))) * color;
	} else {
		return vec4(0.0,0.0,0.0,0.0);
	}
}
*/

void main(void)
{
	vec4 sum;
	if (blur <= 0.0) {
		sum = texture2D(texture, v_texcoord);
	} else {
		sum = vec4(0.0);
		for(int c = -2; c < 3; c++) {
			sum += texture2D(texture, v_texcoord + vec2( float(c)*blur, 0 ) ) / 5.0;
		}
	}
#ifdef undefined
		( texture2D(texture, v_texcoord)
		 +tryMotion( vec2(0,cd) )
		 +tryMotion( vec2(ca,cb) )
		 +tryMotion( vec2(cc,cc) )
		 +tryMotion( vec2(cb,ca) )
		 +tryMotion( vec2(cd,0) )
		 +tryMotion( vec2(cb,-ca) )
		 +tryMotion( vec2(cc,-cc) )
		 +tryMotion( vec2(ca,-cb) )
		 +tryMotion( vec2(0,-cd) )
		 +tryMotion( vec2(-ca,-cb) )
		 +tryMotion( vec2(-cc,-cc) )
		 +tryMotion( vec2(-cb,-ca) )
		 +tryMotion( vec2(-cd,0) )
		 +tryMotion( vec2(-cb,ca) )
		 +tryMotion( vec2(-cc,cc) )
		 +tryMotion( vec2(-ca,cb) )
		 +tryMotion( vec2(0,dd) )
		 +tryMotion( vec2(da,db) )
		 +tryMotion( vec2(dc,dc) )
		 +tryMotion( vec2(db,da) )
		 +tryMotion( vec2(dd,0) )
		 +tryMotion( vec2(db,-da) )
		 +tryMotion( vec2(dc,-dc) )
		 +tryMotion( vec2(da,-db) )
		 +tryMotion( vec2(0,-dd) )
		 +tryMotion( vec2(-da,-db) )
		 +tryMotion( vec2(-dc,-dc) )
		 +tryMotion( vec2(-db,-da) )
		 +tryMotion( vec2(-dd,0) )
		 +tryMotion( vec2(-db,da) )
		 +tryMotion( vec2(-dc,dc) )
		 +tryMotion( vec2(-da,db) )
		   );
#endif
		   
	gl_FragColor = sum;// / sum.a;
}