#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float elapsed_time_s;

uniform vec3 camera_position; // Defined in world space

out VS_OUT {
    vec3 vertex;
    vec3 view;
    vec3 normal;
    vec3 binormal;
    vec3 tangent;
    vec2 texture_coordinates;
    vec2 normal_coord_0;
    vec2 normal_coord_1;
    vec2 normal_coord_2;
} vs_out;

struct Wave {
    vec2 position;
    vec2 direction;
    float amplitude;
    float frequency;
    float phase;
    int sharpness;
};

float wave_direction_freq(Wave w, float time) 
{
    return (w.position.x * w.direction.x + w.position.y * w.direction.y) * w.frequency + w.phase * time;
}

float wave_pow(Wave w, int power, float time) 
{
    return w.amplitude * pow(sin(wave_direction_freq(w, time)) * 0.5 + 0.5, power);
}

float wave_derivative_start(Wave w, float time) 
{
    return 0.5 * w.sharpness * w.frequency * wave_pow(w, w.sharpness - 1, time) * cos(wave_direction_freq(w, time));
}

float wave_derivative_x(Wave w, float time) 
{
    return wave_derivative_start(w, time) * w.direction.x;
}

float wave_derivative_z(Wave w, float time) 
{
    return wave_derivative_start(w, time) * w.direction.y;
}

float wave(Wave w, float time)
{
    return wave_pow(w, w.sharpness, time);
}

void main()
{
    Wave wave1;
    wave1.position = vertex.xz;
    wave1.direction = vec2(-1.0, 0.0);
    wave1.amplitude = 1.0;
    wave1.frequency = 0.2;
    wave1.phase = 0.5;
    wave1.sharpness = 2;

    Wave wave2;
    wave2.position = vertex.xz;
    wave2.direction = vec2(-0.7, 0.7);
    wave2.amplitude = 0.5;
    wave2.frequency = 0.4;
    wave2.phase = 1.3;
    wave2.sharpness = 2;

    vec3 displaced_vertex = vertex;
    displaced_vertex.y += wave(wave1, elapsed_time_s) + wave(wave2, elapsed_time_s);
    vec4 vertex_pos = vertex_model_to_world * vec4(displaced_vertex, 1.0);

    float height_derivative_x = wave_derivative_x(wave1, elapsed_time_s) + wave_derivative_x(wave2, elapsed_time_s);
    float height_derivative_z = wave_derivative_z(wave1, elapsed_time_s) + wave_derivative_z(wave2, elapsed_time_s);

    vec3 tangent = normalize(vec3(1, height_derivative_x, 0));
    vec3 binormal = normalize(vec3(0, height_derivative_z, 1));
    vec3 normal = normalize(vec3(-height_derivative_x, 1, -height_derivative_z));

    vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));
    vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
    vs_out.binormal = vec3(normal_model_to_world * vec4(binormal, 0.0));
    vs_out.tangent = vec3(normal_model_to_world * vec4(tangent, 0.0));

    vs_out.view = camera_position - vertex_pos.xyz;
    vs_out.texture_coordinates = texcoord.xy;

    vec2 tex_scale = vec2(8, 4);
    float normal_time = mod(elapsed_time_s, 100.0);
    vec2 normal_speed = vec2(-0.005, 0.0);

    vs_out.normal_coord_0.xy = texcoord.xy * tex_scale + normal_time * normal_speed;
    vs_out.normal_coord_1.xy = texcoord.xy * tex_scale * 2 + normal_time * normal_speed * 4;
    vs_out.normal_coord_2.xy = texcoord.xy * tex_scale * 4 + normal_time * normal_speed * 8;

    gl_Position = vertex_world_to_clip * vertex_pos;
}
