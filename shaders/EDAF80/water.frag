#version 410

uniform vec3 light_position;
uniform float elapsed_time_s;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
    vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
    vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);

    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(fs_in.vertex);

    float facing = 1.0 - max(dot(V, N), 0.0);

	vec3 L = normalize(light_position - fs_in.vertex);
	//frag_color = vec4(1.0) * clamp(dot(normalize(fs_in.normal), L), 0.0, 1.0);
    vec4 water_color = mix(color_deep, color_shallow, facing);
	frag_color = water_color;
}
