#version 330 core

#define EPSILON 0.00024414

uniform sampler2D velocity;
uniform sampler2D vorticity;
uniform float scale;
uniform float delta_t;
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
    float R = texture2D(vorticity, pxR).x;
    float L = texture2D(vorticity, pxL).x;
    float B = texture2D(vorticity, pxB).x;
    float T = texture2D(vorticity, pxT).x;
    float C = texture2D(vorticity, coord).x;

    vec2 force = vec2(abs(T) - abs(B), abs(R) - abs(L)) / (2 * gs);
    float mag_sq = max(EPSILON, dot(force,force));
    force *= inversesqrt(mag_sq);
    force *= scale * C * vec2(1,-1);

    vec2 v = texture2D(velocity, coord).xy;
    v += delta_t * force;

    FragColor = vec4(v, 0.0, 1.0);
}