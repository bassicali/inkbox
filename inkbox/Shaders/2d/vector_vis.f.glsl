#version 330 core

uniform sampler2D field;
uniform vec4 bias;
uniform vec4 scale;

varying vec2 coord;

out vec4 FragColor;

void main()
{
    vec4 vis = bias + scale * texture2D(field, coord);
    FragColor = vis;
}