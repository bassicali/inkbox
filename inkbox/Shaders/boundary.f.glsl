#version 330 core

uniform sampler2D field;
uniform vec2 rdv;
uniform vec2 offset;
uniform float scale;

varying vec2 coord;

out vec4 FragColor;

void main()
{
	vec2 value = texture2D(field, coord + (offset * rdv)).xy;
	value = scale * value;
	FragColor = vec4(value, 0.0, 1.0);
}