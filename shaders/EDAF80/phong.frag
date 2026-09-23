#version 410

uniform vec3 light_position;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_map_texture;
uniform sampler2D normal_map_texture;

uniform int use_normal_mapping;

uniform vec3 ambient_colour; // Material ambient
uniform vec3 diffuse_colour; // Material diffuse
uniform vec3 specular_colour; // Material specular

uniform float shininess_value; 
uniform float index_of_refraction_value; 
uniform float opacity_value;

in VS_OUT {
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
    vec3 light;
    vec3 view;
    vec2 texture_coordinates;
} fs_in;

out vec4 frag_color;

void main()
{
    vec3 light = normalize(fs_in.light);
    vec3 normal = normalize(fs_in.normal);
    vec3 view = normalize(fs_in.view);
    vec3 reflect_v = normalize(reflect(-light, normal)); 

    mat3 TBN = mat3(fs_in.tangent, fs_in.binormal, fs_in.normal);

    if (use_normal_mapping == 1) {

    }

    vec3 diffuse = /* diffuse_colour * */ max(dot(normal, light), 0.0) * texture(diffuse_texture, fs_in.texture_coordinates).xyz;

    vec3 specular = specular_colour * pow(max(dot(reflect_v, view), 0.0), shininess_value) * texture(specular_map_texture, fs_in.texture_coordinates).xyz;

    frag_color.xyz = ambient_colour + diffuse + specular;
    frag_color.w = 1.0;

    //frag_color = ;
}
