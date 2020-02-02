uniform sampler2D texture;
varying vec2 v_texcoord;
uniform float py, brightness;
 
void main(void)
{
   vec4 sum = vec4(0.0);
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y - 4.0*py)) * 0.05;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y - 3.0*py)) * 0.09;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y - 2.0*py)) * 0.12;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y - py)) * 0.15;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y)) * 0.16;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y + py)) * 0.15;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y + 2.0*py)) * 0.12;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y + 3.0*py)) * 0.09;
   sum += texture2D(texture, vec2(v_texcoord.x, v_texcoord.y + 4.0*py)) * 0.05;
   
   gl_FragColor = sum * brightness;
}

