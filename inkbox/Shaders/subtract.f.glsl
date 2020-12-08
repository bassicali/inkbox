#version 330 core

precision highp float;

uniform sampler2D a;
uniform sampler2D b;

varying vec2 coord;

out vec4 FragColor;

void main()
{
    vec2 txa = texture2D(a, coord).xy;
    vec2 txb = texture2D(b, coord).xy;
    vec2 diff = txa - txb;

    FragColor = vec4(diff, 0.0, 1.0);
}