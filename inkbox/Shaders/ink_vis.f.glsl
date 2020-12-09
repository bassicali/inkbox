#version 330 core

uniform sampler2D field;
uniform vec4 bias;
uniform vec4 scale;

varying vec2 coord;

out vec4 FragColor;

void main()
{
    vec4 tx = texture2D(field, coord);
    float x = sqrt(dot(tx.xy, tx.xy));

    vec4 vis = (x * bias) + scale * tx;
    FragColor = vis;
}