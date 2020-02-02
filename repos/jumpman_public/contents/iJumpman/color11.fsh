// Textured, vary color

precision mediump float;
varying vec4 v_color;

uniform sampler2D texture;
varying vec2 v_texcoord;

varying float v_fog;

void main(void)
{
    gl_FragColor = mix(vec4(0.0,0.0,0.0,1.0),
                        v_color,
                        v_fog)
                * texture2D(texture, v_texcoord);
}