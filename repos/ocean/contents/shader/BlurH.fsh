uniform sampler2D texture;
varying vec2 v_texcoord;
uniform float px;
 
void main(void)
{
   vec4 sum = vec4(0.0);
   sum += texture2D(texture, vec2(v_texcoord.x - 4.0*px, v_texcoord.y)) * 0.05;
   sum += texture2D(texture, vec2(v_texcoord.x - 3.0*px, v_texcoord.y)) * 0.09;
   sum += texture2D(texture, vec2(v_texcoord.x - 2.0*px, v_texcoord.y)) * 0.12;
   sum += texture2D(texture, vec2(v_texcoord.x - px, v_texcoord.y)) * 0.15;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y)) * 0.16;
   sum += texture2D(texture, vec2(v_texcoord.x + px, v_texcoord.y)) * 0.15;
   sum += texture2D(texture, vec2(v_texcoord.x + 2.0*px, v_texcoord.y)) * 0.12;
   sum += texture2D(texture, vec2(v_texcoord.x + 3.0*px, v_texcoord.y)) * 0.09;
   sum += texture2D(texture, vec2(v_texcoord.x + 4.0*px, v_texcoord.y)) * 0.05;
   gl_FragColor = sum;
}

