#version 330 core

precision highp float;

uniform vec2 position;		// Cursor position
uniform vec3 force;			// The force
uniform float radius;		// Radius of gaussian splat
uniform float delta_t;		// Time step
uniform sampler2D velocity;	// Velocity field

varying vec2 coord;
out vec4 FragColor;

void main()
{
	vec2 diff = position - coord;
	float x = -dot(diff,diff) / radius;
	vec3 effect = normalize(force) * exp(x);
	vec3 u0 = texture2D(velocity, coord).xyz;

	FragColor = vec4(u0 + effect, 1.0);
}