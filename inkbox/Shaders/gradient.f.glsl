#version 330 core

precision highp float;

uniform sampler2D field;
uniform vec2 rdv;
uniform float gs;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

out vec4 FragColor;

void main()
{
    float R = texture2D(field, pxR).x;
    float L = texture2D(field, pxL).x;
    float B = texture2D(field, pxB).x;
    float T = texture2D(field, pxT).x;
    
    vec2 gradient = vec2(R-L, T-B)/(2 * gs);
    FragColor = vec4(gradient, 0.0, 1.0);
}