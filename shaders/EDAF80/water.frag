#version 410

uniform float elapsed_time_s;
uniform samplerCube cubemap;
uniform sampler2D normal_map_texture;

uniform vec3 light_position;

in VS_OUT {
    vec3 vertex;
    vec3 view;
    vec3 normal;
    vec3 binormal;
    vec3 tangent;
    vec2 texture_coordinates;
    vec2 normal_coord_0;
    vec2 normal_coord_1;
    vec2 normal_coord_2;
} fs_in;

out vec4 frag_color;

void main()
{
    mat3 TBN = mat3(fs_in.tangent, fs_in.binormal, fs_in.normal);

    vec3 n0 = texture(normal_map_texture, fs_in.normal_coord_0).xyz * 2.0 - 1.0;
    vec3 n1 = texture(normal_map_texture, fs_in.normal_coord_1).xyz * 2.0 - 1.0;
    vec3 n2 = texture(normal_map_texture, fs_in.normal_coord_2).xyz * 2.0 - 1.0;

    vec3 n_bump = normalize(n0 + n1 + n2); // This is in tangent space
    vec3 normal = normalize(TBN * n_bump);

    vec3 view = normalize(fs_in.view);
    vec3 reflect_v = normalize(reflect(-view, normal));
    vec4 reflection = texture(cubemap, reflect_v);

    float eta = 1.0 / 1.33;
    vec3 refraction_v = refract(view, normal, eta);
    vec4 refraction = texture(cubemap, refraction_v);

    float r0 = 0.02037;
    float fresnel = r0 + (1 - r0) * pow(1 - dot(view, normal), 5);

    float facing = 1.0 - max(dot(view, normal), 0.0);
    vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
    vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);
    vec4 water_color = mix(color_deep, color_shallow, facing);

	frag_color = water_color + (reflection * fresnel) + (refraction * (1 - fresnel));
	//frag_color = (refraction * (1 - fresnel));
}
