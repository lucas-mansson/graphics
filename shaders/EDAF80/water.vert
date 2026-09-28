#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float elapsed_time_s;

out VS_OUT {
    vec3 vertex;
    vec3 normal;
} vs_out;

struct Wave {
    vec2 position;
    vec2 direction;
    float amplitude;
    float frequency;
    float phase;
    float sharpness;
};

float wave(Wave w, float time)
{
    return w.amplitude * pow(sin((w.position.x * w.direction.x + w.position.y * w.direction.y) * w.frequency + w.phase * time) * 0.5 + 0.5, w.sharpness);
}

void main()
{
    vec3 displaced_vertex = vertex;
    // Wave 1
    Wave wave1;
    wave1.position = vertex.xz;
    wave1.direction = vec2(-1.0, 0.0);
    wave1.amplitude = 1.0;
    wave1.frequency = 0.2;
    wave1.phase = 0.5;
    wave1.sharpness = 2.0;

    Wave wave2;
    wave2.position = vertex.xz;
    wave2.direction = vec2(-0.7, 0.7);
    wave2.amplitude = 0.5;
    wave2.frequency = 0.4;
    wave2.phase = 1.3;
    wave2.sharpness = 2.0;

    displaced_vertex.y += wave(wave1, elapsed_time_s) + wave(wave2, elapsed_time_s);

    vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));
    vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));

    gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displaced_vertex, 1.0);
}
