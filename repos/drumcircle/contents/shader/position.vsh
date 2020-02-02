// Textured, vary color

#ifdef GL_ES
precision mediump float;
#else
#define lowp
#define mediump
#define highp
#endif

attribute highp vec2 position;
uniform highp mat4 mvp_matrix;

#ifdef USE_HYPER
uniform highp vec2 a,b,c,d; // mobius transform
vec2 cmult(vec2 e, vec2 f) {
//    return vec2(dot(e,-f), dot(e.yx,f));
    return vec2( e.x*f.x - e.y*f.y, e.x*f.y + e.y*f.x );
}
vec2 cdiv(vec2 e, vec2 f) {
    float div = f.x*f.x+f.y*f.y;
    return vec2( e.x*f.x + e.y*f.y, e.y*f.x - e.x*f.y) / div;
}
#endif

attribute vec4 color;
varying vec4 v_color;

#ifdef USE_TEXTURE
attribute vec2 texcoord;
varying vec2 v_texcoord;
#endif

void main()
{
    v_color = color;

#ifdef USE_TEXTURE
    v_texcoord = texcoord;
#endif

#ifdef USE_HYPER
    highp vec2 poincare = cdiv( (cmult(a, position) + b) , (cmult(c, position) + d) );
#define POSITION poincare
#else
#define POSITION position
#endif

    gl_Position = mvp_matrix * vec4(POSITION, 0, 1);
}