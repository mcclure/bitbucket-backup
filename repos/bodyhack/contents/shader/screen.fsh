// Arbitrary screen shader

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highp
#endif

#define SPLIT 0
#define PLAYERS 2
#define RULES 3

uniform sampler2D texture;
varying vec2 v_texcoord;

uniform float px, py, width, height;
uniform mat3 rule[RULES];
uniform float radius[PLAYERS];
uniform vec2 center[PLAYERS];

vec2 wrap(vec2 c) {
	return vec2(mod(c.x, width), mod(c.y, height));
}

vec4 fetch(sampler2D t, vec2 pos, mat3 r, int x, int y) {
	vec2 sample = wrap(pos + vec2(x-1,y-1)*vec2(px,py));
	return texture2D(t, sample) * r[y][x];
}

void accumulate(inout vec4 accumulator, mat3 r, sampler2D t, vec2 pos) {
	for(int x=0;x<3;x++) {
		for(int y=0;y<3;y++) {
			accumulator += fetch(texture, v_texcoord, r, x, y);
		}
	}
}

void main(void)
{
	vec4 accumulator = vec4(0,0,0,0);

	#if !SPLIT
		// Spotlights
		
		for(int z = 0; z < PLAYERS; z++) {
			vec2 abscoord = v_texcoord/height;
			float d = length(center[z] - abscoord);
			
			if (d < radius[z]) {
				accumulate(accumulator, rule[z], texture, v_texcoord);
			}
		}
		
		#if RULES > 2
			// Currently no provision for a third player...
			accumulate(accumulator, rule[2], texture, v_texcoord);
		#endif
	#else
		// Splitscreen
		
		vec2 abscoord = v_texcoord/height;	
		float d1 = length(center[0] - abscoord);
		float d2 = length(center[1] - abscoord);
		int idx = d1 > d2 ? 0 : 1;
		accumulate(accumulator, rule[idx], texture, v_texcoord);
	#endif
	
    gl_FragColor = accumulator;
}