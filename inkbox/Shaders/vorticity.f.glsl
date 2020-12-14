#version 330 core

precision highp float;

uniform sampler2D velocity;
uniform float gs;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

out vec4 FragColor;

void main()
{
    vec2 R = texture2D(velocity, pxR).xy;
    vec2 L = texture2D(velocity, pxL).xy;
    vec2 B = texture2D(velocity, pxB).xy;
    vec2 T = texture2D(velocity, pxT).xy;
    
    float vorticity = ((R.y - L.y)/(2 * gs)) - ((T.x - B.x)/(2 * gs));

    FragColor = vec4(vorticity, 0.0, 0.0, 1.0);
}