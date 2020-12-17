#version 430

#define STEPS 500
#define STEP_SIZE 0.005

varying vec3 coord;
varying vec3 coord_wpos;     // A world-space coordinate on the surface of the cube
varying vec3 cube_wpos;     // Center of the cube in world space

layout(rgba16_snorm) 
uniform image3D field;

uniform vec3 camera_wpos;
uniform vec3 camera_dir;
uniform vec4 bg_colour;
uniform vec3 box_size;

out vec4 FragColor;

////////////////////////////////////////////////////////////////////////

bool inside_cube(vec3 pos)
{
    vec3 q = box_size / 2;
    vec3 diff = abs(pos) - q;
    float dist = length(max(diff, vec3(0, 0, 0)));
    return dist <= 0.001;
}

vec3 world_to_cube(vec3 wpos)
{
    vec3 h = box_size / 2;
    vec3 diff = wpos - cube_wpos;
    return diff * h + h;
}

vec4 ray_march(vec3 pos, vec3 dir)
{
    float alpha = 1.0;
    vec3 colour = vec3(0, 0, 0);

    for (int i = 0; i < STEPS; i++)
    {
        if (!inside_cube(pos))
        {
            //alpha *= (1 - bg_colour.a);
            colour += bg_colour.rgb * alpha;
            break;
        }

        ivec3 cube_coord = ivec3(world_to_cube(pos));
        vec4 value = imageLoad(field, cube_coord);

        alpha *= (1 - value.a);
        colour += value.rgb * alpha;
        
        pos += dir * STEP_SIZE;
    }

    return vec4(colour, alpha);
}

void main()
{
    vec3 ray_pos = coord_wpos;
    vec3 ray_dir = ray_pos - camera_wpos;

    vec4 ray_colour = ray_march(ray_pos, ray_dir);
    FragColor = ray_colour;
}