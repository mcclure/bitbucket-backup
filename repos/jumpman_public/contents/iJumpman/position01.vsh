// Textured, vary color

precision mediump float;

attribute highp vec2 position;
uniform highp mat4 mvp_matrix;

attribute vec4 color;
varying vec4 v_color;

varying float v_fog;

void main()
{
    v_color = color;
    gl_Position = mvp_matrix * vec4(position, 0, 1);
    
    v_fog = clamp(1.0-gl_Position.z, -3.0, 0.0) / 3.0 + 1.0;
}