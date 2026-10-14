#version 410

uniform vec3 base_color;
uniform vec3 light_position;
uniform vec3 camera_position;
uniform float opacity;   // NYTT: 1.0 = helt solid, 0.0 = helt genomskinlig

in VS_OUT {
	vec3 world_pos;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 N = normalize(fs_in.normal);
	vec3 L = normalize(light_position - fs_in.world_pos);
	vec3 V = normalize(camera_position - fs_in.world_pos);

	if (dot(N, L) < 0.0)
		N = -N;

	vec3 R = reflect(-L, N);

	vec3 ambient  = 0.25 * base_color;
	vec3 diffuse  = max(dot(N, L), 0.0) * base_color;
	vec3 specular = pow(max(dot(R, V), 0.0), 32.0) * vec3(0.3);

	frag_color = vec4(ambient + diffuse + specular, opacity);
}