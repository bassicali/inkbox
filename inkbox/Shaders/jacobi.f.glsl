#version 330 core

precision highp float;

uniform float beta;
uniform float alpha;
uniform vec2 rdv;
uniform sampler2D x;
uniform sampler2D b;

varying vec2 coord;
varying vec2 pxT;
varying vec2 pxB;
varying vec2 pxL;
varying vec2 pxR;

out vec4 FragColor;

void main()
{
    vec3 xL = texture2D(x, pxL).xyz;
    vec3 xR = texture2D(x, pxR).xyz;
    vec3 xB = texture2D(x, pxB).xyz;
    vec3 xT = texture2D(x, pxT).xyz;
    vec3 bC = texture2D(b, coord).xyz;

    vec3 result = (xL + xR + xB + xT + (alpha * bC)) / beta;

    FragColor = vec4(result, 1.0);
}