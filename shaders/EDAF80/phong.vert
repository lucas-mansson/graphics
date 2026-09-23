#version 410

// PHONG SHADER

// Remember how we enabled vertex attributes in assignment 2 and attached some
// data to each of them, here we retrieve that data. Attribute 0 pointed to the
// vertices inside the OpenGL buffer object, so if we say that our input
// variable `vertex` is at location 0, which corresponds to attribute 0 of our
// vertex array, vertex will be effectively filled with vertices from our
// buffer.
// Similarly, normal is set to location 1, which corresponds to attribute 1 of
// the vertex array, and therefore will be filled with normals taken out of our
// buffer.
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform mat4 ModelViewProj; // Model -> Clip space
uniform mat4 World; // Model -> World space
uniform mat4 WorldIT; // Inverse transpose

uniform vec3 light_position; // Defined in world space
uniform vec3 camera_position; // Defined in world space

// This is the custom output of this shader. If you want to retrieve this data
// from another shader further down the pipeline, you need to declare the exact
// same structure as in (for input), with matching name for the structure
// members and matching structure type. Have a look at
// shaders/EDAF80/diffuse.frag.
out VS_OUT {
	vec3 normal;
    vec3 light;
    vec3 view;
    vec2 texture_coordinates;
} vs_out;

void main()
{
    // World space version
    vec3 vertex_world_position = (vertex_model_to_world * vec4(vertex,1)).xyz;

    vs_out.normal = (normal_model_to_world * vec4(normal, 0)).xyz;
    vs_out.light = light_position - vertex_world_position;
    vs_out.view = camera_position - vertex_world_position;
    vs_out.texture_coordinates = texcoord.xy;


    gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}

