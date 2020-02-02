// Arbitrary screen shader

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highpw
#endif

uniform sampler2D texture;
varying vec2 v_texcoord;

uniform float px, py, brightness;

void main(void)
{	
	vec4 color = texture2D(texture, v_texcoord);
    float power = 1.0 - color.r;
	power = 1.0 - (power*power);
    float random = fract(sin(dot(vec3(v_texcoord.x, v_texcoord.y, brightness),vec3(12.9898,78.233,42.6243))) * 43758.5453);
    if (power > random)
        gl_FragColor = vec4(1.0,1.0,1.0,color.a);
    else
        gl_FragColor = vec4(0.0,0.0,0.0,color.a);
}