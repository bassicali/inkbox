#version 330 core

precision highp float;

layout (location=0)
in vec3 vertex;

varying vec3 coord;
varying vec3 coord_wpos;
varying vec3 cube_wpos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 cube_pos;

void main()
{
	gl_Position = proj * view * model * vec4(vertex.xyz, 1.0);
	coord = gl_Position.xyz;
	coord_wpos = (model * vec4(vertex.xyz, 1.0)).xyz;
	cube_wpos = (model * vec4(cube_pos.xyz, 0)).xyz;
}