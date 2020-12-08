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
    vec2 xL = texture2D(x, pxL).xy;
    vec2 xR = texture2D(x, pxR).xy;
    vec2 xB = texture2D(x, pxB).xy;
    vec2 xT = texture2D(x, pxT).xy;
    vec2 bC = texture2D(b, coord).xy;

    vec2 result = (xL + xR + xB + xT + (alpha * bC)) / beta;

    FragColor = vec4(result, 0.0, 1.0);
}