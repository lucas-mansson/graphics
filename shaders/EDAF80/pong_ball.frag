#version 410

uniform vec3  light_position;
uniform vec3  camera_position;
uniform float shininess;

in VS_OUT {
	vec3 world_pos;
	vec3 normal;
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 N = normalize(fs_in.normal);
	vec3 L = normalize(light_position - fs_in.world_pos);
	vec3 V = normalize(camera_position - fs_in.world_pos);
	vec3 R = reflect(-L, N);

	//  vit färg 
	vec3 albedo = vec3(1.0);

	vec3 ambient  = 0.25 * albedo;
	vec3 diffuse  = max(dot(N, L), 0.0) * albedo;
	vec3 specular = pow(max(dot(R, V), 0.0), shininess) * vec3(0.8);

	frag_color = vec4(ambient + diffuse + specular, 1.0);
}