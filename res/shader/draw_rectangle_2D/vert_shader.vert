#version 450

layout (location = 0) in vec2 in_position;

layout (location = 1) in vec2 in_translation;
layout (location = 2) in vec2 in_size;
layout (location = 3) in vec2 in_anchor_point;
layout (location = 4) in vec3 in_instance_color;
layout (location = 5) in float in_rotation;

layout (location = 0) out vec3 frag_color;

layout (push_constant) uniform constants {
    layout (offset = 0) vec2 screen_size;
} push_constant;

vec2 norm_position(vec2 position) {
    return vec2(position.x / push_constant.screen_size.x, position.y / push_constant.screen_size.y);
}

vec2 rotate(float rotation, vec2 position) {
    float r = radians(rotation);
    return vec2(position.x * cos(r) - position.y * sin(r), position.x * sin(r) + position.y * cos(r));
}

vec2 flipped_position(vec2 norm_position) {
    vec2 rate_position = vec2(-1.f + norm_position.x * 2, -1.f + (1.f - norm_position.y) * 2);
    return rate_position;
}

void main() {
    vec2 translate_anchor = (in_position - in_anchor_point) * in_size;
    vec2 rectangle_position = in_translation + rotate(in_rotation, translate_anchor);
    gl_Position = vec4(flipped_position(norm_position(rectangle_position)), 0.f, 1.f);
    frag_color = in_instance_color;
}