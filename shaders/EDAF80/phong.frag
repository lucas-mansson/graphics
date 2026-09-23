#version 410

uniform vec3 light_position;
uniform sampler2D tex;

uniform vec3 ka; // Material ambient
uniform vec3 kd; // Material diffuse
uniform vec3 ks; // Material specular
uniform float shininess; 

in VS_OUT {
	vec3 normal;
    vec3 light;
    vec3 view;
    vec2 texture_coordinates;
} fs_in;

out vec4 frag_color;

void main()
{
	// vec3 L = normalize(light_position - fs_in.vertex);
	// frag_color = vec4(1.0) * clamp(dot(normalize(fs_in.normal), L), 0.0, 1.0);
    //frag_color = texture(tex, fs_in.normal);
    /*
    vec3 L = normalize(fs_in.light);
    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(fs_in.view);
    vec3 R = normalize(reflect(-L, N)); 

    vec3 diffuse = kd * max(dot(N, L), 0.0);
    //vec3 specular = ks * pow(max(dot(R, V), 0.0), shininess); 

    frag_color.xyz = ambient + diffuse + specular;
    frag_color.w = 1.0;
	//frag_color = texture(tex, ka +  diffuse + specular);
    */

    frag_color = texture(tex, fs_in.texture_coordinates);
}
