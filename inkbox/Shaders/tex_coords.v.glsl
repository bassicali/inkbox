#version 330 core

precision highp float;

layout (location=0)
in vec3 vertex;

uniform vec2 stride;

varying vec2 coord;
varying vec2 pxL;
varying vec2 pxR;
varying vec2 pxT;
varying vec2 pxB;

vec2 centerhalf(vec2 v)
{
    return v * 0.5 + 0.5;
}

void main()
{
    gl_Position = vec4(vertex.xy, 0.0, 1.0);

    coord = centerhalf(vertex.xy);
    pxL = coord - vec2(stride.x, 0);
    pxR = coord + vec2(stride.x, 0);
    pxB = coord - vec2(0, stride.y);
    pxT = coord + vec2(0, stride.y);
}