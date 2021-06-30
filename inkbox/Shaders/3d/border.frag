#version 430

varying vec3 coord_wpos;

uniform bool colour_with_coord;
uniform vec3 colour;

out vec4 FragColor;

void main()
{
    FragColor = vec4(colour_with_coord ? coord_wpos : colour, 1);
}