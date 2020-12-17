#version 330 core

precision highp float;

uniform sampler2D field;
uniform float gs;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

out vec4 FragColor;

void main()
{
    vec2 R = texture2D(field, pxR).xy;
    vec2 L = texture2D(field, pxL).xy;
    vec2 B = texture2D(field, pxB).xy;
    vec2 T = texture2D(field, pxT).xy;
    
    float div = (R.x - L.x)/(2 * gs) + (T.y - B.y)/(2 * gs);

    FragColor = vec4(div, 0.0, 0.0, 1.0);
}