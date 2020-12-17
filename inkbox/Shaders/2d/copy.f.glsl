#version 330 core

precision highp float;

uniform sampler2D field;

varying vec2 coord;

out vec4 FragColor;

void main()
{
    vec4 tx = texture2D(field, coord);
    FragColor = tx;
}