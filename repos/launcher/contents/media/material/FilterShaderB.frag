uniform sampler2D screenTexture;

#define SQRT2 0.707106781

void main(void)
{
	vec2 st = gl_TexCoord[0].st;
	gl_FragColor = texture2D(screenTexture, st);
}
