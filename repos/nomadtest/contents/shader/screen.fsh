uniform sampler2D texture;
varying vec2 v_texcoord;
uniform float px;
 
void main(void)
{
   gl_FragColor = vec4(
	texture2D(texture, vec2(v_texcoord.x-px, v_texcoord.y)).r,
	texture2D(texture, vec2(v_texcoord.x, v_texcoord.y)).g,
	texture2D(texture, vec2(v_texcoord.x+px, v_texcoord.y)).b,
	1.0
   );
}