#version 410

uniform vec3 light_position;
uniform sampler2D my_texture;

uniform vec3 ambient_colour; // Material ambient
uniform vec3 diffuse_colour; // Material diffuse
uniform vec3 specular_colour; // Material specular
uniform float shininess_value; 
uniform float index_of_refraction_value; 
uniform float opacity_value;

in VS_OUT {
	vec3 normal;
    vec3 light;
    vec3 view;
    vec2 texture_coordinates;
} fs_in;

out vec4 frag_color;

void main()
{
    vec3 L = normalize(fs_in.light);
    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(fs_in.view);
    vec3 R = normalize(reflect(-L, N)); 

    vec3 diffuse = diffuse_colour * max(dot(N, L), 0.0) * texture(my_texture, fs_in.texture_coordinates).xyz;
    //vec3 diffuse = texture(my_texture, fs_in.texture_coordinates).rgb;
    vec3 specular = specular_colour * pow(max(dot(R, V), 0.0), shininess_value); 

    frag_color.xyz = ambient_colour + diffuse + specular;
    frag_color.w = 1.0;

    //frag_color = ;
}
