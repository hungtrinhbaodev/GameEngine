#version 450

layout (binding = 0) uniform graphic_uniform {
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec3 in_color;

layout (location = 0) out vec3 frag_color;

layout (push_constant) uniform constants {
    vec2 screen_size;
} push_constant;

vec2 to_screen_rate_position(vec2 position) {
    float rate_x = position.x / push_constant.screen_size.x;
    float rate_y = 1.f - position.y / push_constant.screen_size.y;
    vec2 rate_position = vec2(-1.f + rate_x * 2, -1.f + rate_y * 2);
    return rate_position;
}

void main() {
    gl_Position = vec4(to_screen_rate_position(in_position), 0.f, 1.f);
    frag_color = in_color;
}