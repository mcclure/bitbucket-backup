// Textured, vary color

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highp
#endif

varying vec4 v_color;

#ifdef USE_TEXTURE
uniform sampler2D texture;
varying vec2 v_texcoord;
#endif

void main(void)
{
    gl_FragColor = v_color
#ifdef USE_TEXTURE
                * texture2D(texture, v_texcoord)
#endif
                ;
}