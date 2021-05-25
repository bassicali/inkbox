#version 330 core

precision highp float;

uniform float delta_t;                      // Time step
uniform float dissipation = 1.0f;           // Dissipation factor
uniform sampler2D velocity;                 // The velocity field doing the advecting
uniform sampler2D quantity;                 // The quantity to advect
uniform vec2 rdv;
uniform float gs;

varying vec2 coord;

out vec4 FragColor;

void main()
{
    vec2 u1 = texture2D(velocity, coord).xy;
    vec2 pos0 = coord - delta_t * gs * u1;
    vec3 u0 = dissipation * texture2D(quantity, pos0).xyz;

    FragColor = vec4(u0, 1.0);
}