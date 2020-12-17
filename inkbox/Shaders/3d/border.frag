#version 430

varying vec3 coord_wpos;

out vec4 FragColor;

void main()
{
    FragColor = vec4(coord_wpos, 1);
}